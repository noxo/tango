#ifndef UTILS_IO_H
#define UTILS_IO_H

#include <android/log.h>
#include <sstream>

#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "tango_app", __VA_ARGS__)
#define LOGE(...) \
  __android_log_print(ANDROID_LOG_ERROR, "tango_app", __VA_ARGS__)

namespace oc {
    class IO {

    public:
        static inline std::string GetFileName(std::string dataset, int index, std::string extension) {
            std::ostringstream ss;
            ss << dataset.c_str();
            ss << "/";
            ss << index;
            ss << extension.c_str();
            return ss.str();
        }
    };
}
  
#endif
