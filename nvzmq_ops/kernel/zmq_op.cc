#include "zmq_op.h"
#include <algorithm>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include "logging.hpp"

using namespace tensorflow;
using errors::InvalidArgument;
using errors::DataLoss;
using zmq::pollitem_t;

namespace nvdli {

// A map from string to DataType, only includes strings different between numpy
// dtype and TF dtype
std::unordered_map<std::string, DataType> STRING_DTYPE_MAP = {
    {"float16", DT_HALF}, {"float32", DT_FLOAT}, {"float64", DT_DOUBLE}};

Status ZmqFunctorCPU(OpKernelContext *context, TensorMsg *tensor_msg,
                     vector<zmq::message_t> *parts, Tensor *out) {
    long long num_bytes =
        DataTypeSize(out->dtype()) * out->shape().num_elements();
    auto out_data = out->bit_casted_shaped<char, 1>({num_bytes}).data();
    // Fill TensorMsg._data from multipart msgs.
    if (tensor_msg->_part > 0) {
        int idx = tensor_msg->_part;
        // Data parts not wrapped by msgpack, copy directly.
        memcpy(out_data, (*parts)[idx].data(), (*parts)[idx].size());
    } else {
        // TODO: handle non continuous data likes one in dali.
        memcpy(out_data, tensor_msg->_data.data(), num_bytes);
    }

    return Status::OK();
}

bool UnpackRawMsgpack(std::vector<TensorMsg> &tensor_msgs,
                      vector<zmq::message_t> &parts) {
    try {
        msgpack::object_handle tensor_handle = msgpack::unpack(
            static_cast<const char *const>(parts[0].data()), parts[0].size());
        msgpack::object obj = tensor_handle.get();
        // TODO: if data is always valid as mentioned in doc,
        // we can delete type switch.
        // switch (obj.type) {
        //     case msgpack::type::ARRAY:
        tensor_msgs = obj.as<std::vector<TensorMsg> >();
        return true;
        //     case msgpack::type::MAP:
        //         TensorMsg value = obj.as<TensorMsg>();
        //         tensor_msgs.push_back(value);
        //         return true;
        // }
        // return false;
    } catch (msgpack::type_error e) {
        const char *err_msg = e.what();
        log(ERROR, __FILE__, __LINE__, "msgpack parse error occurred ",
            err_msg);
        return false;
    } catch (msgpack::parse_error e) {
        const char *err_msg = e.what();
        log(ERROR, __FILE__, __LINE__, "msgpack parse error occurred ",
            err_msg);
        return false;
    }
}

bool ParseDataTypeFromString(const string &s, DataType *dt) {
    if (STRING_DTYPE_MAP.find(s) != STRING_DTYPE_MAP.end()) {
        *dt = STRING_DTYPE_MAP[s];
        return true;
    }
    return DataTypeFromString(s, dt);
}

/**
 *  What the Operator will do.
 *  Receive rawdata from zmq, fill data to output tensors.
 *  @param context
 */
void NvZmqOp::ComputeAsync(OpKernelContext *context, DoneCallback done) {
    ZmqConnResource *zmq_conn_res = nullptr;
    // Get zmq connection handler.
    OP_REQUIRES_OK(context, LookupResource(context, HandleFromInput(context, 0),
                                           &zmq_conn_res));
    // Read raw msg from zmq.
    std::vector<zmq::message_t> parts;
    if (!zmq_conn_res->RecvMultiparts(parts)) {
        context->CtxFailure(DataLoss("ZMQ message recv failed."));
        done();
        return;
    }

    std::vector<TensorMsg> tensor_msgs;
    OP_REQUIRES_ASYNC(context, UnpackRawMsgpack(tensor_msgs, parts),
                      DataLoss("Failed to deserialize msgpack payload"), done);

    for (size_t t_idx = 0; t_idx < tensor_msgs.size(); t_idx++) {
        auto &tensor_msg = tensor_msgs[t_idx];
        // Generate tensorflow::Datatype from tensor_meta.dtype and do
        // validation.
        DataType dtype;
        OP_REQUIRES_ASYNC(
            context, ParseDataTypeFromString(tensor_msg._dtype, &dtype),
            InvalidArgument("Failed to parse dtype string to TF dtype",
                            tensor_msg._dtype),
            done);
        auto actual_dtype = DataTypeString(dtype);

        OP_REQUIRES_ASYNC(
            context, types_[t_idx] == dtype,
            InvalidArgument("Type mismatch at index ", std::to_string(t_idx),
                            " between received tensor (", actual_dtype,
                            ") and type (", DataTypeString(types_[t_idx]), ")"),
            done);

        // Init a TensorShape for current output_tensor.
        TensorShape output_shape;
        for (auto &dim : tensor_msg._shape) {
            output_shape.AddDim(dim);
        }

        Tensor *output_tensor = nullptr;
        OP_REQUIRES_OK_ASYNC(context, context->allocate_output(
                                          t_idx, output_shape, &output_tensor),
                             done);

        // TODO: If want a GPU kernel, write a GPUFunctor using cudaMemcpy
        // Since TF automatically handle cpu to gpu tensor copy,
        // currently it's not neccesary.
        ZmqFunctorCPU(context, &tensor_msg, &parts, output_tensor);
    }
    done();
}

}  // nvdli
