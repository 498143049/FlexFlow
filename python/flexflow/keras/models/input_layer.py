import flexflow.core as ff

class Tensor(object):
  def __init__(self, ffmodel=0, batch_shape=0, name=0, dtype=0, meta_only=False, ffhandle=0):
    self.ffhandle = ffhandle
    self.input_layers = []
    self.output_layers = []
    print(dtype)
    if (dtype == "float32" or dtype == ff.DataType.DT_FLOAT):
      self.dtype = ff.DataType.DT_FLOAT
    elif (dtype == "float64" or dtype == ff.DataType.DT_DOUBLE):
      self.dtype = ff.DataType.DT_DOUBLE
    elif (dtype == "int32" or dtype == ff.DataType.DT_INT32):
      self.dtype = ff.DataType.DT_INT32
    elif (dtype == "int64" or dtype == ff.DataType.DT_INT64):
      self.dtype = ff.DataType.DT_INT64
    else:
      assert 0, "not supported"
    # create a tensor
    if (ffhandle == 0):
      self.batch_shape = batch_shape
      self.name = name
      self.num_dims = len(batch_shape)
      if (meta_only == False):
        self.__create_ff_tensor(ffmodel)
    # init from handle
    else:
      self.name = ""
      self.num_dims = ffhandle.num_dims
      self.batch_shape = ffhandle.dims
    
  def __create_ff_tensor(self, ffmodel):
    if (self.num_dims == 2):
      self.ffhandle = ffmodel.create_tensor_2d(self.batch_shape, self.name, self.dtype);
    elif (self.num_dims == 4):
      self.ffhandle = ffmodel.create_tensor_4d(self.batch_shape, self.name, self.dtype);
    else:
      assert 0, "un-supported dims"
    

class Input(Tensor):
  def __init__(self, shape=None, batch_shape=None,
               name="", dtype=None, sparse=False,
               tensor=None):
    super(Input, self).__init__(0, batch_shape, name, dtype, meta_only=True) 
    