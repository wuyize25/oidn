// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../device.h"

namespace oidn {

#if defined(OIDN_COMPILE_CUDA)
  // Main kernel functions
  namespace
  {
    template<typename F>
    __global__ void basicCUDAKernel(WorkDim<1> globalSize, const F f)
    {
      WorkItem<1> it(globalSize);
      if (it.getId() < it.getRange())
        f(it);
    }

    template<typename F>
    __global__ void basicCUDAKernel(WorkDim<2> globalSize, const F f)
    {
      WorkItem<2> it(globalSize);
      if (it.getId<0>() < it.getRange<0>() &&
          it.getId<1>() < it.getRange<1>())
        f(it);
    }

    template<typename F>
    __global__ void basicCUDAKernel(WorkDim<3> globalSize, const F f)
    {
      WorkItem<3> it(globalSize);
      if (it.getId<0>() < it.getRange<0>() &&
          it.getId<1>() < it.getRange<1>() &&
          it.getId<2>() < it.getRange<2>())
        f(it);
    }

    template<int N, typename F>
    __global__ void groupCUDAKernel(const F f)
    {
      f(WorkGroupItem<N>());
    }
  }

  void checkError(cudaError_t error);
#endif

  class CUDADevice final : public Device
  { 
  public:
    void wait() override;

    // Ops
    std::shared_ptr<Conv> newConv(const ConvDesc& desc) override;
    std::shared_ptr<ConcatConv> newConcatConv(const ConcatConvDesc& desc) override;
    std::shared_ptr<Pool> newPool(const PoolDesc& desc) override;
    std::shared_ptr<Upsample> newUpsample(const UpsampleDesc& desc) override;
    std::shared_ptr<Autoexposure> newAutoexposure(const ImageDesc& srcDesc) override;
    std::shared_ptr<InputProcess> newInputProcess(const InputProcessDesc& desc) override;
    std::shared_ptr<OutputProcess> newOutputProcess(const OutputProcessDesc& desc) override;
    std::shared_ptr<ImageCopy> newImageCopy() override;

    // Memory
    void* malloc(size_t byteSize, Storage storage) override;
    void free(void* ptr, Storage storage) override;
    void memcpy(void* dstPtr, const void* srcPtr, size_t byteSize) override;

  #if defined(OIDN_COMPILE_CUDA)
    // Enqueues a basic kernel
    template<int N, typename F>
    OIDN_INLINE void runKernelAsync(WorkDim<N> globalSize, const F& f)
    {
      // TODO: improve group size computation
      /*
      int minGridSize = 0, blockSize = 0;
      checkError(cudaOccupancyMaxPotentialBlockSize(
        &minGridSize, &blockSize, static_cast<void(*)(WorkDim<N>, const F)>(basicCUDAKernel)));
      */
      WorkDim<N> groupSize = suggestWorkGroupSize(globalSize);
      WorkDim<N> numGroups = ceil_div(globalSize, groupSize);

      basicCUDAKernel<<<numGroups, groupSize>>>(globalSize, f);
      checkError(cudaGetLastError());
    }

    // Enqueues a work-group kernel
    template<int N, typename F>
    OIDN_INLINE void runKernelAsync(WorkDim<N> numGroups, WorkDim<N> groupSize, const F& f)
    {
      groupCUDAKernel<N><<<numGroups, groupSize>>>(f);
      checkError(cudaGetLastError());
    }
  #endif

    // Enqueues a host function
    void runHostFuncAsync(std::function<void()>&& f) override;

    int getComputeCapability() const { return computeCapability; }

  private:
    void init() override;

    // FIXME
    WorkDim<1> suggestWorkGroupSize(WorkDim<1> globalSize) { return 256; }
    WorkDim<2> suggestWorkGroupSize(WorkDim<2> globalSize) { return {16, 16}; }
    WorkDim<3> suggestWorkGroupSize(WorkDim<3> globalSize) { return {1, 16, 16}; }

    int computeCapability;
  };

} // namespace oidn
