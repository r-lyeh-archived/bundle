/******************************************************************************
 * 
 * Copyright (c) 2010-2012, Duane Merrill.  All rights reserved.
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 * 
 ******************************************************************************/

/******************************************************************************
 * CTA-processing functionality for radix sort downsweep scan kernels
 ******************************************************************************/

#pragma once

#include "../radix_sort/sort_utils.cuh"

#include "../util/basic_utils.cuh"
#include "../util/reduction/serial_reduce.cuh"
#include "../util/scan/serial_scan.cuh"
#include "../util/ns_umbrella.cuh"


B40C_NS_PREFIX
namespace b40c {
namespace radix_sort {


/**
 * CTA collective abstraction for ranking keys (unsigned bits) by radix digit.
 *
 * Keys must be in a form suitable for radix ranking (i.e., unsigned bits).
 */
template <
	int 					LOG_CTA_THREADS,
	int 					RADIX_BITS,
	int 					CURRENT_BIT,
	cudaSharedMemConfig 	SMEM_CONFIG = cudaSharedMemBankSizeFourByte>	// Shared memory bank size
struct CtaRadixRank
{
	//---------------------------------------------------------------------
	// Type definitions and constants
	//---------------------------------------------------------------------

	// Integer type for digit counters (to be packed into words of type PackedCounters)
	typedef unsigned short DigitCounter;

	// Integer type for packing DigitCounters into columns of shared memory banks
	typedef typename util::If<(SMEM_CONFIG == cudaSharedMemBankSizeEightByte),
		unsigned long long,
		unsigned int>::Type PackedCounter;

	enum {
		RADIX_DIGITS 				= 1 << RADIX_BITS,

		CTA_THREADS					= 1 << LOG_CTA_THREADS,

		LOG_WARP_THREADS 			= CUB_LOG_WARP_THREADS(__CUB_CUDA_ARCH__),
		WARP_THREADS				= 1 << LOG_WARP_THREADS,

		LOG_WARPS					= LOG_CTA_THREADS - LOG_WARP_THREADS,
		WARPS						= 1 << LOG_WARPS,

		BYTES_PER_COUNTER			= sizeof(DigitCounter),
		LOG_BYTES_PER_COUNTER		= util::Log2<BYTES_PER_COUNTER>::VALUE,

		PACKING_RATIO				= sizeof(PackedCounter) / sizeof(DigitCounter),
		LOG_PACKING_RATIO			= util::Log2<PACKING_RATIO>::VALUE,

		LOG_COUNTER_LANES			= CUB_MAX((RADIX_BITS - LOG_PACKING_RATIO), 0),				// Always at least one lane
		COUNTER_LANES				= 1 << LOG_COUNTER_LANES,

		// The number of packed counters per thread (plus one for padding)
		RAKING_SEGMENT				= COUNTER_LANES + 1,

		LOG_MEM_BANKS				= CUB_LOG_MEM_BANKS(__CUB_CUDA_ARCH__),
		MEM_BANKS					= 1 << LOG_MEM_BANKS,
	};


	/**
	 * Shared memory storage layout
	 */
	struct SmemStorage
	{
		// Storage for scanning local ranks
		volatile PackedCounter		warpscan[WARPS][WARP_THREADS * 3 / 2];

		union
		{
			unsigned char			digit_counter_bytes[1];
			DigitCounter			digit_counters[COUNTER_LANES + 1][CTA_THREADS][PACKING_RATIO];
			PackedCounter			raking_grid[CTA_THREADS][RAKING_SEGMENT];
		};
	};


	//---------------------------------------------------------------------
	// Methods
	//---------------------------------------------------------------------

	/**
	 * Reset shared memory digit counters
	 */
	static __device__ __forceinline__ void ResetCounters(SmemStorage &smem_storage)
	{
		// Reset shared memory digit counters
		#pragma unroll
		for (int LANE = 0; LANE < COUNTER_LANES + 1; LANE++)
		{
			*((PackedCounter*) smem_storage.digit_counters[LANE][threadIdx.x]) = 0;
		}
	}

	//---------------------------------------------------------------------
	// Helper structure for templated iteration.  (NVCC currently won't
	// unroll loops with "unexpected control flow".)
	//---------------------------------------------------------------------

