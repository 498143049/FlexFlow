import flexflow.core as ff

class BaseModel(object):
  def __init__(self):
    self.ffconfig = ff.FFConfig()
    self.ffconfig.parse_args()
    print("Python API batchSize(%d) workersPerNodes(%d) numNodes(%d)" %(self.ffconfig.get_batch_size(), self.ffconfig.get_workers_per_node(), self.ffconfig.get_num_nodes()))
    self.ffmodel = ff.FFModel(self.ffconfig)
    
    self.ffoptimizer = 0
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
    
  def get_layer(self, layer_id):
    return self._layers[layer_id]
    
  def compile(self, optimizer):
    self.ffoptimizer = optimizer
    optimizer.handle = ff.SGDOptimizer(self.ffmodel, optimizer.learning_rate)
    self.ffmodel.set_sgd_optimizer(self.ffoptimizer.handle)
    
  def __create_single_data_loader(self, batch_tensor, full_array):
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
    
  def _create_data_loaders(self, x_train, y_train):
    input_shape = x_train.shape
    self.num_samples = input_shape[0]
    
    print(y_train.shape)
    self.full_input_tensor = self.__create_single_data_loader(self.input_tensor, x_train)
    self.full_label_tensor = self.__create_single_data_loader(self.label_tensor, y_train)
    
  def _train(self, epochs):
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

    self.input_tensor.inline_map(self.ffconfig)
    input_array = self.input_tensor.get_flat_array(self.ffconfig, ff.DataType.DT_FLOAT)
    print(input_array.shape)
    print(input_array)
    self.input_tensor.inline_unmap(self.ffconfig)
    
    self.label_tensor.inline_map(self.ffconfig)
    label_array = self.label_tensor.get_flat_array(self.ffconfig, ff.DataType.DT_INT32)
    print(label_array.shape)
    print(label_array)
    self.label_tensor.inline_unmap(self.ffconfig)