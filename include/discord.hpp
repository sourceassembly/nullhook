#pragma once

#include <string>

namespace discord
{
void LogChat(int entity_idx, const std::string &message, bool team_chat);
void LogReport(unsigned friendsID, const std::string &ingame_name);
} // namespace discord
