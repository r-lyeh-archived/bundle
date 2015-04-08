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
 * Radix sorting problem instance
 ******************************************************************************/

#pragma once

#include "../util/spine.cuh"
#include "../util/basic_utils.cuh"
#include "../util/kernel_props.cuh"
#include "../util/error_utils.cuh"
#include "../util/cta_work_distribution.cuh"
#include "../util/ns_umbrella.cuh"

#include "../radix_sort/sort_utils.cuh"
#include "../radix_sort/pass_policy.cuh"

#include "../radix_sort/upsweep/kernel_policy.cuh"
#include "../radix_sort/upsweep/kernel.cuh"

#include "../radix_sort/spine/kernel_policy.cuh"
#include "../radix_sort/spine/kernel.cuh"

#include "../radix_sort/downsweep/kernel_policy.cuh"
#include "../radix_sort/downsweep/kernel.cuh"
#include "../radix_sort/downsweep/tex_ref.cuh"

B40C_NS_PREFIX
namespace b40c {
namespace radix_sort {



/******************************************************************************
 * Problem instance
 ******************************************************************************/

/**
 * Problem instance
 */
template <
	typename DoubleBuffer,
	typename _SizeT>
struct ProblemInstance
{
	//---------------------------------------------------------------------
	// Type definitions
	//---------------------------------------------------------------------

	typedef typename DoubleBuffer::KeyType 					KeyType;
	typedef typename DoubleBuffer::ValueType 				ValueType;
	typedef _SizeT 											SizeT;

	/**
	 * Upsweep kernel properties
	 */
	struct UpsweepKernelProps : util::KernelProps
	{
		// Upsweep kernel function type
		typedef void (*KernelFunc)(
			SizeT*,
			KeyType*,
			KeyType*,
			util::CtaWorkDistribution<SizeT>);

		// Fields
		KernelFunc 					kernel_func;
		int 						log_tile_elements;
		cudaSharedMemConfig 		sm_bank_config;

		/**
		 * Initializer
		 */
		template <
			typename KernelPolicy,
			typename OpaquePolicy>
		cudaError_t Init(int sm_arch, int sm_count)
		{
			// Initialize fields
			kernel_func 			= upsweep::Kernel<OpaquePolicy>;
			log_tile_elements 		= KernelPolicy::LOG_TILE_ELEMENTS;
			sm_bank_config 			= KernelPolicy::SMEM_CONFIG;

			// Initialize super class
			return util::KernelProps::Init(
				kernel_func,
				KernelPolicy::CTA_THREADS,
				sm_arch,
				sm_count);
		}

		/**
		 * Initializer
		 */
		template <typename KernelPolicy>
		cudaError_t Init(int sm_arch, int sm_count)
		{
			return Init<KernelPolicy, KernelPolicy>(sm_arch, sm_count);
		}
	};


	/**
	 * Spine kernel properties
	 */
	struct SpineKernelProps : util::KernelProps
	{
		// Spine kernel function type
		typedef void (*KernelFunc)(SizeT*, SizeT*, int);

		// Fields
		KernelFunc 					kernel_func;
		int 						log_tile_elements;
		cudaSharedMemConfig 		sm_bank_config;

		/**
		 * Initializer
		 */
		template <
			typename KernelPolicy,
			typename OpaquePolicy>
		cudaError_t Init(int sm_arch, int sm_count)
		{
			// Initialize fields
			kernel_func 			= spine::Kernel<OpaquePolicy>;
			log_tile_elements 		= KernelPolicy::LOG_TILE_ELEMENTS;
			sm_bank_config 			= KernelPolicy::SMEM_CONFIG;

			// Initialize super class
			return util::KernelProps::Init(
				kernel_func,
				KernelPolicy::CTA_THREADS,
				sm_arch,
				sm_count);
		}

		/**
		 * Initializer
		 */
		template <typename KernelPolicy>
		cudaError_t Init(int sm_arch, int sm_count)
		{
			return Init<KernelPolicy, KernelPolicy>(sm_arch, sm_count);
		}
	};


	/**
	 * Downsweep kernel props
	 */
	struct DownsweepKernelProps : util::KernelProps
	{
		// Downsweep kernel function type
		typedef void (*KernelFunc)(
			SizeT*,
			KeyType*,
			KeyType*,
			ValueType*,
			ValueType*,
			util::CtaWorkDistribution<SizeT>);

		// Downsweep texture binding function type
		typedef cudaError_t (*BindTexFunc)(void *, void *, size_t);

		// Fields
		KernelFunc 					kernel_func;
		BindTexFunc					keys_tex_func;
		BindTexFunc					values_tex_func;
		int 						log_tile_elements;
		cudaSharedMemConfig 		sm_bank_config;

