#include <sstream>
#include <fstream>
#include <string>
#include "flexflow_c_internal.h"
#include "flexflow_c.h"

class FFCObjectWrapper {
public:
#define FF_NEW_OPAQUE_WRAPPER(T_, T)                                   \
  static T_ wrap(T t) {                                             \
    T_ t_;                                                          \
    t_.impl = static_cast<void *>(t);                               \
    return t_;                                                      \
  }                                                                 \
  static const T_ wrap_const(const T t) {                           \
    T_ t_;                                                          \
    t_.impl = const_cast<void *>(static_cast<const void *>(t));     \
    return t_;                                                      \
  }                                                                 \
  static T unwrap(T_ t_) {                                          \
    return static_cast<T>(t_.impl);                                 \
  }                                                                 \
  static const T unwrap_const(const T_ t_) {                        \
    return static_cast<const T>(t_.impl);                           \
  }
  
  FF_NEW_OPAQUE_WRAPPER(flexflow_config_t, FFConfig *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_model_t, FFModel *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_tensor_t, Tensor *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_sgd_optimizer_t, SGDOptimizer *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_glorot_uniform_initializer_t, GlorotUniform *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_zero_initializer_t, ZeroInitializer *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_uniform_initializer_t, UniformInitializer *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_norm_initializer_t, NormInitializer *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_op_t, Op *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_parameter_t, Parameter *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_net_config_t, NetConfig *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_dataloader_t, ImgDataLoader *);
  FF_NEW_OPAQUE_WRAPPER(flexflow_dataloader_2d_t, ImgDataLoader2D *);
};

// -----------------------------------------------------------------------
// FFConfig
// -----------------------------------------------------------------------

flexflow_config_t
flexflow_config_create(void)
{
  FFConfig *config = new FFConfig();
  Runtime *runtime = Runtime::get_runtime();
  config->lg_hlr = runtime;
  config->lg_ctx = Runtime::get_context();
  config->field_space = runtime->create_field_space(config->lg_ctx);
  printf("new FFConfig %p\n", config);
  return FFCObjectWrapper::wrap(config);
}

void
flexflow_config_destroy(
  flexflow_config_t handle_)
{
  FFConfig *handle = FFCObjectWrapper::unwrap(handle_);
  printf("delete FFConfig %p\n", handle);
  delete handle;
}

void
flexflow_config_parse_args(
  flexflow_config_t handle_,
  char** argv, 
  int argc)
{
  FFConfig *handle = FFCObjectWrapper::unwrap(handle_);
  handle->parse_args(argv, argc);  
}

void
flexflow_config_parse_args_default(
  flexflow_config_t handle_)
{
  FFConfig *handle = FFCObjectWrapper::unwrap(handle_);
  const InputArgs &command_args = Runtime::get_input_args();
  char **argv = command_args.argv;
  int argc = command_args.argc;
  handle->parse_args(argv, argc);  
}

int
flexflow_config_get_batch_size(
  flexflow_config_t handle_)
{
  FFConfig *handle = FFCObjectWrapper::unwrap(handle_);
  return handle->batchSize;
}

int
flexflow_config_get_workers_per_node(
  flexflow_config_t handle_)
{
  FFConfig *handle = FFCObjectWrapper::unwrap(handle_);
  return handle->workersPerNode;
}

int
flexflow_config_get_num_nodes(
  flexflow_config_t handle_)
{
  FFConfig *handle = FFCObjectWrapper::unwrap(handle_);
  return handle->numNodes;
}

int
flexflow_config_get_epochs(
  flexflow_config_t handle_)
{
  FFConfig *handle = FFCObjectWrapper::unwrap(handle_);
  return handle->epochs;
}

// -----------------------------------------------------------------------
// FFModel
// -----------------------------------------------------------------------

flexflow_model_t
flexflow_model_create(
  flexflow_config_t config_)
{
  FFConfig *config = FFCObjectWrapper::unwrap(config_);
  FFModel *model = new FFModel(*config);
  printf("new FFModel %p\n", model);
  return FFCObjectWrapper::wrap(model);
}

void
flexflow_model_destroy(
  flexflow_model_t handle_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  printf("delete FFModel %p\n", handle); 
  delete handle;
}

void
flexflow_model_reset_metrics(
  flexflow_model_t handle_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  handle->reset_metrics();
}

void
flexflow_model_init_layers(
  flexflow_model_t handle_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  handle->init_layers();
}

void
flexflow_model_prefetch(
  flexflow_model_t handle_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  handle->prefetch();
}

void
flexflow_model_forward(
  flexflow_model_t handle_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  handle->forward();
}

void
flexflow_model_backward(
  flexflow_model_t handle_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  handle->backward();
}

void
flexflow_model_update(
  flexflow_model_t handle_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  handle->update();
}

void
flexflow_model_zero_gradients(
  flexflow_model_t handle_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  handle->zero_gradients();
}

flexflow_tensor_t
flexflow_model_add_conv2d(
  flexflow_model_t handle_,
  const char* name,
  const flexflow_tensor_t input_,
  int out_channels,
  int kernel_h, int kernel_w,
  int stride_h, int stride_w,
  int padding_h, int padding_w,
  enum ActiMode activation /* AC_MODE_NONE */,
  bool use_bias /* True */)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  const Tensor *input = FFCObjectWrapper::unwrap_const(input_);
  Tensor *tensor = new Tensor();
  *tensor = handle->conv2d(name, *input, out_channels, kernel_h, kernel_w, stride_h, stride_w, padding_h, padding_w, activation, use_bias);
  printf("Conv2d new Tensor 4D %p (%d, %d, %d, %d), activation %d, use_bias %d\n", tensor, tensor->adim[0], tensor->adim[1], tensor->adim[2], tensor->adim[3], activation, use_bias);
  return FFCObjectWrapper::wrap(tensor);   
}

flexflow_op_t
flexflow_model_add_conv2d_no_inout(
  flexflow_model_t handle_,
  const char* name,
  int in_channels,
  int out_channels,
  int kernel_h, int kernel_w,
  int stride_h, int stride_w,
  int padding_h, int padding_w,
  enum ActiMode activation /* AC_MODE_NONE */,
  bool use_bias /* True */)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  Conv2D *conv2d = handle->conv2d(name, in_channels, out_channels, kernel_h, kernel_w, stride_h, stride_w, padding_h, padding_w, activation, use_bias);
  Op *op = (Op*)conv2d;
  printf("Conv2d no input %p, activation %d, use_bias %d\n", conv2d, activation, use_bias);
  return FFCObjectWrapper::wrap(op);   
}

flexflow_tensor_t
flexflow_model_add_embedding_with_glorot_uniform_initializer(
  flexflow_model_t handle_,
  const char* name,
  const flexflow_tensor_t input_,
  int num_entires, int out_dim,
  enum AggrMode aggr,
  flexflow_glorot_uniform_initializer_t kernel_initializer_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  const Tensor *input = FFCObjectWrapper::unwrap_const(input_);
  Tensor *tensor = new Tensor();
  GlorotUniform *kernel_initializer = FFCObjectWrapper::unwrap(kernel_initializer_);
  Initializer *initializer = static_cast<Initializer *>(kernel_initializer);
  *tensor = handle->embedding(name, *input, num_entires, out_dim, aggr, initializer);
  printf("Embedding with GlorotUniform  new Tensor 4D %p\n", tensor);
  return FFCObjectWrapper::wrap(tensor);   
}
  
