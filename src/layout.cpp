#include "layout.h"
#include "config.h"
#include "gamefont.h"
#include <fstream>
#include <sstream>

static std::string pathFor(const std::string& name) {
#if defined(HEX_DEV)
    return "src/resources/layouts/" + name + ".layout";
#else
    return "resources/layouts/" + name + ".layout";
#endif
}

Color layoutColor(const std::string& name) {
    if (name == "paper")  return PAPER;
    if (name == "hexred") return HEXRED;
    if (name == "gray")   return (Color){ 120, 120, 120, 255 };
    return INK;
}

static std::string readKey(const std::string& pairs, const std::string& key) {
    std::string needle = key + "=";
    size_t pos = pairs.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    size_t end = pairs.find(' ', pos);
    return pairs.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

static float readFloat(const std::string& pairs, const std::string& key, float def) {
    std::string v = readKey(pairs, key);
    return v.empty() ? def : std::stof(v);
}

static LayoutElement parseLine(const std::string& line) {
    LayoutElement e;

    std::istringstream head(line);
    std::string type;
    head >> type;
    e.type = (type == "button") ? ElemType::Button : ElemType::Label;

    std::string rest = line.substr(type.size());
    std::string pairs = rest;
    size_t tpos = rest.find("text=");
    if (tpos != std::string::npos) {
        e.text = rest.substr(tpos + 5);
        pairs  = rest.substr(0, tpos);
    }

    e.id    = readKey(pairs, "id");
    e.x     = readFloat(pairs, "x", 0);
    e.y     = readFloat(pairs, "y", 0);
    e.w     = readFloat(pairs, "w", 0);
    e.h     = readFloat(pairs, "h", 0);
    e.size  = readFloat(pairs, "size", 24);
    e.align = (readKey(pairs, "align") == "center") ? Align::Center : Align::Left;
    std::string f = readKey(pairs, "font");
    if (!f.empty()) e.font = f;
    std::string c = readKey(pairs, "color");
    if (!c.empty()) e.color = c;
    return e;
}

Layout loadLayout(const std::string& name) {
    Layout layout;
    layout.name = name;

    std::ifstream in(pathFor(name));
    if (!in) {
        TraceLog(LOG_WARNING, "LAYOUT: could not open %s", pathFor(name).c_str());
        return layout;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos || line[first] == '#') continue;
        layout.elements.push_back(parseLine(line));
    }
    return layout;
}

static std::string num(float v) {
    if (v == (long)v) return std::to_string((long)v);
    std::ostringstream os; os << v; return os.str();
}

bool saveLayout(const Layout& layout) {
    std::ofstream out(pathFor(layout.name));
    if (!out) {
        TraceLog(LOG_WARNING, "LAYOUT: could not write %s", pathFor(layout.name).c_str());
        return false;
    }
    out << "# " << layout.name << ".layout\n\n";

    for (const LayoutElement& e : layout.elements) {
        if (e.type == ElemType::Button) {
            out << "button id=" << e.id
                << " x=" << num(e.x) << " y=" << num(e.y)
                << " w=" << num(e.w) << " h=" << num(e.h)
                << " text=" << e.text << "\n";
        } else {
            out << "label  id=" << e.id
                << " align=" << (e.align == Align::Center ? "center" : "left")
                << " x=" << num(e.x) << " y=" << num(e.y)
                << " size=" << num(e.size)
                << " font=" << e.font
                << " color=" << e.color
                << " text=" << e.text << "\n";
        }
    }
    return true;
}

long layoutFileTime(const std::string& name) {
    std::string p = pathFor(name);
    return FileExists(p.c_str()) ? GetFileModTime(p.c_str()) : 0;
}

static Vector2 measureLabel(const LayoutElement& e) {
    return titleMeasure(e.text.c_str(), e.size);
}

Rectangle layoutBounds(const LayoutElement& e) {
    if (e.type == ElemType::Button) return { e.x, e.y, e.w, e.h };

    Vector2 m = measureLabel(e);
    float left = (e.align == Align::Center) ? e.x - m.x / 2.0f : e.x;
    return { left, e.y, m.x, m.y };
}

const LayoutElement* layoutFind(const Layout& layout, const std::string& id) {
    for (const LayoutElement& e : layout.elements) if (e.id == id) return &e;
    return nullptr;
}

void layoutDrawLabels(const Layout& layout, Vector2 offset) {
    for (const LayoutElement& e : layout.elements) {
        if (e.type != ElemType::Label) continue;
        Rectangle b = layoutBounds(e);
        float x = b.x + offset.x, y = b.y + offset.y;
        titleDraw(e.text.c_str(), x, y, e.size, layoutColor(e.color));
    }
}

void menuFromLayout(Menu& menu, std::vector<std::string>& outIds, const Layout& layout) {
    menu.buttons.clear();
    menu.focus = 0;
    outIds.clear();
    for (const LayoutElement& e : layout.elements) {
        if (e.type != ElemType::Button) continue;
        menuAdd(menu, { e.x, e.y, e.w, e.h }, e.text);
        outIds.push_back(e.id);
    }
}