		/**
		 * Initializer
		 */
		template <
			typename KernelPolicy,
			typename OpaquePolicy>
		cudaError_t Init(int sm_arch, int sm_count)
		{
			// Wrapper of downsweep texture types
			typedef downsweep::Textures<
				KeyType,
				ValueType,
				(1 << KernelPolicy::LOG_THREAD_ELEMENTS)> DownsweepTextures;

			// Key texture type
			typedef typename DownsweepTextures::KeyTexType KeyTexType;

			// Value texture type
			typedef typename DownsweepTextures::ValueTexType ValueTexType;

			// Initialize fields
			kernel_func 			= downsweep::Kernel<OpaquePolicy>;
			keys_tex_func 			= (KernelPolicy::LOAD_MODIFIER == util::io::ld::tex) ?
											downsweep::TexKeys<KeyTexType>::BindTexture :
											NULL;
			values_tex_func 		= (KernelPolicy::LOAD_MODIFIER == util::io::ld::tex) ?
											downsweep::TexValues<ValueTexType>::BindTexture :
											NULL;
			log_tile_elements 		= KernelPolicy::LOG_TILE_ELEMENTS;
			sm_bank_config 			= KernelPolicy::SMEM_CONFIG;

			// Initialize super class
			return util::KernelProps::Init(
				kernel_func,
				KernelPolicy::CTA_THREADS,
				sm_arch,
				sm_count);
		}

		/**
		 * Initializer
		 */
		template <typename KernelPolicy>
		cudaError_t Init(int sm_arch, int sm_count)
		{
			return Init<KernelPolicy, KernelPolicy>(sm_arch, sm_count);
		}

		/**
		 * Bind related textures
		 */
		cudaError_t BindTexture(
			KeyType *d_keys0,
			KeyType *d_keys1,
			ValueType *d_values0,
			ValueType *d_values1,
			SizeT num_elements) const
		{
			cudaError_t error = cudaSuccess;
			do {
				// Bind key texture
				if (keys_tex_func) error = keys_tex_func(d_keys0, d_keys1, sizeof(KeyType) * num_elements);
				if (error) break;

				// Bind value texture
				if (values_tex_func) error = values_tex_func(d_values0, d_values1, sizeof(ValueType) * num_elements);
				if (error) break;

			} while (0);

			return error;
		}
	};


	//---------------------------------------------------------------------
	// Fields
	//---------------------------------------------------------------------

	DoubleBuffer		&storage;
	SizeT				num_elements;

	util::Spine			&spine;
	cudaStream_t		stream;
	int			 		max_grid_size;
	bool				debug;


	//---------------------------------------------------------------------
	// Methods
	//---------------------------------------------------------------------

	/**
	 * Constructor
	 */
	ProblemInstance(
		DoubleBuffer	&storage,
		SizeT			num_elements,
		cudaStream_t	stream,
		util::Spine		&spine,
		int			 	max_grid_size,
		bool			debug) :
			storage(storage),
			num_elements(num_elements),
			stream(stream),
			spine(spine),
			max_grid_size(max_grid_size),
			debug(debug)
	{}


