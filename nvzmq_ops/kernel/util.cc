#include "util.h"
#include <zmq.hpp>

namespace nvdli {

std::unordered_map<string, pair<int, bool>> SCHEME_MAP = {
    {"zpush", {ZMQ_PUSH, false}}, {"zpull", {ZMQ_PULL, true}},
    {"zpub", {ZMQ_PUB, true}},    {"zsub", {ZMQ_SUB, false}},
    {"zrpush", {ZMQ_PUSH, true}}, {"zrpull", {ZMQ_PULL, false}},
    {"zrpub", {ZMQ_PUB, false}},  {"zrsub", {ZMQ_SUB, true}}};

UrlInfo UrlParse(const string &url) {
    UrlInfo urlinfo;
    auto pos = url.find_first_of(':');
    if (pos == string::npos || pos + 3 >= url.length()) {
        return urlinfo;
    }
    urlinfo.scheme = url.substr(0, pos);
    urlinfo.netloc = url.substr(pos + 3);
    return urlinfo;
}

bool ParseScheme(Zinfo &zinfo, const UrlInfo &urlinfo) {
    if (SCHEME_MAP.find(urlinfo.scheme) == SCHEME_MAP.end()) {
        return false;
    }
    zinfo.addr = "tcp://" + urlinfo.netloc;
    auto p = SCHEME_MAP[urlinfo.scheme];
    zinfo.pattern = p.first;
    zinfo.is_bind = p.second;
    return true;
}

}  // nvdli
