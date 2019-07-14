/* Copyright 2019 Stanford
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
 */

#ifndef _FLEXFLOW_RUNTIME_H_
#define _FLEXFLOW_RUNTIME_H_
#include "legion.h"
#include "config.h"
#include "initializer.h"
#include "accessor.h"
#include <cudnn.h>
#include <cuda_runtime.h>
#include <curand.h>
#include <cublas_v2.h>
#include <unistd.h>

using namespace Legion;

enum TaskIDs {
  TOP_LEVEL_TASK_ID,
  FF_INIT_TASK_ID,
  IMAGE_INIT_TASK_ID,
  LABEL_INIT_TASK_ID,
  LOAD_IMAGES_TASK_ID,
  NORMALIZE_IMAGES_TASK_ID,
  CONV2D_INIT_TASK_ID,
  CONV2D_INIT_PARA_TASK_ID,
  CONV2D_FWD_TASK_ID,
  CONV2D_BWD_TASK_ID,
  CONV2D_UPD_TASK_ID,
  POOL2D_INIT_TASK_ID,
  POOL2D_FWD_TASK_ID,
  POOL2D_BWD_TASK_ID,
  BATCHNORM_INIT_TASK_ID,
  BATCHNORM_INIT_PARA_TASK_ID,
  BATCHNORM_FWD_TASK_ID,
  BATCHNORM_BWD_TASK_ID,
  LINEAR_INIT_TASK_ID,
  LINEAR_INIT_PARA_TASK_ID,
  LINEAR_FWD_TASK_ID,
  LINEAR_BWD_TASK_ID,
  LINEAR_BWD2_TASK_ID,
  LINEAR_UPD_TASK_ID,
  FLAT_INIT_TASK_ID,
  FLAT_FWD_TASK_ID,
  FLAT_BWD_TASK_ID,
  SOFTMAX_INIT_TASK_ID,
  SOFTMAX_FWD_TASK_ID,
  SOFTMAX_BWD_TASK_ID,
  CONCAT_INIT_TASK_ID,
  CONCAT_FWD_TASK_ID,
  CONCAT_BWD_TASK_ID,
  DUMMY_TASK_ID,
  // Initializer
  GLOROT_INIT_TASK_ID,
  ZEROS_INIT_TASK_ID,
  UNIFORM_INIT_TASK_ID,
  NORMAL_INIT_TASK_ID,
};

enum ActiMode {
  AC_MODE_NONE,
  AC_MODE_RELU,
  AC_MODE_SIGMOID,
  AC_MODE_TANH,
};

enum PoolType {
  POOL_MAX,
  POOL_AVG,
};

enum FieldIDs {
  FID_DATA,
};

struct FFHandler {
  cudnnHandle_t dnn;
  cublasHandle_t blas;
  void *workSpace;
  size_t workSpaceSize;
};

struct Tensor {
  int numDim, adim[MAX_DIM], pdim[MAX_DIM];
  LogicalRegion region, region_grad;
  LogicalPartition part, part_grad;
};

class OpMeta {
public:
  OpMeta(FFHandler _handle) : handle(_handle) {};
public:
  FFHandler handle;
};

class FFModel;
class DataLoader;

class Op {
public:
  Op(std::string _name, Tensor input);
  Op(std::string _name, int num, Tensor* inputs);

  virtual void prefetch(const FFModel&);
  virtual void init(const FFModel&) = 0;
  virtual void forward(const FFModel&) = 0;
  virtual void backward(const FFModel&) = 0;
  virtual void update(const FFModel&) = 0;
public:
  char name[MAX_OPNAME];
  Tensor output;
  Tensor inputs[MAX_NUM_INPUTS];
  LogicalPartition input_lps[MAX_NUM_INPUTS];
  Tensor locals[MAX_NUM_LOCALS];
  OpMeta* meta[MAX_NUM_WORKERS];
  int numLocals;
};

class FFModel {
public:
  FFModel(FFConfig &config);

  // Add a 2D convolutional layer 
  Tensor conv2d(std::string name,
                Tensor input, int outChannels,
                int kernelH, int kernelW,
                int strideH, int strideW,
                int paddingH, int paddingW,
                bool relu = false);
  // Add a 2D pooling layer
  Tensor pool2d(std::string name,
                Tensor input,
                int kernelH, int kernelW,
                int strideH, int strideW,
                int paddingH, int paddingW,
                PoolType type = POOL_MAX, bool relu = true);
  // Add a batch_norm layer
  Tensor batch_norm(std::string name,
                    Tensor input,
                    bool relu = true);
  // Add a dense layer
  Tensor dense(std::string name,
               const Tensor& input,
               int outDim,
               ActiMode activation = AC_MODE_NONE,
               Initializer* kernel_initializer = NULL,
               Initializer* bias_initializer = NULL);
  // Add a linear layer
  Tensor linear(std::string name,
                const Tensor& input,
                int outChannels,
                ActiMode activation = AC_MODE_NONE,
                Initializer* kernel_initializer = NULL,
                Initializer* bias_initializer = NULL);
  // Add a concat layer
  Tensor concat(std::string name,
                int n, Tensor* tensors);
  // Add a flat layer
  Tensor flat(std::string name, Tensor input);
  // Add a softmax layer
  Tensor softmax(std::string name, Tensor input);

