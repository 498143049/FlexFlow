/* Copyright 2017 Stanford, NVIDIA
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

#include <cstdio>
#include "ops.h"
#include "cnn_mapper.h"
#include "inception.h"
#define USE_INCEPTION

// Default Config Parameters
struct DefaultConfig {
  static int batchSize = 64;
  static int inputHeight = 224;
  static int inputWidth = 224;
  static bool profiling = false;
  static float learningRate = 0.01;
  static float weightDecay = 0.0001;
  static size_t workSpaceSize = 2 * 1024 * 1024 * 1024; // 2GB
  static int numNodes = 1;
  static int workersPerNode = 0;
  static int loadersPerNode = 4;
};

using namespace Legion;

LegionRuntime::Logger::Category log_ff("FF");

void parse_input_args(char **argv, int argc, FFConfig& config);
void parse_strategy_file(const std::string &filename, FFConfig& config);
void top_level_task(const Task *task,
                    const std::vector<PhysicalRegion> &regions,
                    Context ctx, Runtime *runtime)
{
  // Init config parameters
  FFConfig config;
  config.epochs = DefaultConfig::epochs;
  config.batchSize = DefaultConfig::batchSize;
  config.inputHeight = DefaultConfig::inputHeight;
  config.inputWidth = DefaultConfig::inputWidth;
  config.profiling = DefaultConfig::profiling;
  config.learningRate = DefaultConfig::learningRate;
  config.weightDecay = DefaultConfig::weightDecay;
  config.workSpaceSize = DefaultConfig::workSpaceSize;
  config.numNodes = DefaultConfig::numNodes;
  config.loadersPerNode = DefaultConfig::loadersPerNode;
  config.workersPerNode = DefaultConfig::workersPerNode;
  config.strategyFile = "";
  config.datasetPath = "";
  config.syntheticInput = false;
  // Parse input arguments
  {
    const InputArgs &command_args = HighLevelRuntime::get_input_args();
    char **argv = command_args.argv;
    int argc = command_args.argc;
    parse_input_args(argv, argc, config);
    
    log_ff.print("batchSize(%d) inputHeight(%d) inputWdith(%d)",
                 config.batchSize, config.inputHeight, config.inputWidth);
    log_ff.print("workersPerNode(%d) loadersPerNode(%d) numNodes(%d)",
                 config.workersPerNode, config.loadersPerNode, config.numNodes);
    if (config.datasetPath.length() == 0)
      log_ff.print("datasetPath(synthetic data)");
    else
      log_ff.print("datasetPath(%s)", config.datasetPath.c_str());
    if (config.strategyFile.length() == 0)
      log_ff.print("strategyFile(Default Data Parallelism)");
    else
      log_ff.print("strategyFile(%s)", config.strategyFile.c_str());
    if (config.workersPerNode == 0) {
      log_ff.print("Missing -ll:gpu (number of GPUs to use on each node)");
    }
  }
  // Parse strategy file
  if (parse_strategy_file(config.strategyFile, config))
  {
    log_ff.print("Error: cannot parse strategy file");
    return;
  }
  config.lg_ctx = ctx;
  config.lg_hlr = runtime;
  FFModel ff(config, ctx, runtime);
  // Init CUDA libraries on each worker
  ArgumentMap local_args;
  size_t workSpaceSize = config.workSpaceSize;
  Rect<3> task_rect(Point<1>(0),
                    Point<1>(config.workersPerNode * config.numNodes - 1));
  IndexSpaceT<3> task_is = runtime->create_index_space(ctx, rect);
  IndexLauncher initLauncher(CUDNN_INIT_TASK_ID, task_is,
                    TaskArgument(&workSpaceSize, sizeof(workSpaceSize)), local_args);
  FutureMap fm = runtime->execute_index_space(ctx, initLauncher);
  fm.wait_all_results();
  int idx;
  for (PointInRectIterator<1> it(rect); it(); it++) {
    ff.handlers[idx++] = fm.get_result<FFHandler>(*it);
  }
  ff.add_layers();
  // Initialize every layer
  ff.init_layers();

  double ts_start = Realm::Clock::current_time_in_microseconds();
  for (int i = 0; i < config.numIterations; i++) {
    ff.load_images();
    ff.prefetch();
    ff.forward();
    ff.backward();
    ff.update();
  }
  runtime->issue_execution_fence(ctx);
  TimingLauncher timer(MEASURE_MICRO_SECONDS);
  Future future = runtime->issue_timing_measurement(ctx, timer);
  future.get_void_result();
  double ts_end = Realm::Clock::current_time_in_microseconds();
  double run_time = 1e-6 * (ts_end - ts_start);
  printf("time = %.4fs, tp = %.2f images/s\n", run_time, num_images * num_iterations / run_time);
#ifdef OLD_CODE
  CnnModel model(num_images, height, width, num_par_n, num_par_h, num_par_w,
                 fc_num_par_n, fc_num_par_c, profiling, learning_rate,
                 num_loaders_per_node, num_nodes, ctx, runtime);
  // First, create cnnContexts
  ArgumentMap local_args;
  size_t workSpaceSize = (size_t) 2 * 1024 * 1024 * 1024;
  IndexLauncher init_launcher(CNN_INIT_TASK_ID, model.part_is,
                              TaskArgument(&workSpaceSize, sizeof(workSpaceSize)), local_args);
  FutureMap fm = runtime->execute_index_space(ctx, init_launcher);
  fm.wait_all_results();
  Rect<3> rect = runtime->get_index_space_domain(ctx, model.part_is);
  int idx = 0;
  for (PointInRectIterator<3> it(rect); it(); it++) {
    model.cnn_handlers[idx++] = fm.get_result<CnnHandle>(*it);
  }

  // Construct model (AlexNet)
#ifdef USE_ALEXNET
  Tensor t = model.add_conv_layer(model.input_image, 64, 11, 11, 4, 4, 2, 2);
  t = model.add_pool_layer(t, 3, 3, 2, 2, 0, 0);
  t = model.add_conv_layer(t, 192, 5, 5, 1, 1, 2, 2);
  t = model.add_pool_layer(t, 3, 3, 2, 2, 0, 0);
  t = model.add_conv_layer(t, 384, 3, 3, 1, 1, 1, 1);
  t = model.add_conv_layer(t, 256, 3, 3, 1, 1, 1, 1);
  t = model.add_conv_layer(t, 256, 3, 3, 1, 1, 1, 1);
  t = model.add_pool_layer(t, 3, 3, 2, 2, 0, 0);
  t = model.add_flat_layer(t);
  t = model.add_linear_layer(t, 4096);
  t = model.add_linear_layer(t, 4096);
  t = model.add_linear_layer(t, 1000, false/*relu*/);
  t = model.add_softmax_layer(t);
