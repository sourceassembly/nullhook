#pragma once

#include <cstdint>
#include <inetchannel.h>
#include "core/vfunc.hpp"
#include "core/vtables.hpp"

class CNetChan
{
public:
    struct Layout
    {
        std::uint32_t m_nOutSequenceNr    = uint32_t(vtables::netchan::out_sequence_nr);
        std::uint32_t m_nInSequenceNr     = uint32_t(vtables::netchan::in_sequence_nr);
        std::uint32_t m_nOutSequenceNrAck = uint32_t(vtables::netchan::out_sequence_nr_ack);
        std::uint32_t m_nOutReliableState = uint32_t(vtables::netchan::out_reliable_state);
        std::uint32_t m_nInReliableState  = uint32_t(vtables::netchan::in_reliable_state);
        std::uint32_t m_nChokedPackets    = uint32_t(vtables::netchan::choked_packets);
    };

    static Layout &layout()
    {
        static Layout live;
        return live;
    }
    static void InitLayout(const std::uint8_t *get_seq, const std::uint8_t *set_choked, const std::uint8_t *get_rel);

    template <typename T>
    T &at(std::uint32_t off)
    {
        return *reinterpret_cast<T *>(reinterpret_cast<char *>(this) + off);
    }

    int &m_nOutSequenceNr()
    {
        return at<int>(layout().m_nOutSequenceNr);
    }
    int &m_nInSequenceNr()
    {
        return at<int>(layout().m_nInSequenceNr);
    }
    int &m_nOutSequenceNrAck()
    {
        return at<int>(layout().m_nOutSequenceNrAck);
    }
    int &m_nOutReliableState()
    {
        return at<int>(layout().m_nOutReliableState);
    }
    int &m_nInReliableState()
    {
        return at<int>(layout().m_nInReliableState);
    }
    int &m_nChokedPackets()
    {
        return at<int>(layout().m_nChokedPackets);
    }

    const char *GetName() const
    {
        return vfunc<const char *(*)(const CNetChan *)>(const_cast<CNetChan *>(this), vtables::netchan::get_name)(this);
    }
    const char *GetAddress() const
    {
        return vfunc<const char *(*)(const CNetChan *)>(const_cast<CNetChan *>(this), vtables::netchan::get_address)(this);
    }
    float GetLatency(int flow) const
    {
        return vfunc<float (*)(const CNetChan *, int)>(const_cast<CNetChan *>(this), vtables::netchan::get_latency)(this, flow);
    }
    float GetAvgLatency(int flow) const
    {
        return vfunc<float (*)(const CNetChan *, int)>(const_cast<CNetChan *>(this), vtables::netchan::get_avg_latency)(this, flow);
    }
    void Shutdown(const char *reason)
    {
        vfunc<void (*)(CNetChan *, const char *)>(this, vtables::netchan::shutdown)(this, reason);
    }
    bool SendNetMsg(INetMessage &msg, bool force_reliable = false, bool voice = false)
    {
        return vfunc<bool (*)(CNetChan *, INetMessage &, bool, bool)>(this, vtables::netchan::send_net_msg)(this, msg, force_reliable, voice);
    }
    bool Transmit(bool only_reliable = false)
    {
        return vfunc<bool (*)(CNetChan *, bool)>(this, vtables::netchan::transmit)(this, only_reliable);
    }
    bool IsLoopback() const
    {
        return vfunc<bool (*)(const CNetChan *)>(const_cast<CNetChan *>(this), vtables::netchan::is_loopback)(this);
    }
    void SetChoked()
    {
        vfunc<void (*)(CNetChan *)>(this, vtables::netchan::set_choked)(this);
    }
    int SendDatagram(bf_write *data)
    {
        return vfunc<int (*)(CNetChan *, bf_write *)>(this, vtables::netchan::send_datagram)(this, data);
    }
    bool CanPacket() const
    {
        return vfunc<bool (*)(const CNetChan *)>(const_cast<CNetChan *>(this), vtables::netchan::can_packet)(this);
    }
    INetChannel *AsINetChannel()
    {
        return reinterpret_cast<INetChannel *>(this);
    }
};

inline CNetChan *NetChan(void *ch)
{
    return reinterpret_cast<CNetChan *>(ch);
}
