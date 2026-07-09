#include "progress.h"
#include "storage.h"
#include <string>
#include <sstream>

Progress loadProgress() {
    Progress p;
    std::string data = storageRead("progress");
    if (data.empty()) return p;

    std::istringstream in(data);
    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (val.empty()) continue;
        if (key == "done") {
            p.done = (unsigned long long)std::stoull(val);
        } else if (key == "seen") {
            p.seenRules = (std::stoi(val) != 0);
        } else if (key == "seenW") {
            p.seenWalls = (std::stoi(val) != 0);
        } else if (key == "seenP") {
            p.seenPortals = (std::stoi(val) != 0);
        } else if (key == "seenO") {
            p.seenDailyOps = (std::stoi(val) != 0);
        } else if (key == "dLast") {
            p.dailyLastDay = std::stol(val);
        } else if (key == "dStreak") {
            p.dailyStreak = std::stoi(val);
        } else if (key == "dBest") {
            p.dailyBest = std::stoi(val);
        }
    }
    return p;
}

void saveProgress(const Progress& p) {
    std::ostringstream out;
    out << "done=" << p.done << "\n";
    out << "seen=" << (p.seenRules ? 1 : 0) << "\n";
    out << "seenW=" << (p.seenWalls ? 1 : 0) << "\n";
    out << "seenP=" << (p.seenPortals ? 1 : 0) << "\n";
    out << "seenO=" << (p.seenDailyOps ? 1 : 0) << "\n";
    out << "dLast=" << p.dailyLastDay << "\n";
    out << "dStreak=" << p.dailyStreak << "\n";
    out << "dBest=" << p.dailyBest << "\n";
    storageWrite("progress", out.str());
}

int firstIncomplete(const Progress& p, int count) {
    for (int i = 0; i < count; i++) {
        if (!levelDone(p, i)) return i;
    }
    return count;
}