#endif

  // Construct model (VGG-16Net)
#ifdef USE_VGG
  Tensor t = model.add_conv_layer(model.input_image, 64, 3, 3, 1, 1, 1, 1);
  t = model.add_conv_layer(t, 64, 3, 3, 1, 1, 1, 1);
  t = model.add_pool_layer(t, 2, 2, 2, 2, 0, 0);
  t = model.add_conv_layer(t, 128, 3, 3, 1, 1, 1, 1);
  t = model.add_conv_layer(t, 128, 3, 3, 1, 1, 1, 1);
  t = model.add_pool_layer(t, 2, 2, 2, 2, 0, 0);
  t = model.add_conv_layer(t, 256, 3, 3, 1, 1, 1, 1);
  t = model.add_conv_layer(t, 256, 3, 3, 1, 1, 1, 1);
  t = model.add_conv_layer(t, 256, 3, 3, 1, 1, 1, 1);
  t = model.add_pool_layer(t, 2, 2, 2, 2, 0, 0);
  t = model.add_conv_layer(t, 512, 3, 3, 1, 1, 1, 1);
  t = model.add_conv_layer(t, 512, 3, 3, 1, 1, 1, 1);
  t = model.add_conv_layer(t, 512, 3, 3, 1, 1, 1, 1);
  t = model.add_pool_layer(t, 2, 2, 2, 2, 0, 0);
  t = model.add_conv_layer(t, 512, 3, 3, 1, 1, 1, 1);
  t = model.add_conv_layer(t, 512, 3, 3, 1, 1, 1, 1);
  t = model.add_conv_layer(t, 512, 3, 3, 1, 1, 1, 1);
  t = model.add_pool_layer(t, 2, 2, 2, 2, 0, 0);
  t = model.add_flat_layer(t);
  t = model.add_linear_layer(t, 4096);
  t = model.add_linear_layer(t, 4096);
  t = model.add_linear_layer(t, 1000, false/*relu*/);
  t = model.add_softmax_layer(t);
#endif

  // Construct model (Inception-V3)
