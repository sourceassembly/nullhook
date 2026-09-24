#pragma once

#include <cstdint>
#include "core/vtables.hpp"

class CNetChan;

class ClientState
{
public:
    struct Layout
    {
        std::uint32_t m_NetChannel         = uint32_t(vtables::client_state::net_channel);
        std::uint32_t m_nSignonState       = uint32_t(vtables::client_state::signon_state);
        std::uint32_t m_nDeltaTick         = uint32_t(vtables::client_state::delta_tick);
        std::uint32_t lastoutgoingcommand  = uint32_t(vtables::client_state::lastoutgoingcommand);
        std::uint32_t chokedcommands       = uint32_t(vtables::client_state::chokedcommands);
        std::uint32_t last_command_ack     = uint32_t(vtables::client_state::last_command_ack);
        std::uint32_t m_flNextCmdTime      = uint32_t(vtables::client_state::signon_state) + 4;
    };

    static Layout &layout()
    {
        static Layout live;
        return live;
    }
    static void InitLayout(void *self, const std::uint8_t *lea, const std::uint8_t *force_full, const std::uint8_t *cl_move);

    template <typename T>
    T &at(std::uint32_t off)
    {
        return *reinterpret_cast<T *>(reinterpret_cast<char *>(this) + off);
    }

    CNetChan *&m_NetChannel()
    {
        return at<CNetChan *>(layout().m_NetChannel);
    }
    int &m_nSignonState()
    {
        return at<int>(layout().m_nSignonState);
    }
    int &m_nDeltaTick()
    {
        return at<int>(layout().m_nDeltaTick);
    }
    int &lastoutgoingcommand()
    {
        return at<int>(layout().lastoutgoingcommand);
    }
    int &chokedcommands()
    {
        return at<int>(layout().chokedcommands);
    }
    int &last_command_ack()
    {
        return at<int>(layout().last_command_ack);
    }
    double &m_flNextCmdTime()
    {
        return at<double>(layout().m_flNextCmdTime);
    }
};

using CBaseClientState = ClientState;
