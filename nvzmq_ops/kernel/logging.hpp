#ifndef NVTFZMQ_LOGGING_H_
#define NVTFZMQ_LOGGING_H_

#include <sstream>

#include "tensorflow/core/platform/logging.h"

using namespace tensorflow;

namespace nvdli {

  template <typename LogItem>
  static void log_dump(std::ostringstream& output, LogItem&& last_item) {
    output << last_item;
  }

  template <typename LogItem, typename... LogItems>
  static void log_dump(std::ostringstream& output, LogItem&& item, LogItems&&... items) {
    output << item;
    log_dump(output, items...);
  }

  template <typename LogItem, typename... LogItems>
  static void log(int severity, const char* fname, size_t line_no, LogItem& item, LogItems&... items) {
    std::ostringstream log_msg;
    log_dump(log_msg, item, items...);
    internal::LogString(fname, line_no, severity, log_msg.str());
  }

} //nvdli

#endif