#include "cuda_helper.h"
#include "flexflow_dataloader.h"

void ImgDataLoader::load_label(const Task *task,
                               const std::vector<PhysicalRegion> &regions,
                               Context ctx,
                               Runtime* runtime)
{
  assert(regions.size() == 2);
  assert(task->regions.size() == 2);
  SampleIdxs* meta = (SampleIdxs*) task->local_args;
  TensorAccessorR<int, 2> acc_full_label(
      regions[0], task->regions[0], FID_DATA, ctx, runtime);
  TensorAccessorW<int, 2> acc_batch_label(
      regions[1], task->regions[1], FID_DATA, ctx, runtime, false/*readOutput*/);
  int batch_size = acc_batch_label.rect.hi[1] - acc_batch_label.rect.lo[1] + 1;
  //FIXME: currently assume continous indices
  assert(batch_size == meta->num_samples);
  for (int i = 1; i < batch_size; i++)
    assert(meta->idxs[i] == meta->idxs[0] + i);
  const int* input_zc = acc_full_label.ptr + meta->idxs[0];
  copy_kernel<<<GET_BLOCKS(acc_batch_label.rect.volume()), CUDA_NUM_THREADS>>>(
    acc_batch_label.ptr, input_zc, acc_batch_label.rect.volume());
  checkCUDA(cudaDeviceSynchronize());
}

void ImgDataLoader4D::load_input(const Task *task,
                                 const std::vector<PhysicalRegion> &regions,
                                 Context ctx,
                                 Runtime* runtime)
{
  assert(regions.size() == 2);
  assert(task->regions.size() == 2);
  SampleIdxs* meta = (SampleIdxs*) task->local_args;
  TensorAccessorR<float, 4> acc_full_input(
      regions[0], task->regions[0], FID_DATA, ctx, runtime);
  TensorAccessorW<float, 4> acc_batch_input(
      regions[1], task->regions[1], FID_DATA, ctx, runtime, false/*readOutput*/);
  int batch_size = acc_batch_input.rect.hi[3] - acc_batch_input.rect.lo[3] + 1;
  int channels = acc_batch_input.rect.hi[2] - acc_batch_input.rect.lo[2] + 1;
  int height = acc_batch_input.rect.hi[1] - acc_batch_input.rect.lo[1] + 1;
  int width = acc_batch_input.rect.hi[0] - acc_batch_input.rect.lo[0] + 1;
  //FIXME: currently assume continous indices
  assert(batch_size == meta->num_samples);
  for (int i = 1; i < batch_size; i++)
    assert(meta->idxs[i] == meta->idxs[0] + i);
  const float* input_zc = acc_full_input.ptr + meta->idxs[0] * channels * height * width;
  //printf("load input %d %d %d %d\n", meta->idxs[0], channels, height, width);
  copy_kernel<<<GET_BLOCKS(acc_batch_input.rect.volume()), CUDA_NUM_THREADS>>>(
      acc_batch_input.ptr, input_zc, acc_batch_input.rect.volume());
  checkCUDA(cudaDeviceSynchronize());
}

void ImgDataLoader2D::load_input(const Task *task,
                               const std::vector<PhysicalRegion> &regions,
                               Context ctx,
                               Runtime* runtime)
{
  assert(regions.size() == 2);
  assert(task->regions.size() == 2);
  SampleIdxs* meta = (SampleIdxs*) task->local_args;
  TensorAccessorR<float, 2> acc_full_input(
      regions[0], task->regions[0], FID_DATA, ctx, runtime);
  TensorAccessorW<float, 2> acc_batch_input(
      regions[1], task->regions[1], FID_DATA, ctx, runtime, false/*readOutput*/);
  int batch_size = acc_batch_input.rect.hi[1] - acc_batch_input.rect.lo[1] + 1;
  int width = acc_batch_input.rect.hi[0] - acc_batch_input.rect.lo[0] + 1;
  //FIXME: currently assume continous indices
  assert(batch_size == meta->num_samples);
  for (int i = 1; i < batch_size; i++)
    assert(meta->idxs[i] == meta->idxs[0] + i);
  const float* input_zc = acc_full_input.ptr + meta->idxs[0] * width;
  //printf("load input %d %d %d %d\n", meta->idxs[0], channels, height, width);
  copy_kernel<<<GET_BLOCKS(acc_batch_input.rect.volume()), CUDA_NUM_THREADS>>>(
      acc_batch_input.ptr, input_zc, acc_batch_input.rect.volume());
  checkCUDA(cudaDeviceSynchronize());
}

