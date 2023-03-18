#include "FileUtil.h"

//fopen()函数用于打开文件,返回一个文件流指针
//第一个参数为文件名,第二个参数为打开方式 ae 以追加的方式打开文件,
FileUtil::FileUtil(const std::string& filename)
    :fp_(fopen(filename.c_str(), "ae")),
    writtenBytes_(0)
{
    setbuffer(fp_, buffer_, sizeof buffer_);
    //setbuffer()函数用来设置文件流的缓冲区
    //第一个参数为文件流指针，第二个参数为缓冲区指针，第三个参数为缓冲区大小
}

FileUtil::~FileUtil(){
    fclose(fp_);
}

void FileUtil::append(const char* data, const size_t len){
   size_t written = 0;
   while(written != len){
       size_t remain = len - written;
       size_t n = write(data + written, remain);
       //write()需要提前设置缓冲区，否则会出现段错误
       //write()函数用来将数据写入文件
       if(n != remain){
           int err = ferror(fp_);
           if(err){
               fprintf(stderr, "FileUtil::append() failed %s\n", getErrnoMsg(err));
           }
           break;
       }
       written += n;
   }
    writtenBytes_ += written;
}

void FileUtil::flush(){
    fflush(fp_);//刷新缓冲区到文件
}

size_t FileUtil::write(const char *data, size_t len){
    return fwrite_unlocked(data, 1, len, fp_);
    //fwrite_unlocked()函数用来将数据写入文件
    //第一个参数为数据指针，第二个参数为数据块大小，第三个参数为数据块个数，第四个参数为文件流指针
}