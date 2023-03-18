#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include "Logging.h"

#include <string>
#include <stdio.h>

//封装了文件操作，实现了文件的写入，刷新
class FileUtil {
public:
    explicit FileUtil(const std::string& filename);
    ~FileUtil();

    void append(const char* data, const size_t len);
    void flush();
    off_t writtenBytes() const { return writtenBytes_; }
private:
    size_t write(const char *data, size_t len);
    FILE* fp_;
    char buffer_[64 * 1024];
    off_t writtenBytes_;//已经写入的字节数
};




#endif