  void add_layers();
  void init_layers();
  void load_images(int batch_id);
  void prefetch();
  void forward();
  void backward();
  void update();
public:
  FFConfig config;
  Tensor inputImage, inputRaw, inputLabel;
  std::vector<Op*> layers;
  FFHandler handlers[MAX_NUM_WORKERS];
  DataLoader *dataLoader;
private:
  IndexSpace get_or_create_task_is(ParallelConfig pc);
  std::map<ParallelConfig, IndexSpace, ParaConfigCompare> taskIs;
};

class Conv2D : public Op {
public:
  Conv2D(std::string name, FFConfig config,
         Tensor input, IndexSpaceT<4> task_is,
         int inChannels, int outChannels,
         int kernelH, int kernelW,
         int strideH, int strideW,
         int paddingH, int paddingW,
         bool relu, bool first_layer);

  void init(const FFModel&);

  void forward(const FFModel&);

  void backward(const FFModel&);

  void update(const FFModel&);

  static OpMeta* init_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);
  static void init_para_task(const Task *task,
                             const std::vector<PhysicalRegion> &regions,
                             Context ctx, Runtime *runtime);

  static void forward_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void backward_task(const Task *task,
                            const std::vector<PhysicalRegion> &regions,
                            Context ctx, HighLevelRuntime *runtime);

  static void update_task(const Task *task,
                          const std::vector<PhysicalRegion> &regions,
                          Context ctx, HighLevelRuntime *runtime);
public:
  IndexSpaceT<4> task_is;
  int in_channels, out_channels;
  int kernel_h, kernel_w, stride_h, stride_w, padding_h, padding_w;
  bool relu, first_layer, profiling;
  int num_replica;
  float learning_rate;
};

class Conv2DMeta : public OpMeta {
public:
  Conv2DMeta(FFHandler handler) : OpMeta(handler) {};
  cudnnTensorDescriptor_t inputTensor, biasTensor, outputTensor;
  cudnnFilterDescriptor_t filterDesc;
  cudnnActivationDescriptor_t actiDesc;
  cudnnConvolutionDescriptor_t convDesc;
  cudnnConvolutionFwdAlgo_t fwdAlgo;
  cudnnConvolutionBwdFilterAlgo_t bwdFilterAlgo;
  cudnnConvolutionBwdDataAlgo_t bwdDataAlgo;
  bool relu, first_layer;
};

class Pool2D : public Op {
public:
  Pool2D(std::string name, FFConfig config,
         Tensor input, IndexSpaceT<4> part_is,
         int kernelH, int kernelW,
         int strideH, int strideW,
         int paddingH, int paddingW,
         PoolType type, bool relu);

  void init(const FFModel&);

  void forward(const FFModel&);

  void backward(const FFModel&);

  void update(const FFModel&);

  static OpMeta* init_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void forward_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void backward_task(const Task *task,
                            const std::vector<PhysicalRegion> &regions,
                            Context ctx, Runtime *runtime);

public:
  IndexSpaceT<4> task_is;
  int kernel_h, kernel_w, stride_h, stride_w, padding_h, padding_w;
  PoolType pool_type;
  bool relu, profiling;
};

class Pool2DMeta : public OpMeta {
public:
  Pool2DMeta(FFHandler handle) : OpMeta(handle) {};
  cudnnTensorDescriptor_t inputTensor, outputTensor;
  cudnnActivationDescriptor_t actiDesc;
  cudnnPoolingDescriptor_t poolDesc;
  bool relu;
};

class BatchNorm : public Op {
public:
  BatchNorm(std::string name, FFConfig config,
            Tensor input, IndexSpaceT<4> part_is,
            bool relu);

  void init(const FFModel&);

  void forward(const FFModel&);

  void backward(const FFModel&);

  void update(const FFModel&);

  static OpMeta* init_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);
  static void init_para_task(const Task *task,
                             const std::vector<PhysicalRegion> &regions,
                             Context ctx, Runtime *runtime);

  static void forward_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void backward_task(const Task *task,
                            const std::vector<PhysicalRegion> &regions,
                            Context ctx, Runtime *runtime);
public:
  IndexSpaceT<4> task_is;
  bool relu, profiling;
  int num_replica;
};

class BatchNormMeta : public OpMeta {
public:
  BatchNormMeta(FFHandler handle) : OpMeta(handle) {};
  cudnnTensorDescriptor_t inputTensor, outputTensor, biasTensor;
  cudnnActivationDescriptor_t actiDesc;
  cudnnBatchNormMode_t mode;
  float *runningMean, *runningVar, *saveMean, *saveVar;
  bool relu;
};

class Linear : public Op {
public:
  Linear(std::string name,
         const FFConfig& config,
         const Tensor& input,
         const IndexSpaceT<2>& part_is,
         int outChannels,
         ActiMode activation,
         Initializer* kernel_initializer,
         Initializer* bias_initializer);

  void init(const FFModel&);

