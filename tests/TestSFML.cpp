// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// SFML 3.x integration tests. Verify that the system-installed SFML headers
// link and function correctly. Tests that require a display (X11/Wayland) are
// skipped when $DISPLAY / $WAYLAND_DISPLAY are not set, so this suite runs
// safely in headless CI.

#include "Harness.h"

#include <cstdlib>
#include <cstring>
#include <cstdint>

// SFML headers — same includes the engine uses
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Network/Http.hpp>
#include <SFML/Audio/SoundSource.hpp>

// ============================================================================
// 1. Compile-time version check
// ============================================================================

LTE_TEST(SFML_VersionMacros) {
  // SFML 3.x defines SFML_VERSION_MAJOR / MINOR / PATCH
  // After the 3.1 upgrade these should be >= 3, >= 1.
  LTE_CHECK(SFML_VERSION_MAJOR >= 3);
  LTE_CHECK(SFML_VERSION_MINOR >= 1);
  // PATCH can be anything; just verify the macro exists and is numeric.
  int patch = SFML_VERSION_PATCH;
  LTE_CHECK(patch >= 0);
  (void)patch;
}

// ============================================================================
// 2. sf::Time and sf::Clock
// ============================================================================

LTE_TEST(SFML_TimeConstructors) {
  using namespace sf;

  // zero
  Time t0 = Time::Zero;
  LTE_CHECK_EQ(t0.asSeconds(), 0.0f);
  LTE_CHECK_EQ(t0.asMilliseconds(), std::int32_t(0));
  LTE_CHECK_EQ(t0.asMicroseconds(), std::int64_t(0));

  // from seconds
  Time t1 = seconds(1.5f);
  LTE_CHECK(t1.asSeconds() > 1.49f && t1.asSeconds() < 1.51f);

  // from milliseconds
  Time t2 = milliseconds(200);
  LTE_CHECK_EQ(t2.asMilliseconds(), std::int32_t(200));

  // from microseconds
  Time t3 = microseconds(500000);
  LTE_CHECK_EQ(t3.asMicroseconds(), std::int64_t(500000));

  // arithmetic
  Time sum = t1 + t2;
  LTE_CHECK(sum.asMilliseconds() > 1699 && sum.asMilliseconds() < 1701);
  Time diff = t1 - t2;
  LTE_CHECK(diff.asMilliseconds() > 1299 && diff.asMilliseconds() < 1301);

  // comparison
  LTE_CHECK(t1 > t2);
  LTE_CHECK(t2 < t1);
  LTE_CHECK(t0 == Time::Zero);
}

LTE_TEST(SFML_ClockTiming) {
  using namespace sf;
  Clock clock;
  Time elapsed = clock.getElapsedTime();
  // Clock should have started; elapsed should be >= 0 and small.
  LTE_CHECK(elapsed.asMilliseconds() >= 0);
  LTE_CHECK(elapsed.asMilliseconds() < 500);  // should be nearly instant

  // restart returns old time and resets
  Time old = clock.restart();
  LTE_CHECK(old.asMilliseconds() >= 0);
  Time now2 = clock.getElapsedTime();
  LTE_CHECK(now2.asMilliseconds() < old.asMilliseconds() + 100);
}

// ============================================================================
// 3. sf::Vector2 / sf::Vector3
// ============================================================================

LTE_TEST(SFML_Vector2Basics) {
  using namespace sf;
  Vector2f a(1.0f, 2.0f);
  Vector2f b(3.0f, 4.0f);
  LTE_CHECK_EQ(a.x, 1.0f);
  LTE_CHECK_EQ(a.y, 2.0f);

  Vector2f c = a + b;
  LTE_CHECK_EQ(c.x, 4.0f);
  LTE_CHECK_EQ(c.y, 6.0f);

  Vector2f d = b - a;
  LTE_CHECK_EQ(d.x, 2.0f);
  LTE_CHECK_EQ(d.y, 2.0f);

  // aggregate init (C++17)
  Vector2i e{10, 20};
  LTE_CHECK_EQ(e.x, 10);
  LTE_CHECK_EQ(e.y, 20);
}

LTE_TEST(SFML_Vector3Basics) {
  using namespace sf;
  Vector3f a(1.0f, 2.0f, 3.0f);
  Vector3f b(4.0f, 5.0f, 6.0f);
  Vector3f c = a + b;
  LTE_CHECK_EQ(c.x, 5.0f);
  LTE_CHECK_EQ(c.y, 7.0f);
  LTE_CHECK_EQ(c.z, 9.0f);
}

