#include "raylib.h"
#include "config.h"
#include "screen.h"
#include "gamefont.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

static std::unique_ptr<Screen> current;

#if !defined(PLATFORM_WEB)
static bool shouldQuit = false;
#endif

static void UpdateDrawFrame() {
    ScreenType next = current->update();

    if (next == ScreenType::QUIT) {
    #if defined(PLATFORM_WEB)
        emscripten_cancel_main_loop();
    #else
        shouldQuit = true;
    #endif
    } else if (next != ScreenType::NONE) {
        current = createScreen(next);
    }

    BeginDrawing();
    current->draw();
    EndDrawing();
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hexactly");

    SetExitKey(KEY_NULL);

#if !defined(PLATFORM_WEB)
    ChangeDirectory(GetApplicationDirectory());
#endif

    loadFonts();

    current = createScreen(ScreenType::TITLE);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose() && !shouldQuit) {
        UpdateDrawFrame();
    }
#endif

    unloadFonts();
    CloseWindow();
    return 0;
}
