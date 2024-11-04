#include "common/util.h"

std::string JoinPath(const std::string& path, const std::string& filename) {
    if (path.empty()) {
        return filename;
    }

    // 检查路径最后一个字符是否是路径分隔符
    char last_char = path[path.size() - 1];
    std::string separator;

#if defined(_WIN32) || defined(_WIN64)
    // Windows平台
    separator = "\\";
    if (last_char != '\\' && last_char != '/') {
        return path + separator + filename;
    }
#else
    // 类Unix平台
    separator = "/";
    if (last_char != '/') {
        return path + separator + filename;
    }
#endif

    // 如果路径已经有了分隔符，直接拼接文件名
    return path + filename;
}