flexflow_tensor_t
flexflow_model_add_embedding_with_zero_initializer(
  flexflow_model_t handle_,
  const char* name,
  const flexflow_tensor_t input_,
  int num_entires, int out_dim,
  enum AggrMode aggr,
  flexflow_zero_initializer_t kernel_initializer_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  const Tensor *input = FFCObjectWrapper::unwrap_const(input_);
  Tensor *tensor = new Tensor();
  ZeroInitializer *kernel_initializer = FFCObjectWrapper::unwrap(kernel_initializer_);
  Initializer *initializer = static_cast<Initializer *>(kernel_initializer);
  *tensor = handle->embedding(name, *input, num_entires, out_dim, aggr, initializer);
  printf("Embedding with ZeroInitializer new Tensor 4D %p\n", tensor);
  return FFCObjectWrapper::wrap(tensor);  
}
  
flexflow_tensor_t
flexflow_model_add_embedding_with_uniform_initializer(
  flexflow_model_t handle_,
  const char* name,
  const flexflow_tensor_t input_,
  int num_entires, int out_dim,
  enum AggrMode aggr,
  flexflow_uniform_initializer_t kernel_initializer_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  const Tensor *input = FFCObjectWrapper::unwrap_const(input_);
  Tensor *tensor = new Tensor();
  UniformInitializer *kernel_initializer = FFCObjectWrapper::unwrap(kernel_initializer_);
  Initializer *initializer = static_cast<Initializer *>(kernel_initializer);
  *tensor = handle->embedding(name, *input, num_entires, out_dim, aggr, initializer);
  printf("Embedding with UniformInitializer new Tensor 4D %p\n", tensor);
  return FFCObjectWrapper::wrap(tensor);  
}
  
flexflow_tensor_t
flexflow_model_add_embedding_with_norm_initializer(
  flexflow_model_t handle_,
  const char* name,
  const flexflow_tensor_t input_,
  int num_entires, int out_dim,
  enum AggrMode aggr,
  flexflow_norm_initializer_t kernel_initializer_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  const Tensor *input = FFCObjectWrapper::unwrap_const(input_);
  Tensor *tensor = new Tensor();
  NormInitializer *kernel_initializer = FFCObjectWrapper::unwrap(kernel_initializer_);
  Initializer *initializer = static_cast<Initializer *>(kernel_initializer);
  *tensor = handle->embedding(name, *input, num_entires, out_dim, aggr, initializer);
  printf("Embedding with NormInitializer new Tensor 4D %p\n", tensor);
  return FFCObjectWrapper::wrap(tensor);    
}

flexflow_tensor_t
flexflow_model_add_pool2d(
  flexflow_model_t handle_,
  const char* name,
  flexflow_tensor_t input_,
  int kernel_h, int kernel_w,
  int stride_h, int stride_w,
  int padding_h, int padding_w,
  enum PoolType type /* POOL_MAX */, 
  enum ActiMode activation /* AC_MODE_NONE */)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  Tensor *input = FFCObjectWrapper::unwrap(input_);
  Tensor *tensor = new Tensor();
  *tensor = handle->pool2d(name, *input, kernel_h, kernel_w, stride_h, stride_w, padding_h, padding_w, type, activation);
  printf("Pool2d new Tensor 4D %p (%d, %d, %d, %d), pool %d, activation %d\n", tensor, tensor->adim[0], tensor->adim[1], tensor->adim[2], tensor->adim[3], type, activation);
  return FFCObjectWrapper::wrap(tensor); 
}

flexflow_op_t
flexflow_model_add_pool2d_no_inout(
  flexflow_model_t handle_,
  const char* name,
  int kernel_h, int kernel_w,
  int stride_h, int stride_w,
  int padding_h, int padding_w,
  enum PoolType type /* POOL_MAX */, 
  enum ActiMode activation /* AC_MODE_NONE */)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  Pool2D *pool2d = handle->pool2d(name, kernel_h, kernel_w, stride_h, stride_w, padding_h, padding_w, type, activation);
  Op *op = (Op*)pool2d;
  printf("Pool2d no input %p, pool %d, activation %d\n", pool2d, type, activation);
  return FFCObjectWrapper::wrap(op); 
}

flexflow_tensor_t
flexflow_model_add_dense_with_default_initializer(
  flexflow_model_t handle_,
  const char* name,
  const flexflow_tensor_t input_,
  int out_dim,
  enum ActiMode activation /* AC_MODE_NONE */,
  bool use_bias /* true */)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  const Tensor *input = FFCObjectWrapper::unwrap_const(input_);
  Tensor *tensor = new Tensor();
  *tensor = handle->dense(name, *input, out_dim, activation, use_bias);
  printf("Dense default new Tensor 4D %p (%d, %d, %d, %d), activation %d, use_bias %d\n", tensor, tensor->adim[0], tensor->adim[1], tensor->adim[2], tensor->adim[3], activation, use_bias);
  return FFCObjectWrapper::wrap(tensor); 
}

flexflow_op_t
flexflow_model_add_dense_with_default_initializer_no_inout(
  flexflow_model_t handle_,
  const char* name,
  int in_dim,
  int out_dim,
  enum ActiMode activation /* AC_MODE_NONE */,
  bool use_bias /* true */)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  Linear *linear = handle->dense(name, in_dim, out_dim, activation, use_bias);
  printf("Dense default no input 4D %p, activation %d, use_bias %d\n", linear, activation, use_bias);
  return FFCObjectWrapper::wrap(linear); 
}

flexflow_tensor_t
flexflow_model_add_concat(
  flexflow_model_t handle_,
  const char* name,
  int n,
  flexflow_tensor_t* input_,
  int axis)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  Tensor *tensor = new Tensor();
  std::vector<Tensor> input_vec;
  for (int i = 0; i < n; i++ ) {
    Tensor *t = FFCObjectWrapper::unwrap(input_[i]);
    input_vec.push_back(*t);
  }
  *tensor = handle->concat(name, n, input_vec.data(), axis);
  printf("concat new Tensor 4D %p\n", tensor);
  return FFCObjectWrapper::wrap(tensor); 
}

flexflow_tensor_t
flexflow_model_add_flat(
  flexflow_model_t handle_,
  const char* name,
  flexflow_tensor_t input_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  Tensor *input = FFCObjectWrapper::unwrap(input_);
  Tensor *tensor = new Tensor();
  *tensor = handle->flat(name, *input);
  printf("Flat new Tensor 4D %p\n", tensor);
  return FFCObjectWrapper::wrap(tensor);  
}

flexflow_op_t
flexflow_model_add_flat_no_inout(
  flexflow_model_t handle_,
  const char* name)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  Flat *flat = handle->flat(name);
  Op *op = (Op*)flat;
  printf("Flat no input %p\n", flat);
  return FFCObjectWrapper::wrap(op);  
}

flexflow_tensor_t
flexflow_model_add_softmax(
  flexflow_model_t handle_,
  const char* name,
  const flexflow_tensor_t input_,
  const flexflow_tensor_t label_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  Tensor *input = FFCObjectWrapper::unwrap(input_);
  Tensor *label = FFCObjectWrapper::unwrap(label_);
  Tensor *tensor = new Tensor();
  *tensor = handle->softmax(name, *input, *label);
  printf("Softmax new Tensor 4D %p\n", tensor);
  return FFCObjectWrapper::wrap(tensor);   
}

