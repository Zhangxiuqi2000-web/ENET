#ifndef _RTMPHANDSHAKE_H_
#define _RTMPHANDSHAKE_H_

#include "../EdoyunNet/BufferReader.h"

class RtmpHandShake
{
public:
    enum State
    {
        HANDSHAKE_C0C1,
        HANDSHAKE_S0S1S2,
        HANDSHAKE_C2,
        HANDSHAKE_COMPLETE
    };

    RtmpHandShake(State state);
    virtual ~RtmpHandShake();

    int Parse(BufferReader& in_buffer, char* res_buf, uint32_t res_buf_size);

    bool IsComplete() const{
        return handshake_state_ == HANDSHAKE_COMPLETE;
    }

    int BuildC0C1(char* buf, uint32_t buf_size);

private:
    State handshake_state_;
};
#endif