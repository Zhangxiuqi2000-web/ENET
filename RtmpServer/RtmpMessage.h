#ifndef _RTMPMESSAGE_H_
#define _RTMPMESSAGE_H_

#include <cstdint>
#include <memory>

//+--------------+----------------+--------------------+------------+
//| Basic Header | Message Header | Extended TimeStamp | Chunk Data |
//+--------------+----------------+--------------------+------------+

//Message Header
struct RtmpMessageHeader
{
    uint8_t timestamp[3];
    uint8_t length[3];
    uint8_t type_id;
    uint8_t stream_id[4];  //小端存储
};

struct RtmpMessage
{
    uint32_t timestamp = 0;
    uint32_t length = 0;
    uint32_t type_id = 0;
    uint32_t stream_id = 0;
    uint32_t extend_timestamp = 0;

    uint64_t _timestamp = 0;
    uint8_t codeID = 0;

    uint8_t csid = 0;
    uint32_t index = 0;
    std::shared_ptr<char> playload = nullptr;

    void Clear()
    {
        index = 0;
        timestamp = 0;
        extend_timestamp = 0;
        if(length > 0)
        {
            playload.reset(new char[length], std::default_delete<char[]>());
        }
    }

    bool IsComplete() const
    {
        if(index == length && length > 0 && playload != nullptr)
        {
            return true;
        }
        return false;
    }
};
#endif

