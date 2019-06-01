# nvda-zmq-op

This is a tensorflow operator that allows you to pull tensors from an upstream pipeline using ZMQ

# Requirements
Please check or install your requirements in following order.
- gnu gcc & g++ >= 5
- c++11
- python3
- cmake

# Install instructions
Check permission of requirements.sh, makesure it can be executed.
```
python3 -m pip install .
or
pip3 install .
```

# Examples
There are examples in `./example/` folder. It's .ipynb file and can be run by jupyter notebook.
```
python3 -m pip install --upgrade pip
python3 -m pip install jupyter
python3 -m pip install ipykernel
python3 -m ipykernel install --user
jupyter notebook --ip=127.0.0.1
```

Init submodule tensorcom `git submodule update --init --recursive`

Install pytorch for tensorcom server-mnist.

Then you can run mnist data server in `tensorcom/` dir with `./serve-mnist zpub://127.0.0.1:7880` 

# Tests & Dev
1.  Build source
```
mkdir build
cd build
cmake ..
make
```
2. After running the build steps you can invoke the tests at `DLinput-TF/` with `python3 test/zmq_ops_test.py`

3. Examples
    * To init submodel: `git submodule update --init --recursive`
    * perftest and cmp need add following codes to tensorcom/zcom.py if using imagenet
        ```
        #insert under line 122
        self.socket.setsockopt(zmq.SNDBUF, 10240)
        self.socket.setsockopt(zmq.SNDHWM, 200)
        self.socket.setsockopt(zmq.RCVBUF, 10240)
        self.socket.setsockopt(zmq.RCVHWM, 200)
        ```
    * Simple_training need to fix [this](https://github.com/tensorflow/tensorflow/issues/24520) bug as PR[#24522](https://github.com/tensorflow/tensorflow/pull/24522) in installed tensorflow python source code currently.

      TODO: If it is merged into master and be released in the future, delete this instruction.
        ```
        locate training_utils.py
        ```
        modify the one in `/home/<your username>/.local/lib/python3.5` and the other in `/usr/local/lib/python3.5`
        
        e.g. on my machine one is `/home/xxx/.local/lib/python3.5/site-packages/tensorflow/python/keras/engine/training_utils.py`
        
        search `standardize_single_array` 

        ```
        # change 
        if (x.shape is not None
            and len(x.shape) == 1
            and (expected_shape is None or len(expected_shape) != 1)):

        # to
        if tensor_util.is_tensor(x):
            x_shape_ndims = array_ops.rank(x)
        else:
            x_shape_ndims = len(x.shape)
        if (x_shape_ndims == 1
            and (expected_shape is None or len(expected_shape) != 1)):
        ```


# Documentation
Python documentation is automatically generated at build, use `help()` in python on entities to access the documentation

## Structure
```
DLinput-TF
|--zmq_ops
    |--kernel       # c++ kernel codes
        |-- zmq_ops.cc  # Op registration file
        |-- zmq_base.h  # zmq connection resource base header & source file
    |--zmq_ops.py   # wrap kernel
    |--__init__.py  # python package init file
|--test # unittests
|--example  # some examples
```

## Coding style: Google 4 indent
format all c++ header and source files in current directories
```
./format.sh
```