void
flexflow_model_set_sgd_optimizer(
  flexflow_model_t handle_, 
  flexflow_sgd_optimizer_t optimizer_)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  SGDOptimizer *optimizer = FFCObjectWrapper::unwrap(optimizer_);
  handle->optimizer = static_cast<Optimizer *>(optimizer);
}

void
flexflow_model_print_layers(
  flexflow_model_t handle_, 
  int id)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  handle->print_layers(id);
}

flexflow_op_t
flexflow_model_get_layer_by_id(
  flexflow_model_t handle_,
  int layer_id)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  Op* layer = handle->layers[layer_id];
  return FFCObjectWrapper::wrap(layer);  
}

flexflow_tensor_t
flexflow_model_get_tensor_by_id(
  flexflow_model_t handle_,
  int layer_id)
{
  FFModel *handle = FFCObjectWrapper::unwrap(handle_);
  Tensor *tensor = &(handle->parameters[layer_id].tensor);
  return FFCObjectWrapper::wrap(tensor);  
}

// -----------------------------------------------------------------------
// Tensor
// -----------------------------------------------------------------------

flexflow_tensor_t
flexflow_tensor_4d_create(
  flexflow_model_t model_,
  const int* dims, 
  const char* pc_name, 
  enum DataType data_type, 
  bool create_grad /* true */)
{
  Tensor *tensor = new Tensor();
  FFModel *model = FFCObjectWrapper::unwrap(model_);
  *tensor = model->create_tensor<4>(dims, pc_name, data_type, create_grad);
  printf("new Tensor 4D %p (%d, %d, %d, %d)\n", tensor, tensor->adim[0], tensor->adim[1], tensor->adim[2], tensor->adim[3]);
  return FFCObjectWrapper::wrap(tensor);
}

flexflow_tensor_t
flexflow_tensor_2d_create(
  flexflow_model_t model_,
  const int* dims, 
  const char* pc_name, 
  enum DataType data_type, 
  bool create_grad /* true */)
{
  Tensor *tensor = new Tensor();
  FFModel *model = FFCObjectWrapper::unwrap(model_);
  *tensor = model->create_tensor<2>(dims, pc_name, data_type, create_grad);
  printf("new Tensor 2D %p (%d, %d, %d, %d)\n", tensor, tensor->adim[0], tensor->adim[1], tensor->adim[2], tensor->adim[3]);
  return FFCObjectWrapper::wrap(tensor);
}

void
flexflow_tensor_destroy(
  flexflow_tensor_t handle_)
{
  Tensor *handle = FFCObjectWrapper::unwrap(handle_);
  printf("delete Tensor %p\n", handle);
  delete handle;
}

void
flexflow_tensor_inline_map(
  flexflow_tensor_t handle_,
  flexflow_config_t config_)
{
  Tensor *handle = FFCObjectWrapper::unwrap(handle_);
  FFConfig *config = FFCObjectWrapper::unwrap(config_);  
  handle->inline_map(*config);
}

void  
flexflow_tensor_inline_unmap(
  flexflow_tensor_t handle_,
  flexflow_config_t config_)
{
  Tensor *handle = FFCObjectWrapper::unwrap(handle_);
  FFConfig *config = FFCObjectWrapper::unwrap(config_);  
  handle->inline_unmap(*config);
}

float*  
flexflow_tensor_get_raw_ptr_float(
  flexflow_tensor_t handle_,
  flexflow_config_t config_)
{
  Tensor *handle = FFCObjectWrapper::unwrap(handle_);
  FFConfig *config = FFCObjectWrapper::unwrap(config_);  
  float *raw_ptr = handle->get_raw_ptr<float>(*config);
  return raw_ptr;
}

int32_t*  
flexflow_tensor_get_raw_ptr_int32(
  flexflow_tensor_t handle_,
  flexflow_config_t config_)
{
  Tensor *handle = FFCObjectWrapper::unwrap(handle_);
  FFConfig *config = FFCObjectWrapper::unwrap(config_);  
  int32_t *raw_ptr = handle->get_raw_ptr<int32_t>(*config);
  return raw_ptr;
}

int
flexflow_tensor_get_num_dims(
  flexflow_tensor_t handle_)
{
  Tensor *handle = FFCObjectWrapper::unwrap(handle_);
  return handle->numDim;
}

int*
flexflow_tensor_get_dims(
  flexflow_tensor_t handle_)
{
  Tensor *handle = FFCObjectWrapper::unwrap(handle_);
  printf("dims [%d, %d, %d, %d]\n", handle->adim[0], handle->adim[1], handle->adim[2], handle->adim[3]);
  return &(handle->adim[0]);
}

void
flexflow_tensor_attach_raw_ptr(
  flexflow_tensor_t handle_,
  flexflow_config_t config_,
  uintptr_t ptr,
  bool column_major)
{
  Tensor *handle = FFCObjectWrapper::unwrap(handle_);
  FFConfig *config = FFCObjectWrapper::unwrap(config_);  
  void *raw_ptr = (void*)ptr;
  handle->attach_raw_ptr(*config, raw_ptr, column_major);  
}

void
flexflow_tensor_detach_raw_ptr(
  flexflow_tensor_t handle_,
  flexflow_config_t config_)
{
  Tensor *handle = FFCObjectWrapper::unwrap(handle_);
  FFConfig *config = FFCObjectWrapper::unwrap(config_);
  handle->detach_raw_ptr(*config);  
}

bool
flexflow_tensor_is_mapped(
  flexflow_tensor_t handle_)
{
  Tensor *handle = FFCObjectWrapper::unwrap(handle_);
  return handle->physical_region.is_mapped();
}

// -----------------------------------------------------------------------
// SGDOptimizer
// -----------------------------------------------------------------------

flexflow_sgd_optimizer_t
flexflow_sgd_optimizer_create(
  flexflow_model_t model_,
  double lr, /* 0.01f */
  double momentum, /* 0.0f */
  bool nesterov, /* false */
  double weight_decay /* 0.0f */ )
{
  const FFModel *model = FFCObjectWrapper::unwrap_const(model_);
  SGDOptimizer *optimizer = new SGDOptimizer(model, lr, momentum, nesterov, weight_decay);
  printf("new SGDOptimizer %p\n", optimizer);
  return FFCObjectWrapper::wrap(optimizer);
}

void 
flexflow_sgd_optimizer_destroy(
  flexflow_sgd_optimizer_t handle_)
{
  SGDOptimizer *handle = FFCObjectWrapper::unwrap(handle_);
  printf("delete SGDOptimizer %p\n", handle);
  delete handle;
}

// -----------------------------------------------------------------------
// GlorotUniform
// -----------------------------------------------------------------------

flexflow_glorot_uniform_initializer_t
flexflow_glorot_uniform_initializer_create(
  int seed)
{
  GlorotUniform *initializer = new GlorotUniform(seed);
  return FFCObjectWrapper::wrap(initializer); 
}

void  
flexflow_glorot_uniform_initializer_destroy(
  flexflow_glorot_uniform_initializer_t handle_)
{
  GlorotUniform *handle = FFCObjectWrapper::unwrap(handle_);
  delete handle;
}

// -----------------------------------------------------------------------
// ZeroInitializer
// -----------------------------------------------------------------------

flexflow_zero_initializer_t
flexflow_zero_initializer_create(void)
{
  ZeroInitializer *initializer = new ZeroInitializer();
  return FFCObjectWrapper::wrap(initializer); 
}

