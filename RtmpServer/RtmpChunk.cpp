#include "RtmpChunk.h"
#include <string.h>

int RtmpChunk::stream_id_ = 0;

RtmpChunk::RtmpChunk()
{
    state_ = PARSE_HEADER;
    chunk_stream_id_ = -1;
    ++stream_id_;
}

RtmpChunk::~RtmpChunk()
{
}

int RtmpChunk::Parse(BufferReader &in_buffer, RtmpMessage &out_rtmp_msg)
{
    int ret = 0;
    if(!in_buffer.ReadableBytes())
    {
        return 0;
    }

    if(state_ == PARSE_HEADER)
    {
        ret = ParseChunkHeader(in_buffer);
    }
    else
    {
        ret = ParseChunkBody(in_buffer);
        if(ret > 0 && chunk_stream_id_ >= 0)
        {
            auto& rtmp_msg = rtmp_message_[chunk_stream_id_];
            if(rtmp_msg.index == rtmp_msg.length)  //该message已经完整
            {
                if(rtmp_msg.timestamp >= 0xffffff)
                {
                    rtmp_msg._timestamp += rtmp_msg.extend_timestamp;
                }
                else
                {
                    rtmp_msg._timestamp += rtmp_msg.timestamp;
                }

                out_rtmp_msg = rtmp_msg;
                chunk_stream_id_ = -1;
                rtmp_msg.Clear();
            }
        }
    }
    return ret;
}

int RtmpChunk::CreatChunk(uint32_t csid, RtmpMessage &in_msg, char *buf, uint32_t buf_size)
{
    uint32_t buf_offset = 0, playload_offset = 0;
    uint32_t capacity = in_msg.length + in_msg.length / out_chunk_size_ *5;
    if(buf_size < capacity)
    {
        return -1;
    }

    buf_offset += CreatBasicHeader(0, csid, buf + buf_offset);
    buf_offset += CreatMessageHeader(0, in_msg, buf + buf_offset);
    if(in_msg._timestamp >= 0xffffff)
    {
        WriteUint32BE((char*)(buf + buf_offset), (uint32_t)in_msg.extend_timestamp);
        buf_offset += 4;
    }
    while(in_msg.length > 0)
    {
        if(in_msg.length > out_chunk_size_)
        {
            memcpy(buf + buf_offset, in_msg.playload.get() + playload_offset, out_chunk_size_);
            playload_offset += out_chunk_size_;
            buf_offset += out_chunk_size_;
            in_msg.length -= out_chunk_size_;

            buf_offset += CreatBasicHeader(3, csid, buf + buf_offset);
            if(in_msg._timestamp >= 0xffffff)
            {
                WriteUint32BE(buf + buf_offset, (uint32_t)in_msg.extend_timestamp);
                buf_offset += 4;
            }
        }
        else  //最后一个包
        {
            memcpy(buf + buf_offset, in_msg.playload.get() + playload_offset, in_msg.length);
            buf_offset += in_msg.length;
            in_msg.length = 0;
            break;
        }
    }
    return buf_offset;
}

int RtmpChunk::ParseChunkHeader(BufferReader &buffer)
{
    uint32_t bytes_used = 0;
    uint8_t* buf = (uint8_t*)buffer.Peek();
    uint32_t buf_size = buffer.ReadableBytes();

    //解析fmt
    uint8_t flags = buf[bytes_used];
    uint8_t fmt = flags << 6;
    if(fmt >= 4)  //fmt = 0,1,2,3
    {
        return -1;
    }
    bytes_used += 1;

    //解析csid
    uint8_t csid = flags & 0x3f;  //1字节类型
    if(csid == 0) //2字节类型
    {
        if(buf_size < bytes_used + 2)
        {
            return 0;
        }
        csid += buf[bytes_used] + 64;
        bytes_used += 1;
    }
    else if(csid == 1)  //3字节类型
    {
        if(buf_size < bytes_used + 3)
        {
            return 0;
        }
        csid += buf[bytes_used + 1] * 255 + buf[bytes_used] + 64;
        bytes_used += 2;
    }

    //解析message header
    uint32_t msg_head_len = KChunkMessageHeaderLength[fmt];  //根据fmt查找massageheader类型：11，7，3，0
    if(buf_size < (bytes_used + msg_head_len))
    {
        return 0;
    }
    RtmpMessageHeader msg_header;
    memset(&msg_header, 0, sizeof(RtmpMessageHeader));
    memcpy(&msg_header, buf + bytes_used, msg_head_len);  //将buf中的massageheader部分拷贝到msg_header中单独解析
    bytes_used += msg_head_len;

    auto& rtmp_msg = rtmp_message_[csid];
    chunk_stream_id_ = rtmp_msg.csid = csid;

    if(fmt == 0 || fmt == 1)
    {
        uint32_t length = ReadUint24BE((char*)msg_header.length);  //读取msg_header中关于信息长度的信息
        if(rtmp_msg.length != length || !rtmp_msg.playload)
        {
            rtmp_msg.length = length;
            rtmp_msg.playload.reset(new char[rtmp_msg.length], std::default_delete<char[]>());
        }
        rtmp_msg.index = 0;
        rtmp_msg.type_id = msg_header.type_id;
    }
    if(fmt == 0)
    {
        rtmp_msg.stream_id = ReadUint32LE((char*)msg_header.stream_id);
    }

    uint32_t timestamp = ReadUint24BE((char*)msg_header.timestamp);
    uint32_t extend_timestamp = 0;
    if(timestamp >= 0xffffff || rtmp_msg.timestamp >= 0xffffff)
    {
        if(buf_size < bytes_used + 4)
        {
            return 0;
        }
        extend_timestamp = ReadUint32BE((char*)buf + bytes_used);
        bytes_used += 4;
    }

    if(rtmp_msg.index == 0)  //消息的首个数据块，初始化时间戳
    {
        if(fmt == 0)
        {
            rtmp_msg._timestamp = 0;
            rtmp_msg.timestamp = timestamp;
            rtmp_msg.extend_timestamp = extend_timestamp;
        }
        else
        {
            if(rtmp_msg.timestamp >= 0xffffff)
            {
                rtmp_msg.extend_timestamp += extend_timestamp;
            }
            else
            {
                rtmp_msg.timestamp += timestamp;
            }
        }
    }

    state_ = PARSE_BODY;
    buffer.Retrieve(bytes_used);
    return bytes_used;
}

