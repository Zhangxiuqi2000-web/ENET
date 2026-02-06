#ifndef _BUFFERREADER_H_
#define _BUFFERREADER_H_
#include <vector>
#include <cstdint>
#include <string>

uint32_t ReadUint32BE(char* data);  //4字节大端读
uint32_t ReadUint32LE(char* data);  //4字节小端读
uint32_t ReadUint24BE(char* data);  //3字节大端读
uint32_t ReadUint24LE(char* data);  //3字节小端读
uint16_t ReadUint16BE(char* data);  //2字节大端读
uint16_t ReadUint16LE(char* data);  //2字节小端读

class BufferReader
{
public:
    BufferReader(uint32_t initial_size = 2048);
    virtual ~BufferReader();

    inline uint32_t ReadableBytes() const{return writer_index_ - reader_index_;}  //获取当前缓冲区可读数据大小
    inline uint32_t WritableBytes() const{return buffer_.size() - writer_index_;}  //获取当前缓冲区可写数据大小，如果写入数据大需要扩容

    char* Peek(){return Begin() + reader_index_;}  //获取可读数据的首地址
    const char* Peek()const{return Begin() + reader_index_;}

    void RetrieveAll()  //初始化读写索引
    {
        writer_index_ = 0;
        reader_index_ = 0;
    }
    void Retrieve(size_t len)  //缓冲区读取len的数据后，更新读写索引
    {
        if(len < ReadableBytes())
        {
            reader_index_ += len;
            if(reader_index_ == writer_index_)
            {
                RetrieveAll();
            }
        }
        else
        {
            RetrieveAll();
        }
    }

    int read(int fd);  //向缓冲区写入数据
    uint32_t ReadAll(std::string& data);  //获取缓冲区所有数据
    uint32_t Size() const{return buffer_.size();}

private:
    char* Begin()
    {
        return &*buffer_.begin();
    }

    const char* Begin() const
    {
        return &*buffer_.begin();
    }

    char* BeginWrite()
    {
        return Begin() + writer_index_;
    }

    const char* BeginWrite() const
    {
        return Begin() + writer_index_;
    }
private:
    std::vector<char> buffer_;
    size_t reader_index_ = 0;
    size_t writer_index_ = 0;
    static const uint32_t MAX_BYTES_PER_READ = 4096;
    static const uint32_t MAX_BUFFER_SIZE = 1024 * 100000;
};
#endif