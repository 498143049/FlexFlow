import flexflow.core as ff
import math

from .base_layer import Layer
from flexflow.keras.models.input_layer import Tensor, Input

class Conv2D(Layer):
  def __init__(self, filters, input_shape=(0,), kernel_size=0, strides=0, padding=0, data_format=None, dilation_rate=(1, 1), activation=None, use_bias=True, kernel_initializer='glorot_uniform', bias_initializer='zeros', kernel_regularizer=None, bias_regularizer=None, activity_regularizer=None, kernel_constraint=None, bias_constraint=None, name="conv2d"):
    super(Conv2D, self).__init__(name, "Conv2D") 
    
    self.input_shape = (0, 0, 0, 0)
    self.output_shape = (0, 0, 0, 0)
    self.out_channels = filters
    assert len(kernel_size)==2, "wrong dim of kernel_size"
    self.kernel_size = kernel_size
    assert len(strides)==2, "wrong dim of stride"
    self.stride = strides
    self.padding = padding
    if (activation == None):
      self.activation = ff.ActiMode.AC_MODE_NONE
    elif(activation =="relu"):
      self.activation = ff.ActiMode.AC_MODE_RELU
    else:
      assert 0, "activation is not supported"
    if (len(input_shape) == 4):
      self.in_channels = input_shape[1]
      self.calculate_inout_shape(input_shape[1], input_shape[2], input_shape[3], input_shape[0])
    elif (len(input_shape) == 3):
      self.in_channels = input_shape[0]
      self.calculate_inout_shape(input_shape[0], input_shape[1], input_shape[2])
    else:
      self.in_channels = 0
    self.use_bias = use_bias
    
  def calculate_inout_shape(self, input_d, input_w, input_h, input_b=0):
    assert input_w != 0, "wrong input_w"
    assert input_h != 0, "wrong input_h"
    assert input_d != 0, "wrong input_d"
    self.input_shape = (input_b, input_d, input_w, input_h)
    self.in_channels = input_d
    output_w = 1 + math.floor((input_w + 2 * self.padding[0] - self.kernel_size[0]) / self.stride[0])
    output_h = 1 + math.floor((input_h + 2 * self.padding[1] - self.kernel_size[1]) / self.stride[1])
    output_d = self.out_channels
    self.output_shape = (input_b, output_d, output_w, output_h)
    print("conv2d input ", self.input_shape)
    print("conv2d output ", self.output_shape)
  
  def verify_meta_data(self):
    assert self.input_shape != (0, 0, 0, 0), "[Conv2D]: input shape is wrong"
    assert self.output_shape != (0, 0, 0, 0), "[Conv2D]: output shape is wrong"
    assert self.in_channels != 0, "[Conv2D]: in channels is wrong"
    assert self.out_channels != 0, "[Conv2D]: out channels is wrong"
    
  def get_summary(self):
    summary = "%s%s\t\t%s\t%s\n"%(self._get_summary_name(), self.output_shape, self.input_shape, self._get_summary_connected_to())
    return summary
    
  def get_weights(self, ffmodel):
    return self._get_weights(ffmodel)
    
  def set_weights(self, ffmodel, kernel, bias):
    self._set_weights(ffmodel, kernel, bias)
    
  def __call__(self, input_tensor):
    assert input_tensor.num_dims == 4, "[Conv2D]: shape of input tensor is wrong"
    # not the first layer
    if (self.in_channels == 0):
      in_dims = input_tensor.batch_shape
      self.calculate_inout_shape(in_dims[1], in_dims[2], in_dims[3], in_dims[0])
    
    output_tensor = Tensor(batch_shape=self.output_shape, dtype=input_tensor.dtype, meta_only=True)
    
    self.__verify_inout_tensor_shape(input_tensor, output_tensor)
    self.input_tensors.append(input_tensor)
    self.output_tensors.append(output_tensor)
    
    output_tensor.set_from_layer(self)
    # this is the first layer
    if (isinstance(input_tensor, Input) == True):
      input_tensor.set_to_layer(self)
    else:
      assert input_tensor.from_layer != 0, "[Conv2D]: check input tensor"
      self.prev_layers.append(input_tensor.from_layer)
      input_tensor.from_layer.next_layers.append(self)

    return output_tensor
    
  def __verify_inout_tensor_shape(self, input_tensor, output_tensor):
    assert input_tensor.num_dims == 4, "[Conv2D]: check input tensor dims"
    assert input_tensor.batch_shape[1] == self.input_shape[1]
    assert input_tensor.batch_shape[2] == self.input_shape[2]
    assert input_tensor.batch_shape[3] == self.input_shape[3]
    assert output_tensor.num_dims == 4, "[Conv2D]: check output tensor dims"
    assert output_tensor.batch_shape[1] == self.output_shape[1]
    assert output_tensor.batch_shape[2] == self.output_shape[2]
    assert output_tensor.batch_shape[3] == self.output_shape[3]