int RtmpChunk::ParseChunkBody(BufferReader &buffer)
{
    uint32_t bytes_used = 0;
    uint8_t* buf = (uint8_t*)buffer.Peek();
    uint32_t buf_size = buffer.ReadableBytes();

    if(chunk_stream_id_ < 0)  //块头解析失败
    {
        return -1;
    }

    auto& rtmp_msg = rtmp_message_[chunk_stream_id_];
    uint32_t chunk_size = rtmp_msg.length - rtmp_msg.index;
    if(chunk_size > in_chunk_size_)
    {
        chunk_size = in_chunk_size_;
    }

    if(buf_size < (chunk_size + bytes_used))
    {
        return 0;
    }

    if(rtmp_msg.index + chunk_size > rtmp_msg.length)
    {
        return -1;
    }

    //使用 memcpy 将当前块的数据从缓冲区 buf 拷贝到 RtmpMessage 中的 playload（有效载荷）位置。
    //更新 bytes_used 和 rtmp_msg.index，表示已成功读取的字节数和当前消息中已接收的数据长度。
    memcpy(rtmp_msg.playload.get() + rtmp_msg.index, buf + bytes_used, chunk_size);
    bytes_used += chunk_size;
    rtmp_msg.index += chunk_size;

    //如果 index 大于或等于消息长度，表示整个消息已经完整接收，可以切换状态为 PARSE_HEADER。
    //如果 index 是 in_chunk_size_ 的整数倍，也表示该消息的一个完整块已解析完。
    if(rtmp_msg.index >= rtmp_msg.length || rtmp_msg.index % in_chunk_size_ == 0) 
    {
        state_ = PARSE_HEADER;
    }

    buffer.Retrieve(bytes_used);
    return bytes_used;
}

int RtmpChunk::CreatBasicHeader(uint8_t fmt, uint32_t csid, char *buf)
{
    int len = 0;
    if(csid >= 64 + 225)  //说明这个basic header占3字节
    {
        buf[len++] = (fmt << 6) | 1;
        buf[len++] = (csid - 64) & 0xff;
        buf[len++] = ((csid - 64) >> 8) & 0xff;
    }
    else if(csid >= 64)  //占2字节
    {
        buf[len++] = (fmt << 6) | 0;
        buf[len++] = (csid - 64) & 0xff;
    }
    else  //占1字节
    {
        buf[len++] = (fmt << 6) | csid;
    }
    return len;
}

//Message Header根据fmt不同有11，7，3，0四种类型，对应fmt=0，1，2，3
int RtmpChunk::CreatMessageHeader(uint8_t fmt, RtmpMessage &rtmp_msg, char *buf)
{
    int len = 0;

    if(fmt <= 2)
    {
        if(rtmp_msg.timestamp < 0xffffff)  //timestamp有三个字节，超过要填入extend_timestamp
        {
            WriteUint24BE((char*)buf, (uint32_t)rtmp_msg.timestamp);
        }
        else
        {
            WriteUint24BE((char*)buf, 0xffffff);
        }
        len += 3;
    }
    if(fmt <= 1)
    {
        WriteUint24BE((char*)(buf + len), rtmp_msg.length);
        len += 3;
        buf[len++] = rtmp_msg.type_id;
    }
    if(fmt == 0)  //stream_id用小端存储
    {
        WriteUint32LE((char*)(buf + len), rtmp_msg.stream_id);
        len += 4;
    }
    return len;
}
