#include "options_screen.h"
#include "config.h"
#include "audio.h"
#include "gamefont.h"
#include "raylib.h"

OptionsScreen::OptionsScreen() {
    layout = loadLayout("options");
    menuFromLayout(menu, ids, layout);
    mtime = layoutFileTime("options");
}

ScreenType OptionsScreen::update() {
    if (editorTick(editor, layout, menu, ids, mtime)) return ScreenType::NONE;

    int a = menuUpdate(menu, GetFrameTime());
    if (a >= 0) {
        const std::string& id = ids[a];
        if (id == "mus_minus") audioSetMusicVolume(audioGetMusicVolume() - 0.1f);
        if (id == "mus_plus")  audioSetMusicVolume(audioGetMusicVolume() + 0.1f);
        if (id == "sfx_minus" || id == "sfx_plus") {
            audioSetSfxVolume(audioGetSfxVolume() + (id == "sfx_plus" ? 0.1f : -0.1f));
            playMerge();
        }
        if (id == "back") return ScreenType::TITLE;
        audioSaveSettings();
    }
    return ScreenType::NONE;
}

void OptionsScreen::draw() {
    ClearBackground(PAPER);

    layoutDrawLabels(layout);

    const char* mus = TextFormat("Music: %d", (int)(audioGetMusicVolume() * 100 + 0.5f));
    titleDraw(mus, SCREEN_WIDTH/2 - titleMeasure(mus, 24).x/2, 264, 24, INK);

    const char* sfx = TextFormat("SFX: %d", (int)(audioGetSfxVolume() * 100 + 0.5f));
    titleDraw(sfx, SCREEN_WIDTH/2 - titleMeasure(sfx, 24).x/2, 364, 24, INK);

    menuDraw(menu);

    editorDrawOverlay(editor, layout);
}
