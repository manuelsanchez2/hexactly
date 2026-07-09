#include "editor.h"
#include "config.h"
#include "gamefont.h"
#include <cmath>

static const float GRID = 16.0f;

static float snap(float v) { return std::round(v / GRID) * GRID; }

static void moveTo(LayoutElement& e, float nx, float ny) { e.x = nx; e.y = ny; }

bool editorUpdate(Editor& ed, Layout& layout) {
    if (IsKeyPressed(KEY_F1)) {
        ed.active = !ed.active;
        ed.selected = -1;
        ed.dragging = false;
    }
    if (ed.savedFlash > 0) ed.savedFlash -= GetFrameTime();
    if (!ed.active) return false;

    bool changed = false;
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    Vector2 mouse = GetMousePosition();
    int n = (int)layout.elements.size();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ed.selected = -1;
        for (int i = n - 1; i >= 0; i--) {
            if (CheckCollisionPointRec(mouse, layoutBounds(layout.elements[i]))) {
                ed.selected = i;
                ed.dragging = true;
                ed.dragGrab = { mouse.x - layout.elements[i].x,
                                mouse.y - layout.elements[i].y };
                break;
            }
        }
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) ed.dragging = false;

    if (ed.dragging && ed.selected >= 0) {
        float nx = mouse.x - ed.dragGrab.x;
        float ny = mouse.y - ed.dragGrab.y;
        if (shift) { nx = snap(nx); ny = snap(ny); }
        LayoutElement& e = layout.elements[ed.selected];
        if (nx != e.x || ny != e.y) { moveTo(e, nx, ny); changed = true; }
    }

    if (ed.selected >= 0 && ed.selected < n) {
        LayoutElement& e = layout.elements[ed.selected];
        float step = shift ? GRID : 1.0f;

        if (IsKeyPressed(KEY_LEFT))  { e.x -= step; changed = true; }
        if (IsKeyPressed(KEY_RIGHT)) { e.x += step; changed = true; }
        if (IsKeyPressed(KEY_UP))    { e.y -= step; changed = true; }
        if (IsKeyPressed(KEY_DOWN))  { e.y += step; changed = true; }

        if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
            layout.elements.erase(layout.elements.begin() + ed.selected);
            ed.selected = -1;
            ed.dragging = false;
            changed = true;
        }

        if (IsKeyPressed(KEY_TAB) && n > 0) {
            ed.selected = (ed.selected + 1) % n;
        }
    } else if (IsKeyPressed(KEY_TAB) && n > 0) {
        ed.selected = 0;
    }

    bool cmd = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
               IsKeyDown(KEY_LEFT_SUPER)   || IsKeyDown(KEY_RIGHT_SUPER);
    if (cmd && IsKeyPressed(KEY_S)) {
        if (saveLayout(layout)) ed.savedFlash = 1.5f;
    }

    return changed;
}

bool editorTick(Editor& ed, Layout& layout, Menu& menu,
                std::vector<std::string>& ids, long& mtime) {
    if (!ed.active) {
        long t = layoutFileTime(layout.name);
        if (t != 0 && t != mtime) {
            layout = loadLayout(layout.name);
            menuFromLayout(menu, ids, layout);
            mtime = t;
        }
    }

    if (editorUpdate(ed, layout)) {
        menuFromLayout(menu, ids, layout);
        mtime = layoutFileTime(layout.name);
    }
    return ed.active;
}

static void drawHelp(const Editor& ed, const Layout& layout) {
    int pad = 10, lh = 20;
    Rectangle panel = { 8, 8, 470, ed.selected >= 0 ? 116.0f : 96.0f };
    DrawRectangleRec(panel, (Color){ 20, 20, 20, 200 });
    DrawRectangleLinesEx(panel, 1, (Color){ 255, 255, 255, 60 });

    int x = (int)panel.x + pad, y = (int)panel.y + pad;
    DrawText(TextFormat("EDITOR  [%s]   F1 to exit", layout.name.c_str()),
             x, y, 18, (Color){ 255, 220, 120, 255 }); y += lh + 2;
    DrawText("Click select  Drag move  Arrows nudge  Shift = grid16",
             x, y, 16, RAYWHITE); y += lh;
    DrawText("Del remove   Ctrl/Cmd+S save   Tab cycle",
             x, y, 16, RAYWHITE); y += lh;

    if (ed.selected >= 0 && ed.selected < (int)layout.elements.size()) {
        const LayoutElement& e = layout.elements[ed.selected];
        DrawText(TextFormat("selected: %s   x=%d y=%d",
                            e.id.empty() ? "(no id)" : e.id.c_str(),
                            (int)e.x, (int)e.y),
                 x, y, 16, (Color){ 120, 220, 255, 255 });
    }
}

void editorDrawOverlay(Editor& ed, const Layout& layout) {
    if (!ed.active) return;

    for (const LayoutElement& e : layout.elements) {
        DrawRectangleLinesEx(layoutBounds(e), 1, (Color){ 120, 120, 128, 120 });
    }
    if (ed.selected >= 0 && ed.selected < (int)layout.elements.size()) {
        Rectangle b = layoutBounds(layout.elements[ed.selected]);
        DrawRectangleLinesEx({ b.x - 3, b.y - 3, b.width + 6, b.height + 6 }, 2, HEXRED);
    }

    drawHelp(ed, layout);

    if (ed.savedFlash > 0) {
        const char* msg = "saved!";
        int w = MeasureText(msg, 24);
        DrawText(msg, SCREEN_WIDTH - w - 20, 20, 24, (Color){ 120, 220, 120, 255 });
    }
}
