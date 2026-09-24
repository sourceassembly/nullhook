#include "common.hpp"
#include "discord.hpp"
#include "init.hpp"

#include <hacks/Spam.hpp>
#include <settings/Bool.hpp>
#include <settings/String.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <dlfcn.h>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace discord
{
static settings::Boolean enable{ "discord.enable", "false" };
static settings::Boolean relay_chat{ "discord.chat", "true" };
static settings::Boolean log_reports{ "discord.reports", "true" };
static settings::String chat_webhook{ "discord.chat.webhook", "" };
static settings::String report_webhook{ "discord.reports.webhook", "" };
static settings::Boolean no_ipc{ "discord.chat.no-ipc", "true" };
static settings::Boolean no_spam{ "discord.chat.no-spam", "true" };

// SteamID64 = universe public + individual + account id. Stable protocol constant.
static constexpr uint64_t k_steamid64_base = 76561197960265728ULL;

enum
{
    CURLOPT_WRITEDATA       = 10001,
    CURLOPT_URL             = 10002,
    CURLOPT_POSTFIELDS      = 10015,
    CURLOPT_USERAGENT       = 10018,
    CURLOPT_HTTPHEADER      = 10023,
    CURLOPT_CUSTOMREQUEST   = 10036,
    CURLOPT_TIMEOUT         = 13,
    CURLOPT_FOLLOWLOCATION  = 52,
    CURLOPT_POSTFIELDSIZE   = 60,
    CURLOPT_WRITEFUNCTION   = 20011,
};

struct CurlApi
{
    void *lib{};
    void *(*easy_init)(){};
    int (*easy_setopt)(void *, int, ...){};
    int (*easy_perform)(void *){};
    void (*easy_cleanup)(void *){};
    void *(*slist_append)(void *, const char *){};
    void (*slist_free_all)(void *){};
    const char *(*easy_strerror)(int){};
};

static CurlApi curl;

static bool load_curl()
{
    if (curl.lib)
        return curl.easy_init && curl.easy_setopt && curl.easy_perform && curl.easy_cleanup;
    curl.lib = dlopen("libcurl.so.4", RTLD_NOW | RTLD_LOCAL);
    if (!curl.lib)
        curl.lib = dlopen("libcurl.so", RTLD_NOW | RTLD_LOCAL);
    if (!curl.lib)
    {
        logging::Info("discord: libcurl not found (need libcurl.so.4)");
        return false;
    }
    auto sym = [](const char *name) { return dlsym(curl.lib, name); };
    curl.easy_init      = reinterpret_cast<void *(*)()>(sym("curl_easy_init"));
    curl.easy_setopt    = reinterpret_cast<int (*)(void *, int, ...)>(sym("curl_easy_setopt"));
    curl.easy_perform   = reinterpret_cast<int (*)(void *)>(sym("curl_easy_perform"));
    curl.easy_cleanup   = reinterpret_cast<void (*)(void *)>(sym("curl_easy_cleanup"));
    curl.slist_append   = reinterpret_cast<void *(*)(void *, const char *)>(sym("curl_slist_append"));
    curl.slist_free_all = reinterpret_cast<void (*)(void *)>(sym("curl_slist_free_all"));
    curl.easy_strerror  = reinterpret_cast<const char *(*)(int)>(sym("curl_easy_strerror"));
    if (!curl.easy_init || !curl.easy_setopt || !curl.easy_perform || !curl.easy_cleanup)
    {
        logging::Info("discord: libcurl missing symbols");
        return false;
    }
    return true;
}

extern "C" {
static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto *out = static_cast<std::string *>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}
}

