#pragma once

#include "nav.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>

class CNavFile
{
public:
    // Intended to use with engine->GetLevelName() or mapname from server_spawn
    // GameEvent Change it if you get the nav file from elsewhere
    explicit CNavFile(const char *szLevelname)
    {
        if (!szLevelname)
            return;

        //        m_mapName = std::string("tf/");
        //        std::string map(szLevelname);

        //        if (map.find("maps/") == std::string::npos)
        //            m_mapName.append("maps/");

        //        m_mapName.append(szLevelname);
        //        int dotpos = m_mapName.find('.');
        //        m_mapName  = m_mapName.substr(0, dotpos);
        //        m_mapName.append(".nav");
        m_mapName.append(szLevelname);

        std::ifstream fs(m_mapName, std::ios::binary);

        if (!fs.is_open())
        {
            //.nav file does not exist
            return;
        }

        uint32_t magic;
        fs.read((char *) &magic, sizeof(uint32_t));

        if (magic != 0xFEEDFACE)
        {
            // Wrong magic number
            return;
        }

        uint32_t version;
        fs.read((char *) &version, sizeof(uint32_t));

        if (version < 16) // 16 is latest for TF2
        {
            // Version is too old
            return;
        }

        uint32_t subVersion;
        fs.read((char *) &subVersion, sizeof(uint32_t));

        if (subVersion != 2) // 2 for TF2
        {
            // Not TF2 nav file
            return;
        }

        // We do not really need to check the size
        uint32_t bspSize;
        fs.read((char *) &bspSize, sizeof(uint32_t));
        fs.read((char *) &m_isAnalized, sizeof(unsigned char));
        uint16_t placeCount;
        fs.read((char *) &placeCount, sizeof(uint16_t));

        // TF2 does not use places, but in case they exist
        uint16_t len;
        for (int i = 0; i < placeCount; ++i)
        {
            fs.read((char *) &len, sizeof(uint16_t));

            CNavPlace place{};

            uint16_t read_len = std::min<uint16_t>(len, sizeof(place.m_name) - 1);
            fs.read((char *) &place.m_name, read_len);
            if (len > read_len)
                fs.ignore(len - read_len);

            m_places.push_back(place);
        }

        fs.read((char *) &m_hasUnnamedAreas, sizeof(unsigned char));

        uint32_t areaCount;
        fs.read((char *) &areaCount, sizeof(uint32_t));

        for (size_t i = 0; i < areaCount; ++i)
        {
            CNavArea area;
            fs.read((char *) &area.m_id, sizeof(uint32_t));
            fs.read((char *) &area.m_attributeFlags, sizeof(uint32_t));
            fs.read((char *) &area.m_nwCorner, sizeof(Vector));
            fs.read((char *) &area.m_seCorner, sizeof(Vector));
            fs.read((char *) &area.m_neZ, sizeof(float));
            fs.read((char *) &area.m_swZ, sizeof(float));

            area.m_center[0] = (area.m_nwCorner[0] + area.m_seCorner[0]) / 2.0f;
            area.m_center[1] = (area.m_nwCorner[1] + area.m_seCorner[1]) / 2.0f;
            area.m_center[2] = (area.m_nwCorner[2] + area.m_seCorner[2]) / 2.0f;

            if ((area.m_seCorner.x - area.m_nwCorner.x) > 0.0f && (area.m_seCorner.y - area.m_nwCorner.y) > 0.0f)
            {
                area.m_invDxCorners = 1.0f / (area.m_seCorner.x - area.m_nwCorner.x);
                area.m_invDyCorners = 1.0f / (area.m_seCorner.y - area.m_nwCorner.y);
            }
            else
                area.m_invDxCorners = area.m_invDyCorners = 0.0f;

            // Change the tolerance if you wish
            area.m_minZ = std::min(area.m_seCorner.z, area.m_nwCorner.z) - 18.f;
            area.m_maxZ = std::max(area.m_seCorner.z, area.m_nwCorner.z) + 18.f;

            for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
            {
                uint32_t connectionCount;
                fs.read((char *) &connectionCount, sizeof(uint32_t));

                for (size_t j = 0; j < connectionCount; j++)
                {
                    NavConnect connect;

                    fs.read((char *) &connect.id, sizeof(uint32_t));

                    // Connection to the same area?
                    if (connect.id == area.m_id)
                        continue;

                    // Note: If connection directions matter to you, uncomment
                    // this
                    area.m_connections /*[dir]*/.push_back(connect);
                }
            }

            uint8_t hidingSpotCount;
            fs.read((char *) &hidingSpotCount, sizeof(uint8_t));

            for (size_t j = 0; j < hidingSpotCount; j++)
            {
                HidingSpot spot;
                fs.read((char *) &spot.m_id, sizeof(uint32_t));
                fs.read((char *) &spot.m_pos, sizeof(Vector));
                fs.read((char *) &spot.m_flags, sizeof(unsigned char));

                area.m_hidingSpots.push_back(spot);
            }

            fs.read((char *) &area.m_encounterSpotCount, sizeof(uint32_t));

            for (size_t j = 0; j < area.m_encounterSpotCount; j++)
            {
                SpotEncounter spot;
                fs.read((char *) &spot.from.id, sizeof(uint32_t));
                fs.read((char *) &spot.fromDir, sizeof(unsigned char));
                fs.read((char *) &spot.to.id, sizeof(uint32_t));
                fs.read((char *) &spot.toDir, sizeof(unsigned char));

                unsigned char spotcount;
                fs.read((char *) &spotcount, sizeof(unsigned char));

                for (int s = 0; s < spotcount; ++s)
                {
                    SpotOrder order{};
                    fs.read((char *) &order.id, sizeof(uint32_t));
                    fs.read((char *) &order.t, sizeof(unsigned char));
                    spot.spots.push_back(order);
                }

                area.m_spotEncounters.push_back(spot);
            }

            fs.read((char *) &area.m_indexType, sizeof(uint16_t));

            // TF2 does not use ladders either
            for (int dir = 0; dir < NUM_LADDER_DIRECTIONS; dir++)
            {
                uint32_t laddercount;
                fs.read((char *) &laddercount, sizeof(uint32_t));

                for (size_t j = 0; j < laddercount; j++)
                {
                    int temp;
                    fs.read((char *) &temp, sizeof(uint32_t));
                }
            }

            for (float &j : area.m_earliestOccupyTime)
                fs.read((char *) &j, sizeof(float));

            for (float &j : area.m_lightIntensity)
                fs.read((char *) &j, sizeof(float));

            fs.read((char *) &area.m_visibleAreaCount, sizeof(uint32_t));

            for (size_t j = 0; j < area.m_visibleAreaCount; ++j)
            {
                AreaBindInfo info;
                fs.read((char *) &info.id, sizeof(uint32_t));
                fs.read((char *) &info.attributes, sizeof(unsigned char));

                area.m_potentiallyVisibleAreas.push_back(info);
            }

            fs.read((char *) &area.m_inheritVisibilityFrom, sizeof(uint32_t));

            // TF2 Specific area flags
            fs.read((char *) &area.m_TFattributeFlags, sizeof(uint32_t));

            m_areas.push_back(area);
        }

        fs.close();

        // Fill connection for every area with their area ptrs instead of IDs
        // This will come in handy in path finding

        std::unordered_map<uint32_t, CNavArea *> id_to_area;
        id_to_area.reserve(m_areas.size());
        for (auto &area : m_areas)
            id_to_area[area.m_id] = &area;

        for (auto &area : m_areas)
        {
            for (auto &connection : area.m_connections)
            {
                auto found = id_to_area.find(connection.id);
                if (found != id_to_area.end())
                    connection.area = found->second;
                else
                    connection.area = nullptr;
            }

            // Fill potentially visible areas as well
            for (auto &bindinfo : area.m_potentiallyVisibleAreas)
            {
                auto found = id_to_area.find(bindinfo.id);
                if (found != id_to_area.end())
                    bindinfo.area = found->second;
                else
                    bindinfo.area = nullptr;
            }
        }

        for (auto &area : m_areas)
            std::erase_if(area.m_connections, [](const NavConnect &connection) { return connection.area == nullptr; });
        m_isOK = true;
    }

    std::string m_mapName;
    bool m_isAnalized{};
    std::vector<CNavPlace> m_places;
    bool m_hasUnnamedAreas{};
    std::vector<CNavArea> m_areas;
    bool m_isOK = false;
};