void  
flexflow_zero_initializer_destroy(
  flexflow_zero_initializer_t handle_)
{
  ZeroInitializer *handle = FFCObjectWrapper::unwrap(handle_);
  delete handle;
}

// -----------------------------------------------------------------------
// UniformInitializer
// -----------------------------------------------------------------------

flexflow_uniform_initializer_t
flexflow_uniform_initializer_create(
  int seed, 
  float min, 
  float max)
{
  UniformInitializer *initializer = new UniformInitializer(seed, min, max);
  return FFCObjectWrapper::wrap(initializer);  
}

void  
flexflow_uniform_initializer_destroy(
  flexflow_uniform_initializer_t handle_)
{
  UniformInitializer *handle = FFCObjectWrapper::unwrap(handle_);
  delete handle;
}

// -----------------------------------------------------------------------
// NormInitializer
// -----------------------------------------------------------------------

flexflow_norm_initializer_t
flexflow_norm_initializer_create(
  int seed, 
  float mean, 
  float stddev)
{
  NormInitializer *initializer = new NormInitializer(seed, mean, stddev);
  return FFCObjectWrapper::wrap(initializer);  
}

void  
flexflow_norm_initializer_destroy(
  flexflow_norm_initializer_t handle_)
{
  NormInitializer *handle = FFCObjectWrapper::unwrap(handle_);
  delete handle;
}

// -----------------------------------------------------------------------
// NetConfig
// -----------------------------------------------------------------------
flexflow_net_config_t
flexflow_net_config_create()
{
  NetConfig *netconfig = new NetConfig();
  return FFCObjectWrapper::wrap(netconfig);  
}

void
flexflow_net_config_destroy(
  flexflow_net_config_t handle_)
{
  NetConfig *handle = FFCObjectWrapper::unwrap(handle_);
  delete handle;
}

const char*
flexflow_net_config_get_dataset_path(
  flexflow_net_config_t handle_)
{
  NetConfig *handle = FFCObjectWrapper::unwrap(handle_);
  const char *cstr = handle->dataset_path.c_str();
  return cstr;
}

// -----------------------------------------------------------------------
// DataLoader
// -----------------------------------------------------------------------

flexflow_dataloader_t
flexflow_dataloader_create(
  flexflow_model_t ffmodel_, 
  flexflow_net_config_t netconfig_,
  flexflow_tensor_t input_, 
  flexflow_tensor_t label_)
{
  FFModel *ffmodel = FFCObjectWrapper::unwrap(ffmodel_);
  NetConfig *netconfig = FFCObjectWrapper::unwrap(netconfig_);
  Tensor *input = FFCObjectWrapper::unwrap(input_);
  Tensor *label = FFCObjectWrapper::unwrap(label_);
  ImgDataLoader *dataloader = new ImgDataLoader(*ffmodel, *netconfig, *input, *label);
  return FFCObjectWrapper::wrap(dataloader);  
}

flexflow_dataloader_t
flexflow_dataloader_create_v2(
  flexflow_model_t ffmodel_, 
  flexflow_net_config_t netconfig_,
  flexflow_tensor_t input_, 
  flexflow_tensor_t label_,
  flexflow_tensor_t full_input_, 
  flexflow_tensor_t full_label_,
  int num_samples)
{
  FFModel *ffmodel = FFCObjectWrapper::unwrap(ffmodel_);
  NetConfig *netconfig = FFCObjectWrapper::unwrap(netconfig_);
  Tensor *input = FFCObjectWrapper::unwrap(input_);
  Tensor *label = FFCObjectWrapper::unwrap(label_);
  Tensor *full_input = FFCObjectWrapper::unwrap(full_input_);
  Tensor *full_label = FFCObjectWrapper::unwrap(full_label_);
  ImgDataLoader *dataloader = new ImgDataLoader(*ffmodel, *netconfig, *input, *label, *full_input, *full_label, num_samples);
  return FFCObjectWrapper::wrap(dataloader);  
}

void  
flexflow_dataloader_destroy(
  flexflow_dataloader_t handle_)
{
  ImgDataLoader *handle = FFCObjectWrapper::unwrap(handle_);
  delete handle;
}

void
flexflow_dataloader_set_num_samples(
  flexflow_dataloader_t handle_,
  int samples)
{
  ImgDataLoader *handle = FFCObjectWrapper::unwrap(handle_);
  handle->num_samples = samples;  
  printf("dataloader set number of samples %d\n", samples);
}

int
flexflow_dataloader_get_num_samples(
  flexflow_dataloader_t handle_)
{
  ImgDataLoader *handle = FFCObjectWrapper::unwrap(handle_);
  return handle->num_samples;
}

void
flexflow_dataloader_reset(
  flexflow_dataloader_t handle_)
{
  ImgDataLoader *handle = FFCObjectWrapper::unwrap(handle_);
  handle->reset();
}

void
flowflow_dataloader_next_batch(
  flexflow_dataloader_t handle_,
  flexflow_model_t ffmodel_)
{
  ImgDataLoader *handle = FFCObjectWrapper::unwrap(handle_);
  FFModel *ffmodel = FFCObjectWrapper::unwrap(ffmodel_);
  handle->next_batch(*ffmodel);
}


//////

flexflow_dataloader_2d_t
flexflow_dataloader_2d_create_v2(
  flexflow_model_t ffmodel_, 
  flexflow_net_config_t netconfig_,
  flexflow_tensor_t input_, 
  flexflow_tensor_t label_,
  flexflow_tensor_t full_input_, 
  flexflow_tensor_t full_label_,
  int num_samples)
{
  FFModel *ffmodel = FFCObjectWrapper::unwrap(ffmodel_);
  NetConfig *netconfig = FFCObjectWrapper::unwrap(netconfig_);
  Tensor *input = FFCObjectWrapper::unwrap(input_);
  Tensor *label = FFCObjectWrapper::unwrap(label_);
  Tensor *full_input = FFCObjectWrapper::unwrap(full_input_);
  Tensor *full_label = FFCObjectWrapper::unwrap(full_label_);
  ImgDataLoader2D *dataloader = new ImgDataLoader2D(*ffmodel, *netconfig, *input, *label, *full_input, *full_label, num_samples);
  return FFCObjectWrapper::wrap(dataloader);  
}

void  
flexflow_dataloader_2d_destroy(
  flexflow_dataloader_2d_t handle_)
{
  ImgDataLoader2D *handle = FFCObjectWrapper::unwrap(handle_);
  delete handle;
}

void
flexflow_dataloader_2d_set_num_samples(
  flexflow_dataloader_2d_t handle_,
  int samples)
{
  ImgDataLoader2D *handle = FFCObjectWrapper::unwrap(handle_);
  handle->num_samples = samples;  
  printf("dataloader set number of samples %d\n", samples);
}

int
flexflow_dataloader_2d_get_num_samples(
  flexflow_dataloader_2d_t handle_)
{
  ImgDataLoader2D *handle = FFCObjectWrapper::unwrap(handle_);
  return handle->num_samples;
}

void
flexflow_dataloader_2d_reset(
  flexflow_dataloader_2d_t handle_)
{
  ImgDataLoader2D *handle = FFCObjectWrapper::unwrap(handle_);
  handle->reset();
}

void
flowflow_dataloader_2d_next_batch(
  flexflow_dataloader_2d_t handle_,
  flexflow_model_t ffmodel_)
{
  ImgDataLoader2D *handle = FFCObjectWrapper::unwrap(handle_);
  FFModel *ffmodel = FFCObjectWrapper::unwrap(ffmodel_);
  handle->next_batch(*ffmodel);
}

