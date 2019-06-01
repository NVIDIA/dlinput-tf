#ifndef NVTFZMQ_ZMQ_BASE_H_
#define NVTFZMQ_ZMQ_BASE_H_

#include <string>
#include <vector>
#include <zmq.hpp>
#include "datatypes.h"
#include "logging.hpp"
#include "tensorflow/core/framework/resource_mgr.h"

using std::string;
using std::vector;
using tensorflow::ResourceBase;

namespace nvdli {

/**
 * A resource contrains zmq context and socket.
 * ResourceBase class definition:
 * https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/framework/resource_mgr.h
 */
class ZmqConnResource : public ResourceBase {
   public:
    ZmqConnResource(Zinfo zinfo) : ctx_(1), socket(ctx_, zinfo.pattern) {
        try {
            const char *c = zinfo.addr.c_str();
            log(INFO, __FILE__, __LINE__, "Connecting to zmq with address: ",
                c);
            if (zinfo.is_bind) {
                socket.bind(zinfo.addr);
            } else {
                socket.connect(zinfo.addr);
            }
            socket.setsockopt(ZMQ_LINGER, 0);
            // TODO: keep or delete opt settings
            socket.setsockopt(ZMQ_RCVHWM, zinfo.hwm);
            socket.setsockopt(ZMQ_RCVBUF, zinfo.buff_size);
            if (zinfo.pattern == ZMQ_SUB) {
                socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
            }
        } catch (zmq::error_t e) {
            const char *err_msg = e.what();
            log(ERROR, __FILE__, __LINE__, "Failed to connect to zmq ",
                err_msg);
        }
    }

    ~ZmqConnResource() {}

    string DebugString() override { return "ZmqCtx"; }

    zmq::context_t *GetCtx() { return &ctx_; }

    bool Recv(zmq::message_t *msg) {
        // TODO: reconsider blocking and timeout.
        // poll_item_ = pollitem_t({static_cast<void *>(socket), 0, ZMQ_POLLIN,
        // 0});

        // // If using timeout, the third param should be set to TIMEOUT.
        // zmq::poll(&poll_item_, 1, -1);
        // if (poll_item_.revents & ZMQ_POLLIN) {
        bool ret = false;
        {
            std::lock_guard<std::mutex> guard(mu_);
            ret = socket.recv(msg);
        }
        // } else {
        //     context->CtxFailure(
        //         errors::Cancelled("Waiting for zmq input timeout."));
        //     return;
        // }
        return ret;
    }

    /**
     * Receive multipart msgs from zmq socket.
     * Can also hanfle single part msg.
     * If single part, output a list with only one msg.
     * @param parts
     * @return num of multiparts
     */
    int RecvMultiparts(vector<zmq::message_t> &parts) {
        bool has_next = true;
        {
            std::lock_guard<std::mutex> guard(mu_);
            while (has_next) {
                zmq::message_t msg;
                if (!socket.recv(&msg)) {
                    break;
                }
                has_next = msg.more();
                parts.push_back(std::move(msg));
            }
        }
        return parts.size();
    }

   private:
    // const int TIMEOUT = 3000;
    std::mutex mu_;
    zmq::context_t ctx_;
    zmq::socket_t socket;
    // zmq::pollitem_t poll_item_;
};

}  // nvdli

#endif  // NVTFZMQ_ZMQ_BASE_H_
