#pragma once

#include "raylib.h"
#include "ui.h"
#include <string>
#include <vector>

enum class ElemType { Label, Button };
enum class Align     { Left, Center };

struct LayoutElement {
    ElemType    type = ElemType::Label;
    std::string id;
    std::string text;

    float x = 0, y = 0, w = 0, h = 0;

    float       size  = 24;
    Align       align = Align::Left;
    std::string font  = "plain";
    std::string color = "ink";
};

struct Layout {
    std::string                name;
    std::vector<LayoutElement> elements;
};

Layout loadLayout(const std::string& name);
bool   saveLayout(const Layout& layout);

long   layoutFileTime(const std::string& name);

Color  layoutColor(const std::string& name);

Rectangle layoutBounds(const LayoutElement& e);

const LayoutElement* layoutFind(const Layout& layout, const std::string& id);

void   layoutDrawLabels(const Layout& layout, Vector2 offset = { 0, 0 });

void   menuFromLayout(Menu& menu, std::vector<std::string>& outIds, const Layout& layout);
