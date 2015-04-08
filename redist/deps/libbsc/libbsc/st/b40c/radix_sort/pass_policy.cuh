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
 * Radix sort policy
 ******************************************************************************/

#pragma once

#include "../util/basic_utils.cuh"
#include "../util/io/modified_load.cuh"
#include "../util/io/modified_store.cuh"
#include "../util/ns_umbrella.cuh"

#include "../radix_sort/pass_policy.cuh"
#include "../radix_sort/upsweep/kernel_policy.cuh"
#include "../radix_sort/spine/kernel_policy.cuh"
#include "../radix_sort/downsweep/kernel_policy.cuh"

B40C_NS_PREFIX
namespace b40c {
namespace radix_sort {


/******************************************************************************
 * Dispatch policy
 ******************************************************************************/

/**
 * Alternative policies for how much dynamic smem should be allocated to each kernel
 */
enum DynamicSmemConfig
{
	DYNAMIC_SMEM_NONE,			// No dynamic smem for kernels
	DYNAMIC_SMEM_UNIFORM,		// Pad with dynamic smem so all kernels get the same total smem allocation
	DYNAMIC_SMEM_LCM,			// Pad with dynamic smem so kernel occupancy a multiple of the lowest occupancy
};


/**
 * Dispatch policy
 */
template <
	int					_TUNE_ARCH,
	int 				_RADIX_BITS,
	DynamicSmemConfig 	_DYNAMIC_SMEM_CONFIG,
	bool 				_UNIFORM_GRID_SIZE>
struct DispatchPolicy
{
	enum {
		TUNE_ARCH					= _TUNE_ARCH,
		RADIX_BITS					= _RADIX_BITS,
		UNIFORM_GRID_SIZE 			= _UNIFORM_GRID_SIZE,
	};

	static const DynamicSmemConfig 	DYNAMIC_SMEM_CONFIG = _DYNAMIC_SMEM_CONFIG;
};


/******************************************************************************
 * Pass policy
 ******************************************************************************/

/**
 * Pass policy
 */
template <
	typename 	_UpsweepPolicy,
	typename 	_SpinePolicy,
	typename 	_DownsweepPolicy,
	typename 	_DispatchPolicy>
struct PassPolicy
{
	typedef _UpsweepPolicy			UpsweepPolicy;
	typedef _SpinePolicy 			SpinePolicy;
	typedef _DownsweepPolicy 		DownsweepPolicy;
	typedef _DispatchPolicy 		DispatchPolicy;
};


/******************************************************************************
 * Tuned pass policy specializations
 ******************************************************************************/

/**
 * Problem size enumerations
 */
enum ProblemSize
{
	LARGE_PROBLEM,		// > 32K elements
	SMALL_PROBLEM		// <= 32K elements
};


/**
 * Tuned pass policy specializations
 */
template <
	int 			TUNE_ARCH,
	typename 		ProblemInstance,
	ProblemSize 	PROBLEM_SIZE,
	int 			BITS_REMAINING,
	int 			CURRENT_BIT,
	int 			CURRENT_PASS>
struct TunedPassPolicy;


/**
 * SM20
 */
template <typename ProblemInstance, ProblemSize PROBLEM_SIZE, int BITS_REMAINING, int CURRENT_BIT, int CURRENT_PASS>
struct TunedPassPolicy<200, ProblemInstance, PROBLEM_SIZE, BITS_REMAINING, CURRENT_BIT, CURRENT_PASS>
{
	enum {
		TUNE_ARCH			= 200,
		KEYS_ONLY 			= util::Equals<typename ProblemInstance::ValueType, util::NullType>::VALUE,
		PREFERRED_BITS		= 5,
		RADIX_BITS 			= CUB_MIN(BITS_REMAINING, (BITS_REMAINING % PREFERRED_BITS == 0) ? PREFERRED_BITS : PREFERRED_BITS - 1),
		EARLY_EXIT 			= false,
		LARGE_DATA			= (sizeof(typename ProblemInstance::KeyType) > 4) || (sizeof(typename ProblemInstance::ValueType) > 4),
	};

	// Dispatch policy
	typedef DispatchPolicy <
		TUNE_ARCH,								// TUNE_ARCH
		RADIX_BITS,								// RADIX_BITS
		DYNAMIC_SMEM_NONE, 						// UNIFORM_SMEM_ALLOCATION
		true> 									// UNIFORM_GRID_SIZE
			DispatchPolicy;