static bool http_request(const char *method, const std::string &url, const std::string &body, std::string *response)
{
    if (!load_curl())
        return false;
    void *easy = curl.easy_init();
    if (!easy)
        return false;

    std::string resp;
    curl.easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl.easy_setopt(easy, CURLOPT_WRITEFUNCTION, &write_cb);
    curl.easy_setopt(easy, CURLOPT_WRITEDATA, &resp);
    curl.easy_setopt(easy, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");
    curl.easy_setopt(easy, CURLOPT_TIMEOUT, 15L);
    curl.easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);

    void *headers = nullptr;
    if (body.size() || !strcmp(method, "POST"))
    {
        if (curl.slist_append)
            headers = curl.slist_append(nullptr, "Content-Type: application/json");
        if (headers)
            curl.easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
        curl.easy_setopt(easy, CURLOPT_POSTFIELDS, body.c_str());
        curl.easy_setopt(easy, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
        curl.easy_setopt(easy, CURLOPT_CUSTOMREQUEST, method);
    }

    int rc = curl.easy_perform(easy);
    if (headers && curl.slist_free_all)
        curl.slist_free_all(headers);
    curl.easy_cleanup(easy);
    if (response)
        *response = std::move(resp);
    if (rc != 0)
    {
        logging::Info("discord: curl %s", curl.easy_strerror ? curl.easy_strerror(rc) : "error");
        return false;
    }
    return true;
}

static std::string json_escape(const std::string &in)
{
    std::string out;
    out.reserve(in.size() + 8);
    for (unsigned char c : in)
    {
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 0x20)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            }
            else
                out.push_back(static_cast<char>(c));
            break;
        }
    }
    return out;
}

static std::string xml_text(const std::string &xml, const char *tag)
{
    std::string open  = std::string("<") + tag + ">";
    std::string close = std::string("</") + tag + ">";
    auto s            = xml.find(open);
    if (s == std::string::npos)
        return {};
    s += open.size();
    auto e = xml.find(close, s);
    if (e == std::string::npos)
        return {};
    std::string v = xml.substr(s, e - s);
    if (v.size() >= 12 && !v.compare(0, 9, "<![CDATA["))
    {
        auto end = v.find("]]>");
        if (end != std::string::npos)
            v = v.substr(9, end - 9);
    }
    auto repl = [&](const char *from, const char *to)
    {
        size_t pos = 0;
        while ((pos = v.find(from, pos)) != std::string::npos)
        {
            v.replace(pos, strlen(from), to);
            pos += strlen(to);
        }
    };
    repl("&amp;", "&");
    repl("&lt;", "<");
    repl("&gt;", ">");
    repl("&quot;", "\"");
    repl("&apos;", "'");
    return v;
}

static std::string fix_avatar_url(std::string url)
{
    if (url.rfind("http://", 0) == 0)
        url.replace(0, 4, "https");
    auto repl_prefix = [&](const char *from, const char *to)
    {
        if (url.find(from) == 0)
            url.replace(0, strlen(from), to);
    };
    repl_prefix("https://steamcdn-a.akamaihd.net/steamcommunity/public/images/avatars/", "https://avatars.steamstatic.com/");
    repl_prefix("https://cdn.akamai.steamstatic.com/steamcommunity/public/images/avatars/", "https://avatars.steamstatic.com/");
    repl_prefix("https://avatars.akamai.steamstatic.com/", "https://avatars.steamstatic.com/");
    return url;
}

struct SteamProfile
{
    std::string name;
    std::string avatar;
};

static std::unordered_map<unsigned, SteamProfile> profile_cache;

static SteamProfile fetch_profile(unsigned friendsID, const std::string &fallback_name)
{
    auto it = profile_cache.find(friendsID);
    if (it != profile_cache.end())
        return it->second;

    SteamProfile p;
    p.name = fallback_name;
    uint64_t sid = k_steamid64_base + friendsID;
    std::string url = "https://steamcommunity.com/profiles/" + std::to_string(sid) + "?xml=1";
    std::string xml;
    if (http_request("GET", url, {}, &xml) && xml.find("<profile>") != std::string::npos)
    {
        auto steam_name = xml_text(xml, "steamID");
        auto avatar     = xml_text(xml, "avatarFull");
        if (steam_name.empty())
            steam_name = xml_text(xml, "steamID64");
        if (!steam_name.empty())
            p.name = steam_name;
        if (!avatar.empty())
            p.avatar = fix_avatar_url(avatar);
    }
    profile_cache.emplace(friendsID, p);
    return p;
}

static bool webhook_ok(const std::string &url)
{
    return url.rfind("https://", 0) == 0 && url.find("discord") != std::string::npos && url.find("/api/webhooks/") != std::string::npos;
}

static std::string pick_webhook(bool reports)
{
    const std::string &primary  = reports ? *report_webhook : *chat_webhook;
    const std::string &fallback = reports ? *chat_webhook : *report_webhook;
    if (webhook_ok(primary))
        return primary;
    if (webhook_ok(fallback))
        return fallback;
    return {};
}

