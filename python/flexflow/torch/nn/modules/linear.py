from flexflow.core import *

from .op import Op

class Linear(Op):
  def __init__(self, in_features, out_features, bias=True):
    print("create Linear")
    self.in_features = in_features
    self.out_features = out_features
    self.handle = 0
    
  def __call__(self, input):
    return self.forward(input)
      
  def forward(self, input):
    input_tensor = input[0]
    ffmodel = input[1]
    print("linear forward ", self._layer_id);
    output_tensor = self.handle.init_input(ffmodel, input_tensor);
    return [output_tensor, ffmodel]