	/**
	 * DispatchPass
	 */
	cudaError_t DispatchPass(
		int 							radix_bits,
		const UpsweepKernelProps 		&upsweep_props,
		const SpineKernelProps			&spine_props,
		const DownsweepKernelProps		&downsweep_props,
		bool							unform_grid_size,
		DynamicSmemConfig				dynamic_smem_config,
		int 							current_pass = 0)
	{
		cudaError_t error = cudaSuccess;

		do {
			// Compute sweep grid size
			int log_schedule_granularity = CUB_MAX(
				upsweep_props.log_tile_elements,
				downsweep_props.log_tile_elements);
			int sweep_grid_size = downsweep_props.OversubscribedGridSize(
				1 << log_schedule_granularity,
				num_elements,
				max_grid_size);

			// Compute spine elements (rounded up to nearest tile size)
			SizeT spine_elements = CUB_ROUND_UP_NEAREST(
				(sweep_grid_size << radix_bits),			// Each CTA produces a partial for every radix digit
				(1 << spine_props.log_tile_elements));		// Number of partials per tile

			// Make sure our spine is big enough
			error = spine.Setup(sizeof(SizeT) * spine_elements);
			if (error) break;

			// Bind downsweep textures on first pass
			if (current_pass == 0)
			{
				error = downsweep_props.BindTexture(
					storage.d_keys[storage.selector],
					storage.d_keys[storage.selector ^ 1],
					storage.d_values[storage.selector],
					storage.d_values[storage.selector ^ 1],
					num_elements);
				if (error) break;
			}

			// Obtain a CTA work distribution
			util::CtaWorkDistribution<SizeT> work(
				num_elements,
				sweep_grid_size,
				log_schedule_granularity);

			// Grid size tuning
			int grid_size[3] = {sweep_grid_size, 1, sweep_grid_size};
			if (unform_grid_size)
			{
				// Make sure that all kernels launch the same number of CTAs
				grid_size[1] = grid_size[0];
			}

			// Smem allocation tuning
			int dynamic_smem[3] = {0, 0, 0};

			if (dynamic_smem_config == DYNAMIC_SMEM_UNIFORM)
			{
				// Pad with dynamic smem so all kernels get the same total smem allocation
				int max_static_smem = CUB_MAX(
					upsweep_props.kernel_attrs.sharedSizeBytes,
					CUB_MAX(
						spine_props.kernel_attrs.sharedSizeBytes,
						downsweep_props.kernel_attrs.sharedSizeBytes));

				dynamic_smem[0] = max_static_smem - upsweep_props.kernel_attrs.sharedSizeBytes;
				dynamic_smem[1] = max_static_smem - spine_props.kernel_attrs.sharedSizeBytes;
				dynamic_smem[2] = max_static_smem - downsweep_props.kernel_attrs.sharedSizeBytes;
			}
			else if (dynamic_smem_config == DYNAMIC_SMEM_LCM)
			{
				// Pad upsweep/downsweep with dynamic smem so kernel occupancy a multiple of the lowest occupancy
				int min_occupancy = CUB_MIN(upsweep_props.max_cta_occupancy, downsweep_props.max_cta_occupancy);
				dynamic_smem[0] = upsweep_props.SmemPadding(min_occupancy);
				dynamic_smem[2] = downsweep_props.SmemPadding(min_occupancy);
			}

			// Print debug info
			if (debug)
			{
				work.Print();
				printf(
					"Upsweep:   tile size(%d), occupancy(%d), grid_size(%d), threads(%d), dynamic smem(%d)\n"
					"Spine:     tile size(%d), occupancy(%d), grid_size(%d), threads(%d), dynamic smem(%d)\n"
					"Downsweep: tile size(%d), occupancy(%d), grid_size(%d), threads(%d), dynamic smem(%d)\n",
					(1 << upsweep_props.log_tile_elements), upsweep_props.max_cta_occupancy, grid_size[0], upsweep_props.threads, dynamic_smem[0],
					(1 << spine_props.log_tile_elements), spine_props.max_cta_occupancy, grid_size[1], spine_props.threads, dynamic_smem[1],
					(1 << downsweep_props.log_tile_elements), downsweep_props.max_cta_occupancy, grid_size[2], downsweep_props.threads, dynamic_smem[2]);
				fflush(stdout);
			}

			//
			// Upsweep
			//

			// Set shared mem bank mode
			cudaSharedMemConfig old_sm_config;
			cudaDeviceGetSharedMemConfig(&old_sm_config);
			if (old_sm_config != upsweep_props.sm_bank_config)
				cudaDeviceSetSharedMemConfig(upsweep_props.sm_bank_config);

			// Upsweep reduction into spine
			upsweep_props.kernel_func<<<grid_size[0], upsweep_props.threads, dynamic_smem[0], stream>>>(
				(SizeT*) spine(),
				storage.d_keys[storage.selector],
				storage.d_keys[storage.selector ^ 1],
				work);

			if (debug) {
				error = cudaThreadSynchronize();
				if (error = util::B40CPerror(error, "Upsweep kernel failed ", __FILE__, __LINE__)) break;
			}

			//
			// Spine
			//

			// Set shared mem bank mode
			if (spine_props.sm_bank_config != upsweep_props.sm_bank_config)
				cudaDeviceSetSharedMemConfig(spine_props.sm_bank_config);

			// Spine scan
			spine_props.kernel_func<<<grid_size[1], spine_props.threads, dynamic_smem[1], stream>>>(
				(SizeT*) spine(),
				(SizeT*) spine(),
				spine_elements);

			if (debug) {
				error = cudaThreadSynchronize();
				if (error = util::B40CPerror(error, "Spine kernel failed ", __FILE__, __LINE__)) break;
			}

			//
			// Downsweep
			//

			// Set shared mem bank mode
			if (downsweep_props.sm_bank_config != spine_props.sm_bank_config)
				cudaDeviceSetSharedMemConfig(downsweep_props.sm_bank_config);

			// Downsweep scan from spine
			downsweep_props.kernel_func<<<grid_size[2], downsweep_props.threads, dynamic_smem[2], stream>>>(
				(SizeT *) spine(),
				storage.d_keys[storage.selector],
				storage.d_keys[storage.selector ^ 1],
				storage.d_values[storage.selector],
				storage.d_values[storage.selector ^ 1],
				work);

			if (debug) {
				error = cudaThreadSynchronize();
				if (error = util::B40CPerror(error, "Downsweep kernel failed ", __FILE__, __LINE__)) break;
			}

			// Restore smem bank mode
			if (old_sm_config != downsweep_props.sm_bank_config)
				cudaDeviceSetSharedMemConfig(old_sm_config);

		} while(0);

		return error;
	}
};




} // namespace radix_sort
} // namespace b40c
B40C_NS_POSTFIX
