#include "RtmpConnection.h"
#include "RtmpServer.h"
#include "rtmp.h"

RtmpConnection::RtmpConnection(std::shared_ptr<RtmpServer> rtmp_server, TaskScheduler *scheduler, int socket)
    :RtmpConnection(scheduler, socket, rtmp_server.get())
{
    rtmp_handshake_.reset(new RtmpHandShake(RtmpHandShake::HANDSHAKE_C0C1));
    rtmp_server_ = rtmp_server;
}

RtmpConnection::RtmpConnection(TaskScheduler *scheduler, int socket, Rtmp* rtmp)
    :TcpConnection(scheduler, socket)
    ,rtmp_chunk_(new RtmpChunk())
    ,conn_state_(HANDSHAKE)
{
    peer_width_ = rtmp->GetPeerBandwidth();
    acknowledgement_size_ = rtmp->GetAcknowledgementSize();
    max_chunk_size_ = rtmp->GetChunkSize();
    stream_path_ = rtmp->GetStreamPath();
    stream_name_ = rtmp->GetStreamName();
    app_ = rtmp->GetApp();

    //设置读回调函数处理读数据
    this->SetReadCallBack([this](std::shared_ptr<TcpConnection> conn, BufferReader& buffer){
        return this->OnRead(buffer);
    });

    //设置关闭回调释放资源
    this->SetCloseCallBack([this](std::shared_ptr<TcpConnection> conn){
        this->OnClose();
    });
}

RtmpConnection::~RtmpConnection()
{
}

//读回调
bool RtmpConnection::OnRead(BufferReader &buffer)
{
    bool ret = true;
    if(rtmp_handshake_->IsComplete())  //完成握手后才能发送message
    {
        ret = HandleChunk(buffer);
    }
    else
    {
        std::shared_ptr<char> res(new char[4096], std::default_delete<char[]>());
        int res_size = rtmp_handshake_->Parse(buffer, res.get(), 4096);
        if(res_size < 0)
        {
            ret = false;
        }
        if(res_size > 0)
        {
            this->Send(res.get(), res_size);
        }
        if(rtmp_handshake_->IsComplete())
        {
            if(buffer.ReadableBytes() > 0)
            {
                ret = HandleChunk(buffer);
            }
        }
    }
    return ret;
}

//连接关闭回调
void RtmpConnection::OnClose()
{
    this->HandleDeleteStream();
}

//块处理
bool RtmpConnection::HandleChunk(BufferReader &buffer)
{
    int ret = -1;
    do{
        RtmpMessage rtmp_msg;
        ret = rtmp_chunk_->Parse(buffer, rtmp_msg);
        if(ret >= 0)  //解析成功，处理msg
        {
            if(rtmp_msg.IsComplete())
            {
                if(!HandleMessage(rtmp_msg))
                {
                    return false;
                }
            }
            if(ret = 0)  //缓存区没有数据
            {
                break;
            }
        }
        else
        {
            return false;
        }
    }while(buffer.ReadableBytes() > 0);
    return true;
}

//rtmp数据处理
bool RtmpConnection::HandleMessage(RtmpMessage &rtmp_msg)
{
    bool ret = true;
    switch (rtmp_msg.type_id)
    {
    case RTMP_VIDEO:
        ret = HandleVideo(rtmp_msg);
        break;
    case RTMP_AUDIO:
        ret = HandleAudio(rtmp_msg);
        break;
    case RTMP_INVOKE:
        ret = HandleInvoke(rtmp_msg);
        break;
    case RTMP_NOTIFY:
        ret = HandleNotify(rtmp_msg);
        break;
    case RTMP_SET_CHUNK_SIZE:
        rtmp_chunk_->SetInChunkSize(ReadUint32BE(rtmp_msg.playload.get()));
    default:
        break;
    }
    return true;
}

