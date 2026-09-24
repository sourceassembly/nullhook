#pragma once

#include "common.hpp"
#include <algorithm>

struct SpotEncounter;

class CNavPlace
{
public:
    char m_name[256];
};

enum NavDirType
{
    NORTH = 0,
    EAST  = 1,
    SOUTH = 2,
    WEST  = 3,

    NUM_DIRECTIONS
};

enum
{
    MAX_NAV_TEAMS = 2
};

/**
 * A HidingSpot is a good place for a bot to crouch and wait for enemies
 */
class HidingSpot
{
public:
    enum
    {
        IN_COVER          = 0x01, // in a corner with good hard cover nearby
        GOOD_SNIPER_SPOT  = 0x02, // had at least one decent sniping corridor
        IDEAL_SNIPER_SPOT = 0x04, // can see either very far, or a large area, or both
        EXPOSED           = 0x08  // spot in the open, usually on a ledge or cliff
    };

    bool HasGoodCover() const
    {
        return (m_flags & IN_COVER) != 0;
    } // return true if hiding spot is in cover
    bool IsGoodSniperSpot() const
    {
        return (m_flags & GOOD_SNIPER_SPOT) != 0;
    }
    bool IsIdealSniperSpot() const
    {
        return (m_flags & IDEAL_SNIPER_SPOT) != 0;
    }
    bool IsExposed() const
    {
        return (m_flags & EXPOSED) != 0;
    }

    Vector m_pos;          // world coordinates of the spot
    unsigned int m_id;     // this spot's unique ID
    unsigned char m_flags; // bit flags
};

class CNavArea;
struct SpotEncounter;
struct NavConnect;

struct AreaBindInfo
{
    union
    {
        CNavArea *area;
        unsigned int id = 0;
    };

    unsigned char attributes{}; // VisibilityType
};

enum NavAttributeType
{
    NAV_MESH_INVALID = 0,
    NAV_MESH_CROUCH  = 0x0000001,      // must crouch to use this node/area
    NAV_MESH_JUMP    = 0x0000002,      // must jump to traverse this area (only used during generation)
    NAV_MESH_PRECISE   = 0x0000004,    // do not adjust for obstacles, just move along area
    NAV_MESH_NO_JUMP   = 0x0000008,    // inhibit discontinuity jumping
    NAV_MESH_STOP      = 0x0000010,    // must stop when entering this area
    NAV_MESH_RUN       = 0x0000020,    // must run to traverse this area
    NAV_MESH_WALK      = 0x0000040,    // must walk to traverse this area
    NAV_MESH_AVOID     = 0x0000080,    // avoid this area unless alternatives are too dangerous
    NAV_MESH_TRANSIENT = 0x0000100,    // area may become blocked, and should be periodically checked
    NAV_MESH_DONT_HIDE   = 0x0000200,  // area should not be considered for hiding spot generation
    NAV_MESH_STAND       = 0x0000400,  // bots hiding in this area should stand
    NAV_MESH_NO_HOSTAGES = 0x0000800,  // hostages shouldn't use this area
    NAV_MESH_STAIRS      = 0x0001000,  // this area represents stairs, do not attempt to climb or jump them - just walk up
    NAV_MESH_NO_MERGE     = 0x0002000, // don't merge this area with adjacent areas
    NAV_MESH_OBSTACLE_TOP = 0x0004000, // this nav area is the climb point on the tip of an obstacle
    NAV_MESH_CLIFF        = 0x0008000, // this nav area is adjacent to a drop of at least CliffHeight

    NAV_MESH_FIRST_CUSTOM = 0x00010000, // apps may define custom app-specific bits starting with this value
    NAV_MESH_LAST_CUSTOM = 0x04000000,  // apps must not define custom app-specific bits higher than with this value

    NAV_MESH_HAS_ELEVATOR = 0x40000000, // area is in an elevator's path
    NAV_MESH_NAV_BLOCKER  = 0x80000000, // area is blocked by nav blocker (Alas, needed to hijack a bit in the attributes to get within a cache line [7/24/2008 tom])
};

enum TFNavAttributeType
{
    TF_NAV_INVALID = 0x00000000,

