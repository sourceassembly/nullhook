/*
    This file is part of Cathook.
    Cathook is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    Cathook is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with Cathook. If not, see <https://www.gnu.org/licenses/>.
*/

// Codeowners: TotallyNotElite

#include "common.hpp"
#include "micropather.h"
#include "CNavFile.h"
#include "Aimbot.hpp"
#include "MiscAimbot.hpp"
#include "NavBot.hpp"
#include "navparser.hpp"
#if ENABLE_VISUALS
#include "drawing.hpp"
#endif

#include <memory>
#include <functional>
#include <unordered_set>
#include <boost/container_hash/hash.hpp>

extern settings::Boolean roll_speedhack;
extern settings::Boolean roll_speedhack_navbot;

namespace navparser
{
static settings::Boolean enabled("nav.enabled", "false");
static settings::Boolean draw("nav.draw", "false");
static settings::Boolean look{ "nav.look-at-path", "false" };
static settings::Boolean look_legit{ "nav.look-at-path-legit", "false" };
static settings::Boolean draw_debug_areas("nav.draw.debug-areas", "false");
static settings::Boolean log_pathing{ "nav.log", "false" };
static settings::Int stuck_time{ "nav.stuck-time", "800" };
static settings::Int aim_speed{ "nav.smooth-speed", "7" };
static settings::Int vischeck_cache_time{ "nav.vischeck-cache.time", "240" };
static settings::Boolean vischeck_runtime{ "nav.vischeck-runtime.enabled", "true" };
static settings::Int vischeck_time{ "nav.vischeck-runtime.delay", "2000" };
static settings::Int stuck_detect_time{ "nav.anti-stuck.detection-time", "3" };
// How long until accumulated "Stuck time" expires
static settings::Int stuck_expire_time{ "nav.anti-stuck.expire-time", "10" };
// How long we should blacklist the node after being stuck for too long?
static settings::Int stuck_blacklist_time{ "nav.anti-stuck.blacklist-time", "120" };
static settings::Int sticky_ignore_time{ "nav.ignore.sticky-time", "15" };
static settings::Boolean path_during_setup{ "nav.path-during-setup", "false" };

static struct
{
    std::string ready_reason = "never ran";
    std::string navto_reason;
    std::string nav_path;
    std::string level_name;
    Vector navto_dest;
    size_t areas = 0, connections = 0, isolated_areas = 0;
    long long last_solve_ns = 0;
    int last_solve_result = -1;
    size_t last_solve_nodes = 0;
    unsigned int navto_ok = 0, navto_fail = 0, abandons = 0, cancels = 0;
    unsigned long long cm_calls = 0;
    unsigned int edge_pass = 0, edge_cachedok = 0, edge_selfbl = 0, edge_freebl = 0, edge_height = 0, edge_cachedbad = 0, edge_rayfail = 0;
    std::string rayfail_detail;
} navdebug;

const char *getPriorityName(int priority)
{
    switch (priority)
    {
    case 0:
        return "none";
    case patrol:
        return "patrol";
    case lowprio_health:
        return "lowprio_health";
    case staynear:
        return "staynear";
    case run_reload:
        return "run_reload";
    case snipe_sentry:
        return "snipe_sentry";
    case followbot:
        return "followbot";
    case ammo:
        return "ammo";
    case capture:
        return "capture";
    case prio_melee:
        return "melee";
    case engineer:
        return "engineer";
    case health:
        return "health";
    case danger:
        return "danger";
    default:
        return "?";
    }
}

// Cast a Ray and return if it hit
static bool CastRay(Vector origin, Vector endpos, unsigned mask, ITraceFilter *filter)
{
    trace_t trace;
    Ray_t ray;

    ray.Init(origin, endpos);

    // This was found to be So inefficient that it is literally unusable for our purposes. it is almost 1000x slower than the above.
    // ray.Init(origin, target, -right * HALF_PLAYER_WIDTH, right * HALF_PLAYER_WIDTH);

    PROF_SECTION(IEVV_TraceRay);
    g_ITrace->TraceRay(ray, mask, filter, &trace);

    return trace.DidHit();
}

static bool PassableSide(Vector origin, Vector target, Vector offset, unsigned int mask)
{
    trace_t trace;
    Ray_t ray;
    ray.Init(origin - offset, target - offset);
    g_ITrace->TraceRay(ray, mask, &trace::filter_navigation, &trace);
    if (!trace.DidHit())
        return true;
    if (!trace.startsolid && !trace.allsolid)
        return false;
    ray.Init(origin - offset * 0.5f, target - offset * 0.5f);
    g_ITrace->TraceRay(ray, mask, &trace::filter_navigation, &trace);
    return !trace.DidHit();
}

// Vischeck that considers player width
static bool IsPlayerPassableNavigation(Vector origin, Vector target, unsigned int mask = MASK_PLAYERSOLID)
{
    Vector delta = target - origin;
    delta.z    = 0.0f;
    if (delta.Length() < 16.0f)
        return true;

    if (std::fabs(target.z - origin.z) <= PLAYER_JUMP_HEIGHT)
        target.z = origin.z;

    Vector right(-delta.y, delta.x, 0.0f);
    right.NormalizeInPlace();
    Vector offset = right * HALF_PLAYER_WIDTH;

    return PassableSide(origin, target, offset, mask) && PassableSide(origin, target, -offset, mask);
}

enum class NavState
{
    Unavailable = 0,
    Active
};

struct CachedConnection
{
    int expire_tick;
    bool vischeck_state;
    bool stuck = false;
};

struct CachedStucktime
{
    int expire_tick;
    int time_stuck;
};

struct ConnectionInfo
{
    enum State
    {
        // Tried using this connection, failed for some reason
        STUCK,
    };
    int expire_tick;
    State state;
};

// Returns corrected "current_pos"
Vector handleDropdown(Vector current_pos, Vector next_pos)
{
    Vector to_target = (next_pos - current_pos);
    // Only do it if we'd fall quite a bit
    if (-to_target.z > PLAYER_JUMP_HEIGHT)
    {
        to_target.z = 0;
        to_target.NormalizeInPlace();
        Vector angles;
        VectorAngles(to_target, angles);
        // We need to really make sure we fall, so we go two times as far out as we should have to
        current_pos = GetForwardVector(current_pos, angles, PLAYER_WIDTH * 2.0f);
    }
    return current_pos;
}

class navPoints
{
public:
    Vector current;
    Vector center;
    // The above but on the "next" vector, used for height checks.
    Vector center_next;
    Vector next;
    navPoints(Vector A, Vector B, Vector C, Vector D) : current(A), center(B), center_next(C), next(D){};
};

// This function ensures that vischeck and pathing use the same logic.
navPoints determinePoints(CNavArea *current, CNavArea *next)
{
    auto area_center = current->m_center;
    auto next_center = next->m_center;
    // Gets a vector on the edge of the current area that is as close as possible to the center of the next area
    auto area_closest = current->getNearestPoint(next_center.AsVector2D());
    // Do the same for the other area
    auto next_closest = next->getNearestPoint(area_center.AsVector2D());

    // Use one of them as a center point, the one that is either x or y alligned with a center
    // Of the areas.
    // This will avoid walking into walls.
    auto center_point = area_closest;

    // Determine if alligned, if not, use the other one as the center point
    if (center_point.x != area_center.x && center_point.y != area_center.y && center_point.x != next_center.x && center_point.y != next_center.y)
    {
        center_point = next_closest;
        // Use the point closest to next_closest on the "original" mesh for z
        center_point.z = current->getNearestPoint(next_closest.AsVector2D()).z;
    }

    // Nearest point to center on "next"m used for height checks
    auto center_next = next->getNearestPoint(center_point.AsVector2D());

    return navPoints(area_center, center_point, center_next, next_center);
};

static int NavRayHit(Vector origin, Vector target, trace_t *out)
{
    Vector delta = target - origin;
    delta.z      = 0.0f;
    if (delta.Length() < 16.0f)
        return 0;
    if (std::fabs(target.z - origin.z) <= PLAYER_JUMP_HEIGHT)
        target.z = origin.z;
    Vector right(-delta.y, delta.x, 0.0f);
    right.NormalizeInPlace();
    Vector offset = right * HALF_PLAYER_WIDTH;
    Ray_t ray;
    ray.Init(origin - offset, target - offset);
    g_ITrace->TraceRay(ray, MASK_PLAYERSOLID, &trace::filter_navigation, out);
    if (out->DidHit())
        return 1;
    ray.Init(origin + offset, target + offset);
    g_ITrace->TraceRay(ray, MASK_PLAYERSOLID, &trace::filter_navigation, out);
    return out->DidHit() ? 2 : 0;
}

static std::string NavEdgeFailDetail(CNavArea *from, CNavArea *to, const navPoints &points)
{
    trace_t tr;
    int ray         = NavRayHit(points.current, points.center, &tr);
    const char *seg = "cur>ctr";
    if (!ray)
    {
        ray = NavRayHit(points.center, points.next, &tr);
        seg = "ctr>next";
    }
    if (!ray)
        return format("e", (int) from->m_id, ">", (int) to->m_id, " flaky(none hit on retry)");
    int idx = -1, cid = -1;
    if (tr.m_pEnt)
    {
        auto *hit = reinterpret_cast<IClientEntity *>(tr.m_pEnt);
        idx       = EntIndex(hit);
        if (!IDX_GOOD(idx) || g_IEntityList->GetClientEntity(idx) != hit)
            idx = -1;
        else if (auto *cc = EntClientClass(hit))
            cid = cc->m_ClassID;
    }
    return format("e", (int) from->m_id, ">", (int) to->m_id, " ", seg, " ray", ray, " frac:", tr.fraction, " ss:", (int) tr.startsolid, " as:", (int) tr.allsolid, " ent:", idx, " cid:", cid, " at ", (int) tr.endpos.x, ",", (int) tr.endpos.y, ",", (int) tr.endpos.z);
}

class Map : public micropather::Graph
{
public:
    CNavFile navfile;
    NavState state;
    micropather::MicroPather pather{ this, 3000, 6, true };
    std::string mapname;
    std::unordered_map<std::pair<CNavArea *, CNavArea *>, CachedConnection, boost::hash<std::pair<CNavArea *, CNavArea *>>> vischeck_cache;
    std::unordered_map<std::pair<CNavArea *, CNavArea *>, CachedStucktime, boost::hash<std::pair<CNavArea *, CNavArea *>>> connection_stuck_time;
    // This is a pure blacklist that does not get cleared and is for free usage internally and externally, e.g. blacklisting where enemies are standing
    // This blacklist only gets cleared on map change, and can be used time independantly.
    // the enum is the Blacklist reason, so you can easily edit it
    std::unordered_map<CNavArea *, BlacklistReason> free_blacklist;
    // When the local player stands on one of the nav squares the free blacklist should NOT run
    bool free_blacklist_blocked = false;

