#ifndef _RTMPCONNECTION_H_
#define _RTMPCONNECTION_H_

#include "../EdoyunNet/TcpConnection.h"
#include "amf.h"
#include "RtmpChunk.h"
#include "RtmpSink.h"
#include "RtmpHandShake.h"
#include "rtmp.h"

class RtmpServer;
class RtmpSession;

class RtmpConnection: public TcpConnection, public RtmpSink
{
public:
    enum ConnectionState
    {
        HANDSHAKE,
        START_CONNECT,
        START_CREAT_STREAM,
        START_DELET_STREAM,
        START_PLAY,
        START_PUBLISH,
    };

    RtmpConnection(std::shared_ptr<RtmpServer> rtmp_server, TaskScheduler* scheduler, int socket);
    virtual ~RtmpConnection();

    virtual bool IsPlayer() override{return conn_state_ == START_PLAY;}
    virtual bool IsPublisher() override{return conn_state_ == START_PUBLISH;}
    virtual bool IsPlaying() override{return is_playing_;}
    virtual bool IsPublishing() override{return is_publishing_;}

    virtual uint32_t GetID() override{return this->GetSocket();};

private:
    RtmpConnection(TaskScheduler* scheduler, int socket, Rtmp* rtmp);

    bool OnRead(BufferReader& buffer);
    void OnClose();

    bool HandleChunk(BufferReader& buffer);
    bool HandleMessage(RtmpMessage& rtmp_msg);

    bool HandleInvoke(RtmpMessage& rtmp_msg);
    bool HandleNotify(RtmpMessage& rtmp_msg);
    bool HandleAudio(RtmpMessage& rtmp_msg);
    bool HandleVideo(RtmpMessage& rtmp_msg);

    bool HandleConnect();
    bool HandleCreateStream();
    bool HandlePublish();
    bool HandlePlay();
    bool HandleDeleteStream();

    void SetPeerBandWidth();
    void SetAcknowledgementSize();
    void SetChunkSize();

    bool SendInvoke(uint32_t csid, std::shared_ptr<char> playload, uint32_t playload_size);
    bool SendNotify(uint32_t csid, std::shared_ptr<char> playload, uint32_t playload_size);
    void SendRtmpChunks(uint32_t csid, RtmpMessage& rtmp_msg);
    bool IsKeyFrame(std::shared_ptr<char> data, uint32_t size);

    virtual bool SendMetaData(AmfObjects metaData) override;  //发送消息，通知后面的数据为元数据
    virtual bool SendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> playload, uint32_t playload_size) override;

private:
    ConnectionState conn_state_;
    std::shared_ptr<RtmpHandShake> rtmp_handshake_;
    std::shared_ptr<RtmpChunk> rtmp_chunk_;

    std::weak_ptr<RtmpServer> rtmp_server_;
    std::weak_ptr<RtmpSession> rtmp_session_;

    uint32_t peer_width_ = 5000000;  //带宽
    uint32_t acknowledgement_size_ = 5000000;
    uint32_t max_chunk_size_ = 128;
    uint32_t stream_id_ = 0;

    AmfObjects meta_data_;
    AmfDecoder amf_decoder_;
    AmfEncoder amf_encoder_;

    bool is_playing_ = false;
    bool is_publishing_ = false;

    std::string app_;  //应用名称：直播或录播
    std::string stream_name_;
    std::string stream_path_;

    bool has_key_frame = false;

    //元数据
    std::shared_ptr<char> avc_sequence_header_;
    std::shared_ptr<char> aac_sequence_header_;
    uint32_t avc_sequence_header_size_ = 0;
    uint32_t aac_sequence_header_size_ = 0;

};
#endif