//invoke消息处理
bool RtmpConnection::HandleInvoke(RtmpMessage &rtmp_msg)
{
    bool ret = true;
    amf_decoder_.reset();

    int bytes_used = amf_decoder_.decode(rtmp_msg.playload.get(), rtmp_msg.length, 1);
    if(bytes_used < 0)
    {
        return false;
    }

    std::string method = amf_decoder_.getString();
    if(rtmp_msg.stream_id == 0)
    {
        bytes_used += amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used, rtmp_msg.length - bytes_used);
        //处理连接或创建流
        if(method == "connect")
        {
            ret = HandleConnect();
        }
        else if(method == "createStream")
        {
            ret = HandleCreateStream();
        }
    }
    else if(rtmp_msg.stream_id == stream_id_)
    {
        bytes_used += amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used, rtmp_msg.length - bytes_used, 3);
        stream_name_ = amf_decoder_.getString();
        stream_path_ = "/" + app_ + "/" + stream_name_;

        if(rtmp_msg.length > bytes_used)
        {
            bytes_used += amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used, rtmp_msg.length - bytes_used);
        }

        if(method == "publish")
        {
            ret = HandlePublish();
        }
        else if(method == "play")
        {
            ret = HandlePlay();
        }
        else if(method == "DeleteStream")
        {
            ret = HandleDeleteStream();
        }
    }
    return ret;
}

//notify消息处理
bool RtmpConnection::HandleNotify(RtmpMessage &rtmp_msg)
{
    amf_decoder_.reset();

    int bytes_used = amf_decoder_.decode(rtmp_msg.playload.get(), rtmp_msg.length, 1);
    std::string method = amf_decoder_.getString();
    if(method == "@setDataFrame")
    {
        amf_decoder_.reset();
        bytes_used += amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used, rtmp_msg.length - bytes_used, 1);
        if(bytes_used < 0)
        {
            return false;
        }
        //是否元数据
        if(amf_decoder_.getString() == "onMetaData")
        {
            amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used, rtmp_msg.length - bytes_used);
            meta_data_ = amf_decoder_.getObjects();
        }
        //设置元数据
        //获取session设置元数据
        auto server = rtmp_server_.lock();
        if(!server)
        {
            return false;
        }
        auto session = rtmp_session_.lock();
        if(session)
        {
            session->SendMetaData(meta_data_);
        }
    }
    return true;
}

//音频数据处理
bool RtmpConnection::HandleAudio(RtmpMessage &rtmp_msg)
{
    uint8_t type = RTMP_AUDIO;
    uint8_t* playload = (uint8_t*)rtmp_msg.playload.get();
    uint32_t length = rtmp_msg.length;

    uint8_t sound_format = (playload[0] >> 4) & 0x0f;  //音频格式
    uint8_t code_id = playload[0] & 0x0f;  //音频编码id

    auto server = rtmp_server_.lock();
    if(!server)
    {
        return false;
    }
    auto session = rtmp_session_.lock();
    if(!session)
    {
        return false;
    }

    if(sound_format == RTMP_CODEC_ID_AAC && playload[1] == 0)  //编码器为AAC，并且此音频数据是AAC序列头
    {
        aac_sequence_header_size_ = rtmp_msg.length;
        aac_sequence_header_.reset(new char[aac_sequence_header_size_], std::default_delete<char[]>());
        memcpy(aac_sequence_header_.get(), rtmp_msg.playload.get(), aac_sequence_header_size_);

        //获取session设置aac序列头
        session->SetAacSequenceHeader(aac_sequence_header_, aac_sequence_header_size_);
        type = RTMP_AAC_SEQUENCE_HEADER;
    }

    //获取session发送音频数据
    session->SendMediaData(type, rtmp_msg.timestamp, rtmp_msg.playload, rtmp_msg.length);
    return true;
}

//视频数据处理
bool RtmpConnection::HandleVideo(RtmpMessage &rtmp_msg)
{
    uint8_t type = RTMP_VIDEO;
    uint8_t* playload = (uint8_t*)rtmp_msg.playload.get();
    uint32_t length = rtmp_msg.length;

    uint8_t frame_type = (playload[0] >> 4) & 0x0f;  //帧类型
    uint8_t code_id = playload[0] & 0x0f;  //视频编码id

    auto server = rtmp_server_.lock();
    if(!server)
    {
        return false;
    }
    auto session = rtmp_session_.lock();
    if(!session)
    {
        return false;
    }

    if(code_id == RTMP_CODEC_ID_H264 && frame_type == 1 && playload[1] == 0)  //编码器为H264，并且此视频数据是H264序列头
    {
        avc_sequence_header_size_ = rtmp_msg.length;
        avc_suquence_header_.reset(new char[avc_sequence_header_size_], std::default_delete<char[]>());
        memcpy(avc_suquence_header_.get(), rtmp_msg.playload.get(), avc_sequence_header_size_);

        //获取session设置h264序列头
        session->SetAvcSequenceHeader(avc_suquence_header_, avc_sequence_header_size_);
        type = RTMP_AVC_SEQUENCE_HEADER;
    }

    //获取session发送视频数据
    session->SendMediaData(type, rtmp_msg.timestamp, rtmp_msg.playload, rtmp_msg.length);
    return true;
}

