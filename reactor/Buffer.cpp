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
    else if (sep_ == 2)
    {
        buf_.append(data, size);
        buf_.append("\r\n\r\n", 4);  // 添加HTTP报文结束分隔符
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

    else if (sep_ == 2)
    {
        // 查找HTTP报文结束符\r\n\r\n
        size_t pos = buf_.find("\r\n\r\n");
        if (pos == std::string::npos) return false;  // 未找到完整报文
        
        // 包含结束符在内的完整报文长度
        size_t msglen = pos + 4;
        
        // 处理带Content-Length的HTTP报文（兼容POST等有请求体的情况）
        const std::string &header = buf_.substr(0, pos);
        size_t cl_pos = header.find("Content-Length: ");
        if (cl_pos != std::string::npos)
        {
            // 解析Content-Length值
            size_t cl_end = header.find("\r\n", cl_pos);
            if (cl_end != std::string::npos)
            {
                std::string cl_str = header.substr(cl_pos + 16, cl_end - (cl_pos + 16));
                size_t content_len = std::stoul(cl_str);
                
                // 检查是否包含完整的请求体
                if (buf_.size() < msglen + content_len)
                {
                    return false;  // 报文不完整
                }
                msglen += content_len;  // 总长度 = 头部长度 + 内容长度
            }
        }

        // 提取完整报文
        ss = buf_.substr(0, msglen);
        buf_.erase(0, msglen);
    }

    return true;
}
