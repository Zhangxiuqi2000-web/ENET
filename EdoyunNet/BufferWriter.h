#ifndef _BUFFERWRITER_H_
#define _BUFFERWRITER_H_

#include <memory>
#include <queue>

void WriteUint32BE(char* p, uint32_t value);  //4字节大端写
void WriteUint32LE(char* p, uint32_t value);  //4字节小端写
void WriteUint24BE(char* p, uint32_t value);  //3字节大端写
void WriteUint24LE(char* p, uint32_t value);  //3字节小端写
void WriteUint16BE(char* p, uint32_t value);  //2字节大端写
void WriteUint16LE(char* p, uint32_t value);  //2字节小端写

class BufferWriter
{
public:
	BufferWriter(int capacity = kMaxQueueLength);
	~BufferWriter() {}

	bool Append(std::shared_ptr<char> data, uint32_t size, uint32_t index=0);
	bool Append(const char* data, uint32_t size, uint32_t index=0);
	int Send(int sockfd);

	bool IsEmpty() const 
	{ return buffer_.empty(); }

	bool IsFull() const 
	{ return ((int)buffer_.size() >= max_queue_length_ ? true : false); }

	uint32_t Size() const 
	{ return (uint32_t)buffer_.size(); }
	
private:
	typedef struct 
	{
		std::shared_ptr<char> data;
		uint32_t size;
		uint32_t writeIndex;
	} Packet;

	std::queue<Packet> buffer_;  		
	int max_queue_length_ = 0;
	 
	static const int kMaxQueueLength = 10000;
};
#endif