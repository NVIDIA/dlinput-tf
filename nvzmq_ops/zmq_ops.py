from tensorflow.python.framework import load_library
from tensorflow.python.platform import resource_loader

zmq_ops = load_library.load_op_library(
    resource_loader.get_path_to_datafile('nvzmq_ops.so'))
zmq_conn_handle = zmq_ops.zmq_conn_handle
zmq_op = zmq_ops.nv_zmq

'''
TODO: update when kernel changes.
'''
class ZmqOp(object):
    def __init__(self, address, zmq_hwm=0, zmq_buff=0):
        self._zmq_conn_handle = zmq_conn_handle(address, zmq_hwm, zmq_buff)
        self._address = address

    @property
    def address(self):
        return self._address
    
    def pull(self, types):
        return zmq_op(handle=self._zmq_conn_handle, types=types)