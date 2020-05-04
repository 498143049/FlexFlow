import flexflow.core as ff

from flexflow.keras.layers import Conv2D, MaxPooling2D, Flatten, Dense, Activation

class Sequential(object):
  def __init__(self):
    self.ffconfig = ff.FFConfig()
    self.ffconfig.parse_args()
    print("Python API batchSize(%d) workersPerNodes(%d) numNodes(%d)" %(self.ffconfig.get_batch_size(), self.ffconfig.get_workers_per_node(), self.ffconfig.get_num_nodes()))
    self.ffmodel = ff.FFModel(self.ffconfig)
    
    self._layers = dict()
    self._nb_layers = 0
    self.input_tensor = 0
    self.output_tensor = 0
    self.label_tensor = 0
    self.full_input_tensor = 0
    self.full_label_tensor = 0
    self.num_samples = 0
    self.dataloaders = []
    self.dataloaders_dim = []
    self.use_v2 = False
    
  def _create_layer_and_init_inout(self, verify_inout_shape=True):
    int_t = 0
    out_t = 0
    for layer_id in self._layers:
      layer = self._layers[layer_id]
      assert layer.layer_id == layer_id, "wrong layer id"
      if (layer_id == 0):
        in_t = self.input_tensor
      else:
        in_t = out_t
        
      if (isinstance(layer, Conv2D) == True):
        out_t = self.ffmodel.conv2d(layer.name, in_t, layer.out_channels, layer.kernel_size[0], layer.kernel_size[1], layer.stride[0], layer.stride[1], layer.padding[0], layer.padding[1], layer.activation, layer.use_bias)
      elif (isinstance(layer, MaxPooling2D) == True):
        out_t = self.ffmodel.pool2d(layer.name, in_t, layer.kernel_size[0], layer.kernel_size[1], layer.stride[0], layer.stride[1], layer.padding[0], layer.padding[1])
      elif (isinstance(layer, Flatten) == True):
        out_t = self.ffmodel.flat(layer.name, in_t)
      elif (isinstance(layer, Dense) == True):
        out_t = self.ffmodel.dense(layer.name, in_t, layer.out_channels, layer.activation)
      elif (isinstance(layer, Activation) == True):
        assert layer_id == self._nb_layers-1, "softmax is not in the last layer"
        out_t = self.ffmodel.softmax("softmax", in_t, self.label_tensor)
        
      if (verify_inout_shape == True):
        layer.verify_inout_shape(in_t, out_t) 
      layer.handle = self.ffmodel.get_layer_by_id(layer_id)
      print(layer.handle)
    self.output_tensor = out_t
    
  # def _init_inout(self, input_tensor, label_tensor):
  #   self.input_tensor = input_tensor
  #   for layer_id in self._layers:
  #     layer = self._layers[layer_id]
  #     if (layer_id == 0):
  #       self.output_tensor = layer.handle.init_inout(self.ffmodel, input_tensor);
  #     else:
  #       if (isinstance(layer, Activation) == True):
  #         assert layer_id == self._nb_layers-1, "softmax is not in the last layer"
  #         self.output_tensor = self.ffmodel.softmax("softmax", self.output_tensor, label_tensor)
  #         layer.handle = self.ffmodel.get_layer_by_id(layer_id)
  #       else:
  #         self.output_tensor = layer.handle.init_inout(self.ffmodel, self.output_tensor);
    
  def add(self, layer):
    self._layers[self._nb_layers] = layer
    layer.layer_id = self._nb_layers
    self._nb_layers += 1
    
    if (isinstance(layer, Conv2D) == True):
      if (layer.layer_id > 0):
        prev_layer = self._layers[layer.layer_id-1]
        layer.calculate_inout_shape(prev_layer.output_shape[1], prev_layer.output_shape[2], prev_layer.output_shape[3], prev_layer.output_shape[0])
    elif (isinstance(layer, MaxPooling2D) == True):
      assert layer.layer_id != 0, "maxpool2d can not be the 1st layer"
      prev_layer = self._layers[layer.layer_id-1]
      layer.calculate_inout_shape(prev_layer.output_shape[1], prev_layer.output_shape[2], prev_layer.output_shape[3], prev_layer.output_shape[0])
    elif (isinstance(layer, Flatten) == True):
      assert layer.layer_id != 0, "flatten can not be the 1st layer"
      prev_layer = self._layers[layer.layer_id-1]
      layer.calculate_inout_shape(prev_layer.output_shape)
    elif (isinstance(layer, Dense) == True):
      if (layer.layer_id > 0):
        prev_layer = self._layers[layer.layer_id-1]
        layer.calculate_inout_shape(prev_layer.output_shape[1])
  
  # def add_v2(self, layer):
  #   self.use_v2 = True
  #   self._layers[self._nb_layers] = layer
  #   layer.layer_id = self._nb_layers
  #   self._nb_layers += 1
  #
  #   if (isinstance(layer, Conv2D) == True):
  #     if (layer.layer_id > 0):
  #       prev_layer = self._layers[layer.layer_id-1]
  #       layer.calculate_inout_shape(prev_layer.output_shape[1], prev_layer.output_shape[2], prev_layer.output_shape[3], prev_layer.output_shape[0])
  #   elif (isinstance(layer, MaxPooling2D) == True):
  #     assert layer.layer_id != 0, "maxpool2d can not be the 1st layer"
  #     prev_layer = self._layers[layer.layer_id-1]
  #     layer.calculate_inout_shape(prev_layer.output_shape[1], prev_layer.output_shape[2], prev_layer.output_shape[3], prev_layer.output_shape[0])
  #   elif (isinstance(layer, Flatten) == True):
  #     assert layer.layer_id != 0, "flatten can not be the 1st layer"
  #     prev_layer = self._layers[layer.layer_id-1]
  #     layer.calculate_inout_shape(prev_layer.output_shape)
  #   elif (isinstance(layer, Dense) == True):
  #     if (layer.layer_id > 0):
  #       prev_layer = self._layers[layer.layer_id-1]
  #       layer.calculate_inout_shape(prev_layer.output_shape[1])
  #
  #   if (isinstance(layer, Conv2D) == True):
  #     layer.handle = self.ffmodel.conv2d_v2(layer.name, layer.in_channels, layer.out_channels, layer.kernel_size[0], layer.kernel_size[1], layer.stride[0], layer.stride[1], layer.padding[0], layer.padding[1], layer.activation, layer.use_bias)
  #   elif (isinstance(layer, MaxPooling2D) == True):
  #     layer.handle = self.ffmodel.pool2d_v2(layer.name, layer.kernel_size[1], layer.kernel_size[0], layer.stride[0], layer.stride[1], layer.padding[0], layer.padding[1])
  #   elif (isinstance(layer, Flatten) == True):
  #     layer.handle = self.ffmodel.flat_v2(layer.name)
  #   elif (isinstance(layer, Dense) == True):
  #     layer.handle = self.ffmodel.dense_v2(layer.name, layer.in_channels, layer.out_channels, layer.activation)
  #   elif (isinstance(layer, Activation) == True):
  #     print("add softmax")
  #   else:
  #     assert 0, "unknow layer"
      
  def compile(self, optimizer):
    self.ffoptimizer = optimizer
    optimizer.handle = ff.SGDOptimizer(self.ffmodel, optimizer.learning_rate)
    self.ffmodel.set_sgd_optimizer(self.ffoptimizer.handle)
    
  def create_input_and_label_tensor(self, input_shape, label_shape):
    if (len(input_shape) == 2):
      dims_input = [self.ffconfig.get_batch_size(), input_shape[1]]
      self.input_tensor = self.ffmodel.create_tensor_2d(dims_input, "", ff.DataType.DT_FLOAT);
      
    elif (len(input_shape) == 4):
      dims_input = [self.ffconfig.get_batch_size(), input_shape[1], input_shape[2], input_shape[3]]
      self.input_tensor = self.ffmodel.create_tensor_4d(dims_input, "", ff.DataType.DT_FLOAT);
    
    dims_label = [self.ffconfig.get_batch_size(), 1]
    self.label_tensor = self.ffmodel.create_tensor_2d(dims_label, "", ff.DataType.DT_INT32);
  
  def create_single_data_loader(self, batch_tensor, full_array):
    array_shape = full_array.shape
    num_dim = len(array_shape)
    print(array_shape)
    
    if (full_array.dtype == "float32"):
      datatype = ff.DataType.DT_FLOAT
    elif (full_array.dtype == "int32"):
      datatype = ff.DataType.DT_INT32
    else:
      assert 0, "unsupported datatype"

    if (num_dim == 2):
      dims_input = [self.num_samples, array_shape[1]]
      full_tensor = self.ffmodel.create_tensor_2d(dims_input, "", datatype);
    elif (num_dim == 4):
      dims_input = [self.num_samples, array_shape[1], array_shape[2], array_shape[3]]
      full_tensor = self.ffmodel.create_tensor_4d(dims_input, "", datatype);
    else:
      assert 0, "unsupported dims"
      
    full_tensor.attach_numpy_array(self.ffconfig, full_array)
    dataloader = ff.SingleDataLoader(self.ffmodel, batch_tensor, full_tensor, self.num_samples, datatype) 
    self.dataloaders.append(dataloader)
    self.dataloaders_dim.append(num_dim)
    full_tensor.detach_numpy_array(self.ffconfig)
    
    return full_tensor
    
    
  def create_data_loaders(self, x_train, y_train):
    input_shape = x_train.shape
    self.num_samples = input_shape[0]
    
    print(y_train.shape)
    self.full_input_tensor = self.create_single_data_loader(self.input_tensor, x_train)
    self.full_label_tensor = self.create_single_data_loader(self.label_tensor, y_train)
      
  def train(self, epochs):
    ts_start = self.ffconfig.get_current_time()
    for epoch in range(0,epochs):
      for dataloader in self.dataloaders:
        dataloader.reset()
      self.ffmodel.reset_metrics()
      iterations = self.num_samples / self.ffconfig.get_batch_size()

      for iter in range(0, int(iterations)):
        for dataloader in self.dataloaders:
          dataloader.next_batch(self.ffmodel)
        if (epoch > 0):
          self.ffconfig.begin_trace(111)
        self.ffmodel.forward()
        #for layer_id in self._layers:
         # layer = self._layers[layer_id]
        #  layer.handle.forward(self.ffmodel)
        self.ffmodel.zero_gradients()
        self.ffmodel.backward()
        self.ffmodel.update()
        if (epoch > 0):
          self.ffconfig.end_trace(111)

    ts_end = self.ffconfig.get_current_time()
    run_time = 1e-6 * (ts_end - ts_start);
    print("epochs %d, ELAPSED TIME = %.4fs, interations %d, samples %d, THROUGHPUT = %.2f samples/s\n" %(epochs, run_time, int(iterations), self.num_samples, self.num_samples * epochs / run_time));
    
    self.label_tensor.inline_map(self.ffconfig)
    label_array = self.label_tensor.get_array(self.ffconfig, ff.DataType.DT_INT32)
    print(label_array.shape)
    print(label_array)
    self.label_tensor.inline_unmap(self.ffconfig)
    
  def fit(self, input_tensor, label_tensor, epochs=1):
    self.create_input_and_label_tensor(input_tensor.shape, label_tensor.shape)
    self.create_data_loaders(input_tensor, label_tensor)
    
    # if (self.use_v2 == True):
    #   self._init_inout(input_tensor, label_tensor)
    # else:
    self._create_layer_and_init_inout()        
    self.ffmodel.init_layers()
    
    self.train(epochs)
    
    