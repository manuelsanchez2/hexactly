#include "storage.h"

#if defined(PLATFORM_WEB)

#include <emscripten.h>
#include <cstdlib>

EM_JS(char*, jsStorageGet, (const char* key), {
    var v = localStorage.getItem("hexactly:" + UTF8ToString(key));
    if (v === null) return 0;
    var len = lengthBytesUTF8(v) + 1;
    var buf = _malloc(len);
    stringToUTF8(v, buf, len);
    return buf;
});

EM_JS(void, jsStorageSet, (const char* key, const char* val), {
    try { localStorage.setItem("hexactly:" + UTF8ToString(key), UTF8ToString(val)); } catch (e) {}
});

std::string storageRead(const std::string& key) {
    char* p = jsStorageGet(key.c_str());
    if (!p) return "";
    std::string s(p);
    free(p);
    return s;
}

void storageWrite(const std::string& key, const std::string& text) {
    jsStorageSet(key.c_str(), text.c_str());
}

#else

#include <cstdlib>
#include <fstream>
#include <sstream>
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

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

static void makeDir(const std::string& p) {
#if defined(_WIN32)
    _mkdir(p.c_str());
#else
    mkdir(p.c_str(), 0755);
#endif
}

static std::string filePath(const std::string& key) {
    std::string dir = baseDataDir() + "/Hexactly";
    for (size_t i = 1; i < dir.size(); i++)
        if (dir[i] == '/') makeDir(dir.substr(0, i));
    makeDir(dir);
    return dir + "/" + key + ".txt";
}

std::string storageRead(const std::string& key) {
    std::ifstream in(filePath(key));
    if (!in) return "";
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void storageWrite(const std::string& key, const std::string& text) {
    std::ofstream out(filePath(key));
    if (!out) return;
    out << text;
}

#endif
