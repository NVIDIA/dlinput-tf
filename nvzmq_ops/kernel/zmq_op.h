#ifndef NVTFZMQ_ZMQ_OP_H_
#define NVTFZMQ_ZMQ_OP_H_

#include <vector>
#include "datatypes.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/resource_mgr.h"
#include "tensorflow/core/framework/resource_op_kernel.h"
#include "tensorflow/core/framework/tensor_shape.h"
#include "util.h"
#include "zmq_base.h"

using zmq::message_t;
using std::string;
using namespace tensorflow;

namespace nvdli {

/**
 * A ZmqRecourceHandleOp is managed by TF ResourceManager.
 * Only create new resource if necessary. Can guarantee one zmq_context in one
 * process.
 * ResourceOpKernel class definition:
 * https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/framework/resource_op_kernel.h
 */
class ZmqConnHandleOp : public ResourceOpKernel<ZmqConnResource> {
   public:
    explicit ZmqConnHandleOp(OpKernelConstruction *ctx)
        : ResourceOpKernel(ctx) {
        OP_REQUIRES_OK(ctx, ctx->GetAttr("address", &addr_));
        OP_REQUIRES_OK(ctx, ctx->GetAttr("zmq_hwm", &hwm_));
        OP_REQUIRES_OK(ctx, ctx->GetAttr("zmq_buff", &buff_size_));
    }

    ZmqConnResource *GetResource() LOCKS_EXCLUDED(mu_) {
        mutex_lock lock(mu_);
        return resource_;
    }

   private:
    Status CreateResource(ZmqConnResource **zmq_ctx)
        EXCLUSIVE_LOCKS_REQUIRED(mu_) override {
        auto urlinfo = UrlParse(addr_);
        Zinfo zinfo;
        if (!ParseScheme(zinfo, urlinfo)) {
            const char *c = addr_.c_str();
            log(ERROR, __FILE__, __LINE__, "Invalid scheme from url: ", c);
            return Status(error::INVALID_ARGUMENT, "Invalid url");
        }
        zinfo.hwm = hwm_;
        zinfo.buff_size = buff_size_;
        *zmq_ctx = new ZmqConnResource(zinfo);
        return Status::OK();
    }

    string addr_;
    int hwm_;
    int buff_size_;
};

/**
 * AsyncOpKernel class definition:
 * https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/framework/op_kernel.h
 *
 */
class NvZmqOp : public AsyncOpKernel {
   public:
    explicit NvZmqOp(OpKernelConstruction *context) : AsyncOpKernel(context) {
        OP_REQUIRES_OK(context, context->GetAttr("types", &types_));
    }

    virtual ~NvZmqOp() {}

    void ComputeAsync(OpKernelContext *context, DoneCallback done) override;

   private:
    DataTypeVector types_;
};

Status ZmqFunctorCPU(OpKernelContext *context, TensorMsg *tensor_msg,
                     std::vector<zmq::message_t> *parts, Tensor *out);

// Register c++ kernel
REGISTER_KERNEL_BUILDER(Name("NvZmq").Device(DEVICE_CPU), NvZmqOp);
REGISTER_KERNEL_BUILDER(Name("ZmqConnHandle").Device(DEVICE_CPU),
                        ZmqConnHandleOp);

}  // nvdli

#endif
