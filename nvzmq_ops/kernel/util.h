#ifndef NVTFZMQ_UTIL_H
#define NVTFZMQ_UTIL_H

#include <string>
#include <unordered_map>
#include <utility>
#include "datatypes.h"

using std::string;
using std::pair;

namespace nvdli {

// A map from scheme to (zmq pattern, is_bind)
// Alternative design: enum to pair/struct
extern std::unordered_map<string, pair<int, bool>> SCHEME_MAP;

// Have to guarantee url is valid XXX://XXX
// No strict code handler for invalid url or error.
UrlInfo UrlParse(const string &url);

/**
 * Convert scheme to (zmq_pattern, is_bind) pair
 * @param scheme
 * @return
 */
bool ParseScheme(Zinfo &zinfo, const UrlInfo &urlinfo);

}  // nvdli

#endif  // NVTFZMQ_UTIL_H
