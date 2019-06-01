#include "tensorflow/core/framework/common_shape_fns.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference.h"

using namespace tensorflow;

namespace nvdli {

REGISTER_OP("NvZmq")
    .Input("handle: resource")
    .Attr("types: list(numbertype)")
    .Output("queue_msg: types")
    .SetIsStateful()
    .Doc(R"doc(
    Creates tensors from upstream ZMQ using msgpack-c

    handle:  resource - The zmq connection resource
    types:   numbertype   - The type of tensor any tensorflow numeric type is valid

    Returns
            tensors - A list of tensors corresponding to the provided input types

    Throws
            zmq_error - If any errors occur while attempting to connect or read from the zmq
            msgpack_error - If any errors occur while attempting to deserialize the binary queue data
            InvalidArgument - If any type mismatches between the provided type or binary data types occur

    Encoding Format:
        Tensor msgpack is encoded into message pack json like data, and upstream stages are expected to adhere to the following format:

        inline format:
        [{_dtype, _shape, _data}, {_dtype, _shape, _data}]

        multipart messages format:
        [{_dtype, _shape, _part}, {_dtype, _shape, __part}, {_dtype, _shape, __part}], b'data', b'data', b'data'
        
        P.S. b means binary

        string _dtype;               // datatype of the incoming tensor.
        std::vector<size_t> _shape;  // shape of the tensor.
        std::vector<char> _data;     // binary data of the tensor.
        int _part;                   // index of data in multipart messages

    The dtype int is encoded using the tensorflow DataType type, with the encoding defined in:
    https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/framework/types.proto
    )doc");

REGISTER_OP("ZmqConnHandle")
    .Attr("address: string")
    .Attr("zmq_hwm: int")
    .Attr("zmq_buff: int")
    .Attr("container: string = ''")
    .Attr("shared_name: string = ''")
    .Output("handle: resource")
    .SetIsStateful()
    .SetShapeFn(tensorflow::shape_inference::ScalarShape)
    .Doc(R"doc(
    Creates zmq context and socket

    address:    string  - The endpoint for the zmq socket to connect to
    zmq_hwm:    int     - The max number of zmq receive queue
    zmq_buff:   int     - The receive buffer size of zmq socket

    Returns
            resource -  The zmq connection resource

    Throws
            InvalidArgument - Invalid path for creating, binding/connecting the zmq socket.

    )doc");
}  // nvdli