// -----------------------------------------------------------------------
// Timer
// -----------------------------------------------------------------------

double
flexflow_get_current_time(
  flexflow_config_t config_)
{
  FFConfig *config = FFCObjectWrapper::unwrap(config_);
  config->lg_hlr->issue_execution_fence(config->lg_ctx);
  TimingLauncher timer(MEASURE_MICRO_SECONDS);
  Future future = config->lg_hlr->issue_timing_measurement(config->lg_ctx, timer);
  future.get_void_result();
  double ts_start = Realm::Clock::current_time_in_microseconds();
  return ts_start;
}

// -----------------------------------------------------------------------
// Trace
// -----------------------------------------------------------------------

void
flexflow_begin_trace(
  flexflow_config_t config_, 
  int trace_id)
{
  FFConfig *config = FFCObjectWrapper::unwrap(config_);
  config->lg_hlr->begin_trace(config->lg_ctx, trace_id);
}

void
flexflow_end_trace(
  flexflow_config_t config_, 
  int trace_id)
{
  FFConfig *config = FFCObjectWrapper::unwrap(config_);
  config->lg_hlr->end_trace(config->lg_ctx, trace_id);
}

// -----------------------------------------------------------------------
// Op
// -----------------------------------------------------------------------

flexflow_tensor_t
flexflow_op_get_weight(
  flexflow_op_t handle_)
{
  Op *handle = FFCObjectWrapper::unwrap(handle_);
  Tensor *tensor = handle->get_weight();
  return FFCObjectWrapper::wrap(tensor);  
} 

flexflow_tensor_t
flexflow_op_get_bias(
  flexflow_op_t handle_)
{
  Op *handle = FFCObjectWrapper::unwrap(handle_);
  Tensor *tensor = handle->get_bias();
  return FFCObjectWrapper::wrap(tensor);  
}

flexflow_tensor_t
flexflow_op_get_input_by_id(
  flexflow_op_t handle_,
  int id)
{
  Op *handle = FFCObjectWrapper::unwrap(handle_);
  Tensor *tensor = &(handle->inputs[id]);
  return FFCObjectWrapper::wrap(tensor); 
}

flexflow_tensor_t
flexflow_op_get_output(
  flexflow_op_t handle_)
{
  Op *handle = FFCObjectWrapper::unwrap(handle_);
  Tensor *tensor = &(handle->output);
  return FFCObjectWrapper::wrap(tensor);     
}

void
flexflow_op_init(
  flexflow_op_t handle_,
  flexflow_model_t model_)
{
  Op *handle = FFCObjectWrapper::unwrap(handle_);
  FFModel *model = FFCObjectWrapper::unwrap(model_);
  handle->init(*model);
} 

flexflow_tensor_t
flexflow_op_init_inout(
  flexflow_op_t handle_,
  flexflow_model_t model_,
  flexflow_tensor_t input_)
{
  Op *handle = FFCObjectWrapper::unwrap(handle_);
  FFModel *model = FFCObjectWrapper::unwrap(model_);
  Tensor *input = FFCObjectWrapper::unwrap(input_);
  Tensor *tensor = new Tensor();
  *tensor = handle->init_inout(*model, *input);
  return FFCObjectWrapper::wrap(tensor);   
}

void
flexflow_op_forward(
  flexflow_op_t handle_,
  flexflow_model_t model_)
{
  Op *handle = FFCObjectWrapper::unwrap(handle_);
  FFModel *model = FFCObjectWrapper::unwrap(model_);
  handle->forward(*model);
}

void
flexflow_op_add_to_model(
  flexflow_op_t handle_,
  flexflow_model_t model_)
{
  Op *handle = FFCObjectWrapper::unwrap(handle_);
  FFModel *model = FFCObjectWrapper::unwrap(model_);
  handle->add_to_model(*model);
}

// -----------------------------------------------------------------------
// Parameter
// -----------------------------------------------------------------------

flexflow_tensor_t
flexflow_parameter_get_tensor(
  flexflow_parameter_t handle_)
{
  Parameter *handle = FFCObjectWrapper::unwrap(handle_);
  Tensor *tensor = &(handle->tensor);
  return FFCObjectWrapper::wrap(tensor);  
}  

int*
flexflow_malloc_int(
  int size)
{
  int *ptr = NULL;
  uintptr_t intptr; 
  ptr = (int*)malloc(sizeof(int) * size);
  for (int i = 0; i < size; i++) {
    ptr[i] = 1;
  }
  intptr = (uintptr_t)(ptr);
  printf("malloc int %p, %ld, size %ld\n", ptr, intptr, size);
  return ptr;
}

void
flexflow_print_array_int(
  int *base_ptr,
  int size)
{
  printf("base_ptr %p\n", base_ptr);
  for (int i = 0; i < size; i++) {
    printf("%d ", base_ptr[i]);
  }   
  printf("\n");
}

// -----------------------------------------------------------------------
// NetConfig implementation
// -----------------------------------------------------------------------
NetConfig::NetConfig(void)
{
  const InputArgs &command_args = Runtime::get_input_args();
  char **argv = command_args.argv;
  int argc = command_args.argc;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--dataset")) {
      dataset_path = std::string(argv[++i]);
      continue;
    }
  }
}

// -----------------------------------------------------------------------
// ImgDataLoader implementation
// -----------------------------------------------------------------------

ImgDataLoader::ImgDataLoader(FFModel& ff, 
                             const NetConfig& alexnet,
                             Tensor input, Tensor label, Tensor full_input_, Tensor full_label_, int num_samples_)
{
  Context ctx = ff.config.lg_ctx;
  Runtime* runtime = ff.config.lg_hlr;
  num_samples = num_samples_;
  // Create full input
  {
    batch_input = input;
    const int dims[] = {num_samples, input.adim[2], input.adim[1], input.adim[0]};
    full_input = ff.create_tensor<4>(dims, "", DT_FLOAT);
  }
  // Create full label
  {
    batch_label = label;
    const int dims[] = {num_samples, label.adim[0]};
    full_label = ff.create_tensor<2>(dims, "", DT_INT32);
  }
  // Load entire dataset
  // TODO: Use index launcher instead of task launcher
  const NetConfig* ptr = &alexnet;
  TaskLauncher launcher(CUSTOM_CPU_TASK_ID_2,
      TaskArgument(&ptr, sizeof(NetConfig*)));
  // regions[0]: full_input
  launcher.add_region_requirement(
      RegionRequirement(full_input.region, WRITE_ONLY,
                        EXCLUSIVE, full_input.region,
                        MAP_TO_ZC_MEMORY));
  launcher.add_field(0, FID_DATA);
  // regions[1]: full_label
  launcher.add_region_requirement(
      RegionRequirement(full_label.region, WRITE_ONLY,
                        EXCLUSIVE, full_label.region,
                        MAP_TO_ZC_MEMORY));
  launcher.add_field(1, FID_DATA);
  // regions[2]: full_input_
  launcher.add_region_requirement(
      RegionRequirement(full_input_.region, READ_ONLY,
                        EXCLUSIVE, full_input_.region));
  launcher.add_field(2, FID_DATA);
  // regions[3]: full_label_
  launcher.add_region_requirement(
      RegionRequirement(full_label_.region, READ_ONLY,
                        EXCLUSIVE, full_label_.region));
  launcher.add_field(3, FID_DATA);
  Future fu = runtime->execute_task(ctx, launcher);
  fu.wait();
  reset();
  next_batch(ff);
}

