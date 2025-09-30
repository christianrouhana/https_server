#ifndef OSRS_HISCORE_H
#define OSRS_HISCORE_H

#include <cstdint>
#include <map>
#include <string>

namespace osrs {

struct SkillStats {
    int rank = -1;
    int level = 0;
    std::int64_t experience = 0;
};

struct PlayerSnapshot {
    std::string name;
    bool success = false;
    std::string error;
    std::map<std::string, SkillStats> skills;
};

class HiscoreClient {
public:
    explicit HiscoreClient(std::string caCertPath);

    PlayerSnapshot fetchPlayer(const std::string &playerName) const;

private:
    static std::string urlEncode(const std::string &value);
    static PlayerSnapshot parseBody(const std::string &playerName, const std::string &body);

    std::string m_caCertPath;
};

std::string ToJson(const PlayerSnapshot &snapshot);

} // namespace osrs

#endif // OSRS_HISCORE_H