//连接请求处理
bool RtmpConnection::HandleConnect()
{
    if(!amf_decoder_.hasObject("app"))  //是否存在app应用程序
    {
        return false;
    }

    AmfObject amfobj = amf_decoder_.getObject("app");
    app_ = amfobj.amf_string;  //获取应用程序的值
    if(app_ == "")
    {
        return false;
    }

    SetAcknowledgementSize();
    SetPeerBandWidth();
    SetChunkSize();

    //应答
    AmfObjects objects;
    amf_encoder_.reset();
    amf_encoder_.encodeString("_result", 7);
    amf_encoder_.encodeNumber(amf_decoder_.getNumber());
    SendInvoke(RTMP_CHUNK_INVOKE_ID, amf_encoder_.data(), amf_encoder_.size());

    return true;
}

//创建流请求处理
bool RtmpConnection::HandleCreateStream()
{
    int stream_id = rtmp_chunk_->GetStreamID();

    AmfObjects objects;

    amf_encoder_.reset();
    amf_encoder_.encodeString("_result", 7);
    amf_encoder_.encodeNumber(amf_decoder_.getNumber());
    amf_encoder_.encodeObjects(objects);
    amf_encoder_.encodeNumber(stream_id);

    SendInvoke(RTMP_CHUNK_INVOKE_ID, amf_encoder_.data(), amf_encoder_.size());
    stream_id_ = stream_id;

    return true;
}

//推流处理
bool RtmpConnection::HandlePublish()
{
    auto server = rtmp_server_.lock();
    if(!server)
    {
        return false;
    }

    AmfObjects objects;
    amf_encoder_.reset();
    amf_encoder_.encodeString("onStatus", 0);
    amf_encoder_.encodeNumber(0);
    amf_encoder_.encodeObjects(objects);

    bool is_error = false;

    //判断是否已经推流
    if(server->HasPublisher(stream_path_))
    {
        is_error = true;
        objects["level"] = AmfObject(std::string("error"));
        objects["code"] = AmfObject(std::string("NetStream.Publish.Badname"));  //说明当前流已经推送
        objects["description"] = AmfObject(std::string("Stream already publishing."));
    }
    //状态是推流状态时也不能推
    else if(conn_state_ == START_PUBLISH)
    {
        is_error = true;
        objects["level"] = AmfObject(std::string("error"));
        objects["code"] = AmfObject(std::string("NetStream.Publish.Badconnection"));  
        objects["description"] = AmfObject(std::string("Stream already publishing."));
    }
    else
    {
        is_error = false;
        objects["level"] = AmfObject(std::string("status"));
        objects["code"] = AmfObject(std::string("NetStream.Publish.Start"));  
        objects["description"] = AmfObject(std::string("Start publishing."));

        //添加session
        server->AddSession(stream_path_);
        rtmp_session_ = server->GetSession(stream_path_);
        if(server)
        {
            server->NotifyEvent("publish.start", stream_path_);
        }
    }

    amf_encoder_.encodeObjects(objects);
    SendInvoke(RTMP_CHUNK_INVOKE_ID, amf_encoder_.data(), amf_encoder_.size());

    if(is_error)
    {
        return false;
    }
    else
    {
        conn_state_ = START_PUBLISH;
        is_publishing_ = true;
    }

    auto session = rtmp_session_.lock();
    if(session)
    {
        session->AddSink(std::dynamic_pointer_cast<RtmpSink>(shared_from_this()));
    }
    return true;
}