#ifdef USE_INCEPTION
  Tensor t = model.add_conv_layer(model.input_image, 32, 3, 3, 2, 2, 0, 0);
  t = model.add_conv_layer(t, 32, 3, 3, 1, 1, 0, 0);
  t = model.add_conv_layer(t, 64, 3, 3, 1, 1, 1, 1);
  t = model.add_pool_layer(t, 3, 3, 2, 2, 0, 0);
  t = model.add_conv_layer(t, 80, 1, 1, 1, 1, 0, 0);
  t = model.add_conv_layer(t, 192, 3, 3, 1, 1, 1, 1);
  t = model.add_pool_layer(t, 3, 3, 2, 2, 0, 0);
  t = InceptionA(model, t, 32);
  t = InceptionA(model, t, 64);
  t = InceptionA(model, t, 64);
  t = InceptionB(model, t);
  t = InceptionC(model, t, 128);
  t = InceptionC(model, t, 160);
  t = InceptionC(model, t, 160);
  t = InceptionC(model, t, 192);
  t = InceptionD(model, t);
  t = InceptionE(model, t);
  t = InceptionE(model, t);
  t = model.add_pool_layer(t, 8, 8, 1, 1, 0, 0, POOL2D_AVG);
  t = model.add_flat_layer(t);
  t = model.add_linear_layer(t, 1000, false/*relu*/);
  t = model.add_softmax_layer(t);
#endif

  // Construct model (DenseNet121)
#ifdef USE_DENSENET
  Tensor t = model.add_conv_layer(model.input_image, 64, 7, 7, 2, 2, 3, 3, false/*relu*/);
  t = model.add_bn_layer(t, true/*relu*/);
  t = model.add_pool_layer(t, 3, 3, 2, 2, 1, 1);
  int numFeatures = 64;
  t = DenseBlock(model, t, 6, 32);
  numFeatures = (numFeatures + 32 * 6) / 2;
  t = Transition(model, t, numFeatures);
  t = DenseBlock(model, t, 12, 32);
  numFeatures = (numFeatures + 32 * 12) / 2;
  t = Transition(model, t, numFeatures);
  t = DenseBlock(model, t, 24, 32);
  numFeatures = (numFeatures + 32 * 24) / 2;
  t = Transition(model, t, numFeatures);
  t = DenseBlock(model, t, 16, 32);
  t = model.add_pool_layer(t, 7, 7, 1, 1, 0, 0, POOL2D_AVG);
  t = model.add_flat_layer(t);
  t = model.add_linear_layer(t, 1000, false/*relu*/);
  t = model.add_softmax_layer(t);
#endif
  
  // Construct model (Resnet101)
#ifdef USE_RESNET
  Tensor t = model.add_conv_layer(model.input_image, 64, 7, 7, 2, 2, 3, 3);
  t = model.add_pool_layer(t, 3, 3, 2, 2, 1, 1);
  for (int i = 0; i < 3; i++)
    t = BottleneckBlock(model, t, 256, 64, 1);
  for (int i = 0; i < 4; i++) {
    int stride = (i==0) ? 2 : 1;
    t = BottleneckBlock(model, t, 512, 128, stride);
  }
  for (int i = 0; i < 23; i++) {
    int stride = (i==0) ? 2 : 1;
    t = BottleneckBlock(model, t, 1024, 256, stride);
  }
  for (int i = 0; i < 3; i++) {
    int stride = (i==0) ? 2 : 1;
    t = BottleneckBlock(model, t, 2048, 512, stride);
  }
  t = model.add_pool_layer(t, 7, 7, 1, 1, 0, 0, POOL2D_AVG);
  t = model.add_flat_layer(t);
  t = model.add_linear_layer(t, 1000, false/*relu*/);
  t = model.add_softmax_layer(t);
#endif
  // Initialize every layer
  model.init_layers();

  double ts_start = Realm::Clock::current_time_in_microseconds();
  for (int i = 0; i < num_iterations; i++) {
    //model.load_images();
    model.prefetch();
    model.forward();
    model.backward();
    model.update();
  }
  runtime->issue_execution_fence(ctx);
  TimingLauncher timer(MEASURE_MICRO_SECONDS);
  Future future = runtime->issue_timing_measurement(ctx, timer);
  future.get_void_result();
  double ts_end = Realm::Clock::current_time_in_microseconds();
  double run_time = 1e-6 * (ts_end - ts_start);
  printf("time = %.4fs, tp = %.2f images/s\n", run_time, num_images * num_iterations / run_time);