    // Also look for NAV_MESH_NAV_BLOCKER (w/ nav_debug_blocked ConVar).
    TF_NAV_BLOCKED         = 0x00000001, // blocked for some TF-specific reason
    TF_NAV_SPAWN_ROOM_RED  = 0x00000002,
    TF_NAV_SPAWN_ROOM_BLUE = 0x00000004,
    TF_NAV_SPAWN_ROOM_EXIT = 0x00000008,
    TF_NAV_HAS_AMMO        = 0x00000010,
    TF_NAV_HAS_HEALTH      = 0x00000020,
    TF_NAV_CONTROL_POINT   = 0x00000040,

    TF_NAV_BLUE_SENTRY_DANGER = 0x00000080, // sentry can potentially fire upon enemies in this area
    TF_NAV_RED_SENTRY_DANGER  = 0x00000100,

    TF_NAV_BLUE_SETUP_GATE             = 0x00000800, // this area is blocked until the setup period is over
    TF_NAV_RED_SETUP_GATE              = 0x00001000, // this area is blocked until the setup period is over
    TF_NAV_BLOCKED_AFTER_POINT_CAPTURE = 0x00002000, // this area becomes blocked after the first point is capped
    TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE = 0x00004000, // this area is blocked until the first point is capped, then is unblocked
    TF_NAV_BLUE_ONE_WAY_DOOR           = 0x00008000,
    TF_NAV_RED_ONE_WAY_DOOR            = 0x00010000,

    TF_NAV_WITH_SECOND_POINT = 0x00020000, // modifier for BLOCKED_*_POINT_CAPTURE
    TF_NAV_WITH_THIRD_POINT  = 0x00040000, // modifier for BLOCKED_*_POINT_CAPTURE
    TF_NAV_WITH_FOURTH_POINT = 0x00080000, // modifier for BLOCKED_*_POINT_CAPTURE
    TF_NAV_WITH_FIFTH_POINT  = 0x00100000, // modifier for BLOCKED_*_POINT_CAPTURE

    TF_NAV_SNIPER_SPOT = 0x00200000, // this is a good place for a sniper to lurk
    TF_NAV_SENTRY_SPOT = 0x00400000, // this is a good place to build a sentry

    TF_NAV_ESCAPE_ROUTE         = 0x00800000, // for Raid mode
    TF_NAV_ESCAPE_ROUTE_VISIBLE = 0x01000000, // all areas that have visibility to the escape route

    TF_NAV_NO_SPAWNING = 0x02000000, // don't spawn bots in this area

    TF_NAV_RESCUE_CLOSET = 0x04000000, // for respawning friends in Raid mode

    TF_NAV_BOMB_CAN_DROP_HERE = 0x08000000, // the bomb can be dropped here and reached by the invaders in MvM

    TF_NAV_DOOR_NEVER_BLOCKS  = 0x10000000,
    TF_NAV_DOOR_ALWAYS_BLOCKS = 0x20000000,

    TF_NAV_UNBLOCKABLE = 0x40000000, // this area cannot be blocked

    // save/load these manually set flags, and don't clear them between rounds
    TF_NAV_PERSISTENT_ATTRIBUTES = TF_NAV_SNIPER_SPOT | TF_NAV_SENTRY_SPOT | TF_NAV_NO_SPAWNING | TF_NAV_BLUE_SETUP_GATE | TF_NAV_RED_SETUP_GATE | TF_NAV_BLOCKED_AFTER_POINT_CAPTURE | TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE | TF_NAV_BLUE_ONE_WAY_DOOR | TF_NAV_RED_ONE_WAY_DOOR | TF_NAV_DOOR_NEVER_BLOCKS | TF_NAV_DOOR_ALWAYS_BLOCKS | TF_NAV_UNBLOCKABLE | TF_NAV_WITH_SECOND_POINT | TF_NAV_WITH_THIRD_POINT | TF_NAV_WITH_FOURTH_POINT | TF_NAV_WITH_FIFTH_POINT | TF_NAV_RESCUE_CLOSET
};

enum NavTraverseType
{
    // NOTE: First 4 directions MUST match NavDirType
    GO_NORTH = 0,
    GO_EAST,
    GO_SOUTH,
    GO_WEST,

    GO_LADDER_UP,
    GO_LADDER_DOWN,
    GO_JUMP,
    GO_ELEVATOR_UP,
    GO_ELEVATOR_DOWN,

    NUM_TRAVERSE_TYPES
};

enum NavCornerType
{
    NORTH_WEST = 0,
    NORTH_EAST = 1,
    SOUTH_EAST = 2,
    SOUTH_WEST = 3,