bool RtmpConnection::HandlePlay()
{
    auto server = rtmp_server_.lock();
    if(!server)
    {
        return false;
    }

    AmfObjects objects;
    amf_encoder_.reset();

    //发送应答
    amf_encoder_.encodeString("onStatus", 8);
    amf_encoder_.encodeNumber(0);
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetStream.Play.Reset"));
    objects["description"] = AmfObject(std::string("Resetting and playing stream."));
    amf_encoder_.encodeObjects(objects);
    if(!SendInvoke(RTMP_CHUNK_INVOKE_ID, amf_encoder_.data(), amf_encoder_.size()))
    {
        return false;
    }

    //开始拉流
    objects.clear();
    amf_encoder_.reset();
    amf_encoder_.encodeString("onStatus", 8);
    amf_encoder_.encodeNumber(0);
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetStream.Play.Start"));
    objects["description"] = AmfObject(std::string("Start playing."));
    amf_encoder_.encodeObjects(objects);
    if(!SendInvoke(RTMP_CHUNK_INVOKE_ID, amf_encoder_.data(), amf_encoder_.size()))
    {
        return false;
    }

    //通知客户端权限
    amf_encoder_.reset();
    amf_encoder_.encodeString("|RtmpSampleAccess", 17);
    amf_encoder_.encodeBoolean(true);  //允许读
    amf_encoder_.encodeBoolean(true);  //允许写
    if(!this->SendNotify(RTMP_CHUNK_DATA_ID, amf_encoder_.data(), amf_encoder_.size()))
    {
        return false;
    }

    conn_state_ = START_PLAY;
    //session添加客户端
    rtmp_session_ = server->GetSession(stream_path_);
    auto session = rtmp_session_.lock();
    if(session)
    {
        session->AddSink(std::dynamic_pointer_cast<RtmpSink>(shared_from_this()));
    }
    if(server)
    {
        server->NotifyEvent("Play.start", stream_path_);
    }

    return true;
}

bool RtmpConnection::HandleDeleteStream()
{
    auto server = rtmp_server_.lock();
    if(!server)
    {
        return false;
    }

    if(stream_path_ != "")
    {
        //session移除会话
        auto session = rtmp_session_.lock();
        if(session)
        {
            auto conn = std::dynamic_pointer_cast<RtmpSink>(shared_from_this());
            GetTaskScheduler()->AddTimer([session, conn](){
                session->RemoveSink(conn);
                return false;
            }, 1);
            if(is_publishing_)
            {
                server->NotifyEvent("publish.stop", stream_path_);
            }
            else if(is_playing_)
            {
                server->NotifyEvent("play.stop", stream_path_);
            }
        }

        is_playing_ = false;
        is_publishing_ = false;
        has_key_frame = false;
        rtmp_chunk_->Clear();
    }

    return true;
}

void RtmpConnection::SetPeerBandWidth()
{
    std::shared_ptr<char> data(new char[5], std::default_delete<char[]>());
    WriteUint32BE(data.get(), peer_width_);
    data.get()[4] = 2;  //0,1,2
    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_BANDWIDTH_SIZE;
    rtmp_msg.playload = data; 
    rtmp_msg.length = 5;
    SendRtmpChunks(RTMP_CHUNK_CONTROL_ID, rtmp_msg);
}

void RtmpConnection::SetAcknowledgementSize()
{
    std::shared_ptr<char> data(new char[5], std::default_delete<char[]>());
    WriteUint32BE(data.get(), acknowledgement_size_);

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_ACK_SIZE;
    rtmp_msg.playload = data;
    rtmp_msg.length = 4;
    SendRtmpChunks(RTMP_CHUNK_CONTROL_ID, rtmp_msg);
}

void RtmpConnection::SetChunkSize()
{
    rtmp_chunk_->SetOutChunkSize(max_chunk_size_);
    std::shared_ptr<char> data(new char[5], std::default_delete<char[]>());
    WriteUint32BE(data.get(), max_chunk_size_);

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_SET_CHUNK_SIZE;
    rtmp_msg.playload = data;
    rtmp_msg.length = 4;
    SendRtmpChunks(RTMP_CHUNK_CONTROL_ID, rtmp_msg);
}