	// Upsweep kernel policy
	typedef upsweep::KernelPolicy<
		RADIX_BITS,								// RADIX_BITS
		CURRENT_BIT,							// CURRENT_BIT
		CURRENT_PASS,							// CURRENT_PASS
		8,										// MIN_CTA_OCCUPANCY
		7,										// LOG_CTA_THREADS
		((PROBLEM_SIZE == SMALL_PROBLEM) ? 		// LOG_LOAD_VEC_SIZE
			1 :
			(!LARGE_DATA ? 2 : 1)),
		1,										// LOG_LOADS_PER_TILE
		b40c::util::io::ld::NONE,				// LOAD_MODIFIER
		b40c::util::io::st::NONE,				// STORE_MODIFIER
		cudaSharedMemBankSizeFourByte,			// SMEM_CONFIG
		EARLY_EXIT>								// EARLY_EXIT
			UpsweepPolicy;

	// Spine-scan kernel policy
	typedef spine::KernelPolicy<
		8,										// LOG_CTA_THREADS
		2,										// LOG_LOAD_VEC_SIZE
		2,										// LOG_LOADS_PER_TILE
		b40c::util::io::ld::NONE,				// LOAD_MODIFIER
		b40c::util::io::st::NONE,				// STORE_MODIFIER
		cudaSharedMemBankSizeFourByte>			// SMEM_CONFIG
			SpinePolicy;

	// Downsweep kernel policy
	typedef downsweep::KernelPolicy<
		RADIX_BITS,								// RADIX_BITS
		CURRENT_BIT,							// CURRENT_BIT
		CURRENT_PASS,							// CURRENT_PASS
		(KEYS_ONLY && !LARGE_DATA ? 4 : 2), 	// MIN_CTA_OCCUPANCY
		((PROBLEM_SIZE == SMALL_PROBLEM) ?		// LOG_CTA_THREADS
			7 :
			(KEYS_ONLY && !LARGE_DATA ? 7 : 8)),
		((PROBLEM_SIZE == SMALL_PROBLEM) ?		// LOG_THREAD_ELEMENTS
			2 :
			(!LARGE_DATA ? 4 : 3)),
		((PROBLEM_SIZE == SMALL_PROBLEM) ? 		// LOAD_MODIFIER
			b40c::util::io::ld::NONE :
			b40c::util::io::ld::tex),
		b40c::util::io::st::NONE,				// STORE_MODIFIER
		downsweep::SCATTER_TWO_PHASE,			// SCATTER_STRATEGY
		cudaSharedMemBankSizeFourByte,			// SMEM_CONFIG
		EARLY_EXIT>								// EARLY_EXIT
			DownsweepPolicy;
};


/**
 * SM13
 */
template <typename ProblemInstance, ProblemSize PROBLEM_SIZE, int BITS_REMAINING, int CURRENT_BIT, int CURRENT_PASS>
struct TunedPassPolicy<130, ProblemInstance, PROBLEM_SIZE, BITS_REMAINING, CURRENT_BIT, CURRENT_PASS>
{
	enum {
		TUNE_ARCH			= 130,
		KEYS_ONLY 			= util::Equals<typename ProblemInstance::ValueType, util::NullType>::VALUE,
		PREFERRED_BITS		= 5,
		RADIX_BITS 			= CUB_MIN(BITS_REMAINING, (BITS_REMAINING % PREFERRED_BITS == 0) ? PREFERRED_BITS : PREFERRED_BITS - 1),
		EARLY_EXIT 			= false,
		LARGE_DATA			= (sizeof(typename ProblemInstance::KeyType) > 4) || (sizeof(typename ProblemInstance::ValueType) > 4),
	};

	// Dispatch policy
	typedef DispatchPolicy <
		TUNE_ARCH,								// TUNE_ARCH
		RADIX_BITS,								// RADIX_BITS
		DYNAMIC_SMEM_UNIFORM, 					// UNIFORM_SMEM_ALLOCATION
		true> 									// UNIFORM_GRID_SIZE
			DispatchPolicy;

	// Upsweep kernel policy
	typedef upsweep::KernelPolicy<
		RADIX_BITS,								// RADIX_BITS
		CURRENT_BIT,							// CURRENT_BIT
		CURRENT_PASS,							// CURRENT_PASS
		(RADIX_BITS > 4 ? 3 : 6),				// MIN_CTA_OCCUPANCY
		7,										// LOG_CTA_THREADS
		0,										// LOG_LOAD_VEC_SIZE
		(!LARGE_DATA ? 2 : 1),					// LOG_LOADS_PER_TILE
		b40c::util::io::ld::NONE,				// LOAD_MODIFIER
		b40c::util::io::st::NONE,				// STORE_MODIFIER
		cudaSharedMemBankSizeFourByte,			// SMEM_CONFIG
		EARLY_EXIT>								// EARLY_EXIT
			UpsweepPolicy;