    std::unordered_set<CNavArea *> reachable_areas;

    Map(const char *nav_path, std::string level_name) : navfile(nav_path), mapname(std::move(level_name))
    {
        if (!navfile.m_isOK)
            state = NavState::Unavailable;
        else
        {
            state = NavState::Active;
            computeReachableAreas();
        }
    }

    void computeReachableAreas()
    {
        std::unordered_map<CNavArea *, std::vector<CNavArea *>> reverse;
        for (auto &area : navfile.m_areas)
            for (auto &conn : area.m_connections)
                if (conn.area)
                    reverse[conn.area].push_back(&area);

        std::unordered_set<CNavArea *> visited;
        std::vector<CNavArea *> order;
        std::function<void(CNavArea *)> dfs1 = [&](CNavArea *node) {
            visited.insert(node);
            for (auto &conn : node->m_connections)
                if (conn.area && !visited.count(conn.area))
                    dfs1(conn.area);
            order.push_back(node);
        };
        for (auto &area : navfile.m_areas)
            if (!visited.count(&area))
                dfs1(&area);

        visited.clear();
        std::vector<CNavArea *> best_scc;
        std::function<void(CNavArea *, std::vector<CNavArea *> &)> dfs2 = [&](CNavArea *node, std::vector<CNavArea *> &comp) {
            visited.insert(node);
            comp.push_back(node);
            auto it = reverse.find(node);
            if (it != reverse.end())
                for (auto *next : it->second)
                    if (!visited.count(next))
                        dfs2(next, comp);
        };
        for (auto it = order.rbegin(); it != order.rend(); ++it)
        {
            if (visited.count(*it))
                continue;
            std::vector<CNavArea *> comp;
            dfs2(*it, comp);
            if (comp.size() > best_scc.size())
                best_scc = std::move(comp);
        }
        if (best_scc.empty())
            return;

        std::vector<CNavArea *> stack{ best_scc.front() };
        reachable_areas.insert(best_scc.front());
        while (!stack.empty())
        {
            CNavArea *node = stack.back();
            stack.pop_back();
            for (auto &conn : node->m_connections)
                if (conn.area && reachable_areas.insert(conn.area).second)
                    stack.push_back(conn.area);
        }
    }
    float LeastCostEstimate(void *start, void *end) override
    {
        return reinterpret_cast<CNavArea *>(start)->m_center.DistTo(reinterpret_cast<CNavArea *>(end)->m_center);
    }
    void AdjacentCost(void *main, std::vector<micropather::StateCost> *adjacent) override
    {
        CNavArea &area = *reinterpret_cast<CNavArea *>(main);
        for (NavConnect &connection : area.m_connections)
        {
            if (!connection.area)
                continue;
            // An area being entered twice means it is blacklisted from entry entirely
            auto connection_key    = std::pair<CNavArea *, CNavArea *>(connection.area, connection.area);
            auto cached_connection = vischeck_cache.find(connection_key);

            // Entered and marked bad?
            if (cached_connection != vischeck_cache.end() && !cached_connection->second.vischeck_state)
            {
                ++navdebug.edge_selfbl;
                continue;
            }

            // If the extern blacklist is running, ensure we don't try to use a bad area
            bool is_blacklisted = false;
            if (!free_blacklist_blocked)
                for (auto const &entry : free_blacklist)
                {
                    if (entry.first == connection.area)
                    {
                        is_blacklisted = true;
                        break;
                    }
                }
            if (is_blacklisted)
            {
                ++navdebug.edge_freebl;
                continue;
            }

            auto points = determinePoints(&area, connection.area);

            // Apply dropdown
            points.center = handleDropdown(points.center, points.next);

            float height_diff = points.center_next.z - points.center.z;

            // Too high for us to jump!
            if (height_diff > PLAYER_JUMP_HEIGHT)
            {
                ++navdebug.edge_height;
                continue;
            }

            points.current.z += PLAYER_JUMP_HEIGHT;
            points.center.z += PLAYER_JUMP_HEIGHT;
            points.next.z += PLAYER_JUMP_HEIGHT;

            auto key    = std::pair<CNavArea *, CNavArea *>(&area, connection.area);
            auto cached = vischeck_cache.find(key);
            if (cached != vischeck_cache.end())
            {
                if (cached->second.vischeck_state)
                {
                    ++navdebug.edge_cachedok;
                    float cost = points.next.DistTo(points.current);
                    adjacent->push_back(micropather::StateCost{ reinterpret_cast<void *>(connection.area), cost });
                }
                else
                    ++navdebug.edge_cachedbad;
            }
            else
            {
                // Check if there is direct line of sight
                if (IsPlayerPassableNavigation(points.current, points.center) && IsPlayerPassableNavigation(points.center, points.next))
                {
                    ++navdebug.edge_pass;
                    vischeck_cache[key] = { TICKCOUNT_TIMESTAMP(60), true };

                    float cost = points.next.DistTo(points.current);
                    adjacent->push_back(micropather::StateCost{ reinterpret_cast<void *>(connection.area), cost });
                }
                else
                {
                    ++navdebug.edge_rayfail;
                    vischeck_cache[key] = { TICKCOUNT_TIMESTAMP(60), false };
                    if (navdebug.rayfail_detail.empty())
                        navdebug.rayfail_detail = NavEdgeFailDetail(&area, connection.area, points);
                }
            }
        }
    }


