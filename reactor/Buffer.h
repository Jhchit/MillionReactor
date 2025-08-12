#pragma once
#include <string>
#include <string.h>

class Buffer
{
private:
    std::string buf_;
    const uint16_t sep_;    // 报文的分隔符：0-无分隔符(固定长度、视频会议)；1-四字节的报头；2-"\r\n\r\n"分隔符（http协议）。

public:
    Buffer(uint16_t sep = 0);
    ~Buffer();

    void append(const char *data,size_t size);
    void appendwithsep(const char*data,size_t size); //把数据加到buf_中，并附加报文头部
    void erase(size_t pos,size_t nn);                // 从buf_的pos开始，删除nn个字节，pos从0开始。
    size_t size();                                   // 返回buf_的大小
    const char *data();                              // 返回buf_的首地址
    void clear();
    bool pickmessage(std::string &ss);               // 从buf_中拆分出一个报文，存放在ss中，如果buf_中没有报文，返回false。
};

