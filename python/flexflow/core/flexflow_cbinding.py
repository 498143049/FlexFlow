#!/usr/bin/env python

# Copyright 2020 Stanford University
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from __future__ import absolute_import, division, print_function, unicode_literals

import cffi
import os
import subprocess
from enum import Enum

assert 'FF_DIR' in os.environ
_flexflow_cxxheader_dir= os.path.join(os.environ['FF_DIR'], 'include')
_flexflow_cheader_file = os.path.join(os.path.join(os.environ['FF_DIR'], 'python'), 'flexflow_c.h')

_flexflow_cheader = subprocess.check_output(['gcc', '-I', _flexflow_cxxheader_dir, '-E', '-P', _flexflow_cheader_file]).decode('utf-8')
ffi = cffi.FFI()
ffi.cdef(_flexflow_cheader)
ffc = ffi.dlopen(None)

class ActiMode(Enum):
  AC_MODE_NONE = 10
  AC_MODE_RELU = 11
  AC_MODE_SIGMOID = 12
  AC_MODE_TANH = 13
  
class AggrMode(Enum):
  AGGR_MODE_NONE = 20
  AGGR_MODE_SUM = 21
  AGGR_MODE_AVG = 22

class PoolType(Enum):
  POOL_MAX = 30
  POOL_AVG = 31
  
class DataType(Enum):
  DT_FLOAT = 40
  DT_DOUBLE = 41
  DT_INT32 = 42
  DT_INT64 = 43
  DT_BOOLEAN = 44
  
def enum_to_int(enum, enum_item):
  for item in enum:
    if (enum_item == item):
      return item.value
  
  assert 0, "unknow enum type " + str(enum_item) + " " + str(enum)    
  return -1

# -----------------------------------------------------------------------
# FFConfig
# -----------------------------------------------------------------------

class FFConfig(object):
  def __init__(self):
    self.handle = ffc.flexflow_config_create()
    self._handle = ffi.gc(self.handle, ffc.flexflow_config_destroy)
    
  def parse_args(self):
    ffc.flexflow_config_parse_args_default(self.handle)
    
  def get_batch_size(self):
    return ffc.flexflow_config_get_batch_size(self.handle)
  
  def get_workers_per_node(self):
    return ffc.flexflow_config_get_workers_per_node(self.handle)
  
  def get_num_nodes(self):
    return ffc.flexflow_config_get_num_nodes(self.handle)
    
  def get_epochs(self):
    return ffc.flexflow_config_get_epochs(self.handle)

# -----------------------------------------------------------------------
# Tensor
# -----------------------------------------------------------------------

class Tensor(object):
  def __init__(self, handle):
    self.handle = handle
    self._handle = ffi.gc(self.handle, ffc.flexflow_tensor_4d_destroy)

# -----------------------------------------------------------------------
# FFModel
# -----------------------------------------------------------------------
    
