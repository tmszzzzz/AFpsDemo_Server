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

    void ByteWriter::writeF32(float v)
    {
        uint32_t bits = 0;
        std::memcpy(&bits, &v, sizeof(float));
        writeU32(bits); // 继续复用小端的 writeU32
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

    bool ByteReader::readF32(float& v)
    {
        uint32_t bits = 0;
        if (!readU32(bits))
            return false;
        std::memcpy(&v, &bits, sizeof(float));
        return true;
    }

    // 通用：填 header.length / msgId / seq
    static void FillHeader(Message& m, MsgId id, uint32_t seq = 0)
    {
        m.header.msgId = static_cast<uint16_t>(id);
        m.header.seq   = seq;
        // length 在 payload 写完后设置
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

    bool DecodeUdpBind(const Message& msg, UdpBind& out)
    {
        ByteReader r(msg.payload.data(), msg.payload.size());
        if (!r.readU32(out.playerId)) return false;
        return true;
    }

    bool DecodeInputCommand(const Message& msg, InputCommand& out)
    {
        ByteReader r(msg.payload.data(), msg.payload.size());

        if (!r.readU32(out.playerId))   return false;
        if (!r.readU16(out.seq))        return false;
        if (!r.readU32(out.clientTick)) return false;
        if (!r.readF32(out.moveX))      return false;
        if (!r.readF32(out.moveY))      return false;
        if (!r.readF32(out.yaw))        return false;
        if (!r.readF32(out.pitch))      return false;
        if (!r.readU32(out.buttonMask)) return false;
        return true;
    }

    void EncodeWorldSnapshot(const WorldSnapshot& msg, Message& out)
    {
        out.payload.clear();
        FillHeader(out, MsgId::WorldSnapshot);

        ByteWriter w(out.payload);

        // serverTick
        w.writeU32(msg.serverTick);

        // playerCount
        uint16_t count = static_cast<uint16_t>(msg.players.size());
        w.writeU16(count);

        for (uint16_t i = 0; i < count; ++i)
        {
            const auto& p = msg.players[i];

            w.writeU32(p.playerId);
            w.writeU32(p.heroId);

            w.writeF32(p.posX);
            w.writeF32(p.posY);
            w.writeF32(p.posZ);

            w.writeF32(p.velX);
            w.writeF32(p.velY);
            w.writeF32(p.velZ);

            w.writeF32(p.yaw);
            w.writeF32(p.pitch);

            w.writeU8(p.locomotionState);
            w.writeU8(p.actionState);
            w.writeU8(p.activeSkillSlot);
            w.writeU8(p.activeSkillPhase);

            w.writeU32(p.statusFlags);
            w.writeU16(p.health);
            w.writeU16(p.energy);
        }

        out.header.length = static_cast<uint16_t>(sizeof(MsgHeader) + out.payload.size());
    }

    void EncodeGameEvent(const GameEvent& msg, Message& out)
    {
        out.payload.clear();
        FillHeader(out, MsgId::GameEvent);

        ByteWriter w(out.payload);

        w.writeU8(static_cast<uint8_t>(msg.type));
        w.writeU32(msg.serverTick);
        w.writeU32(msg.casterPlayerId);

        w.writeU32(msg.targetId);

        w.writeU8(msg.u8Param0);
        w.writeU8(msg.u8Param1);

        w.writeU32(msg.u32Param0);

        w.writeF32(msg.f32Param0);
        w.writeF32(msg.f32Param1);

        out.header.length = static_cast<uint16_t>(sizeof(MsgHeader) + out.payload.size());
    }

}