	/**
	 * Iterate
	 */
	template <int COUNT, int MAX>
	struct Iterate
	{
		/**
		 * Decode keys.  Decodes the radix digit from the current digit place
		 * and increments the thread's corresponding counter in shared
		 * memory for that digit.
		 *
		 * Saves both (1) the prior value of that counter (the key's
		 * thread-local exclusive prefix sum for that digit), and (2) the shared
		 * memory offset of the counter (for later use).
		 */
		template <typename UnsignedBits, int KEYS_PER_THREAD>
		static __device__ __forceinline__ void DecodeKeys(
			SmemStorage		&smem_storage,							// Shared memory storage
			UnsignedBits 	(&keys)[KEYS_PER_THREAD],				// Key to decode
			DigitCounter 	(&thread_prefixes)[KEYS_PER_THREAD],	// Prefix counter value (out parameter)
			unsigned int 	(&counter_offsets)[KEYS_PER_THREAD])	// Counter smem offset (out parameter)
		{
			// Compute byte offset of smem counter, starting with offset
			// of the thread's packed counter column
			counter_offsets[COUNT] = (threadIdx.x << (LOG_PACKING_RATIO + LOG_BYTES_PER_COUNTER));

			// Add in sub-counter offset
			counter_offsets[COUNT] = Extract<
				CURRENT_BIT + LOG_COUNTER_LANES,
				LOG_PACKING_RATIO,
				LOG_BYTES_PER_COUNTER>(
					keys[COUNT],
					counter_offsets[COUNT]);

			// Add in row offset
			counter_offsets[COUNT] = Extract<
				CURRENT_BIT,
				LOG_COUNTER_LANES,
				LOG_CTA_THREADS + LOG_PACKING_RATIO + LOG_BYTES_PER_COUNTER>(
					keys[COUNT],
					counter_offsets[COUNT]);

			// Pointer to smem digit counter
			DigitCounter* counter = (DigitCounter*) (smem_storage.digit_counter_bytes + counter_offsets[COUNT]);

			// Load thread-exclusive prefix
			thread_prefixes[COUNT] = *counter;

			// Store inclusive prefix
			*counter = thread_prefixes[COUNT] + 1;

			// Iterate next key
			Iterate<COUNT + 1, MAX>::DecodeKeys(smem_storage, keys, thread_prefixes, counter_offsets);
		}


		/**
		 * Compute rank.  Returns the rank of a key given its shared memory
		 * counter offset and its prefix counter value.
		 */
		template <int KEYS_PER_THREAD>
		static __device__ __forceinline__ void UpdateRanks(
			SmemStorage		&smem_storage,							// Shared memory storage
			unsigned int 	(&ranks)[KEYS_PER_THREAD],				// Local ranks (out parameter)
			DigitCounter 	(&thread_prefixes)[KEYS_PER_THREAD],	// Prefix counter value
			unsigned int 	(&counter_offsets)[KEYS_PER_THREAD])	// Counter smem offset
		{
			// Determine counter address
			DigitCounter* counter = (DigitCounter*) (smem_storage.digit_counter_bytes + counter_offsets[COUNT]);

			// Add in CTA exclusive prefix
			ranks[COUNT] = thread_prefixes[COUNT] + *counter;

			// Iterate next key
			Iterate<COUNT + 1, MAX>::UpdateRanks(smem_storage, ranks, thread_prefixes, counter_offsets);
		}
	};

	/**
	 * Terminate iteration
	 */
	template <int MAX>
	struct Iterate<MAX, MAX>
	{
		// DecodeKeys
		template <typename UnsignedBits, int KEYS_PER_THREAD>
		static __device__ __forceinline__ void DecodeKeys(
			SmemStorage		&smem_storage,
			UnsignedBits 	(&keys)[KEYS_PER_THREAD],
			DigitCounter 	(&thread_prefixes)[KEYS_PER_THREAD],
			unsigned int 	(&counter_offsets)[KEYS_PER_THREAD]) {}


		// UpdateRanks
		template <int KEYS_PER_THREAD>
		static __device__ __forceinline__ void UpdateRanks(
			SmemStorage		&smem_storage,
			unsigned int 	(&ranks)[KEYS_PER_THREAD],
			DigitCounter 	(&thread_prefixes)[KEYS_PER_THREAD],
			unsigned int 	(&counter_offsets)[KEYS_PER_THREAD]) {}
	};



