#include "raylib.h"
#include "config.h"
#include "screen.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hexactly");
    SetTargetFPS(60);

    SetExitKey(KEY_NULL);

    std::unique_ptr<Screen> current = createScreen(ScreenType::TITLE);

    while (!WindowShouldClose()) {
        ScreenType next = current->update();

        if (next == ScreenType::QUIT) {
            break;
        }
        if (next != ScreenType::NONE) {
            current = createScreen(next);
        }

        BeginDrawing();
        current->draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
