#include "RtmpHandShake.h"
#include <random>
#include <cstring>

RtmpHandShake::RtmpHandShake(State state)
{
    handshake_state_ = state;
}

RtmpHandShake::~RtmpHandShake()
{
}

int RtmpHandShake::Parse(BufferReader &in_buffer, char *res_buf, uint32_t res_buf_size)
{
    uint8_t* buf = (uint8_t*)in_buffer.Peek();
    uint32_t buf_size = in_buffer.ReadableBytes();
    uint32_t pos = 0;
    uint32_t res_size = 0;
    std::random_device rd;

    if(handshake_state_ == HANDSHAKE_S0S1S2)  //由客户端处理：接收S0S1S2，返回C2
    {
        if(buf_size < 1 + 1536 + 1536)  //S0S1S2大小
        {
            return res_size;
        }
        if(buf[0] != 3)  //判断版本
        {
            return -1;
        }

        pos += 1 + 1536 + 1536;
        res_size = 1536;  //需要发送的C2数据大小

        //准备C2
        memcpy(res_buf, buf + 1, 1536);  //将S1的数据作为C2传回

        handshake_state_ = HANDSHAKE_COMPLETE;
    }
    else if(handshake_state_ == HANDSHAKE_C0C1)  //由服务端处理：接收C0C1，返回S0S1S2
    {
        if(buf_size < 1 + 1536)  //C0C1大小
        {
            return res_size;
        }
        else
        {
            if(buf[0] != 3)
            {
                return -1;
            }

            pos += 1 + 1536;
            res_size = 1 + 1536 + 1536;
            memset(res_buf, 0, res_size);  //清空

            //准备S0
            res_buf[0] = 3;  

            //准备S1：前8字节为0，之后1528为随机数
            char* p = res_buf; p += 9;
            for(int i = 0; i < 1528; i++)
            {
                *p++ = rd();
            }

            //准备S2：内容为C1
            memcpy(p, buf + 1, 1536);
            handshake_state_ = HANDSHAKE_C2;
        }
    }
    else if(handshake_state_ == HANDSHAKE_C2)  //由服务器处理：接收C2
    {
        if(buf_size < 1536)  //C2大小
        {
            return res_size;
        }
        else
        {
            pos += 1536;
            handshake_state_ = HANDSHAKE_COMPLETE;
        }
    }

    in_buffer.Retrieve(pos);
    return res_size;
}

int RtmpHandShake::BuildC0C1(char *buf, uint32_t buf_size)
{
    uint32_t size = 1 + 1536;
    memset(buf, 0, size);

    buf[0] = 3;  //C0，版本号
    std::random_device rd;
    uint8_t* p = (uint8_t*)buf; p += 9;  //C1前四字节为时间戳，4-8字节为空，之后为随机数
    for(int i = 0; i < 1528; i++)
    {
        *p++ = rd();
    }

    return size;
}
