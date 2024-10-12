/*
 * Copyright (c) 2024 Isaac Aronson <i@pingas.org>
 * zlib License, see LICENSE file.
 */

#include "bn_core.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_text_generator.h"
#include "bn_music_items.h"

#include "common_variable_8x16_sprite_font.h"

#include "RtcSceneManager.h"
#include "ezflash.h"

// Inform emulators and cart readers that this game supports RTC (flash saves often tied to this)
alignas(int) __attribute__((used)) const char rtc_hint[] = "SIIRTC_V001\0";

// We do this to make sure the strings actually get emitted after optimizations
// ... At least you know it's bad
static char *init() {
    return const_cast<char *>(rtc_hint);
}

// I tried to put this in its own TU, but I got strcmp multiple definition errors...
RtcSceneManager::RtcSceneManager(bn::sprite_text_generator generator, bn::optional<bn::sprite_ptr> status)
        : textGenerator(
        generator), statusSprite(std::move(status)) {
    sm.Initialize<ClientStates::WelcomeScene>(this);
}

int main() {
    // Hello Butano
    bn::core::init();

    // Set backdrop
    bn::bg_palettes::set_transparent_color(bn::color(16, 20, 16));

    // Set up rendering
    bn::sprite_text_generator textGenerator(common::variable_8x16_sprite_font);
    bn::optional<bn::sprite_ptr> statusSprite;
    textGenerator.set_center_alignment();

    // Set up scene manager
    RtcSceneManager sceneManager(textGenerator, statusSprite);

    // Detect EZ Flash now
    if (detect()) EnableOdeRtc();

    bool musicStarted = false;
    // Main logic loop, attempts to exit to inserted cart on common soft reset key combination
    while (!(bn::keypad::start_held() && bn::keypad::select_held() && bn::keypad::a_held() && bn::keypad::b_held())) {
        // Pump state machine and scene updates
        sceneManager.Update();

        // Call framework update after we're done
        bn::core::update();

        // Start music, but only once after first render
        if (musicStarted) continue;
        bn::music_items::trams.play(1.0, true);
        musicStarted = true;
    }
    return 0;
}
