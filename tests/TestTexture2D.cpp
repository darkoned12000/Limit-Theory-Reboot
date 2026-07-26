// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Harness.h"

#include "LTE/Texture2D.h"
#include "LTE/GLEnum.h"

#include <cstdlib>
#include <filesystem>
#include <GL/glew.h>
#include <SFML/Graphics.hpp>

namespace {
  bool HasDisplay() {
    return std::getenv("DISPLAY") != nullptr ||
           std::getenv("WAYLAND_DISPLAY") != nullptr;
  }
}

LTE_TEST(Texture2D_SaveTo) {
  if (!HasDisplay()) {
    // Skip — OpenGL context requires a display server
    return;
  }

  // Ensure an OpenGL context exists for this thread
  sf::Context context;
  (void)context.setActive(true); // Unused return value is fine here
  glewExperimental = GL_TRUE;
  glewInit();

  uint w = 32;
  uint h = 32;
  String path = "test_texture_output.png";

  // Create a simple RGBA8 texture
  Texture2D tex = Texture_Create(w, h, GL_TextureFormat::RGBA8);
  
  // Test SaveTo
  tex->SaveTo(path, false);

  // Verify file existence
  LTE_CHECK(std::filesystem::exists(std::filesystem::path(path.c_str())));

  // Clean up
  std::filesystem::remove(std::filesystem::path(path.c_str()));
}