	// Spine-scan kernel policy
	typedef spine::KernelPolicy<
		8,										// LOG_CTA_THREADS
		2,										// LOG_LOAD_VEC_SIZE
		0,										// LOG_LOADS_PER_TILE
		b40c::util::io::ld::NONE,				// LOAD_MODIFIER
		b40c::util::io::st::NONE,				// STORE_MODIFIER
		cudaSharedMemBankSizeFourByte>			// SMEM_CONFIG
			SpinePolicy;

	// Downsweep kernel policy
	typedef downsweep::KernelPolicy<
		RADIX_BITS,								// RADIX_BITS
		CURRENT_BIT,							// CURRENT_BIT
		CURRENT_PASS,							// CURRENT_PASS
		(KEYS_ONLY ? 3 : 2),					// MIN_CTA_OCCUPANCY
		6,										// LOG_CTA_THREADS
		(!LARGE_DATA ? 4 : 3),					// LOG_THREAD_ELEMENTS
		b40c::util::io::ld::tex,				// LOAD_MODIFIER
		b40c::util::io::st::NONE,				// STORE_MODIFIER
		downsweep::SCATTER_TWO_PHASE,			// SCATTER_STRATEGY
		cudaSharedMemBankSizeFourByte,			// SMEM_CONFIG
		EARLY_EXIT>								// EARLY_EXIT
			DownsweepPolicy;
};


/**
 * SM10
 */
template <typename ProblemInstance, ProblemSize PROBLEM_SIZE, int BITS_REMAINING, int CURRENT_BIT, int CURRENT_PASS>
struct TunedPassPolicy<100, ProblemInstance, PROBLEM_SIZE, BITS_REMAINING, CURRENT_BIT, CURRENT_PASS>
{
	enum {
		TUNE_ARCH			= 100,
		KEYS_ONLY 			= util::Equals<typename ProblemInstance::ValueType, util::NullType>::VALUE,
		PREFERRED_BITS		= 5,
		RADIX_BITS 			= CUB_MIN(BITS_REMAINING, (BITS_REMAINING % PREFERRED_BITS == 0) ? PREFERRED_BITS : PREFERRED_BITS - 1),
		EARLY_EXIT 			= false,
		LARGE_DATA			= (sizeof(typename ProblemInstance::KeyType) > 4) || (sizeof(typename ProblemInstance::ValueType) > 4),
	};

	// Dispatch policy
	typedef DispatchPolicy <
		TUNE_ARCH,								// TUNE_ARCH
		RADIX_BITS,								// RADIX_BITS
		DYNAMIC_SMEM_LCM, 						// UNIFORM_SMEM_ALLOCATION
		true> 									// UNIFORM_GRID_SIZE
			DispatchPolicy;

	// Upsweep kernel policy
	typedef upsweep::KernelPolicy<
		RADIX_BITS,								// RADIX_BITS
		CURRENT_BIT,							// CURRENT_BIT
		CURRENT_PASS,							// CURRENT_PASS
		2,										// MIN_CTA_OCCUPANCY
		7,										// LOG_CTA_THREADS
		0,										// LOG_LOAD_VEC_SIZE
		0,										// LOG_LOADS_PER_TILE
		b40c::util::io::ld::NONE,				// LOAD_MODIFIER
		b40c::util::io::st::NONE,				// STORE_MODIFIER
		cudaSharedMemBankSizeFourByte,			// SMEM_CONFIG
		EARLY_EXIT>								// EARLY_EXIT
			UpsweepPolicy;

	// Spine-scan kernel policy
	typedef spine::KernelPolicy<
		8,										// LOG_CTA_THREADS
		2,										// LOG_LOAD_VEC_SIZE
		0,										// LOG_LOADS_PER_TILE
		b40c::util::io::ld::NONE,				// LOAD_MODIFIER
		b40c::util::io::st::NONE,				// STORE_MODIFIER
		cudaSharedMemBankSizeFourByte>			// SMEM_CONFIG
			SpinePolicy;

	// Downsweep kernel policy
	typedef downsweep::KernelPolicy<
		RADIX_BITS,								// RADIX_BITS
		CURRENT_BIT,							// CURRENT_BIT
		CURRENT_PASS,							// CURRENT_PASS
		2,										// MIN_CTA_OCCUPANCY
		6,										// LOG_CTA_THREADS
		(!LARGE_DATA ? 4 : 3),					// LOG_THREAD_ELEMENTS
		b40c::util::io::ld::tex,				// LOAD_MODIFIER
		b40c::util::io::st::NONE,				// STORE_MODIFIER
		downsweep::SCATTER_WARP_TWO_PHASE,		// SCATTER_STRATEGY
		cudaSharedMemBankSizeFourByte,			// SMEM_CONFIG
		EARLY_EXIT>								// EARLY_EXIT
			DownsweepPolicy;
};



}// namespace radix_sort
}// namespace b40c
B40C_NS_POSTFIX