static std::string webhook_username(std::string name, int team)
{
    for (char &c : name)
        if (static_cast<unsigned char>(c) < 0x20 || c == '`')
            c = ' ';
    while (!name.empty() && name.front() == ' ')
        name.erase(name.begin());
    while (!name.empty() && name.back() == ' ')
        name.pop_back();
    if (team == TEAM_RED)
        name = "🔴 " + name;
    else if (team == TEAM_BLU)
        name = "🔵 " + name;
    if (name.size() > 80)
        name.resize(80);
    if (name.empty())
        name = "TF2 player";
    return name;
}

static void neutralize_mentions(std::string &s)
{
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '@')
        {
            s.insert(i + 1, "\u200b");
            i++;
        }
    }
}

static std::string current_map()
{
    if (!g_IEngine)
        return {};
    const char *lvl = g_IEngine->GetLevelName();
    if (!lvl || !*lvl)
        return {};
    std::string s(lvl);
    if (s.rfind("maps/", 0) == 0)
        s.erase(0, 5);
    if (s.size() > 4 && s.compare(s.size() - 4, 4, ".bsp") == 0)
        s.erase(s.size() - 4);
    return s;
}

static std::string local_player_name()
{
    player_info_s info{};
    if (g_IEngine && GetPlayerInfo(g_IEngine->GetLocalPlayer(), &info) && info.name[0])
        return info.name;
    if (g_ISteamFriends)
    {
        const char *n = g_ISteamFriends->GetPersonaName();
        if (n && *n)
            return n;
    }
    return {};
}

enum class Kind
{
    Chat,
    Report
};

struct Job
{
    Kind kind{};
    unsigned friendsID{};
    std::string name;
    std::string message;
    std::string webhook;
    std::string reporter;
    std::string map;
    bool team_chat{};
    int team{};
};

static std::mutex mu;
static std::condition_variable cv;
static std::deque<Job> jobs;
static std::atomic<bool> running{ false };
static std::thread worker;

static void post_chat(const Job &job, const SteamProfile &profile)
{
    std::string name = webhook_username(job.name.empty() ? profile.name : job.name, job.team);
    std::string content = job.message;
    neutralize_mentions(content);
    if (job.team_chat)
        content = "*(team)* " + content;
    if (content.size() > 1900)
        content.resize(1900);

    std::string body = "{\"username\":\"" + json_escape(name) + "\"";
    if (!profile.avatar.empty())
        body += ",\"avatar_url\":\"" + json_escape(profile.avatar) + "\"";
    body += ",\"content\":\"" + json_escape(content) + "\"";
    body += ",\"allowed_mentions\":{\"parse\":[]}}";
    http_request("POST", job.webhook, body, nullptr);
}

static void post_report(const Job &job, const SteamProfile &profile)
{
    uint64_t sid           = k_steamid64_base + job.friendsID;
    std::string sid64      = std::to_string(sid);
    std::string profile_url = "https://steamcommunity.com/profiles/" + sid64;
    std::string display     = profile.name.empty() ? job.name : profile.name;
    if (display.empty())
        display = sid64;

    std::string desc = "[**Steam profile**](" + profile_url + ")";
    if (!job.name.empty() && job.name != display)
        desc += "\\nIn-game: **" + json_escape(job.name) + "**";

    std::string body = "{\"allowed_mentions\":{\"parse\":[]},\"embeds\":[{";
    body += "\"title\":\"Reported " + json_escape(display) + "\"";
    body += ",\"url\":\"" + json_escape(profile_url) + "\"";
    body += ",\"color\":15158332";
    body += ",\"description\":\"" + desc + "\"";
    body += ",\"author\":{\"name\":\"" + json_escape(display) + "\",\"url\":\"" + json_escape(profile_url) + "\"";
    if (!profile.avatar.empty())
        body += ",\"icon_url\":\"" + json_escape(profile.avatar) + "\"";
    body += "}";
    if (!profile.avatar.empty())
        body += ",\"thumbnail\":{\"url\":\"" + json_escape(profile.avatar) + "\"}";
    body += ",\"fields\":[";
    body += "{\"name\":\"SteamID64\",\"value\":\"`" + sid64 + "`\",\"inline\":true},";
    body += "{\"name\":\"SteamID3\",\"value\":\"`[U:1:" + std::to_string(job.friendsID) + "]`\",\"inline\":true}";
    body += "]";
    std::string footer = "Reported by " + (job.reporter.empty() ? std::string("unknown") : job.reporter);
    if (!job.map.empty())
        footer += " on " + job.map;
    body += ",\"footer\":{\"text\":\"" + json_escape(footer) + "\"}";
    body += "}]}";
    http_request("POST", job.webhook, body, nullptr);
}

