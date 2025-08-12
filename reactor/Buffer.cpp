#include "Buffer.h"

Buffer::Buffer(uint16_t sep):sep_(sep)
{
}

Buffer::~Buffer()
{
}

void Buffer::append(const char *data,size_t size)
{
    buf_.append(data,size);
}

void Buffer::appendwithsep(const char*data,size_t size)
{
    if(sep_ == 0)
    {
        buf_.append(data,size);
    }
    else if(sep_ == 1)
    {
        buf_.append((char*)&size,4);  // 把报文头部填充到回应报文中。
        buf_.append(data,size);             // 把报文内容填充到回应报文中。
    }
}

void Buffer::erase(size_t pos,size_t nn)
{
    buf_.erase(pos,nn);
}

size_t Buffer::size()
{
    return buf_.size();
}

const char *Buffer::data()
{
    return buf_.data();
}

void Buffer::clear()
{
    buf_.clear();
}

// 从buf_中拆分出一个报文，存放在ss中，如果buf_中没有报文，返回false。
bool Buffer::pickmessage(std::string &ss)
{
    if(buf_.size() == 0)return false;

    if(sep_ == 0)
    {
        ss = buf_;
        buf_.clear();
    }

    else if(sep_ == 1)
    {
        int len;
        memcpy(&len,buf_.data(),4);// 从inputbuffer中获取报文头部。
        
        if(buf_.size()<len+4) return false;// 如果inputbuffer中的数据量小于报文头部，说明inputbuffer中的报文内容不完整。

        ss = buf_.substr(4,len);   // 从inputbuffer中获取一个报文。
        buf_.erase(0,len+4);
    }

    return true;
}