ImgDataLoader::ImgDataLoader(FFModel& ff, 
                             const NetConfig& alexnet,
                             Tensor input, Tensor label)
{
  Context ctx = ff.config.lg_ctx;
  Runtime* runtime = ff.config.lg_hlr;
  num_samples = 0;
  if (alexnet.dataset_path == "") {
    printf("Use random dataset...");
    num_samples = 256 * 10 * ff.config.workersPerNode * ff.config.numNodes;
    printf("Number of random samples = %d\n", num_samples);
  } else {
    printf("Start loading dataset from %s\n", alexnet.dataset_path.c_str());
    size_t filesize = get_file_size(alexnet.dataset_path);
    assert(filesize % 3073 == 0);
    num_samples = filesize / 3073;
  }
  // Create full input
  {
    batch_input = input;
    const int dims[] = {num_samples, input.adim[2], input.adim[1], input.adim[0]};
    full_input = ff.create_tensor<4>(dims, "", DT_FLOAT);
  }
  // Create full label
  {
    batch_label = label;
    const int dims[] = {num_samples, label.adim[0]};
    full_label = ff.create_tensor<2>(dims, "", DT_INT32);
  }
  // Load entire dataset
  // TODO: Use index launcher instead of task launcher
  const NetConfig* ptr = &alexnet;
  TaskLauncher launcher(CUSTOM_CPU_TASK_ID_1,
      TaskArgument(&ptr, sizeof(NetConfig*)));
  // regions[0]: full_input
  launcher.add_region_requirement(
      RegionRequirement(full_input.region, WRITE_ONLY,
                        EXCLUSIVE, full_input.region,
                        MAP_TO_ZC_MEMORY));
  launcher.add_field(0, FID_DATA);
  // regions[1]: full_label
  launcher.add_region_requirement(
      RegionRequirement(full_label.region, WRITE_ONLY,
                        EXCLUSIVE, full_label.region,
                        MAP_TO_ZC_MEMORY));
  launcher.add_field(1, FID_DATA);
  runtime->execute_task(ctx, launcher);
  reset();
  next_batch(ff);
}

void ImgDataLoader::load_entire_dataset_v2(const Task *task,
                                        const std::vector<PhysicalRegion> &regions,
                                        Context ctx, Runtime* runtime)
{
  const NetConfig* alexnet = *((NetConfig**)task->args);
  assert(regions.size() == 4);
  assert(task->regions.size() == regions.size());
  const AccessorWO<float, 4> acc_input(regions[0], FID_DATA);
  const AccessorWO<int, 2> acc_label(regions[1], FID_DATA);
  const AccessorRO<float, 4> acc_input_(regions[2], FID_DATA);
  const AccessorRO<int, 2> acc_label_(regions[3], FID_DATA);
  Rect<4> rect_input = runtime->get_index_space_domain(
      ctx, task->regions[0].region.get_index_space());
  assert(acc_input.accessor.is_dense_arbitrary(rect_input));
  Rect<2> rect_label = runtime->get_index_space_domain(
      ctx, task->regions[1].region.get_index_space());
  Rect<4> rect_input_ = runtime->get_index_space_domain(
      ctx, task->regions[2].region.get_index_space());
  assert(acc_input_.accessor.is_dense_arbitrary(rect_input_));
  Rect<2> rect_label_ = runtime->get_index_space_domain(
      ctx, task->regions[3].region.get_index_space());
  assert(acc_label_.accessor.is_dense_arbitrary(rect_label_));
  float* input_ptr = acc_input.ptr(rect_input.lo);
  int* label_ptr = acc_label.ptr(rect_label.lo);
  const float* input_ptr_ = acc_input_.ptr(rect_input_.lo);
  const int* label_ptr_ = acc_label_.ptr(rect_label_.lo);
  printf("Check ptr input %p %lu %lu, label %p %lu %lu\n", input_ptr_, (uintptr_t)input_ptr_, rect_input.volume(), label_ptr_, (uintptr_t)label_ptr_, rect_label.volume());
  int num_samples = rect_label.hi[1] - rect_label.lo[1] + 1;
  assert(rect_input.hi[3] - rect_input.lo[3] + 1 == num_samples);
  assert(rect_label.volume() == rect_label_.volume());
  assert(rect_input.volume() == rect_input_.volume());
  memcpy(input_ptr, input_ptr_, sizeof(float)*rect_input.volume());
  memcpy(label_ptr, label_ptr_, sizeof(int)*rect_label.volume());
  for (int i = 0; i < 32; i++) {
    printf("%f ", input_ptr[i]);
  }
  printf("\n");
}

__inline__
int calc_offset(int c, int y, int x, int yscale, int xscale)
{
  return (c * yscale * xscale + y * xscale + x);
}

void nearest_neigh(unsigned char* image,
                   unsigned char* buffer,
                   int height, int width,
                   int orig_height, int orig_width,
                   float height_scale, float width_scale)
{
  for (int y = 0; y < height; y++) {
    int y0 = std::min(static_cast<int>(roundf(y * height_scale)), orig_height - 1);
    for (int x = 0; x < width; x++) {
      int x0 = std::min(static_cast<int>(roundf(x * width_scale)), orig_width - 1);
      for (int c = 0; c < 3; c++) {
        int origOffset = calc_offset(y0, x0, c, orig_width, 3);
        int offset = calc_offset(c, y, x, height, width);
        image[offset] = buffer[origOffset];
      }
    }
  }
}

void ImgDataLoader::load_entire_dataset(const Task *task,
                                        const std::vector<PhysicalRegion> &regions,
                                        Context ctx, Runtime* runtime)
{
  const NetConfig* alexnet = *((NetConfig**)task->args);
  assert(regions.size() == 2);
  assert(task->regions.size() == regions.size());
  const AccessorWO<float, 4> acc_input(regions[0], FID_DATA);
  const AccessorWO<int, 2> acc_label(regions[1], FID_DATA);
  Rect<4> rect_input = runtime->get_index_space_domain(
      ctx, task->regions[0].region.get_index_space());
  assert(acc_input.accessor.is_dense_arbitrary(rect_input));
  Rect<2> rect_label = runtime->get_index_space_domain(
      ctx, task->regions[1].region.get_index_space());
  assert(acc_label.accessor.is_dense_arbitrary(rect_label));
  float* input_ptr = acc_input.ptr(rect_input.lo);
  int* label_ptr = acc_label.ptr(rect_label.lo);
  int num_samples = rect_label.hi[1] - rect_label.lo[1] + 1;
  assert(rect_input.hi[3] - rect_input.lo[3] + 1 == num_samples);
  if (alexnet->dataset_path.length() == 0) {
    printf("Start generating random input samples\n");
    for (size_t i = 0; i < rect_label.volume(); i++)
      label_ptr[i] = std::rand() % 10;
    return;
  }
  printf("Start loading %d samples from %s\n",
      num_samples, alexnet->dataset_path.c_str());
  int height = rect_input.hi[1] - rect_input.lo[1] + 1;
  int width = rect_input.hi[0] - rect_input.lo[0] + 1;
  int origHeight = 32;
  int origWidth = 32;
  float heightScale = static_cast<float>(origHeight) / height;
  float widthScale = static_cast<float>(origWidth) / width;
  FILE* file = fopen(alexnet->dataset_path.c_str(), "rb");
  unsigned char* image = (unsigned char*) malloc(3073);
  unsigned char* buffer = (unsigned char*) malloc(3 * height * width);
  for (int i = 0; i < num_samples; i++) {
    size_t ret = fread(buffer, sizeof(unsigned char), 3073, file);
    assert(ret = 3073);
    if ((i+1) % 1000 == 0)
      printf("Loaded %d samples\n", i+1);
    label_ptr[i] = image[0];
    nearest_neigh(image + 1, buffer, height, width,
                  origHeight, origWidth, heightScale, widthScale);
    int input_offset = i * 3 * height * width;
    int buffer_offset = 0;
    for (int h = 0; h < 3*height*width; h++)
        input_ptr[input_offset++] = static_cast<float>(buffer[buffer_offset++]) / 255;
  }
  printf("Finish loading %d samples from %s\n",
      num_samples, alexnet->dataset_path.c_str());
  fclose(file);
}