class FFModel(object):
  def __init__(self, config):
    self.handle = ffc.flexflow_model_create(config.handle)
    self._handle = ffi.gc(self.handle, ffc.flexflow_model_destroy)
    
  def create_tensor_4d(self, dims, name, data_type, create_grad=True):
    c_dims = ffi.new("int[]", dims)
    c_data_type = enum_to_int(DataType, data_type)
    handle = ffc.flexflow_tensor_4d_create(self.handle, c_dims, name.encode('utf-8'), c_data_type, create_grad);
    return Tensor(handle)
    
  def conv2d(self, name, input, out_channels, kernel_h, kernel_w, stride_h, stride_w, padding_h, padding_w, activation=ActiMode.AC_MODE_NONE):
    c_activation = enum_to_int(ActiMode, activation)
    handle = ffc.flexflow_model_add_conv2d(self.handle, name.encode('utf-8'), input.handle, out_channels, kernel_h, kernel_w, stride_h, stride_w, padding_h, padding_w, c_activation)  
    return Tensor(handle)
    
  def embedding(self, name, input, num_entires, out_dim, aggr, kernel_initializer):
    c_aggr = enum_to_int(AggrMode, aggr)
    if type(kernel_initializer) is GlorotUniformInitializer:
      handle = ffc.flexflow_model_add_embedding_with_glorot_uniform_initializer(self.handle, name.encode('utf-8'), input.handle, num_entires, out_dim, c_aggr, kernel_initializer.handle)
    elif type(kernel_initializer) is ZeroInitializer:
      handle = ffc.flexflow_model_add_embedding_with_zero_initializer(self.handle, name.encode('utf-8'), input.handle, num_entires, out_dim, c_aggr, kernel_initializer.handle)
    elif type(kernel_initializer) is UniformInitializer:
      handle = ffc.flexflow_model_add_embedding_with_uniform_initializer(self.handle, name.encode('utf-8'), input.handle, num_entires, out_dim, c_aggr, kernel_initializer.handle)
    elif type(kernel_initializer) is NormInitializer:
      handle = ffc.flexflow_model_add_embedding_with_norm_initializer(self.handle, name.encode('utf-8'), input.handle, num_entires, out_dim, c_aggr, kernel_initializer.handle)
    else:
      assert 0, "unknow initializer type"
    return Tensor(handle)
    
  def pool2d(self, name, input, kernel_h, kernel_w, stride_h, stride_w, padding_h, padding_w, pool_type=PoolType.POOL_MAX, relu=True):
    c_pool_type = enum_to_int(PoolType, pool_type)
    handle = ffc.flexflow_model_add_pool2d(self.handle, name.encode('utf-8'), input.handle, kernel_h, kernel_w, stride_h, stride_w, padding_h, padding_w, c_pool_type, relu)
    return Tensor(handle)

  def dense(self, name, input, out_dim, activation=ActiMode.AC_MODE_NONE, use_bias=True):
    c_activation = enum_to_int(ActiMode, activation)
    handle = ffc.flexflow_model_add_dense_with_default_initializer(self.handle, name.encode('utf-8'), input.handle, out_channels, c_activation, use_bias)
    return Tensor(handle)
    
  def linear(self, name, input, out_channels, activation=ActiMode.AC_MODE_NONE, use_bias=True):
    c_activation = enum_to_int(ActiMode, activation)
    handle = ffc.flexflow_model_add_linear_with_default_initializer(self.handle, name.encode('utf-8'), input.handle, out_channels, c_activation, use_bias)
    return Tensor(handle)
    
  def flat(self, name, input):
    handle = ffc.flexflow_model_add_flat(self.handle, name.encode('utf-8'), input.handle)
    return Tensor(handle)
    
  def softmax(self, name, input):
    handle = ffc.flexflow_model_add_softmax(self.handle, name.encode('utf-8'), input.handle)
    return Tensor(handle)
    
  def reset_metrics(self):
    ffc.flexflow_model_reset_metrics(self.handle)
    
  def init_layers(self):
    ffc.flexflow_model_init_layers(self.handle)
    
  def prefetch(self):
    ffc.flexflow_model_prefetch(self.handle)
    
  def forward(self):
    ffc.flexflow_model_forward(self.handle)
    
  def backward(self):
    ffc.flexflow_model_backward(self.handle)
    
  def update(self):
    ffc.flexflow_model_update(self.handle)
    
  def zero_gradients(self):
    ffc.flexflow_model_zero_gradients(self.handle)
  
  def set_sgd_optimizer(self, optimizer):
    ffc.flexflow_model_set_sgd_optimizer(self.handle, optimizer.handle)

# -----------------------------------------------------------------------
# SGDOptimizer
# -----------------------------------------------------------------------
    
class SGDOptimizer(object):
  def __init__(self, ffmodel, lr=0.01, momentum=0.0, nesterov=False, weight_decay=0.0):
    self.handle = ffc.flexflow_sgd_optimizer_create(ffmodel.handle, lr, momentum, nesterov, weight_decay)
    self._handle = ffi.gc(self.handle, ffc.flexflow_sgd_optimizer_destroy)  
    
# -----------------------------------------------------------------------
# GlorotUniform
# -----------------------------------------------------------------------

class GlorotUniformInitializer(object):
  def __init__(self, seed):
    self.handle = ffc.flexflow_glorot_uniform_initializer_create(seed)
    self._handle = ffi.gc(self.handle, ffc.flexflow_glorot_uniform_initializer_destroy)
    
# -----------------------------------------------------------------------
# ZeroInitializer
# -----------------------------------------------------------------------

class ZeroInitializer(object):
  def __init__(self):
    self.handle = ffc.flexflow_zero_initializer_create()
    self._handle = ffi.gc(self.handle, ffc.flexflow_zero_initializer_destroy)  
    
# -----------------------------------------------------------------------
# UniformInitializer
# -----------------------------------------------------------------------

class UniformInitializer(object):
  def __init__(self, seed, minv, maxv):
    self.handle = ffc.flexflow_uniform_initializer_create(seed, minv, maxv)
    self._handle = ffi.gc(self.handle, ffc.flexflow_uniform_initializer_destroy)
    
# -----------------------------------------------------------------------
# NormInitializer
# -----------------------------------------------------------------------

class NormInitializer(object):
  def __init__(self, seed, meanv, stddev):
    self.handle = ffc.flexflow_norm_initializer_create(seed, meanv, stddev)
    self._handle = ffi.gc(self.handle, ffc.flexflow_norm_initializer_destroy)
  