    CNavArea *findClosestNavSquare(const Vector &vec, bool require_reachable = false)
    {
        auto vec_corrected = vec;
        vec_corrected.z += PLAYER_JUMP_HEIGHT;
        float overall_best_dist = FLT_MAX, best_dist = FLT_MAX;

        CNavArea *overall_best_square = nullptr, *best_square = nullptr;

        for (auto &i : navfile.m_areas)
        {
            if (i.m_connections.empty())
                continue;

            if (require_reachable && !reachable_areas.count(&i))
                continue;

            // Marked bad, do not use if local origin
            if (g_pLocalPlayer->v_Origin == vec)
            {
                auto key   = std::pair<CNavArea *, CNavArea *>(&i, &i);
                auto found = vischeck_cache.find(key);
                if (found != vischeck_cache.end() && !found->second.vischeck_state)
                    continue;
            }

            float dist = i.m_center.DistToSqr(vec);
            if (dist < best_dist)
            {
                best_dist   = dist;
                best_square = &i;
            }

            if (overall_best_dist <= dist)
                continue;

            auto center_corrected = i.m_center;
            center_corrected.z += PLAYER_JUMP_HEIGHT;


            if (!i.IsOverlapping(vec) || !IsVectorVisibleNavigation(vec_corrected, center_corrected))
                continue;

            overall_best_dist   = dist;
            overall_best_square = &i;


            if (overall_best_dist == best_dist)
                return overall_best_square;
        }

        return overall_best_square ? overall_best_square : best_square;
    }
    std::vector<void *> findPath(CNavArea *local, CNavArea *dest)
    {
        using namespace std::chrono;

        if (state != NavState::Active)
            return {};

        if (log_pathing)
        {
            logging::Info("Start: (%f,%f,%f)", local->m_center.x, local->m_center.y, local->m_center.z);
            logging::Info("End: (%f,%f,%f)", dest->m_center.x, dest->m_center.y, dest->m_center.z);
        }

        std::vector<void *> pathNodes;
        float cost;

        navdebug.edge_pass = navdebug.edge_cachedok = navdebug.edge_selfbl = navdebug.edge_freebl = navdebug.edge_height = navdebug.edge_cachedbad = navdebug.edge_rayfail = 0;
        navdebug.rayfail_detail.clear();

        auto purgeTransientVischeck = [this]() {
            bool erased = false;
            for (auto it = vischeck_cache.begin(); it != vischeck_cache.end();)
            {
                if (!it->second.vischeck_state && !it->second.stuck)
                {
                    it     = vischeck_cache.erase(it);
                    erased = true;
                }
                else
                    ++it;
            }
            if (erased)
                pather.Reset();
            return erased;
        };

        time_point begin_pathing = high_resolution_clock::now();
        int result               = pather.Solve(reinterpret_cast<void *>(local), reinterpret_cast<void *>(dest), &pathNodes, &cost);
        if (result != micropather::MicroPather::SOLVED && result != micropather::MicroPather::START_END_SAME && purgeTransientVischeck())
        {
            pathNodes.clear();
            result = pather.Solve(reinterpret_cast<void *>(local), reinterpret_cast<void *>(dest), &pathNodes, &cost);
        }
        long long timetaken      = duration_cast<nanoseconds>(high_resolution_clock::now() - begin_pathing).count();
        navdebug.last_solve_result = result;
        navdebug.last_solve_nodes  = pathNodes.size();
        navdebug.last_solve_ns     = timetaken;
        if (result != micropather::MicroPather::SOLVED && result != micropather::MicroPather::START_END_SAME)
        {
            logging::Info("NavDbg: solve fail r=%d | edges pass=%u cok=%u selfbl=%u freebl=%u height=%u cbad=%u rayfail=%u | %s", result, navdebug.edge_pass, navdebug.edge_cachedok, navdebug.edge_selfbl, navdebug.edge_freebl, navdebug.edge_height, navdebug.edge_cachedbad, navdebug.edge_rayfail, navdebug.rayfail_detail.c_str());
            pather.Reset();
        }
        if (log_pathing)
            logging::Info("Pathing: Pather result: %i. Nodes: %zu. Time taken (NS): %lld", result, pathNodes.size(), timetaken);
        // Start and end are the same, return start node
        if (result == micropather::MicroPather::START_END_SAME)
            return { reinterpret_cast<void *>(local) };

        return pathNodes;
    }

    void updateIgnores()
    {
        static Timer update_time;
        if (!update_time.test_and_set(1000))
            return;

        // Sentries make sounds, so we can just rely on soundcache here and always clear sentries
        NavEngine::clearFreeBlacklist(SENTRY);
        // Find sentries and stickies
        for (int i = g_IEngine->GetMaxClients() + 1; i < MAX_ENTITIES; i++)
        {
            CachedEntity* ent = ENTITY(i);
            if (CE_INVALID(ent) || !ent->m_bAlivePlayer() || ent->m_iTeam() == g_pLocalPlayer->team)
                continue;
            bool is_sentry = ent->m_iClassID() == CL_CLASS(CObjectSentrygun);
            bool is_sticky = ent->m_iClassID() == CL_CLASS(CTFGrenadePipebombProjectile) && CE_INT(ent, netvar.iPipeType) == 1 && CE_VECTOR(ent, netvar.vVelocity).IsZero(1.0f);
            // Not sticky/sentry, ignore.
            // (Or dormant sticky)
            if (!is_sentry && (!is_sticky || CE_BAD(ent)))
                continue;
            if (is_sentry)
            {
                if (CE_INT(ent, netvar.m_iSentryState) == 0)
                    continue;

                // Should we even ignore the sentry?
                // Soldier/Heavy do not care about Level 1 or mini sentries
                bool is_strong_class = g_pLocalPlayer->clazz == tf_soldier || g_pLocalPlayer->clazz == tf_heavy;
                int bullet           = CE_INT(ent, netvar.m_iAmmoShells);
                int rocket           = CE_INT(ent, netvar.m_iAmmoRockets);
                if ((is_strong_class && (CE_BYTE(ent, netvar.m_bMiniBuilding) || CE_INT(ent, netvar.iUpgradeLevel) == 1)) || (bullet == 0 && (CE_INT(ent, netvar.iUpgradeLevel) != 3 || rocket == 0)))
                    continue;

                // It's still building/being sapped, ignore.
                // Unless it just was deployed from a carry, then it's dangerous
                if ((!CE_BYTE(ent, netvar.m_bCarryDeploy) && CE_BYTE(ent, netvar.m_bBuilding)) || CE_BYTE(ent, netvar.m_bPlacing) || CE_BYTE(ent, netvar.m_bHasSapper))
                    continue;

                // Get origin of the sentry
                auto building_origin = GetBuildingPosition(ent);
                // For dormant sentries we need to add the jump height to the z
                if (CE_BAD(ent))
                    building_origin.z += PLAYER_JUMP_HEIGHT;
                // Actual building check
                for (auto &i : navfile.m_areas)
                {
                    Vector area = i.m_center;
                    area.z += PLAYER_JUMP_HEIGHT;
                    // Out of range
                    if (building_origin.DistToSqr(area) > (1100 + HALF_PLAYER_WIDTH) * (1100 + HALF_PLAYER_WIDTH))
                        continue;
                    // Check if sentry can see us
                    if (!IsVectorVisibleNavigation(building_origin, area))
                        continue;
                    // Blacklist because it's in view range of the sentry
                    free_blacklist[&i] = SENTRY;
                }
            }
            else
            {
                auto sticky_origin = ent->m_vecOrigin();
                // Make sure the sticky doesn't vischeck from inside the floor
                sticky_origin.z += PLAYER_JUMP_HEIGHT / 2.0f;
                for (auto &i : navfile.m_areas)
                {
                    Vector area = i.m_center;
                    area.z += PLAYER_JUMP_HEIGHT;
                    // Out of range
                    if (sticky_origin.DistToSqr(area) > (130 + HALF_PLAYER_WIDTH) * (130 + HALF_PLAYER_WIDTH))
                        continue;
                    // Check if Sticky can see the reason
                    if (!IsVectorVisibleNavigation(sticky_origin, area))
                        continue;
                    // Blacklist because it's in range of the sticky, but stickies make no noise, so blacklist it for a specific timeframe
                    free_blacklist[&i] = { STICKY, TICKCOUNT_TIMESTAMP(*sticky_ignore_time) };
                }
            }
        }

        static size_t previous_blacklist_size = 0;

        bool erased = false;
        if (previous_blacklist_size != free_blacklist.size())
            erased = true;
        previous_blacklist_size = free_blacklist.size();
        // When we switch to c++20, we can use std::erase_if
        for (auto it = begin(free_blacklist); it != end(free_blacklist);)
        {
            // Clear entries from the free blacklist when expired and if it has a set time
            if (it->second.time && it->second.time < g_GlobalVars->tickcount)
            {
                it     = free_blacklist.erase(it); // previously this was something like m_map.erase(it++);
                erased = true;
            }
            else
                ++it;
        }

        for (auto it = begin(vischeck_cache); it != end(vischeck_cache);)
        {
            if (it->second.expire_tick < g_GlobalVars->tickcount)
            {
                it     = vischeck_cache.erase(it); // previously this was something like m_map.erase(it++);
                erased = true;
            }
            else
                ++it;
        }
        for (auto it = begin(connection_stuck_time); it != end(connection_stuck_time);)
        {
            if (it->second.expire_tick < g_GlobalVars->tickcount)
            {
                it     = connection_stuck_time.erase(it); // previously this was something like m_map.erase(it++);
                erased = true;
            }
            else
                ++it;
        }
        if (erased)
            pather.Reset();
    }

