from flexflow.core import *

import numpy as np

def top_level_task():
  ffconfig = FFConfig()
  ffconfig.parse_args()
  print("Python API batchSize(%d) workersPerNodes(%d) numNodes(%d)" %(ffconfig.get_batch_size(), ffconfig.get_workers_per_node(), ffconfig.get_num_nodes()))
  ffmodel = FFModel(ffconfig)
  
  dims1 = [ffconfig.get_batch_size(), 3, 229, 229]
  input1 = ffmodel.create_tensor_4d(dims1, "", DataType.DT_FLOAT);
  
  dims2 = [ffconfig.get_batch_size(), 16]
  input2 = ffmodel.create_tensor_2d(dims2, "", DataType.DT_FLOAT);
  
  dims_label = [ffconfig.get_batch_size(), 1]
  label = ffmodel.create_tensor_2d(dims_label, "", DataType.DT_INT32);
  
  t1 = ffmodel.conv2d("conv1", input1, 64, 11, 11, 4, 4, 2, 2) 
  t2 = ffmodel.dense("dense1", input2, 8, ActiMode.AC_MODE_RELU)
  #t3 = ffmodel.dense("dense1", t2, 128, ActiMode.AC_MODE_RELU)
  
  # Data Loader
  alexnetconfig = NetConfig()
  dataloader = DataLoader4D(ffmodel, input1, label, ffnetconfig=alexnetconfig)
  
  ffmodel.init_layers()
  
  label.inline_map(ffconfig)
  label_array = label.get_array(ffconfig, DataType.DT_INT32)
  label_array *= 0
  label_array += 1
  print(label_array.shape)
  print(label_array)
  label.inline_unmap(ffconfig)
  
  #weight of conv2d
  t3 = ffmodel.get_tensor_by_id(0)
  t3.inline_map(ffconfig)
  t3_array = t3.get_array(ffconfig, DataType.DT_FLOAT)
  t3_array *= 0.0
  t3_array += 1.2
  print(t3_array.shape)
  t3.inline_unmap(ffconfig)
  
  
  conv_2d1 = ffmodel.get_layer_by_id(0)

  cbias_tensor = conv_2d1.get_bias_tensor()
  cbias_tensor.inline_map(ffconfig)
  cbias = cbias_tensor.get_array(ffconfig, DataType.DT_FLOAT)
  cbias *= 0.0
  cbias += 1.1
  print(cbias.shape)
  print(cbias)
  cbias_tensor.inline_unmap(ffconfig)

  cweight_tensor = conv_2d1.get_weight_tensor()
  cweight_tensor.inline_map(ffconfig)
  cweight = cweight_tensor.get_array(ffconfig, DataType.DT_FLOAT)
  #cweight += 1.2
  ct = 0.0
  for i in range(cweight.shape[0]):
    for j in range(cweight.shape[1]):
      for k in range(cweight.shape[2]):
        for l in range(cweight.shape[3]):
          cweight[i][j][k][l] += ct
          ct += 1.0
  print(cweight.shape)
  # print(cweight.strides)
  print(cweight)
  cweight_tensor.inline_unmap(ffconfig)
  
  dense1 = ffmodel.get_layer_by_id(1)

  dbias_tensor = dense1.get_bias_tensor()
  dbias_tensor.inline_map(ffconfig)
  dbias = dbias_tensor.get_array(ffconfig, DataType.DT_FLOAT)
  dbias *= 0.0
  dbias += 2.1
  print(dbias.shape)
  print(dbias)
  dbias_tensor.inline_unmap(ffconfig)

  dweight_tensor = dense1.get_weight_tensor()
  dweight_tensor.inline_map(ffconfig)
  dweight = dweight_tensor.get_array(ffconfig, DataType.DT_FLOAT)
  #dweight *= 0.0
  #dweight += 2.2
  ct = 0.0
  # for i in range(dweight.shape[0]):
  #   for j in range(dweight.shape[1]):
  #     dweight[i][j] = ct
  #     ct += 1.0
  # print(dweight.shape)
  # print(dweight.strides)
  # print(dweight)
  dweight_tensor.inline_unmap(ffconfig)
  
 # ffmodel.print_layers(0)

if __name__ == "__main__":
  print("alexnet")
  top_level_task()