	/**
	 * Scan shared memory digit counters.
	 */
	static __device__ __forceinline__ void ScanCounters(SmemStorage &smem_storage)
	{
		PackedCounter *raking_segment = smem_storage.raking_grid[threadIdx.x];

		// Upsweep reduce
		PackedCounter raking_partial = util::reduction::SerialReduce<RAKING_SEGMENT>::Invoke(
			raking_segment);

		int warp_id 						= threadIdx.x >> LOG_WARP_THREADS;
		int tid 							= threadIdx.x & (WARP_THREADS - 1);
		volatile PackedCounter *warpscan 	= &smem_storage.warpscan[warp_id][(WARP_THREADS / 2) + tid];

		// Initialize warpscan identity regions
		warpscan[0 - (WARP_THREADS / 2)] = 0;

		// Warpscan
		PackedCounter partial = raking_partial;
		warpscan[0] = partial;

		#pragma unroll
		for (int STEP = 0; STEP < LOG_WARP_THREADS; STEP++)
		{
			warpscan[0] = partial = partial + warpscan[0 - (1 << STEP)];
		}

		// Barrier
		__syncthreads();

		// Scan across warpscan totals
		PackedCounter warpscan_totals = 0;

		#pragma unroll
		for (int WARP = 0; WARP < WARPS; WARP++)
		{
			// Add totals from all previous warpscans into our partial
			PackedCounter warpscan_total = smem_storage.warpscan[WARP][(WARP_THREADS * 3 / 2) - 1];
			if (warp_id == WARP) {
				partial += warpscan_totals;
			}

			// Increment warpscan totals
			warpscan_totals += warpscan_total;
		}

		// Add lower totals from all warpscans into partial's upper
		#pragma unroll
		for (int PACKED = 1; PACKED < PACKING_RATIO; PACKED++)
		{
			partial += warpscan_totals << (sizeof(DigitCounter) * 8 * PACKED);
		}

		// Downsweep scan with exclusive partial
		PackedCounter exclusive_partial = partial - raking_partial;

		exclusive_partial = util::scan::SerialScan<RAKING_SEGMENT>::Invoke(
			raking_segment,
			exclusive_partial);
	}


	//---------------------------------------------------------------------
	// Interface
	//---------------------------------------------------------------------

	/**
	 * Rank keys.
	 */
	template <
		typename UnsignedBits,
		int KEYS_PER_THREAD>
	static __device__ __forceinline__ void RankKeys(
		SmemStorage		&smem_storage,						// Shared memory storage
		UnsignedBits	(&keys)[KEYS_PER_THREAD],			// Keys for this tile
		unsigned int 	(&ranks)[KEYS_PER_THREAD])			// For each key, the local rank within the tile
	{
		DigitCounter	thread_prefixes[KEYS_PER_THREAD];	// For each key, the count of previous keys in this tile having the same digit
		unsigned int 	counter_offsets[KEYS_PER_THREAD];	// For each key, the byte-offset of its corresponding digit counter in smem

		// Reset shared memory digit counters
		ResetCounters(smem_storage);

		// Decode keys and update digit counters
		Iterate<0, KEYS_PER_THREAD>::DecodeKeys(smem_storage, keys, thread_prefixes, counter_offsets);

		__syncthreads();

		// Scan shared memory counters
		ScanCounters(smem_storage);

		__syncthreads();

		// Extract the local ranks of each key
		Iterate<0, KEYS_PER_THREAD>::UpdateRanks(smem_storage, ranks, thread_prefixes, counter_offsets);
	}


	/**
	 * Rank keys.  For the lower RADIX_DIGITS threads, digit counts for each
	 * digit are provided for the corresponding thread-id.
	 */
	template <
		typename UnsignedBits,
		int KEYS_PER_THREAD>
	static __device__ __forceinline__ void RankKeys(
		SmemStorage		&smem_storage,						// Shared memory storage
		UnsignedBits	(&keys)[KEYS_PER_THREAD],			// Keys for this tile
		unsigned int 	(&ranks)[KEYS_PER_THREAD],			// For each key, the local rank within the tile (out parameter)
		unsigned int 	digit_prefixes[RADIX_DIGITS])
	{
		// Rank keys
		RankKeys(smem_storage, keys, ranks);

		// Get the inclusive and exclusive digit totals corresponding to the calling thread.
		if ((CTA_THREADS == RADIX_DIGITS) || (threadIdx.x < RADIX_DIGITS))
		{
			// Initialize digit scan's identity value
			digit_prefixes[threadIdx.x] = 0;

			// Obtain ex/inclusive digit counts.  (Unfortunately these all reside in the
			// first counter column, resulting in unavoidable bank conflicts.)
			int counter_lane = (threadIdx.x & (COUNTER_LANES - 1));
			int sub_counter = threadIdx.x >> (LOG_COUNTER_LANES);
			digit_prefixes[threadIdx.x + 1] = smem_storage.digit_counters[counter_lane + 1][0][sub_counter];
		}
	}
};




} // namespace radix_sort
} // namespace b40c
B40C_NS_POSTFIX