// ============================================================================
// 4. sf::Image (no display needed)
// ============================================================================

LTE_TEST(SFML_ImageCreateAndQuery) {
  using namespace sf;
  // SFML 3.0 constructor: Image(Vector2u size, Color fill)
  Image img({64, 64}, Color::Black);
  auto sz = img.getSize();
  LTE_CHECK_EQ(sz.x, 64u);
  LTE_CHECK_EQ(sz.y, 64u);

  // getPixelsPtr should return non-null for a valid image
  auto const* px = img.getPixelsPtr();
  LTE_CHECK(px != nullptr);

  // pixel at (0,0) should be black (0,0,0,255)
  LTE_CHECK_EQ(px[0], std::uint8_t(0));
  LTE_CHECK_EQ(px[1], std::uint8_t(0));
  LTE_CHECK_EQ(px[2], std::uint8_t(0));
  LTE_CHECK_EQ(px[3], std::uint8_t(255));
}

LTE_TEST(SFML_ImageFromPixels) {
  using namespace sf;
  // Construct from raw pixel buffer (matching Texture2D.cpp pattern)
  std::vector<std::uint8_t> pixels(16 * 16 * 4);
  for (std::size_t i = 0; i < pixels.size(); i += 4) {
    pixels[i + 0] = 255;  // R
    pixels[i + 1] = 128;  // G
    pixels[i + 2] = 0;    // B
    pixels[i + 3] = 255;  // A
  }
  // SFML 3.0 takes span via initializer_list or pointer+size
  Image img({16, 16}, pixels.data());
  auto sz = img.getSize();
  LTE_CHECK_EQ(sz.x, 16u);
  LTE_CHECK_EQ(sz.y, 16u);

  auto const* px = img.getPixelsPtr();
  LTE_CHECK_EQ(px[0], std::uint8_t(255));  // R
  LTE_CHECK_EQ(px[1], std::uint8_t(128));  // G
  LTE_CHECK_EQ(px[2], std::uint8_t(0));    // B
  LTE_CHECK_EQ(px[3], std::uint8_t(255));  // A
}

// ============================================================================
// 5. sf::Keyboard enum values (scoped in SFML 3.0+)
// ============================================================================

LTE_TEST(SFML_KeyboardScopedEnums) {
  // Verify SFML 3.0+ scoped enumerations resolve correctly
  using K = sf::Keyboard::Key;

  LTE_CHECK(K::Unknown == sf::Keyboard::Key::Unknown);
  LTE_CHECK(K::A == sf::Keyboard::Key::A);
  LTE_CHECK(K::Z == sf::Keyboard::Key::Z);
  LTE_CHECK(K::Num0 == sf::Keyboard::Key::Num0);
  LTE_CHECK(K::Num9 == sf::Keyboard::Key::Num9);
  LTE_CHECK(K::Escape == sf::Keyboard::Key::Escape);
  LTE_CHECK(K::Space == sf::Keyboard::Key::Space);
  LTE_CHECK(K::Enter == sf::Keyboard::Key::Enter);
  LTE_CHECK(K::Backspace == sf::Keyboard::Key::Backspace);
  LTE_CHECK(K::Tab == sf::Keyboard::Key::Tab);
  LTE_CHECK(K::LShift == sf::Keyboard::Key::LShift);
  LTE_CHECK(K::F1 == sf::Keyboard::Key::F1);
  LTE_CHECK(K::F12 == sf::Keyboard::Key::F12);

  // verify integer values match expected scan codes for a few keys
  LTE_CHECK_EQ(static_cast<int>(K::A), 0);
  LTE_CHECK_EQ(static_cast<int>(K::Escape), 36);
}

// ============================================================================
// 6. sf::Mouse enum values (scoped in SFML 3.0+)
// ============================================================================

LTE_TEST(SFML_MouseScopedEnums) {
  using B = sf::Mouse::Button;

  LTE_CHECK(B::Left == sf::Mouse::Button::Left);
  LTE_CHECK(B::Right == sf::Mouse::Button::Right);
  LTE_CHECK(B::Middle == sf::Mouse::Button::Middle);
  // SFML 3.0 renamed XButton1/XButton2 to Extra1/Extra2
  LTE_CHECK(B::Extra1 == sf::Mouse::Button::Extra1);
  LTE_CHECK(B::Extra2 == sf::Mouse::Button::Extra2);

  // ButtonCount should match
  LTE_CHECK_EQ(sf::Mouse::ButtonCount, std::uint8_t(5));
}

