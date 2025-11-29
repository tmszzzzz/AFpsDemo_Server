//
// Created by tmsz on 25-11-26.
//

#ifndef DEMO0_SERVER_SERIALIZER_H
#define DEMO0_SERVER_SERIALIZER_H


// protocol/Serializer.h
#include "Messages.h"
#include <vector>
#include <cstdint>
#include <string>

namespace proto
{
    class ByteWriter
    {
    public:
        explicit ByteWriter(std::vector<uint8_t>& buf) : _buf(buf) {}

        void writeU8(uint8_t v);
        void writeU16(uint16_t v);
        void writeU32(uint32_t v);
        void writeString(const std::string& s);

    private:
        std::vector<uint8_t>& _buf;
    };

    class ByteReader
    {
    public:
        ByteReader(const uint8_t* data, size_t size)
                : _data(data), _size(size), _pos(0) {}

        bool readU8(uint8_t& v);
        bool readU16(uint16_t& v);
        bool readU32(uint32_t& v);
        bool readString(std::string& s);

        bool eof() const { return _pos >= _size; }

    private:
        const uint8_t* _data;
        size_t _size;
        size_t _pos;
    };

    // 各种消息的序列化接口
    void EncodeJoinRequest(const JoinRequest& msg, Message& out);
    bool DecodeJoinRequest(const Message& msg, JoinRequest& out);

    void EncodeJoinAccept(const JoinAccept& msg, Message& out);
    bool DecodeJoinAccept(const Message& msg, JoinAccept& out);

    void EncodePing(const Ping& msg, Message& out);
    bool DecodePing(const Message& msg, Ping& out);

    void EncodePong(const Pong& msg, Message& out);
    bool DecodePong(const Message& msg, Pong& out);

    void EncodeUdpBind(const UdpBind& msg, Message& out);
    bool DecodeUdpBind(const Message& msg, UdpBind& out);
}



#endif //DEMO0_SERVER_SERIALIZER_H