template<typename DT>
void SingleDataLoader::load_input_2d(const Task *task,
                                     const std::vector<PhysicalRegion> &regions,
                                     Context ctx,
                                     Runtime* runtime)
{
  assert(regions.size() == 2);
  assert(task->regions.size() == 2);
  SampleIdxs* meta = (SampleIdxs*) task->local_args;
  TensorAccessorR<DT, 2> acc_full_input(
      regions[0], task->regions[0], FID_DATA, ctx, runtime);
  TensorAccessorW<DT, 2> acc_batch_input(
      regions[1], task->regions[1], FID_DATA, ctx, runtime, false/*readOutput*/);
  int batch_size = acc_batch_input.rect.hi[1] - acc_batch_input.rect.lo[1] + 1;
  int width = acc_batch_input.rect.hi[0] - acc_batch_input.rect.lo[0] + 1;
  //FIXME: currently assume continous indices
  assert(batch_size == meta->num_samples);
  for (int i = 1; i < batch_size; i++)
    assert(meta->idxs[i] == meta->idxs[0] + i);
  const DT* input_zc = acc_full_input.ptr + meta->idxs[0] * width;
  //printf("load input %d %d %d %d\n", meta->idxs[0], channels, height, width);
  copy_kernel<DT><<<GET_BLOCKS(acc_batch_input.rect.volume()), CUDA_NUM_THREADS>>>(
      acc_batch_input.ptr, input_zc, acc_batch_input.rect.volume());
  checkCUDA(cudaDeviceSynchronize());
}

template<typename DT>
void SingleDataLoader::load_input_4d(const Task *task,
                                     const std::vector<PhysicalRegion> &regions,
                                     Context ctx,
                                     Runtime* runtime)
{
  assert(regions.size() == 2);
  assert(task->regions.size() == 2);
  SampleIdxs* meta = (SampleIdxs*) task->local_args;
  TensorAccessorR<DT, 4> acc_full_input(
      regions[0], task->regions[0], FID_DATA, ctx, runtime);
  TensorAccessorW<DT, 4> acc_batch_input(
      regions[1], task->regions[1], FID_DATA, ctx, runtime, false/*readOutput*/);
  int batch_size = acc_batch_input.rect.hi[3] - acc_batch_input.rect.lo[3] + 1;
  int channels = acc_batch_input.rect.hi[2] - acc_batch_input.rect.lo[2] + 1;
  int height = acc_batch_input.rect.hi[1] - acc_batch_input.rect.lo[1] + 1;
  int width = acc_batch_input.rect.hi[0] - acc_batch_input.rect.lo[0] + 1;
  //FIXME: currently assume continous indices
  assert(batch_size == meta->num_samples);
  for (int i = 1; i < batch_size; i++)
    assert(meta->idxs[i] == meta->idxs[0] + i);
  const DT* input_zc = acc_full_input.ptr + meta->idxs[0] * channels * height * width;
  //printf("load input %d %d %d %d\n", meta->idxs[0], channels, height, width);
  copy_kernel<DT><<<GET_BLOCKS(acc_batch_input.rect.volume()), CUDA_NUM_THREADS>>>(
      acc_batch_input.ptr, input_zc, acc_batch_input.rect.volume());
  checkCUDA(cudaDeviceSynchronize());
}

template void SingleDataLoader::load_input_4d<float>(const Task *task, const std::vector<PhysicalRegion> &regions, Context ctx, Runtime* runtime);
template void SingleDataLoader::load_input_2d<float>(const Task *task, const std::vector<PhysicalRegion> &regions, Context ctx, Runtime* runtime);
template void SingleDataLoader::load_input_2d<int>(const Task *task, const std::vector<PhysicalRegion> &regions, Context ctx, Runtime* runtime);