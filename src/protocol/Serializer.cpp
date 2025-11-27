//
// Created by tmsz on 25-11-26.
//

// protocol/Serializer.cpp
#include "Serializer.h"
#include <cstring>

namespace proto
{
    // 小端写入
    void ByteWriter::writeU8(uint8_t v)
    {
        _buf.push_back(v);
    }

    void ByteWriter::writeU16(uint16_t v)
    {
        _buf.push_back(static_cast<uint8_t>(v & 0xFF));
        _buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    }

    void ByteWriter::writeU32(uint32_t v)
    {
        _buf.push_back(static_cast<uint8_t>( v        & 0xFF));
        _buf.push_back(static_cast<uint8_t>((v >> 8)  & 0xFF));
        _buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        _buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    }

    void ByteWriter::writeString(const std::string& s)
    {
        writeU16(static_cast<uint16_t>(s.size()));
        _buf.insert(_buf.end(), s.begin(), s.end());
    }

    // 小端读取
    bool ByteReader::readU8(uint8_t& v)
    {
        if (_pos + 1 > _size) return false;
        v = _data[_pos++];
        return true;
    }

    bool ByteReader::readU16(uint16_t& v)
    {
        if (_pos + 2 > _size) return false;
        v = static_cast<uint16_t>(_data[_pos])
            | static_cast<uint16_t>(_data[_pos + 1]) << 8;
        _pos += 2;
        return true;
    }

    bool ByteReader::readU32(uint32_t& v)
    {
        if (_pos + 4 > _size) return false;
        v =  (static_cast<uint32_t>(_data[_pos])      )
             | (static_cast<uint32_t>(_data[_pos + 1]) << 8)
             | (static_cast<uint32_t>(_data[_pos + 2]) << 16)
             | (static_cast<uint32_t>(_data[_pos + 3]) << 24);
        _pos += 4;
        return true;
    }

    bool ByteReader::readString(std::string& s)
    {
        uint16_t len = 0;
        if (!readU16(len)) return false;
        if (_pos + len > _size) return false;
        s.assign(reinterpret_cast<const char*>(_data + _pos), len);
        _pos += len;
        return true;
    }

    // 通用：填 header.length / msgId / seq
    static void FillHeader(Message& m, MsgId id, uint32_t seq = 0)
    {
        m.header.msgId = static_cast<uint16_t>(id);
        m.header.seq   = seq;
        // length 在 payload 写完后设置
    }

    // JoinRequest
    void EncodeJoinRequest(const JoinRequest& msg, Message& out)
    {
        out.payload.clear();
        FillHeader(out, MsgId::JoinRequest);
        ByteWriter w(out.payload);
        w.writeU16(msg.protocolVersion);
        w.writeString(msg.playerName);

        out.header.length = static_cast<uint16_t>(
                sizeof(MsgHeader) + out.payload.size()
        );
    }

    bool DecodeJoinRequest(const Message& msg, JoinRequest& out)
    {
        ByteReader r(msg.payload.data(), msg.payload.size());
        if (!r.readU16(out.protocolVersion)) return false;
        if (!r.readString(out.playerName))   return false;
        return r.eof() || true;
    }

    // JoinAccept
    void EncodeJoinAccept(const JoinAccept& msg, Message& out)
    {
        out.payload.clear();
        FillHeader(out, MsgId::JoinAccept);
        ByteWriter w(out.payload);
        w.writeU32(msg.playerId);
        w.writeU16(msg.serverProtocolVersion);
        out.header.length = static_cast<uint16_t>(
                sizeof(MsgHeader) + out.payload.size()
        );
    }

    bool DecodeJoinAccept(const Message& msg, JoinAccept& out)
    {
        ByteReader r(msg.payload.data(), msg.payload.size());
        if (!r.readU32(out.playerId))               return false;
        if (!r.readU16(out.serverProtocolVersion))  return false;
        return r.eof() || true;
    }

    // Ping
    void EncodePing(const Ping& msg, Message& out)
    {
        out.payload.clear();
        FillHeader(out, MsgId::Ping);
        ByteWriter w(out.payload);
        w.writeU32(msg.clientTime);
        out.header.length = static_cast<uint16_t>(
                sizeof(MsgHeader) + out.payload.size()
        );
    }

    bool DecodePing(const Message& msg, Ping& out)
    {
        ByteReader r(msg.payload.data(), msg.payload.size());
        if (!r.readU32(out.clientTime)) return false;
        return r.eof() || true;
    }

    // Pong
    void EncodePong(const Pong& msg, Message& out)
    {
        out.payload.clear();
        FillHeader(out, MsgId::Pong);
        ByteWriter w(out.payload);
        w.writeU32(msg.clientTime);
        w.writeU32(msg.serverTime);
        out.header.length = static_cast<uint16_t>(
                sizeof(MsgHeader) + out.payload.size()
        );
    }

    bool DecodePong(const Message& msg, Pong& out)
    {
        ByteReader r(msg.payload.data(), msg.payload.size());
        if (!r.readU32(out.clientTime)) return false;
        if (!r.readU32(out.serverTime)) return false;
        return r.eof() || true;
    }
}