    void Reset()
    {
        vischeck_cache.clear();
        connection_stuck_time.clear();
        free_blacklist.clear();
        pather.Reset();
    }

    // Uncesseray thing that is sadly necessary
    void PrintStateInfo(void *) override
    {
    }
};

namespace NavEngine
{
std::unique_ptr<Map> map;
Crumb last_crumb;
std::vector<Crumb> crumbs;
#if ENABLE_VISUALS
static std::mutex navdraw_mutex;
static std::vector<std::string> debug_lines_snapshot;
static std::vector<Vector> crumb_snapshot;
static bool dbg_nav_ready  = false;
static bool dbg_area_valid = false;
static Vector dbg_area_quad[4];
static Vector dbg_area_edge;
#endif

int current_priority    = 0;
bool current_navtolocal = false;
bool repath_on_fail     = false;
Vector last_destination;
static bool map_dirty = true;

static std::string resolveNavPath(const std::string &level_name)
{
    std::vector<std::string> candidates;
    const char *game_dir = g_IEngine->GetGameDirectory();
    if (game_dir)
    {
        candidates.emplace_back(std::string(game_dir) + "/maps/" + level_name + ".nav");
        candidates.emplace_back(std::string(game_dir) + "/download/maps/" + level_name + ".nav");
    }
    char cwd[PATH_MAX + 1];
    if (getcwd(cwd, sizeof(cwd)))
        candidates.emplace_back(std::string(cwd) + "/tf/maps/" + level_name + ".nav");

    for (auto &candidate : candidates)
    {
        std::ifstream fs(candidate, std::ios::binary);
        if (fs.is_open())
            return candidate;
    }
    return candidates.empty() ? "" : candidates.front();
}

void cancelPath();
static void resetAntistuckState();

static void ensureMapLoaded()
{
    if (!map_dirty && map)
        return;
    if (!g_IEngine->IsInGame())
        return;
    std::string level_name = GetLevelName();
    navdebug.level_name = level_name;
    if (level_name.empty())
        return;
    if (map && map->mapname == level_name)
    {
        map->Reset();
        map_dirty = false;
        return;
    }

    crumbs.clear();
    last_crumb.navarea   = nullptr;
    current_priority     = 0;
    repath_on_fail       = false;
    resetAntistuckState();

    std::string nav_path = resolveNavPath(level_name);
    logging::Info("Pathing: Nav File location: %s", nav_path.c_str());
    map       = std::make_unique<Map>(nav_path.c_str(), level_name);
    map_dirty = false;

    navdebug.nav_path       = nav_path;
    navdebug.areas          = map->navfile.m_areas.size();
    navdebug.connections    = 0;
    navdebug.isolated_areas = 0;
    for (auto &area : map->navfile.m_areas)
    {
        navdebug.connections += area.m_connections.size();
        if (area.m_connections.empty())
            ++navdebug.isolated_areas;
    }
}

bool isReady()
{
    ensureMapLoaded();
    auto fail = [](const char *reason)
    {
        navdebug.ready_reason = reason;
        return false;
    };
    if (!enabled && !hacks::tf2::NavBot::isEnabled())
        return fail("nav.enabled and navbot.enabled both off");
    if (!g_IEngine->IsInGame())
        return fail("not in game");
    if (!map)
        return fail("map object not loaded");
    if (map->state != NavState::Active)
        return fail("nav file unavailable/invalid");
    if (path_during_setup)
    {
        navdebug.ready_reason.clear();
        return true;
    }
    if (g_pGameRules->InWaitingForPlayers())
        return fail("waiting for players");
    std::string level_name = GetLevelName();
    if (level_name == "plr_pipeline" || g_pGameRules->RoundMode() > CGameRules::GR_STATE_PREROUND)
    {
        navdebug.ready_reason.clear();
        return true;
    }
    return fail("waiting for round start (preround/setup)");
}

bool hasNavMesh()
{
    ensureMapLoaded();
    return map && map->state == NavState::Active;
}

bool isPathing()
{
    return !crumbs.empty();
}

CNavFile *getNavFile()
{
    return &map->navfile;
}

CNavArea *findClosestNavSquare(const Vector origin)
{
    return map->findClosestNavSquare(origin);
}

std::vector<Crumb> *getCrumbs()
{
    return &crumbs;
}

static Timer inactivity{};
static Timer time_spent_on_crumb{};
static Timer navto_fail_cooldown{};
static Vector last_failed_dest{};
static bool have_failed_dest = false;

static Vector antistuck_anchor{};
static Timer antistuck_window{};
static bool antistuck_armed         = false;
static size_t antistuck_last_crumbs = 0;
static std::vector<Vector> reached_crumb_history;

void abandonPath();

static void resetAntistuckState()
{
    antistuck_armed = false;
    reached_crumb_history.clear();
}

static void noteCrumbReached(const Crumb &crumb)
{
    reached_crumb_history.push_back(crumb.vec);
    if (reached_crumb_history.size() > 8)
        reached_crumb_history.erase(reached_crumb_history.begin());
}

static bool detectCrumbPingPong()
{
    if (reached_crumb_history.size() < 6)
        return false;
    const auto &h = reached_crumb_history;
    const size_t n = h.size();
    const Vector &A = h[n - 6];
    const Vector &B = h[n - 5];
    if (A.DistTo(B) < 60.0f)
        return false;
    for (int k = 0; k < 3; ++k)
    {
        if (h[n - 6 + k * 2].DistTo(A) > 60.0f || h[n - 5 + k * 2].DistTo(B) > 60.0f)
            return false;
    }
    return true;
}

static int clampedBlacklistTime()
{
    int time = *stuck_blacklist_time;
    if (time < 100)
        time = 100;
    else if (time > 500)
        time = 500;
    return time;
}

static void blacklistStuckConnectionAndRepath(const char *reason)
{
    if (!map || crumbs.empty())
        return;
    CNavArea *from = last_crumb.navarea ? last_crumb.navarea : crumbs[0].navarea;
    CNavArea *to   = crumbs[0].navarea;
    if (from && to)
    {
        int blacklist_time = clampedBlacklistTime();
        auto key           = std::pair<CNavArea *, CNavArea *>(from, to);
        map->vischeck_cache[key].expire_tick    = TICKCOUNT_TIMESTAMP(blacklist_time);
        map->vischeck_cache[key].vischeck_state = false;
        map->vischeck_cache[key].stuck          = true;
        map->connection_stuck_time.erase(key);
        if (log_pathing)
            logging::Info("Pathing: antistuck (%s), blacklisted connection %d->%d for %ds", reason, from->m_id, to->m_id, blacklist_time);
    }
    resetAntistuckState();
    abandonPath();
}

bool navTo(const Vector &destination, int priority, bool should_repath, bool nav_to_local, bool is_repath)
{
    auto fail = [&](std::string reason)
    {
        if (log_pathing && reason != navdebug.navto_reason)
            logging::Info("Pathing: navTo failed: %s (dest %.0f,%.0f,%.0f prio %d)", reason.c_str(), destination.x, destination.y, destination.z, priority);
        navdebug.navto_reason = std::move(reason);
        ++navdebug.navto_fail;
        last_failed_dest  = destination;
        have_failed_dest  = true;
        navto_fail_cooldown.update();
        return false;
    };
    if (!isReady())
        return fail("not ready: " + navdebug.ready_reason);
    // Don't path, priority is too low
    if (priority < current_priority)
        return fail(format("priority ", priority, " < current ", current_priority));
    if (have_failed_dest && destination == last_failed_dest && !navto_fail_cooldown.check(500))
    {
        ++navdebug.navto_fail;
        return false;
    }
    if (log_pathing)
        logging::Info("Priority: %d", priority);

    CNavArea *start_area = map->findClosestNavSquare(g_pLocalPlayer->v_Origin);
    CNavArea *dest_area  = map->findClosestNavSquare(destination, true);
    if (!dest_area)
        dest_area = map->findClosestNavSquare(destination);

    if (!start_area)
        return fail("no start area found");
    if (!dest_area)
        return fail("no dest area found");
    auto path = map->findPath(start_area, dest_area);
    if (path.empty())
    {
        const Vector &origin = g_pLocalPlayer->v_Origin;
        std::pair<float, CNavArea *> best[5];
        size_t count = 0;
        for (auto &area : map->navfile.m_areas)
        {
            if (&area == start_area || area.m_connections.empty())
                continue;
            float d = area.m_center.DistToSqr(origin);
            if (count < 5)
            {
                best[count++] = { d, &area };
                continue;
            }
            size_t worst = 0;
            for (size_t k = 1; k < 5; ++k)
                if (best[k].first > best[worst].first)
                    worst = k;
            if (d < best[worst].first)
                best[worst] = { d, &area };
        }
        for (size_t i = 0; i < count; ++i)
        {
            path = map->findPath(best[i].second, dest_area);
            if (!path.empty())
            {
                start_area = best[i].second;
                if (log_pathing)
                    logging::Info("Pathing: navTo recovered via alt start #%d", start_area->m_id);
                break;
            }
        }
    }
    if (path.empty())
        return fail(format("pather no path (result ", navdebug.last_solve_result, ", start #", start_area->m_id, " dest #", dest_area->m_id, ")"));

    if (!nav_to_local)
        path.erase(path.begin());
    crumbs.clear();
    resetAntistuckState();

    for (size_t i = 0; i < path.size(); ++i)
    {
        auto *area = reinterpret_cast<CNavArea *>(path.at(i));

        if (i != path.size() - 1)
        {
            auto *next_area = (CNavArea *) path.at(i + 1);

            auto points = determinePoints(area, next_area);

            points.center = handleDropdown(points.center, points.next);

            crumbs.push_back({ area, points.current });
            crumbs.push_back({ area, points.center });
        }
        else
            crumbs.push_back({ area, area->m_center });
    }

    crumbs.push_back({ nullptr, destination });
    inactivity.update();

    current_priority   = priority;
    current_navtolocal = nav_to_local;
    repath_on_fail     = should_repath;
    // Ensure we know where to go
    if (repath_on_fail)
        last_destination = destination;

    ++navdebug.navto_ok;
    navdebug.navto_reason.clear();
    have_failed_dest = false;
    navdebug.navto_dest = destination;
    if (log_pathing)
        logging::Info("Pathing: navTo ok prio %d dest (%.0f,%.0f,%.0f) nodes %zu crumbs %zu", priority, destination.x, destination.y, destination.z, path.size(), crumbs.size());
    return true;
}

float getPathCost(const Vector &start, const Vector &dest)
{
    if (!isReady() || !map)
        return FLT_MAX;
    CNavArea *start_area = map->findClosestNavSquare(start);
    CNavArea *dest_area  = map->findClosestNavSquare(dest);
    if (!start_area || !dest_area)
        return FLT_MAX;
    if (start_area == dest_area)
        return start.DistTo(dest);

    std::vector<void *> path_nodes;
    float cost = FLT_MAX;
    int result = map->pather.Solve(reinterpret_cast<void *>(start_area), reinterpret_cast<void *>(dest_area), &path_nodes, &cost);
    if (result == micropather::MicroPather::START_END_SAME)
        return start.DistTo(dest);
    if (result != micropather::MicroPather::SOLVED)
        return FLT_MAX;
    return cost;
}

// Use when something unexpected happens, e.g. vischeck fails
void abandonPath()
{
    if (!map)
        return;
    ++navdebug.abandons;
    if (log_pathing)
        logging::Info("Pathing: abandonPath (repath=%d)", repath_on_fail);
    map->pather.Reset();
    crumbs.clear();
    last_crumb.navarea = nullptr;
    resetAntistuckState();
    // We want to repath on failure
    if (repath_on_fail)
        navTo(last_destination, current_priority, true, current_navtolocal, false);
    else
        current_priority = 0;
}

// Use to cancel pathing completely
void cancelPath()
{
    ++navdebug.cancels;
    crumbs.clear();
    last_crumb.navarea = nullptr;
    current_priority   = 0;
    repath_on_fail     = false;
    resetAntistuckState();
}

static Timer last_jump{};
// Used to determine if we want to jump or if we want to crouch
static bool crouch          = false;
static int ticks_since_jump = 0;
static Crumb current_crumb;

static void followCrumbs()
{
    size_t crumbs_amount = crumbs.size();

    // No more crumbs, reset status
    if (!crumbs_amount)
    {
        // Invalidate last crumb
        last_crumb.navarea = nullptr;

        repath_on_fail   = false;
        current_priority = 0;
        return;
    }

    if (current_crumb.navarea != crumbs[0].navarea)
        time_spent_on_crumb.update();
    current_crumb = crumbs[0];

    // Ensure we do not try to walk downwards unless we are falling
    static std::vector<float> fall_vec{};
    Vector vel;
    velocity::EstimateAbsVelocity(RAW_ENT(LOCAL_E), vel);

    fall_vec.push_back(vel.z);
    if (fall_vec.size() > 10)
        fall_vec.erase(fall_vec.begin());

    bool reset_z = true;
    for (auto const &entry : fall_vec)
    {
        if (!(entry <= 0.01f && entry >= -0.01f))
            reset_z = false;
    }
    if (reset_z)
    {
        reset_z = false;

        Ray_t ray;
        trace_t trace;
        Vector end = g_pLocalPlayer->v_Origin;
        end.z -= 100.0f;

        trace::filter_default.SetSelf(RAW_ENT(LOCAL_E));

        ray.Init(g_pLocalPlayer->v_Origin, end, EntOBBMins(RAW_ENT(LOCAL_E)), EntOBBMaxs(RAW_ENT(LOCAL_E)));
        g_ITrace->TraceRay(ray, MASK_PLAYERSOLID, &trace::filter_default, &trace);
        // Only reset if we are standing on a building
        int ground_idx       = trace.m_pEnt ? EntIndex((IClientEntity *) trace.m_pEnt) : -1;
        CachedEntity *ground = ENTITY(ground_idx);
        if (trace.DidHit() && ground && ground->m_Type() == ENTITY_BUILDING)
            reset_z = true;
    }

    Vector current_vec = crumbs[0].vec;
    if (reset_z)
        current_vec.z = g_pLocalPlayer->v_Origin.z;

    auto crumb_reach = [](const Crumb &crumb) { return crumb.navarea ? 50.0f : 20.0f; };

    // We are close enough to the crumb to have reached it
    if (current_vec.DistTo(g_pLocalPlayer->v_Origin) < crumb_reach(crumbs[0]))
    {
        last_crumb = crumbs[0];
        noteCrumbReached(crumbs[0]);
        crumbs.erase(crumbs.begin());
        time_spent_on_crumb.update();
        if (!--crumbs_amount)
            return;
        inactivity.update();
    }

    current_vec = crumbs[0].vec;
    if (reset_z)
        current_vec.z = g_pLocalPlayer->v_Origin.z;

    // We are close enough to the second crumb, Skip both (This is espcially helpful with drop downs)
    if (crumbs.size() > 1 && crumbs[1].vec.DistTo(g_pLocalPlayer->v_Origin) < crumb_reach(crumbs[1]))
    {
        last_crumb = crumbs[1];
        noteCrumbReached(crumbs[0]);
        noteCrumbReached(crumbs[1]);
        crumbs.erase(crumbs.begin(), crumbs.begin() + 2);
        if (crumbs.empty())
            return;
        inactivity.update();
    }

    // If we make any progress at all, reset this
    else
    {
        // If we spend way too long on this crumb, ignore the logic below
        if (!time_spent_on_crumb.check(*stuck_detect_time * 1000))
        {
            Vector vel;
            velocity::EstimateAbsVelocity(RAW_ENT(LOCAL_E), vel);
            // 44.0f -> Revved brass beast, do not use z axis as jumping counts towards that. Yes this will mean long falls will trigger it, but that is not really bad.
            if (!vel.AsVector2D().IsZero(40.0f))
                inactivity.update();
        }
    }

    // Detect when jumping is necessary.
    // 1. No jumping if zoomed (or revved)
    // 2. Jump if its necessary to do so based on z values
    // 3. Jump if stuck (not getting closer) for more than stuck_time/2 (500ms)
    if ((!(g_pLocalPlayer->holding_sniper_rifle && g_pLocalPlayer->bZoomed) && !(g_pLocalPlayer->bRevved || g_pLocalPlayer->bRevving) && (crouch || crumbs[0].vec.z - g_pLocalPlayer->v_Origin.z > 18) && last_jump.check(200)) || (last_jump.check(200) && inactivity.check(*stuck_time / 2)))
    {
        auto local = map->findClosestNavSquare(g_pLocalPlayer->v_Origin);
        // Check if current area allows jumping
        if (!local || !(local->m_attributeFlags & (NAV_MESH_NO_JUMP | NAV_MESH_STAIRS)))
        {
            // Make it crouch until we land, but jump the first tick
            current_user_cmd->buttons |= crouch ? IN_DUCK : IN_JUMP;

            // Only flip to crouch state, not to jump state
            if (!crouch)
            {
                crouch           = true;
                ticks_since_jump = 0;
            }
            ticks_since_jump++;

            // Update jump timer now since we are back on ground
            if (crouch && CE_INT(LOCAL_E, netvar.iFlags) & FL_ONGROUND && ticks_since_jump > 3)
            {
                // Reset
                crouch = false;
                last_jump.update();
            }
        }
    }

    if (roll_speedhack && roll_speedhack_navbot && !g_pLocalPlayer->bZoomed && !(current_user_cmd->buttons & IN_JUMP))
        current_user_cmd->buttons |= IN_DUCK;

    /*if (inactivity.check(*stuck_time) || (inactivity.check(*unreachable_time) && !IsVectorVisible(g_pLocalPlayer->v_Origin, *crumb_vec + Vector(.0f, .0f, 41.5f), false, LOCAL_E, MASK_PLAYERSOLID)))
    {
        if (crumbs[0].navarea)
            ignoremanager::addTime(last_area, *crumb, inactivity);
        repath();
        return;
    }*/

    // Look at path
    if (look && !hacks::shared::aimbot::isAiming())
    {
        Vector next{ crumbs[0].vec.x, crumbs[0].vec.y, g_pLocalPlayer->v_Eye.z };
        next = GetAimAtAngles(g_pLocalPlayer->v_Eye, next);

        // Slow aim to smoothen
        hacks::tf2::misc_aimbot::DoSlowAim(next, *aim_speed);
        current_user_cmd->viewangles = next;
    }

    if (look_legit && !hacks::shared::aimbot::isAiming())
    {
        float best_dist                = FLT_MAX;
        std::optional<Vector> look_vec = std::nullopt;
        for (int i = 1; i <= g_IEngine->GetMaxClients(); i++)
        {
            CachedEntity *ent = ENTITY(i);
            if (i == g_pLocalPlayer->entity_idx || CE_INVALID(ent) || !ent->m_bEnemy())
                continue;
            auto sound = soundcache::GetSoundLocation(i);
            if (!sound)
                continue;
            sound->z += PLAYER_JUMP_HEIGHT;
            if (sound->DistTo(g_pLocalPlayer->v_Eye) < best_dist && (IsVectorVisible(g_pLocalPlayer->v_Eye, *sound, true) || sound->DistTo(g_pLocalPlayer->v_Eye) <= 400.0f))
            {
                best_dist = sound->DistTo(g_pLocalPlayer->v_Eye);
                look_vec  = sound;
            }
        }
        if (look_vec)
        {
            Vector aim_ang = GetAimAtAngles(g_pLocalPlayer->v_Eye, *look_vec);
            hacks::tf2::misc_aimbot::DoSlowAim(aim_ang, 20);
            current_user_cmd->viewangles = aim_ang;
        }
        else
        {
            static Vector next{};
            static bool looked_at_point = true;
            static Timer choose_new_point;

            static int wait_time = 1000;
            static int speed     = 10;

            if (looked_at_point && choose_new_point.test_and_set(wait_time))
            {
                next = { crumbs[0].vec.x, crumbs[0].vec.y, g_pLocalPlayer->v_Eye.z };
                next = GetAimAtAngles(g_pLocalPlayer->v_Eye, next);
                next.x += UniformRandomInt(-1, 1);
                next.y += UniformRandomInt(-45, 45);
                fClampAngle(next);
                looked_at_point = false;
            }

            if ((current_user_cmd->viewangles - next).IsZero(10.0f))
            {
                if (!looked_at_point)
                    choose_new_point.update();
                looked_at_point = true;
                wait_time       = 750 + UniformRandomInt(0, 4000);
                speed           = 10 + UniformRandomInt(0, 2);
            }

            Vector next_slow = next;
            hacks::tf2::misc_aimbot::DoSlowAim(next_slow, speed);
            current_user_cmd->viewangles = next_slow;
        }
    }

    WalkTo(current_vec);
}

static Timer vischeck_timer{};
void vischeckPath()
{
    // No crumbs to check, or vischeck timer should not run yet, bail.
    if (crumbs.size() < 2 || !vischeck_timer.test_and_set(*vischeck_time))
        return;

    // Iterate all the crumbs
    for (int i = 0; i < (int) crumbs.size() - 1; i++)
    {
        auto current_crumb  = crumbs[i];
        auto next_crumb     = crumbs[i + 1];
        if (!current_crumb.navarea || !next_crumb.navarea)
            continue;
        auto current_center = current_crumb.vec;
        auto next_center    = next_crumb.vec;

        current_center.z += PLAYER_JUMP_HEIGHT;
        next_center.z += PLAYER_JUMP_HEIGHT;
        auto key = std::pair<CNavArea *, CNavArea *>(current_crumb.navarea, next_crumb.navarea);
        // Check if we can pass, if not, abort pathing and mark as bad
        if (!IsPlayerPassableNavigation(current_center, next_center))
        {
            // Mark as invalid for a while
            map->vischeck_cache[key] = { TICKCOUNT_TIMESTAMP(*vischeck_cache_time), false, true };
            abandonPath();
            return;
        }
        // Else we can update the cache (if not marked bad before this)
        else if (map->vischeck_cache.find(key) == map->vischeck_cache.end() || map->vischeck_cache[key].vischeck_state)
        {
            map->vischeck_cache[key] = { TICKCOUNT_TIMESTAMP(*vischeck_cache_time), true };
        }
    }
}

static Timer blacklist_check_timer{};
// Check if one of the crumbs is suddenly blacklisted
void checkBlacklist()
{
    // Only check every 500ms
    if (!blacklist_check_timer.test_and_set(500))
        return;

    // Local player is ubered and does not care about the blacklist
    // TODO: Only for damage type things
    if (IsPlayerInvulnerable(LOCAL_E))
    {
        map->free_blacklist_blocked = true;
        map->pather.Reset();
        return;
    }
    CNavArea *local_area = map->findClosestNavSquare(g_pLocalPlayer->v_Origin);
    for (auto const &entry : map->free_blacklist)
    {
        // Local player is in a blocked area, so temporarily remove the blacklist as else we would be stuck
        if (entry.first == local_area)
        {
            map->free_blacklist_blocked = true;
            map->pather.Reset();
            return;
        }
    }

    // Local player is not blocking the nav area, so blacklist should not be marked as blocked
    map->free_blacklist_blocked = false;

    bool should_abandon = false;
    for (auto &crumb : crumbs)
    {
        if (should_abandon)
            break;
        // A path Node is blacklisted, abandon pathing
        for (auto const &entry : map->free_blacklist)
        {
            if (entry.first == crumb.navarea)
                should_abandon = true;
        }
    }
    if (should_abandon)
        abandonPath();
}

void updateStuckTime()
{
    // No crumbs
    if (!crumbs.size())
    {
        antistuck_armed = false;
        return;
    }
    if (!antistuck_armed || crumbs.size() != antistuck_last_crumbs)
    {
        antistuck_anchor      = g_pLocalPlayer->v_Origin;
        antistuck_last_crumbs = crumbs.size();
        antistuck_window.update();
        antistuck_armed = true;
    }
    else if ((g_pLocalPlayer->v_Origin.AsVector2D() - antistuck_anchor.AsVector2D()).Length() > 50.0f)
    {
        antistuck_anchor = g_pLocalPlayer->v_Origin;
        antistuck_window.update();
    }
    else if (antistuck_window.check(*stuck_detect_time * 1000))
    {
        blacklistStuckConnectionAndRepath("no net progress");
        return;
    }

    if (detectCrumbPingPong())
    {
        blacklistStuckConnectionAndRepath("crumb ping-pong");
        return;
    }

    // We're stuck, add time to connection
    if (inactivity.check(*stuck_time / 2))
    {
        CNavArea *from = last_crumb.navarea ? last_crumb.navarea : crumbs[0].navarea;
        CNavArea *to   = crumbs[0].navarea;
        if (!from || !to)
        {
            if (inactivity.check(*stuck_time))
                abandonPath();
            return;
        }
        auto key = std::pair<CNavArea *, CNavArea *>(from, to);

        // Expires in 10 seconds
        map->connection_stuck_time[key].expire_tick = TICKCOUNT_TIMESTAMP(*stuck_expire_time);
        // Stuck for one tick
        map->connection_stuck_time[key].time_stuck += 1;

        // We are stuck for too long, blastlist node for a while and repath
        if (map->connection_stuck_time[key].time_stuck > TIME_TO_TICKS(*stuck_detect_time))
            blacklistStuckConnectionAndRepath("inactivity");
    }
}

#if ENABLE_VISUALS
static void updateDrawSnapshot();
#endif

static Vector teleport_check_origin{};
static bool have_teleport_check_origin = false;
static void checkTeleportReset()
{
    const Vector &origin = g_pLocalPlayer->v_Origin;
    if (!have_teleport_check_origin)
    {
        teleport_check_origin       = origin;
        have_teleport_check_origin  = true;
        return;
    }
    float jumped          = origin.DistTo(teleport_check_origin);
    teleport_check_origin = origin;
    if (jumped > 400.0f && isPathing())
    {
        if (log_pathing)
            logging::Info("Pathing: teleport detected (%.0f units), resetting path", jumped);
        cancelPath();
    }
}

static void CreateMove()
{
    ++navdebug.cm_calls;
    ensureMapLoaded();
#if ENABLE_VISUALS
    updateDrawSnapshot();
#endif
    const bool hard_unavailable = !g_IEngine->IsInGame() || !map || map->state != NavState::Active || (!enabled && !hacks::tf2::NavBot::isEnabled());
    if (hard_unavailable)
    {
        if (isPathing() || current_priority)
            cancelPath();
        return;
    }
    if (!isReady())
        return;
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
    {
        cancelPath();
        return;
    }

    checkTeleportReset();

    if (vischeck_runtime)
        vischeckPath();
    checkBlacklist();

    followCrumbs();
    updateStuckTime();
    map->updateIgnores();

    if (log_pathing)
    {
        static Timer status_timer{};
        if (status_timer.test_and_set(2000))
            logging::Info("NavDbg: ready=1 crumbs=%zu prio=%s(%d) areas=%zu conns=%zu iso=%zu bl=%zu vc=%zu fail='%s'", crumbs.size(), getPriorityName(current_priority), current_priority, navdebug.areas, navdebug.connections, navdebug.isolated_areas, map->free_blacklist.size(), map->vischeck_cache.size(), navdebug.navto_reason.c_str());
    }
}

void LevelInit()
{
    map_dirty = true;
    cancelPath();
    have_teleport_check_origin = false;
}

// Return the whole thing
std::unordered_map<CNavArea *, BlacklistReason> *getFreeBlacklist()
{
    return &map->free_blacklist;
}

// Return a specific category, we keep the same indexes to provide single element erasing
std::unordered_map<CNavArea *, BlacklistReason> getFreeBlacklist(BlacklistReason reason)
{
    std::unordered_map<CNavArea *, BlacklistReason> return_map;
    for (auto const &entry : map->free_blacklist)
    {
        // Category matches
        if (entry.second.value == reason.value)
            return_map[entry.first] = entry.second;
    }
    return return_map;
}

// Clear whole blacklist
void clearFreeBlacklist()
{
    map->free_blacklist.clear();
}

// Clear by category
void clearFreeBlacklist(BlacklistReason reason)
{
    for (auto it = begin(map->free_blacklist); it != end(map->free_blacklist);)
    {
        if (it->second.value == reason.value)
            it = map->free_blacklist.erase(it); // previously this was something like m_map.erase(it++);
        else
            ++it;
    }
}

static long long msSince(const Timer &t)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(Timer::clock::now() - t.last).count();
}

std::vector<std::string> getDebugInfoLines()
{
    std::vector<std::string> lines;
    lines.push_back(format("cm:", navdebug.cm_calls, " nav:", *enabled ? "1" : "0", "/", hacks::tf2::NavBot::isEnabled() ? "1" : "0", " ready:", navdebug.ready_reason.empty() ? "yes" : format("no(", navdebug.ready_reason, ")")));
    if (map)
        lines.push_back(format("map '", map->mapname, "' ", map->state == NavState::Active ? "Active" : "Unavailable", " ok:", map->navfile.m_isOK ? "1" : "0", " areas:", navdebug.areas, " conns:", navdebug.connections, " iso:", navdebug.isolated_areas, " reach:", map->reachable_areas.size()));
    else
        lines.push_back(format("map: not loaded level '", navdebug.level_name, "'"));
    lines.push_back(format("navfile: '", navdebug.nav_path, "'"));
    lines.push_back(format("ingame:", g_IEngine->IsInGame() ? "1" : "0", " rm:", (int) g_pGameRules->RoundMode(), " crumbs:", crumbs.size(), " prio:", getPriorityName(current_priority), "(", current_priority, ")"));
    if (!crumbs.empty())
    {
        auto &c0 = crumbs[0];
        std::string line = format("c0 #", c0.navarea ? (int) c0.navarea->m_id : -1, " (", (int) c0.vec.x, ",", (int) c0.vec.y, ",", (int) c0.vec.z, ") d:", (int) c0.vec.DistTo(g_pLocalPlayer->v_Origin), "/", (int) (c0.navarea ? 50.0f : 20.0f));
        if (crumbs.size() > 1)
            line += format(" | c1 #", crumbs[1].navarea ? (int) crumbs[1].navarea->m_id : -1, " d:", (int) crumbs[1].vec.DistTo(g_pLocalPlayer->v_Origin));
        line += format(" | last #", crumbs.back().navarea ? (int) crumbs.back().navarea->m_id : -1);
        lines.push_back(line);
    }
    lines.push_back(format("tmr inact:", msSince(inactivity), " crumb:", msSince(time_spent_on_crumb), " | dest (", (int) navdebug.navto_dest.x, ",", (int) navdebug.navto_dest.y, ",", (int) navdebug.navto_dest.z, ") rp:", repath_on_fail ? "1" : "0"));
    lines.push_back(format("navTo: ", navdebug.navto_reason.empty() ? "ok" : navdebug.navto_reason, " ok:", navdebug.navto_ok, " fail:", navdebug.navto_fail));
    lines.push_back(format("solve r:", navdebug.last_solve_result, " n:", navdebug.last_solve_nodes, " us:", navdebug.last_solve_ns / 1000, map ? format(" | bl:", map->free_blacklist.size(), map->free_blacklist_blocked ? "(B)" : "", " vc:", map->vischeck_cache.size(), " sc:", map->connection_stuck_time.size()) : ""));
    lines.push_back(format("edge p:", navdebug.edge_pass, " cok:", navdebug.edge_cachedok, " | rej sb:", navdebug.edge_selfbl, " fb:", navdebug.edge_freebl, " h:", navdebug.edge_height, " cb:", navdebug.edge_cachedbad, " rf:", navdebug.edge_rayfail));
    if (!navdebug.rayfail_detail.empty())
        lines.push_back(format("rf: ", navdebug.rayfail_detail));
    if (map && CE_GOOD(LOCAL_E))
    {
        auto *la = map->findClosestNavSquare(g_pLocalPlayer->v_Origin);
        lines.push_back(la ? format("area #", la->m_id, " a:", la->m_attributeFlags, " tf:", la->m_TFattributeFlags, " c:", la->m_connections.size(), " | ab:", navdebug.abandons, " cc:", navdebug.cancels)
                         : format("local area: NONE | ab:", navdebug.abandons, " cc:", navdebug.cancels));
    }
    else
        lines.push_back(format("ab:", navdebug.abandons, " cc:", navdebug.cancels));
    return lines;
}

void drawDebugInfo()
{
#if ENABLE_VISUALS
    AddSideString("--- NavEngine ---", colors::gui);
    std::lock_guard<std::mutex> lock(navdraw_mutex);
    for (auto &line : debug_lines_snapshot)
        AddSideString(line);
#endif
}

#if ENABLE_VISUALS
static void updateDrawSnapshot()
{
    std::lock_guard<std::mutex> lock(navdraw_mutex);
    debug_lines_snapshot = getDebugInfoLines();
    crumb_snapshot.clear();
    crumb_snapshot.reserve(crumbs.size());
    for (auto &c : crumbs)
        crumb_snapshot.push_back(c.vec);
    dbg_nav_ready  = isReady();
    dbg_area_valid = false;
    if (draw_debug_areas && dbg_nav_ready && CE_GOOD(LOCAL_E) && LOCAL_E->m_bAlivePlayer())
    {
        auto area = map->findClosestNavSquare(g_pLocalPlayer->v_Origin);
        if (area)
        {
            dbg_area_quad[0] = area->m_nwCorner;
            dbg_area_quad[1] = area->getNeCorner();
            dbg_area_quad[2] = area->getSwCorner();
            dbg_area_quad[3] = area->m_seCorner;
            dbg_area_edge    = area->getNearestPoint(g_pLocalPlayer->v_Origin.AsVector2D());
            dbg_area_edge.z += PLAYER_JUMP_HEIGHT;
            dbg_area_valid   = true;
        }
    }
}

void drawNavArea(const Vector quad[4])
{
    Vector nw, ne, sw, se;
    bool nw_screen = draw::WorldToScreen(quad[0], nw);
    bool ne_screen = draw::WorldToScreen(quad[1], ne);
    bool sw_screen = draw::WorldToScreen(quad[2], sw);
    bool se_screen = draw::WorldToScreen(quad[3], se);

    // Nw -> Ne
    if (nw_screen && ne_screen)
        draw::Line(nw.x, nw.y, ne.x - nw.x, ne.y - nw.y, colors::green, 1.0f);
    // Nw -> Sw
    if (nw_screen && sw_screen)
        draw::Line(nw.x, nw.y, sw.x - nw.x, sw.y - nw.y, colors::green, 1.0f);
    // Ne -> Se
    if (ne_screen && se_screen)
        draw::Line(ne.x, ne.y, se.x - ne.x, se.y - ne.y, colors::green, 1.0f);
    // Sw -> Se
    if (sw_screen && se_screen)
        draw::Line(sw.x, sw.y, se.x - sw.x, se.y - sw.y, colors::green, 1.0f);
}

void Draw()
{
    if (!draw)
        return;
    std::lock_guard<std::mutex> lock(navdraw_mutex);
    if (!dbg_nav_ready)
        return;
    if (dbg_area_valid)
    {
        Vector scrEdge;
        if (draw::WorldToScreen(dbg_area_edge, scrEdge))
            draw::Rectangle(scrEdge.x - 2.0f, scrEdge.y - 2.0f, 4.0f, 4.0f, colors::red);
        drawNavArea(dbg_area_quad);
    }

    for (size_t i = 0; i < crumb_snapshot.size(); i++)
    {
        Vector start_pos = crumb_snapshot[i];

        Vector start_screen, end_screen;
        if (draw::WorldToScreen(start_pos, start_screen))
        {
            draw::Rectangle(start_screen.x - 5.0f, start_screen.y - 5.0f, 10.0f, 10.0f, colors::white);

            if (i < crumb_snapshot.size() - 1)
            {
                Vector end_pos = crumb_snapshot[i + 1];
                if (draw::WorldToScreen(end_pos, end_screen))
                    draw::Line(start_screen.x, start_screen.y, end_screen.x - start_screen.x, end_screen.y - start_screen.y, colors::white, 2.0f);
            }
        }
    }
}
#endif

class CLocalPlayerRespawn : public IGameEventListener
{
public:
    void FireGameEvent(KeyValues *event) override
    {
        if (map)
            map->pather.Reset();
    }
};

CLocalPlayerRespawn &listener()
{
    static CLocalPlayerRespawn l{};
    return l;
}
}

