#include "osrs_hiscore.h"
#include "https_client.h"

#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
    constexpr const char *kHost = "secure.runescape.com";
    constexpr int kPort = 443;
    constexpr const char *kEndpoint = "/m=hiscore_oldschool/index_lite.ws";

    const std::array<const char *, 24> kSkillOrder = {
        "Overall",     "Attack",      "Defence",    "Strength",   "Hitpoints",
        "Ranged",      "Prayer",      "Magic",      "Cooking",    "Woodcutting",
        "Fletching",   "Fishing",     "Firemaking", "Crafting",   "Smithing",
        "Mining",      "Herblore",    "Agility",    "Thieving",   "Slayer",
        "Farming",     "Runecraft",   "Hunter",     "Construction"};

    std::string escapeJson(const std::string &value)
    {
        std::ostringstream oss;
        for (char ch : value)
        {
            switch (ch)
            {
            case '\\':
                oss << "\\\\";
                break;
            case '"':
                oss << "\\\"";
                break;
            case '\b':
                oss << "\\b";
                break;
            case '\f':
                oss << "\\f";
                break;
            case '\n':
                oss << "\\n";
                break;
            case '\r':
                oss << "\\r";
                break;
            case '\t':
                oss << "\\t";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20)
                {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(ch))
                        << std::dec << std::setfill(' ');
                }
                else
                {
                    oss << ch;
                }
            }
        }
        return oss.str();
    }
}

namespace osrs {

HiscoreClient::HiscoreClient(std::string caCertPath) : m_caCertPath(std::move(caCertPath)) {}

PlayerSnapshot HiscoreClient::fetchPlayer(const std::string &playerName) const
{
    PlayerSnapshot snapshot;
    snapshot.name = playerName;

    if (playerName.empty())
    {
        snapshot.error = "Player name is empty";
        return snapshot;
    }

    if (m_caCertPath.empty())
    {
        snapshot.error = "CA certificate path is not configured";
        return snapshot;
    }

    https::HttpsClient client(kHost, kPort, m_caCertPath);
    if (!client.connectToServer())
    {
        snapshot.error = "Unable to connect to hiscore service";
        return snapshot;
    }

    std::ostringstream request;
    request << "GET " << kEndpoint << "?player=" << urlEncode(playerName)
            << " HTTP/1.1\r\n";
    request << "Host: " << kHost << "\r\n";
    request << "User-Agent: OSRS-Hiscore-Client/0.1\r\n";
    request << "Connection: close\r\n\r\n";

    if (!client.sendRequest(request.str()))
    {
        snapshot.error = "Failed to issue hiscore request";
        return snapshot;
    }

    std::string response = client.receiveResponse();
    if (response.empty())
    {
        snapshot.error = "Empty response from hiscore service";
        return snapshot;
    }

    const std::string separator = "\r\n\r\n";
    const auto headerEnd = response.find(separator);
    if (headerEnd == std::string::npos)
    {
        snapshot.error = "Malformed HTTP response";
        return snapshot;
    }

    std::string header = response.substr(0, headerEnd);
    std::string body = response.substr(headerEnd + separator.size());

    std::istringstream headerStream(header);
    std::string httpVersion;
    int statusCode = 0;
    headerStream >> httpVersion >> statusCode;
    if (!headerStream || statusCode == 0)
    {
        snapshot.error = "Failed to parse hiscore status";
        return snapshot;
    }

    if (statusCode == 404)
    {
        snapshot.error = "Player not found";
        return snapshot;
    }

    if (statusCode != 200)
    {
        snapshot.error = "Hiscore service returned status " + std::to_string(statusCode);
        return snapshot;
    }

    snapshot = parseBody(playerName, body);
    if (!snapshot.success && snapshot.error.empty())
    {
        snapshot.error = "Unable to parse hiscore payload";
    }

    return snapshot;
}

std::string HiscoreClient::urlEncode(const std::string &value)
{
    std::ostringstream encoded;
    encoded << std::hex << std::uppercase;

    for (unsigned char c : value)
    {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded << static_cast<char>(c);
        }
        else if (c == ' ')
        {
            encoded << "%20";
        }
        else
        {
            encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c)
                    << std::setfill(' ');
        }
    }

    encoded << std::dec << std::nouppercase;
    return encoded.str();
}

PlayerSnapshot HiscoreClient::parseBody(const std::string &playerName, const std::string &body)
{
    PlayerSnapshot snapshot;
    snapshot.name = playerName;

    std::istringstream stream(body);
    std::string line;
    bool parsedAny = false;

    std::size_t skillIndex = 0;
    while (skillIndex < kSkillOrder.size() && std::getline(stream, line))
    {
        if (line.empty())
        {
            continue; // skip blank lines without advancing the skill index
        }

        std::istringstream lineStream(line);
        std::string rankToken;
        std::string levelToken;
        std::string xpToken;

        if (!std::getline(lineStream, rankToken, ',') || !std::getline(lineStream, levelToken, ',') ||
            !std::getline(lineStream, xpToken, ','))
        {
            continue; // malformed line, try the next one without advancing the index
        }

        try
        {
            SkillStats stats;
            stats.rank = std::stoi(rankToken);
            stats.level = std::stoi(levelToken);
            stats.experience = std::stoll(xpToken);
            snapshot.skills.emplace(kSkillOrder[skillIndex], stats);
            parsedAny = true;
        }
        catch (const std::exception &)
        {
            // Parsing failed; ignore this line and look for the next valid one.
            continue;
        }

        ++skillIndex;
    }

    snapshot.success = parsedAny;
    if (!parsedAny)
    {
        snapshot.error = "No skill data available";
    }

    return snapshot;
}

std::string ToJson(const PlayerSnapshot &snapshot)
{
    std::ostringstream json;
    json << "{";
    json << "\"name\":\"" << escapeJson(snapshot.name) << "\",";
    json << "\"success\":" << (snapshot.success ? "true" : "false");

    if (!snapshot.success && !snapshot.error.empty())
    {
        json << ",\"error\":\"" << escapeJson(snapshot.error) << "\"";
    }

    if (snapshot.success)
    {
        json << ",\"skills\":{";
        std::size_t index = 0;
        for (const auto &entry : snapshot.skills)
        {
            if (index++ > 0)
            {
                json << ',';
            }
            json << "\"" << escapeJson(entry.first) << "\":{";
            json << "\"rank\":" << entry.second.rank << ',';
            json << "\"level\":" << entry.second.level << ',';
            json << "\"experience\":" << entry.second.experience;
            json << "}";
        }
        json << "}";
    }

    json << "}";
    return json.str();
}

} // namespace osrs
