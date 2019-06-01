#ifndef NVTFZMQ_DATATYPES_H_
#define NVTFZMQ_DATATYPES_H_

#include <msgpack.hpp>
#include <sstream>
#include <string>
#include <vector>

using std::string;

namespace nvdli {

// Tensorflow Op readable data. msgpack dict with only _dtype, _shape, _data
struct TensorMsg {
    string _dtype;               // datatype of the incoming tensor.
    std::vector<size_t> _shape;  // shape of the tensor.
    std::vector<char> _data;     // binary data of the tensor.
    int _part;                   // index of data in multipart messages
    // TODO _logscale, __scale

    bool operator==(const TensorMsg &tsmsg) const {
        return !_dtype.compare(tsmsg._dtype) &&
               _shape.size() == tsmsg._shape.size() &&
               _data.size() == tsmsg._data.size() &&
               std::equal(_shape.begin(), _shape.end(), tsmsg._shape.begin()) &&
               std::equal(_data.begin(), _data.end(), tsmsg._data.begin());
    }

    string Debug() {
        std::ostringstream ss;
        ss << " _shape";
        for (int i = 0; i < _shape.size(); ++i) {
            ss << " " << _shape[i];
        }
        ss << " _dtype: " << _dtype << " _part: " << _part;
        ss << " _data: " << string(_data.begin(), _data.end());
        return ss.str();
    }

    MSGPACK_DEFINE_MAP(_dtype, _shape, _data, _part);
};

struct UrlInfo {
    string scheme;
    string netloc;
};

struct Zinfo {
    string addr;
    int pattern;
    int hwm;
    int buff_size;
    bool is_bind;
};

}  // nvdli

#endif  // NVDLI_DATATYPES_H_
