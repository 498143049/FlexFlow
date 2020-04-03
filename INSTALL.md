# FlexFlow Installation
FlexFlow can be built from source code using the following instructions.

## Build FlexFlow Runtime

* To get started, clone the FlexFlow source code from github.
```
git clone --recursive https://github.com/flexflow/FlexFlow.git
cd FlexFlow
```
The `FF_HOME` exvirnoment is used for building and running FlexFlow. You can add the following line in `~/.bashrc`.
```
export FF_HOME=/path/to/FlexFlow
```

* Build the Protocol Buffer library.
Skip this step if the Protocol Buffer library is already installed.
```
cd protobuf
git submodule update --init --recursive
./autogen.sh
./configure
make
```
* Build the NCCL library.
```
cd nccl
make -j src.build NVCC_GENCODE="-gencode=arch=compute_XX,code=sm_XX"
```
Replace XX with the compatability of your GPU devices (e.g., 70 for Volta GPUs and 60 for Pascal GPUs).

* Build a DNN model in FlexFlow
Use the following command line to build a DNN model (e.g., InceptionV3). See the [examples](examples) folders for more existing FlexFlow applications.
```
./ffcompile.sh examples/InceptionV3
```