    NUM_CORNERS
};

enum NavRelativeDirType
{
    FORWARD = 0,
    RIGHT,
    BACKWARD,
    LEFT,
    UP,
    DOWN,

    NUM_RELATIVE_DIRECTIONS
};

enum LadderDirectionType
{
    LADDER_UP = 0,
    LADDER_DOWN,

    NUM_LADDER_DIRECTIONS
};

class CNavArea
{
public:
    uint32_t m_id;
    int32_t m_attributeFlags;
    int32_t m_TFattributeFlags;
    Vector m_nwCorner;
    Vector m_seCorner;
    Vector m_center;
    float m_invDxCorners;
    float m_invDyCorners;
    float m_neZ;
    float m_swZ;
    float m_minZ;
    float m_maxZ;
    std::vector<NavConnect> m_connections;
    std::vector<HidingSpot> m_hidingSpots;
    std::vector<SpotEncounter> m_spotEncounters;
    uint32_t m_encounterSpotCount;
    uint16_t m_indexType;
    float m_earliestOccupyTime[MAX_NAV_TEAMS];
    float m_lightIntensity[NUM_CORNERS];
    uint32_t m_visibleAreaCount;
    uint32_t m_inheritVisibilityFrom;
    std::vector<AreaBindInfo> m_potentiallyVisibleAreas;

    // Check if the given point is overlapping the area
    // @return True if 'pos' is within 2D extents of area.
    bool IsOverlapping(const Vector &vecPos, float flTolerance = 0) const
    {
        if (vecPos.x + flTolerance < this->m_nwCorner.x)
            return false;

        if (vecPos.x - flTolerance > this->m_seCorner.x)
            return false;

        if (vecPos.y + flTolerance < this->m_nwCorner.y)
            return false;

        if (vecPos.y - flTolerance > this->m_seCorner.y)
            return false;

        return true;
    }

    // Check if the point is within the 3D bounds of this area
    bool Contains(Vector &vecPoint) const
    {
        if (!IsOverlapping(vecPoint))
            return false;

        if (vecPoint.z > m_maxZ)
            return false;

        if (vecPoint.z < m_minZ)
            return false;

        return true;
    }

    inline Vector getSwCorner() const
    {
        return { m_nwCorner.x, m_seCorner.y, m_swZ };
    }
    inline Vector getNeCorner() const
    {
        return { m_seCorner.x, m_nwCorner.y, m_neZ };
    }

    float GetZ(float x, float y) const
    {   
        if (m_invDxCorners == 0.0f || m_invDyCorners == 0.0f)
            return m_neZ;

        float u = (x - m_nwCorner.x) * m_invDxCorners;
        float v = (y - m_nwCorner.y) * m_invDyCorners;
        u = std::clamp(u, 0.0f, 1.0f);

        v = std::clamp(v, 0.0f, 1.0f);

        float northZ = m_nwCorner.z + u * (m_neZ - m_nwCorner.z);
        float southZ = m_swZ + u * (m_seCorner.z - m_swZ);

        return northZ + v * (southZ - northZ);
    }

    Vector getNearestPoint(const Vector2D &point) const
    {
        Vector close(0.0f);
        float x, y, z;

        x = std::clamp(point.x, m_nwCorner.x, m_seCorner.x);

        y = std::clamp(point.y, m_nwCorner.y, m_seCorner.y);

        z = GetZ(x, y);

        close.Init(x, y, z);
        return close;
    }
};

struct NavConnect
{
    NavConnect()
    {
        id     = 0;
        length = -1;
    }

    union
    {
        unsigned int id;
        CNavArea *area{};
    };

    mutable float length;

    bool operator==(const NavConnect &other) const
    {
        return area == other.area;
    }
};

struct SpotOrder
{
    float t; // parametric distance along ray where this spot first has LOS to our path

    union
    {
        HidingSpot *spot; // the spot to look at
        unsigned int id;  // spot ID for save/load
    };
};

/**
 * This struct stores possible path segments thru a CNavArea, and the dangerous
 * spots to look at as we traverse that path segment.
 */
struct SpotEncounter
{
    NavConnect from;
    NavDirType fromDir;
    NavConnect to;
    NavDirType toDir;
    // Ray path;                  // the path segment
    std::vector<SpotOrder> spots; // list of spots to look at, in order of occurrence
};