Vector loc;

static CatCommand nav_set("nav_set", "Debug nav find", []() { loc = g_pLocalPlayer->v_Origin; });

static CatCommand nav_path("nav_path", "Debug nav path", []() { NavEngine::navTo(loc, 20, true, true, false); });

static CatCommand nav_path_noreapth("nav_path_norepath", "Debug nav path", []() { NavEngine::navTo(loc, 20, false, true, false); });

static CatCommand nav_debug_dump("nav_debug_dump", "Dump navengine/navbot debug state to the log",
                                 []()
                                 {
                                     for (auto &line : NavEngine::getDebugInfoLines())
                                         logging::Info("navdbg: %s", line.c_str());
                                     for (auto &line : hacks::tf2::NavBot::getDebugInfoLines())
                                         logging::Info("navdbg: %s", line.c_str());
                                 });

static CatCommand nav_init("nav_init", "Reload nav mesh",
                           []()
                           {
                               NavEngine::map.reset();
                               NavEngine::LevelInit();
                           });

static CatCommand nav_debug_check("nav_debug_check", "Perform nav checks between two areas. First area: cat_nav_set Second area: Your location while running this command.",
                                  []()
                                  {
                                      if (!NavEngine::isReady())
                                          return;
                                      auto next    = NavEngine::map->findClosestNavSquare(g_pLocalPlayer->v_Origin);
                                      auto current = NavEngine::map->findClosestNavSquare(loc);

                                      auto points = determinePoints(current, next);

                                      points.center = handleDropdown(points.center, points.next);

                                      // Too high for us to jump!
                                      if (points.center_next.z - points.center.z > PLAYER_JUMP_HEIGHT)
                                      {
                                          return logging::Info("Nav: Area too high!");
                                      }

                                      points.current.z += PLAYER_JUMP_HEIGHT;
                                      points.center.z += PLAYER_JUMP_HEIGHT;
                                      points.next.z += PLAYER_JUMP_HEIGHT;

                                      if (IsPlayerPassableNavigation(points.current, points.center) && IsPlayerPassableNavigation(points.center, points.next))
                                      {
                                          logging::Info("Nav: Area is player passable!");
                                      }
                                      else
                                      {
                                          logging::Info("Nav: Area is NOT player passable! %.2f,%.2f,%.2f %.2f,%.2f,%.2f %.2f,%.2f,%.2f", points.current.x, points.current.y, points.current.z, points.center.x, points.center.y, points.center.z, points.next.x, points.next.y, points.next.z);
                                      }
                                  });