void ImgDataLoader::next_batch(FFModel& ff)
{
  Context ctx = ff.config.lg_ctx;
  Runtime* runtime = ff.config.lg_hlr;
  // Load input
  {
    IndexSpaceT<4> task_is = IndexSpaceT<4>(ff.get_or_create_task_is(4, ""));
    Rect<4> rect = runtime->get_index_space_domain(ctx, task_is);
    ArgumentMap argmap;
    int idx = next_index;
    for (PointInRectIterator<4> it(rect); it(); it++) {
      SampleIdxs meta;
      assert(ff.config.batchSize % (rect.hi[3] - rect.lo[3] + 1) == 0);
      meta.num_samples = ff.config.batchSize / (rect.hi[3] - rect.lo[3] + 1);
      for (int i = 0; i < meta.num_samples; i++)
        meta.idxs[i] = idx++;
      argmap.set_point(*it, TaskArgument(&meta, sizeof(SampleIdxs)));
    }
    IndexLauncher launcher(CUSTOM_GPU_TASK_ID_1, task_is,
                           TaskArgument(NULL,0), argmap,
                           Predicate::TRUE_PRED, false/*must*/, 0/*mapper_id*/,
                           FFConfig::get_hash_id(""));
    launcher.add_region_requirement(
        RegionRequirement(full_input.region, 0/*projection id*/,
                          READ_ONLY, EXCLUSIVE, full_input.region,
                          MAP_TO_ZC_MEMORY));
    launcher.add_field(0, FID_DATA);
    launcher.add_region_requirement(
        RegionRequirement(batch_input.part, 0/*projection id*/,
                          WRITE_ONLY, EXCLUSIVE, batch_input.region));
    launcher.add_field(1, FID_DATA);
    runtime->execute_index_space(ctx, launcher);
  }
  // Load label
  {
    IndexSpaceT<2> task_is = IndexSpaceT<2>(ff.get_or_create_task_is(2, ""));
    Rect<2> rect = runtime->get_index_space_domain(ctx, task_is);
    ArgumentMap argmap;
    int idx = next_index;
    for (PointInRectIterator<2> it(rect); it(); it++) {
      SampleIdxs meta;
      assert(ff.config.batchSize % (rect.hi[1] - rect.lo[1] + 1) == 0);
      meta.num_samples = ff.config.batchSize / (rect.hi[1] - rect.lo[1] + 1);
      for (int i = 0; i < meta.num_samples; i++)
        meta.idxs[i] = idx++;
      argmap.set_point(*it, TaskArgument(&meta, sizeof(SampleIdxs)));
    }
    IndexLauncher launcher(CUSTOM_GPU_TASK_ID_2, task_is,
                           TaskArgument(NULL,0), argmap,
                           Predicate::TRUE_PRED, false/*must*/, 0/*mapper_id*/,
                           FFConfig::get_hash_id(""));
    launcher.add_region_requirement(
        RegionRequirement(full_label.region, 0/*projection id*/,
                          READ_ONLY, EXCLUSIVE, full_label.region,
                          MAP_TO_ZC_MEMORY));
    launcher.add_field(0, FID_DATA);
    launcher.add_region_requirement(
        RegionRequirement(batch_label.part, 0/*projection id*/,
                          WRITE_ONLY, EXCLUSIVE, batch_label.region));
    launcher.add_field(1, FID_DATA);
    runtime->execute_index_space(ctx, launcher);
  }
  next_index += ff.config.batchSize;
}

void ImgDataLoader::reset()
{
  next_index = 0;
}

size_t ImgDataLoader::get_file_size(const std::string& filename)
{
  std::streampos begin,end;
  std::ifstream file(filename.c_str(), std::ios::binary);
  begin = file.tellg();
  file.seekg (0, std::ios::end);
  end = file.tellg();
  file.close();
  size_t filesize = end - begin;
  return filesize;
}

ImgDataLoader2D::ImgDataLoader2D(FFModel& ff, 
                             const NetConfig& alexnet,
                             Tensor input, Tensor label, Tensor full_input_, Tensor full_label_, int num_samples_)
{
  Context ctx = ff.config.lg_ctx;
  Runtime* runtime = ff.config.lg_hlr;
  num_samples = num_samples_;
  // Create full input
  {
    batch_input = input;
    const int dims[] = {num_samples, input.adim[0]};
    full_input = ff.create_tensor<2>(dims, "", DT_FLOAT);
  }
  // Create full label
  {
    batch_label = label;
    const int dims[] = {num_samples, label.adim[0]};
    full_label = ff.create_tensor<2>(dims, "", DT_INT32);
  }
  // Load entire dataset
  // TODO: Use index launcher instead of task launcher
  const NetConfig* ptr = &alexnet;
  TaskLauncher launcher(CUSTOM_CPU_TASK_ID_3,
      TaskArgument(&ptr, sizeof(NetConfig*)));
  // regions[0]: full_input
  launcher.add_region_requirement(
      RegionRequirement(full_input.region, WRITE_ONLY,
                        EXCLUSIVE, full_input.region,
                        MAP_TO_ZC_MEMORY));
  launcher.add_field(0, FID_DATA);
  // regions[1]: full_label
  launcher.add_region_requirement(
      RegionRequirement(full_label.region, WRITE_ONLY,
                        EXCLUSIVE, full_label.region,
                        MAP_TO_ZC_MEMORY));
  launcher.add_field(1, FID_DATA);
  // regions[2]: full_input_
  launcher.add_region_requirement(
      RegionRequirement(full_input_.region, READ_ONLY,
                        EXCLUSIVE, full_input_.region));
  launcher.add_field(2, FID_DATA);
  // regions[3]: full_label_
  launcher.add_region_requirement(
      RegionRequirement(full_label_.region, READ_ONLY,
                        EXCLUSIVE, full_label_.region));
  launcher.add_field(3, FID_DATA);
  Future fu = runtime->execute_task(ctx, launcher);
  fu.wait();
  reset();
  next_batch(ff);
}