// ============================================================================
// 7. sf::VideoMode
// ============================================================================

LTE_TEST(SFML_VideoModeConstruct) {
  using namespace sf;
  // SFML 3.0 uses Vector2u for size
  VideoMode vm({800, 600}, 32);
  LTE_CHECK_EQ(vm.size.x, 800u);
  LTE_CHECK_EQ(vm.size.y, 600u);
  LTE_CHECK_EQ(vm.bitsPerPixel, 32u);

  // isValid should return true for a reasonable mode
  LTE_CHECK(vm.isValid());
}

// ============================================================================
// 8. sf::RenderWindow creation (requires display)
// ============================================================================

static bool HasDisplay() {
  return std::getenv("DISPLAY") != nullptr ||
         std::getenv("WAYLAND_DISPLAY") != nullptr;
}

LTE_TEST(SFML_RenderWindowCreation) {
  if (!HasDisplay()) {
    // Skip — no display server available
    return;
  }

  using namespace sf;
  ContextSettings glSettings;
  glSettings.depthBits = 24;
  glSettings.stencilBits = 8;
  glSettings.antiAliasingLevel = 0;
  glSettings.majorVersion = 3;
  glSettings.minorVersion = 3;
  glSettings.attributeFlags = ContextSettings::Attribute::Default;
  glSettings.sRgbCapable = false;

  RenderWindow window(
    VideoMode({320, 240}, 32),
    "SFML Test",
    Style::None,
    State::Windowed,
    glSettings);

  LTE_CHECK(window.isOpen());
  auto sz = window.getSize();
  LTE_CHECK_EQ(sz.x, 320u);
  LTE_CHECK_EQ(sz.y, 240u);

  // Verify we have an OpenGL context
  LTE_CHECK(window.setActive(true));

  window.close();
  LTE_CHECK(!window.isOpen());
}

LTE_TEST(SFML_RenderWindowResize) {
  if (!HasDisplay()) return;

  using namespace sf;
  RenderWindow window(
    VideoMode({640, 480}, 32),
    "SFML Resize Test",
    Style::None,
    State::Windowed);

  LTE_CHECK(window.isOpen());
  window.setSize({320, 240});
  auto sz = window.getSize();
  // setSize may not produce exact values due to OS rounding, but close
  LTE_CHECK(sz.x >= 300u && sz.x <= 340u);
  LTE_CHECK(sz.y >= 220u && sz.y <= 260u);

  window.close();
}

// ============================================================================
// 9. sf::SoundSource::Status enum (audio backend)
// ============================================================================

LTE_TEST(SFML_SoundSourceStatus) {
  // Verify the status enum used by sf::Sound / sf::Music
  // This is the enum SFML.cpp uses for IsStopped().
  using Status = sf::SoundSource::Status;
  LTE_CHECK(Status::Stopped != Status::Playing);
  LTE_CHECK(Status::Playing != Status::Paused);
  LTE_CHECK(Status::Paused != Status::Stopped);

  // Verify the expected integer values (stopped=0, paused=1, playing=2)
  LTE_CHECK_EQ(static_cast<int>(Status::Stopped), 0);
  LTE_CHECK_EQ(static_cast<int>(Status::Paused), 1);
  LTE_CHECK_EQ(static_cast<int>(Status::Playing), 2);
}

// ============================================================================
// 10. sf::Http (network — compile/link check only)
// ============================================================================

LTE_TEST(SFML_HttpApiSurface) {
  // We can't make real network requests in unit tests, but we can verify
  // the sf::Http API compiles and links correctly. This catches header
  // mismatches between the SFML 3.1 dev package and the engine.
  using namespace sf;
  // sf::Http should be constructible with a host string
  Http http("example.com");
  // sf::Http::Request should be constructible
  Http::Request req("/", Http::Request::Method::Get);
  // sf::Http::Response should be default-constructible
  Http::Response resp;
  // Verify Response::Status enum exists
  auto status = resp.getStatus();
  (void)status;
  (void)http;
  (void)req;
  LTE_CHECK(true);  // if we got here, the API compiles
}