static CatCommand nav_debug_blacklist("nav_debug_blacklist", "Blacklist connection between two areas for 30s. First area: cat_nav_set Second area: Your location while running this command.",
                                      []()
                                      {
                                          if (!NavEngine::isReady())
                                              return;
                                          auto next    = NavEngine::map->findClosestNavSquare(g_pLocalPlayer->v_Origin);
                                          auto current = NavEngine::map->findClosestNavSquare(loc);

                                          std::pair<CNavArea *, CNavArea *> key(current, next);
                                          NavEngine::map->vischeck_cache[key].expire_tick    = TICKCOUNT_TIMESTAMP(30);
                                          NavEngine::map->vischeck_cache[key].vischeck_state = false;
                                          NavEngine::map->vischeck_cache[key].stuck          = true;
                                          NavEngine::map->pather.Reset();
                                          logging::Info("Nav: Connection %d->%d Blacklisted.", current->m_id, next->m_id);
                                      });

static InitRoutine init(
    []()
    {
        g_IGameEventManager->AddListener(&NavEngine::listener(), "localplayer_respawn", false);
        EC::Register(
            EC::Shutdown, []() { g_IGameEventManager->RemoveListener(&NavEngine::listener()); }, "navengine_shutdown");
        EC::Register(EC::CreateMove_NoEnginePred, NavEngine::CreateMove, "navengine_cm");
        EC::Register(EC::LevelInit, NavEngine::LevelInit, "navengine_levelinit");
#if ENABLE_VISUALS
        EC::Register(EC::Draw, NavEngine::Draw, "navengine_draw");
#endif
        enabled.installChangeCallback(
            [](settings::VariableBase<bool> &, bool after)
            {
                if (after && g_IEngine->IsInGame())
                    NavEngine::LevelInit();
            });
    });

} // namespace navparser