void ImgDataLoader2D::load_entire_dataset_v2(const Task *task,
                                        const std::vector<PhysicalRegion> &regions,
                                        Context ctx, Runtime* runtime)
{
  const NetConfig* alexnet = *((NetConfig**)task->args);
  assert(regions.size() == 4);
  assert(task->regions.size() == regions.size());
  const AccessorWO<float, 2> acc_input(regions[0], FID_DATA);
  const AccessorWO<int, 2> acc_label(regions[1], FID_DATA);
  const AccessorRO<float, 2> acc_input_(regions[2], FID_DATA);
  const AccessorRO<int, 2> acc_label_(regions[3], FID_DATA);
  Rect<2> rect_input = runtime->get_index_space_domain(
      ctx, task->regions[0].region.get_index_space());
  assert(acc_input.accessor.is_dense_arbitrary(rect_input));
  Rect<2> rect_label = runtime->get_index_space_domain(
      ctx, task->regions[1].region.get_index_space());
  Rect<2> rect_input_ = runtime->get_index_space_domain(
      ctx, task->regions[2].region.get_index_space());
  assert(acc_input_.accessor.is_dense_arbitrary(rect_input_));
  Rect<2> rect_label_ = runtime->get_index_space_domain(
      ctx, task->regions[3].region.get_index_space());
  assert(acc_label_.accessor.is_dense_arbitrary(rect_label_));
  float* input_ptr = acc_input.ptr(rect_input.lo);
  int* label_ptr = acc_label.ptr(rect_label.lo);
  const float* input_ptr_ = acc_input_.ptr(rect_input_.lo);
  const int* label_ptr_ = acc_label_.ptr(rect_label_.lo);
  printf("Check ptr input %p %lu %lu, label %p %lu %lu\n", input_ptr_, (uintptr_t)input_ptr_, rect_input.volume(), label_ptr_, (uintptr_t)label_ptr_, rect_label.volume());
  int num_samples = rect_label.hi[1] - rect_label.lo[1] + 1;
  assert(rect_input.hi[1] - rect_input.lo[1] + 1 == num_samples);
  assert(rect_label.volume() == rect_label_.volume());
  assert(rect_input.volume() == rect_input_.volume());
  memcpy(input_ptr, input_ptr_, sizeof(float)*rect_input.volume());
  memcpy(label_ptr, label_ptr_, sizeof(int)*rect_label.volume());
  for (int i = 0; i < 32; i++) {
    printf("%f ", input_ptr[i]);
  }
  printf("\n");
}

void ImgDataLoader2D::next_batch(FFModel& ff)
{
  Context ctx = ff.config.lg_ctx;
  Runtime* runtime = ff.config.lg_hlr;
  // Load input
  {
    IndexSpaceT<2> task_is = IndexSpaceT<2>(ff.get_or_create_task_is(2, ""));
    Rect<2> rect = runtime->get_index_space_domain(ctx, task_is);
    ArgumentMap argmap;
    int idx = next_index;
    for (PointInRectIterator<2> it(rect); it(); it++) {
      SampleIdxs meta;
      assert(ff.config.batchSize % (rect.hi[1] - rect.lo[1] + 1) == 0);
      meta.num_samples = ff.config.batchSize / (rect.hi[1] - rect.lo[1] + 1);
      for (int i = 0; i < meta.num_samples; i++)
        meta.idxs[i] = idx++;
      argmap.set_point(*it, TaskArgument(&meta, sizeof(SampleIdxs)));
    }
    IndexLauncher launcher(CUSTOM_GPU_TASK_ID_3, task_is,
                           TaskArgument(NULL,0), argmap,
                           Predicate::TRUE_PRED, false/*must*/, 0/*mapper_id*/,
                           FFConfig::get_hash_id(""));
    launcher.add_region_requirement(
        RegionRequirement(full_input.region, 0/*projection id*/,
                          READ_ONLY, EXCLUSIVE, full_input.region,
                          MAP_TO_ZC_MEMORY));
    launcher.add_field(0, FID_DATA);
    launcher.add_region_requirement(
        RegionRequirement(batch_input.part, 0/*projection id*/,
                          WRITE_ONLY, EXCLUSIVE, batch_input.region));
    launcher.add_field(1, FID_DATA);
    runtime->execute_index_space(ctx, launcher);
  }
  // Load label
  {
    IndexSpaceT<2> task_is = IndexSpaceT<2>(ff.get_or_create_task_is(2, ""));
    Rect<2> rect = runtime->get_index_space_domain(ctx, task_is);
    ArgumentMap argmap;
    int idx = next_index;
    for (PointInRectIterator<2> it(rect); it(); it++) {
      SampleIdxs meta;
      assert(ff.config.batchSize % (rect.hi[1] - rect.lo[1] + 1) == 0);
      meta.num_samples = ff.config.batchSize / (rect.hi[1] - rect.lo[1] + 1);
      for (int i = 0; i < meta.num_samples; i++)
        meta.idxs[i] = idx++;
      argmap.set_point(*it, TaskArgument(&meta, sizeof(SampleIdxs)));
    }
    IndexLauncher launcher(CUSTOM_GPU_TASK_ID_4, task_is,
                           TaskArgument(NULL,0), argmap,
                           Predicate::TRUE_PRED, false/*must*/, 0/*mapper_id*/,
                           FFConfig::get_hash_id(""));
    launcher.add_region_requirement(
        RegionRequirement(full_label.region, 0/*projection id*/,
                          READ_ONLY, EXCLUSIVE, full_label.region,
                          MAP_TO_ZC_MEMORY));
    launcher.add_field(0, FID_DATA);
    launcher.add_region_requirement(
        RegionRequirement(batch_label.part, 0/*projection id*/,
                          WRITE_ONLY, EXCLUSIVE, batch_label.region));
    launcher.add_field(1, FID_DATA);
    runtime->execute_index_space(ctx, launcher);
  }
  next_index += ff.config.batchSize;
}

void ImgDataLoader2D::reset()
{
  next_index = 0;
}

void register_c_custom_tasks()
{
  // Load entire dataset
  {
    TaskVariantRegistrar registrar(CUSTOM_CPU_TASK_ID_1, "Load Entire Dataset");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<ImgDataLoader::load_entire_dataset>(
        registrar, "Load Entire Dataset Task");
  }
  // Load entire dataset2
  {
    TaskVariantRegistrar registrar(CUSTOM_CPU_TASK_ID_2, "Load Entire Dataset2");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<ImgDataLoader::load_entire_dataset_v2>(
        registrar, "Load Entire Dataset Task v2");
  }
  // Load input
  {
    TaskVariantRegistrar registrar(CUSTOM_GPU_TASK_ID_1, "Load Inputs");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<ImgDataLoader::load_input>(
        registrar, "Load Input Task");
  }
  // Load label
  {
    TaskVariantRegistrar registrar(CUSTOM_GPU_TASK_ID_2, "Load Labels");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<ImgDataLoader::load_label>(
        registrar, "Load Label Task");
  }
  
  // Load entire dataset2
  {
    TaskVariantRegistrar registrar(CUSTOM_CPU_TASK_ID_3, "2DLoad Entire Dataset2");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<ImgDataLoader2D::load_entire_dataset_v2>(
        registrar, "2DLoad Entire Dataset Task v2");
  }
  // Load input
  {
    TaskVariantRegistrar registrar(CUSTOM_GPU_TASK_ID_3, "2DLoad Inputs");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<ImgDataLoader2D::load_input>(
        registrar, "2DLoad Input Task");
  }
  // Load label
  {
    TaskVariantRegistrar registrar(CUSTOM_GPU_TASK_ID_4, "2DLoad Labels");
    registrar.add_constraint(ProcessorConstraint(Processor::TOC_PROC));
    registrar.set_leaf();
    Runtime::preregister_task_variant<ImgDataLoader2D::load_label>(
        registrar, "2DLoad Label Task");
  }
}