static void worker_loop()
{
    if (!load_curl())
        return;

    auto last_post = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    while (running)
    {
        Job job;
        {
            std::unique_lock<std::mutex> lock(mu);
            cv.wait(lock, [] { return !jobs.empty() || !running; });
            if (!running && jobs.empty())
                break;
            job = std::move(jobs.front());
            jobs.pop_front();
        }

        SteamProfile profile = fetch_profile(job.friendsID, job.name);
        auto now = std::chrono::steady_clock::now();
        auto wait = std::chrono::milliseconds(800) - (now - last_post);
        if (wait > std::chrono::milliseconds(0))
            std::this_thread::sleep_for(wait);
        if (job.kind == Kind::Chat)
            post_chat(job, profile);
        else
            post_report(job, profile);
        last_post = std::chrono::steady_clock::now();
    }
}

static void ensure_worker()
{
    static std::once_flag once;
    std::call_once(once,
                   []
                   {
                       running = true;
                       worker  = std::thread(worker_loop);
                   });
}

static void enqueue(Job job)
{
    if (job.webhook.empty() || !webhook_ok(job.webhook))
        return;
    ensure_worker();
    {
        std::lock_guard<std::mutex> lock(mu);
        if (jobs.size() > 256)
        {
            if (job.kind == Kind::Chat)
                return;
            jobs.pop_front();
        }
        jobs.push_back(std::move(job));
    }
    cv.notify_one();
}

void LogChat(int entity_idx, const std::string &message, bool team_chat)
{
    if (!enable || !relay_chat)
        return;
    if (message.empty())
        return;
    if (no_spam && hacks::shared::spam::isActive() && entity_idx == g_IEngine->GetLocalPlayer())
        return;

    player_info_s info{};
    if (!GetPlayerInfo(entity_idx, &info) || !info.friendsID)
        return;
    if (no_ipc && (playerlist::AccessData(info.friendsID).state == playerlist::k_EState::IPC || playerlist::AccessData(info.friendsID).state == playerlist::k_EState::CAT))
        return;

    Job job;
    job.kind       = Kind::Chat;
    job.friendsID  = info.friendsID;
    job.name       = info.name;
    job.message    = message;
    job.webhook    = pick_webhook(false);
    job.team_chat  = team_chat;
    job.team       = g_pPlayerResource ? g_pPlayerResource->GetTeam(entity_idx) : 0;
    enqueue(std::move(job));
}

void LogReport(unsigned friendsID, const std::string &ingame_name)
{
    if (!enable || !log_reports || !friendsID)
        return;
    Job job;
    job.kind      = Kind::Report;
    job.friendsID = friendsID;
    job.name      = ingame_name;
    job.webhook   = pick_webhook(true);
    job.reporter  = local_player_name();
    job.map       = current_map();
    enqueue(std::move(job));
}

static CatCommand discord_test("discord_test", "Send a test Discord webhook message",
                               []()
                               {
                                   if (!enable)
                                   {
                                       logging::Info("discord: enable discord.enable first");
                                       return;
                                   }
                                   unsigned self = g_ISteamUser ? g_ISteamUser->GetSteamID().GetAccountID() : 0;
                                   if (relay_chat && webhook_ok(pick_webhook(false)))
                                   {
                                       Job job;
                                       job.kind      = Kind::Chat;
                                       job.friendsID = self;
                                       job.name      = local_player_name();
                                       job.message   = "cathook discord chat relay test";
                                       job.webhook   = pick_webhook(false);
                                       job.team      = g_pLocalPlayer ? g_pLocalPlayer->team : 0;
                                       enqueue(std::move(job));
                                   }
                                   if (log_reports && webhook_ok(pick_webhook(true)))
                                       LogReport(self, local_player_name());
                                   logging::Info("discord: queued test message");
                               });

static InitRoutine init([]() {
    EC::Register(
        EC::Shutdown,
        []()
        {
            running = false;
            cv.notify_all();
            if (worker.joinable())
                worker.join();
        },
        "shutdown_discord");
});
} // namespace discord
