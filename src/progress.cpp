#include "progress.h"
#include <cstdlib>
#include <fstream>
#include <string>
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

static const char* APP_FOLDER = "Hexactly";

static void makeDir(const std::string& p) {
#if defined(_WIN32)
    _mkdir(p.c_str());
#else
    mkdir(p.c_str(), 0755);
#endif
}

static void ensureDir(const std::string& path) {
    for (size_t i = 1; i < path.size(); i++)
        if (path[i] == '/') makeDir(path.substr(0, i));
    makeDir(path);
}

static std::string baseDataDir() {
#if defined(_WIN32)
    const char* appdata = std::getenv("APPDATA");
    return appdata ? std::string(appdata) : std::string(".");
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    return home ? std::string(home) + "/Library/Application Support" : std::string(".");
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg) return std::string(xdg);
    const char* home = std::getenv("HOME");
    return home ? std::string(home) + "/.local/share" : std::string(".");
#endif
}

static std::string savePath() {
    std::string dir = baseDataDir() + "/" + APP_FOLDER;
    ensureDir(dir);
    return dir + "/progress.txt";
}

Progress loadProgress() {
    Progress p;
    std::ifstream in(savePath());
    if (!in) return p;
    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        if (key == "done") {
            p.done = (unsigned long long)std::stoull(line.substr(eq + 1));
        } else if (key == "seen") {
            p.seenRules = (std::stoi(line.substr(eq + 1)) != 0);
        }
    }
    return p;
}

void saveProgress(const Progress& p) {
    std::ofstream out(savePath());
    if (!out) return;
    out << "done=" << p.done << "\n";
    out << "seen=" << (p.seenRules ? 1 : 0) << "\n";
}

int firstIncomplete(const Progress& p, int count) {
    for (int i = 0; i < count; i++) {
        if (!levelDone(p, i)) return i;
    }
    return count;
}