bool RtmpConnection::SendInvoke(uint32_t csid, std::shared_ptr<char> playload, uint32_t playload_size)
{
    if(this->is_closed_)
    {
        return false;
    }

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_INVOKE;
    rtmp_msg.timestamp = 0;
    rtmp_msg.stream_id = stream_id_;
    rtmp_msg.playload = playload;
    rtmp_msg.length = playload_size;

    SendRtmpChunks(csid, rtmp_msg);
    return true;
}

bool RtmpConnection::SendNotify(uint32_t csid, std::shared_ptr<char> playload, uint32_t playload_size)
{
    if(this->is_closed_)
    {
        return false;
    }

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_NOTIFY;
    rtmp_msg.timestamp = 0;
    rtmp_msg.stream_id = stream_id_;
    rtmp_msg.playload = playload;
    rtmp_msg.length = playload_size;

    SendRtmpChunks(csid, rtmp_msg);
    return true;
}

void RtmpConnection::SendRtmpChunks(uint32_t csid, RtmpMessage &rtmp_msg)
{
    uint32_t capacity = rtmp_msg.length + rtmp_msg.length / max_chunk_size_ * 5 + 1024;
    std::shared_ptr<char> buffer(new char[capacity], std::default_delete<char[]>());
    int size = rtmp_chunk_->CreatChunk(csid, rtmp_msg, buffer.get(), capacity);
    if(size > 0)
    {
        this->Send(buffer.get(), size);
    }
}

bool RtmpConnection::IsKeyFrame(std::shared_ptr<char> data, uint32_t size)
{
    uint8_t frame_type = (data.get()[0] >> 4) & 0x0f;
    uint8_t code_id = data.get()[0] & 0x0f;
    return frame_type == 1 && code_id == RTMP_CODEC_ID_H264;
}

bool RtmpConnection::SendMetaData(AmfObjects metaData)
{
    if(this->is_closed_ || meta_data_.size() == 0)
    {
        return false;
    }

    amf_encoder_.reset();
    amf_encoder_.encodeString("onMetaData", 10);
    amf_encoder_.encodeECMA(meta_data_);
    if(!SendNotify(RTMP_CHUNK_DATA_ID, amf_encoder_.data(), amf_encoder_.size()))
    {
        return false;
    }
    return true;
}

bool RtmpConnection::SendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> playload, uint32_t playload_size)
{
    if(this->is_closed_ || playload_size == 0)
    {
        return false;
    }

    is_playing_ = true;

    //如果是音频序列头或视频序列头
    if(type == RTMP_AAC_SEQUENCE_HEADER)
    {
        aac_sequence_header_ = playload;
        aac_sequence_header_size_ = playload_size;
    }
    else if(type == RTMP_AVC_SEQUENCE_HEADER)
    {
        avc_suquence_header_ = playload;
        avc_sequence_header_size_ = playload_size;
    }

    //如果数据包既不是序列头也没有收到关键帧
    if(!has_key_frame && avc_sequence_header_size_ > 0 && type != RTMP_AVC_SEQUENCE_HEADER && type != RTMP_AAC_SEQUENCE_HEADER)
    {
        //判断是否为关键帧
        if(IsKeyFrame(playload, playload_size))
        {
            has_key_frame = true;
        }
        else
        {
            return false;
        }
    }

    //收到关键帧后开始发送message
    RtmpMessage rtmp_msg;
    rtmp_msg._timestamp = timestamp;
    rtmp_msg.stream_id = stream_id_;
    rtmp_msg.playload = playload;
    rtmp_msg.length = playload_size;

    //发送视频或音频数据
    if(type == RTMP_VIDEO || type == RTMP_AVC_SEQUENCE_HEADER)
    {
        rtmp_msg.type_id = RTMP_VIDEO;
        SendRtmpChunks(RTMP_CHUNK_VIDEO_ID, rtmp_msg);
    }
    else if(type == RTMP_AUDIO || type == RTMP_AAC_SEQUENCE_HEADER)
    {
        rtmp_msg.type_id = RTMP_AUDIO;
        SendRtmpChunks(RTMP_CHUNK_AUDIO_ID, rtmp_msg);
    }
    return true;
}