  void forward(const FFModel&);

  void backward(const FFModel&);

  void update(const FFModel&);

  static OpMeta* init_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);
  static void init_para_task(const Task *task,
                             const std::vector<PhysicalRegion> &regions,
                             Context ctx, Runtime *runtime);

  static void forward_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void backward_task(const Task *task,
                            const std::vector<PhysicalRegion> &regions,
                            Context ctx, Runtime *runtime);

  static void backward2_task(const Task *task,
                            const std::vector<PhysicalRegion> &regions,
                            Context ctx, Runtime *runtime);

  static void update_task(const Task *task,
                          const std::vector<PhysicalRegion> &regions,
                          Context ctx, Runtime *runtime);
public:
  IndexSpaceT<2> task_is;
  LogicalPartition replica_sub_lps[MAX_NUM_WORKERS];
  bool profiling;
  int num_replica, fc_num_par_c;
  ActiMode activation;
};

class LinearMeta : public OpMeta {
public:
  LinearMeta(FFHandler handle) : OpMeta(handle) {};
  cudnnTensorDescriptor_t outputTensor;
  cudnnActivationDescriptor_t actiDesc;
  int in_channels, out_channels, batch_size;
  ActiMode activation;
  float *one_ptr, *pre_relu;
};

class Flat : public Op {
public:
  Flat(std::string name, FFConfig config,
       Tensor input,
       IndexSpaceT<4> part_is_3d,
       IndexSpaceT<2> part_is_2d);

  void init(const FFModel&);

  void forward(const FFModel&);

  void backward(const FFModel&);

  void update(const FFModel&);

  static OpMeta* init_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void forward_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void backward_task(const Task *task,
                            const std::vector<PhysicalRegion> &regions,
                            Context ctx, Runtime *runtime);
public:
  IndexSpaceT<3> task_is_3d;
  IndexSpaceT<2> task_is_2d;
  LogicalPartition flat_lp, flat_grad_lp;
};

class FlatMeta : public OpMeta {
public:
  FlatMeta(FFHandler handle) : OpMeta(handle) {};
};

class Softmax : public Op {
public:
  Softmax(std::string name, FFConfig config,
          Tensor input, IndexSpaceT<1> part_is);

  void init(const FFModel&);

  void forward(const FFModel&);

  void backward(const FFModel&);

  void update(const FFModel&);

  static OpMeta* init_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void forward_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void backward_task(const Task *task,
                            const std::vector<PhysicalRegion> &regions,
                            Context ctx, Runtime *runtime);
public:
  IndexSpaceT<1> task_is;
  LogicalPartition input_grad_lp;
  bool profiling;
};

class SoftmaxMeta : public OpMeta {
public:
  SoftmaxMeta(FFHandler handle) : OpMeta(handle) {};
#ifndef DISABLE_COMPUTATION
  cudnnTensorDescriptor_t inputTensor;
#endif
};

class Concat : public Op {
public:
  Concat(std::string name, FFConfig config,
         int n, Tensor* inputs, IndexSpaceT<3> part_is);

  void init(const FFModel&);

  void forward(const FFModel&);

  void backward(const FFModel&);

  void update(const FFModel&);

  static OpMeta* init_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void forward_task(const Task *task,
                           const std::vector<PhysicalRegion> &regions,
                           Context ctx, Runtime *runtime);

  static void backward_task(const Task *task,
                            const std::vector<PhysicalRegion> &regions,
                            Context ctx, Runtime *runtime);
public:
  int num_inputs;
  IndexSpaceT<3> task_is;
};

class ConcatMeta : public OpMeta {
public:
  ConcatMeta(FFHandler handle) : OpMeta(handle) {};
};

class UtilityTasks {
public:
  static FFHandler init_cuda_task(const Task *task,
                                  const std::vector<PhysicalRegion> &regions,
                                  Context ctx, Runtime *runtime);
  static void dummy_task(const Task *task,
                         const std::vector<PhysicalRegion> &regions,
                         Context ctx, Runtime *runtime);
  static void init_images_task(const Task *task,
                               const std::vector<PhysicalRegion> &regions,
                               Context ctx, Runtime *runtime);
  static void init_labels_task(const Task *task,
                               const std::vector<PhysicalRegion> &regions,
                               Context ctx, Runtime *runtime);
  static void load_images_task(const Task *task,
                               const std::vector<PhysicalRegion> &regions,
                               Context ctx, Runtime *runtime);
  static void normalize_images_task(const Task *task,
                                    const std::vector<PhysicalRegion> &regions,
                                    Context ctx, Runtime *runtime);
};

struct Sample {
  int label;
  char file[MAX_FILE_LENGTH];
};

struct DataLoadMeta {
  int numSamples;
  Sample samples[MAX_SAMPLES_PER_LOAD];
};

// class DataLoader
class DataLoader {
public:
  DataLoader(std::string);
  bool get_samples(int numSamples, DataLoadMeta &meta);
  bool shuffle_samples(void);
public:
  std::vector<Sample> samples;
  std::vector<Sample>::const_iterator sampleIter;
};

#endif//_FLEXFLOW_RUNTIME_H_