#endif
}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);

  {
    TaskVariantRegistrar registrar(TOP_LEVEL_TASK_ID, "top_level");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<top_level_task>(registrar, "top_level");
  }

  // CNN_INIT_TASK
  {
    TaskVariantRegistrar registrar(CNN_INIT_TASK_ID, "cnn_init_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<CnnHandle, init_cudnn>(registrar, "cnn_init_task");
  }

  // IMAGE_INIT_TASK
  {
    TaskVariantRegistrar registrar(IMAGE_INIT_TASK_ID, "image_init_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<CnnModel::init_images_task>(registrar, "image_init_task");
  }

  // LOAD_IMAGES_TASK
  {
    TaskVariantRegistrar registrar(LOAD_IMAGES_TASK_ID, "load_images_task");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<CnnModel::load_images_task>(registrar, "load_images_task");
  }

  // LOAD_IMAGES_TASK
  {
    TaskVariantRegistrar registrar(NORMALIZE_IMAGES_TASK_ID, "normalize_images_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<CnnModel::normalize_images_task>(registrar, "normalize_images_task");
  }

  // LABEL_INIT_TASK
  {
    TaskVariantRegistrar registrar(LABEL_INIT_TASK_ID, "label_init_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<CnnModel::init_labels_task>(registrar, "label_init_task");
  }
  // Conv2D task
  {
    TaskVariantRegistrar registrar(CONV2D_INIT_TASK_ID, "conv2d_init_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<OpMeta*, Conv2D::init_task>(registrar, "conv2d_init_task");
  }
  {
    TaskVariantRegistrar registrar(CONV2D_INIT_PARA_TASK_ID, "conv2d_init_para_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Conv2D::init_para_task>(registrar, "conv2d_init_para_task");
  }
  {
    TaskVariantRegistrar registrar(CONV2D_FWD_TASK_ID, "conv2d_fwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Conv2D::forward_task>(registrar, "conv2d_fwd_task");
  }
  {
    TaskVariantRegistrar registrar(CONV2D_BWD_TASK_ID, "conv2d_bwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Conv2D::backward_task>(registrar, "conv2d_bwd_task");
  }
  {
    TaskVariantRegistrar registrar(CONV2D_UPD_TASK_ID, "conv2d_upd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Conv2D::update_task>(registrar, "conv2d_upd_task");
  }
  // Pooling2D task
  {
    TaskVariantRegistrar registrar(POOL2D_INIT_TASK_ID, "pooling2d_init_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<OpMeta*, Pooling2D::init_task>(registrar, "pooling2d_init_task");
  }
  {
    TaskVariantRegistrar registrar(POOL2D_FWD_TASK_ID, "pooling2d_fwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Pooling2D::forward_task>(registrar, "pooling2d_fwd_task");
  }
  {
    TaskVariantRegistrar registrar(POOL2D_BWD_TASK_ID, "pooling2d_bwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Pooling2D::backward_task>(registrar, "pooling2d_bwd_task");
  }
  // BatchNorm task
  {
    TaskVariantRegistrar registrar(BATCHNORM_INIT_TASK_ID, "bn_init_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<OpMeta*, BatchNorm::init_task>(registrar, "bn_init_task");
  }
  {
    TaskVariantRegistrar registrar(BATCHNORM_INIT_PARA_TASK_ID, "bm_init_para_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<BatchNorm::init_para_task>(registrar, "bm_init_para_task");
  }
  {
    TaskVariantRegistrar registrar(BATCHNORM_FWD_TASK_ID, "bn_fwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<BatchNorm::forward_task>(registrar, "bn_fwd_task");
  }
  {
    TaskVariantRegistrar registrar(BATCHNORM_BWD_TASK_ID, "bn_bwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<BatchNorm::backward_task>(registrar, "bn_bwd_task");
  }
  // Linear task
  {
    TaskVariantRegistrar registrar(LINEAR_INIT_TASK_ID, "linear_init_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<OpMeta*, Linear::init_task>(registrar, "linear_init_task");
  }
  {
    TaskVariantRegistrar registrar(LINEAR_INIT_PARA_TASK_ID, "linear_init_para_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Linear::init_para_task>(registrar, "linear_init_para_task");
  }
  {
    TaskVariantRegistrar registrar(LINEAR_FWD_TASK_ID, "linear_fwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Linear::forward_task>(registrar, "linear_fwd_task");
  }
  {
    TaskVariantRegistrar registrar(LINEAR_BWD_TASK_ID, "linear_bwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Linear::backward_task>(registrar, "linear_bwd_task");
  }
  {
    TaskVariantRegistrar registrar(LINEAR_BWD2_TASK_ID, "linear_bwd_task (aggregate replica)");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Linear::backward2_task>(registrar, "linear_bwd_task (aggregate replica)");
  }
  {
    TaskVariantRegistrar registrar(LINEAR_UPD_TASK_ID, "linear_upd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Linear::update_task>(registrar, "linear_upd_task");
  }
  // Flat task
  {
    TaskVariantRegistrar registrar(FLAT_INIT_TASK_ID, "flat_init_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<OpMeta*, Flat::init_task>(registrar, "flat_init_task");
  }
  {
    TaskVariantRegistrar registrar(FLAT_FWD_TASK_ID, "flat_fwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Flat::forward_task>(registrar, "flat_fwd_task");
  }
  {
    TaskVariantRegistrar registrar(FLAT_BWD_TASK_ID, "flat_bwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Flat::backward_task>(registrar, "flat_bwd_task");
  }

  // Softmax task
  {
    TaskVariantRegistrar registrar(SOFTMAX_INIT_TASK_ID, "softmax_init_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<OpMeta*, Softmax::init_task>(registrar, "softmax_init_task");
  }
  {
    TaskVariantRegistrar registrar(SOFTMAX_FWD_TASK_ID, "softmax_fwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Softmax::forward_task>(registrar, "softmax_fwd_task");
  }
  {
    TaskVariantRegistrar registrar(SOFTMAX_BWD_TASK_ID, "softmax_bwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Softmax::backward_task>(registrar, "softmax_bwd_task");
  }

  // Concat task
  {
    TaskVariantRegistrar registrar(CONCAT_INIT_TASK_ID, "concat_init_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<OpMeta*, Concat::init_task>(registrar, "concat_init_task");
  }
  {
    TaskVariantRegistrar registrar(CONCAT_FWD_TASK_ID, "concat_fwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Concat::forward_task>(registrar, "concat_fwd_task");
  }
  {
    TaskVariantRegistrar registrar(CONCAT_BWD_TASK_ID, "concat_bwd_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Concat::backward_task>(registrar, "concat_bwd_task");
  }
  // DUMMY task
  {
    TaskVariantRegistrar registrar(DUMMY_TASK_ID, "dummy_task");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<Op::dummy_task>(registrar, "dummy_task");
  }

  Runtime::add_registration_callback(update_mappers);
  return Runtime::start(argc, argv);
}

void parse_input_args(char **argv, int argc, FFConfig& config)
{
  for (int i = 1; i < argc; i++)
  {
    if ((!strcmp(argv[i], "-e")) || (!strcmp(argv[i], "--epochs"))) {
      config.epochs = atoi(argv[++i]);
      continue;
    }
    if ((!strcmp(argv[i], "-b")) || (!strcmp(argv[i], "--batch-size"))) {
      config.batchSize = atoi(argv[++i]);
      continue;
    }
    if ((!strcmp(argv[i], "--lr")) || (!strcmp(argv[i], "--learning-rate"))) {
      config.learningRate = atof(argv[++i]);
      continue;
    }
    if ((!strcmp(argv[i], "--wd")) || (!strcmp(argv[i], "--weight-decay"))) {
      config.weightDecay = atof(argv[++i]);
      continue;
    }
    if ((!strcmp(argv[i], "-p")) || (!strcmp(argv[i], "--print-freq"))) {
      config.printFreq = atoi(argv[++i]);
      continue;
    }
    if ((!strcmp(argv[i], "-d")) || (!strcmp(argv[i], "--dataset"))) {
      config.datasetPath = std::string(argv[++i]);
      continue;
    }
    if ((!strcmp(argv[i], "-s")) || (!strcmp(argv[i], "--strategy"))) {
      config.strategyFile = std::string(argv[++i]);
      continue;
    }
    if (!strcmp(argv[i], "-ll:gpu"))
    {
      config.workersPerNode = atoi(argv[++i]);
      continue;
    }
    if (!strcmp(argv[i], "-ll:cpu"))
    {
      config.loadersPerNode = atoi(argv[++i]);
      continue;
    }
  }
}

bool parse_strategy_file(std::string filename, FFConfig& config)
{
  FILE* file;
  if ((file = fopen(filename.c_str(), "r")) == NULL) {
    log_ff.print("Cannot open strategy file (%s)", filename.c_str());
    return false;
  }
  fclose(file);
}
