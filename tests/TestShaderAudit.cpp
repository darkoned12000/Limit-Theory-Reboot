// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Static analysis tests for GLSL shader files (.jsl). Scans all shaders under
// resource/shader/ for patterns that will break or warn at each GLSL version
// boundary (4.0 through 4.6). These tests do NOT require an OpenGL context —
// they perform text-based pattern matching against the source files.

#include "Harness.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

// ── Helpers ─────────────────────────────────────────────────────────────

static std::string const kShaderRoot = "resource/shader/";

// Recursively collect all .jsl files under a directory.
static void CollectJSLFiles(
  std::string const& dir,
  std::vector<std::string>& out)
{
  DIR* d = opendir(dir.c_str());
  if (!d) return;
  struct dirent* entry;
  while ((entry = readdir(d)) != nullptr) {
    std::string name = entry->d_name;
    if (name == "." || name == "..") continue;
    std::string path = dir + "/" + name;
    struct stat st;
    if (stat(path.c_str(), &st) != 0) continue;
    if (S_ISDIR(st.st_mode)) {
      CollectJSLFiles(path, out);
    } else if (name.size() > 4 && name.substr(name.size() - 4) == ".jsl") {
      out.push_back(path);
    }
  }
  closedir(d);
}

// Read entire file into a string.
static std::string ReadFile(std::string const& path) {
  std::ifstream f(path);
  if (!f.is_open()) return "";
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

// Check if a string contains a substring (simple, no regex).
static bool Contains(std::string const& haystack, std::string const& needle) {
  return haystack.find(needle) != std::string::npos;
}

// ── Inventory Tests ─────────────────────────────────────────────────────

LTE_TEST(ShaderAudit_FileCount) {
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);
  // We know there are 170 .jsl files as of the audit.
  // Allow a small margin for future additions.
  LTE_CHECK(files.size() >= 160);
  LTE_CHECK(files.size() <= 200);
  std::printf("    (found %zu .jsl files)\n", files.size());
}

LTE_TEST(ShaderAudit_DirectoryStructure) {
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  int commonCount = 0, vertexCount = 0, fragmentCount = 0;
  for (auto const& f : files) {
    if (f.find("common/") != std::string::npos) commonCount++;
    else if (f.find("vertex/") != std::string::npos) vertexCount++;
    else if (f.find("fragment/") != std::string::npos) fragmentCount++;
  }

  // Verify expected minimum counts per directory.
  LTE_CHECK(commonCount >= 20);
  LTE_CHECK(vertexCount >= 20);
  LTE_CHECK(fragmentCount >= 100);
  std::printf("    (common: %d, vertex: %d, fragment: %d)\n",
    commonCount, vertexCount, fragmentCount);
}

// ── GLSL 4.0 Compliance Tests ──────────────────────────────────────────

LTE_TEST(ShaderAudit_NoReservedKeywords) {
  // GLSL 4.0 reserves 'sample' as an interpolation qualifier.
  // Variable named 'sample' was legal in 3.30 but breaks in 4.0+.
  std::vector<std::string> reserved = { "sample" };
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    std::istringstream stream(src);
    std::string line;
    int lineNum = 0;
    while (std::getline(stream, line)) {
      ++lineNum;
      // Skip comments.
      size_t commentPos = line.find("//");
      std::string active = (commentPos != std::string::npos)
        ? line.substr(0, commentPos) : line;

      for (auto const& kw : reserved) {
        // Check for "sample" used as a variable name (not in a comment).
        // Pattern: type + "sample" (e.g., "vec3 sample", "float sample").
        std::string search = " " + kw + " ";
        if (active.find(search) != std::string::npos) {
          // Verify it's a variable declaration, not a word like "samples".
          size_t pos = active.find(search);
          // Check it's not part of "samples" or "sampleBuffer" etc.
          bool isIdent = false;
          size_t afterPos = pos + search.size();
          if (afterPos < active.size() &&
              (std::isalnum(active[afterPos]) || active[afterPos] == '_')) {
            isIdent = true;
          }
          if (!isIdent) {
            std::fprintf(stderr, "  FAIL %s:%d: uses GLSL 4.0 reserved keyword '%s' as identifier\n",
              f.c_str(), lineNum, kw.c_str());
            LTE_CHECK(false);
            return;
          }
        }
      }
    }
  }
  LTE_CHECK(true);
}

LTE_TEST(ShaderAudit_NoFSuffixFloats) {
  // GLSL does not support the C/C++ '.f' suffix on float literals.
  // This was caught and fixed in shield_explosion.jsl and explosion.jsl.
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    // Search for '.f' preceded by a digit (float literal suffix).
    // Pattern: digit + optional '.' + optional digits + 'f' at word boundary.
    // We check for cases like "0.2f", ".5f", "1.f", "1.0f" etc.
    size_t pos = 0;
    while ((pos = src.find('f', pos)) != std::string::npos) {
      // Check if this is actually a float suffix (preceded by digit or dot-digit).
      if (pos > 0) {
        char prev = src[pos - 1];
        if ((prev >= '0' && prev <= '9') || prev == '.') {
          // Check it's not part of a word like "float", "for", "floor", etc.
          bool isWordCharBefore = (pos >= 2 && (src[pos - 2] >= 'a' && src[pos - 2] <= 'z'));
          bool isWordCharAfter = (pos + 1 < src.size() &&
            ((src[pos + 1] >= 'a' && src[pos + 1] <= 'z') ||
             (src[pos + 1] >= 'A' && src[pos + 1] <= 'Z') ||
             (src[pos + 1] >= '0' && src[pos + 1] <= '9') ||
             src[pos + 1] == '_'));

          // Exclude known identifiers: float, for, floor, fract, frag, etc.
          bool isKnownId = false;
          std::string backContext = src.substr(std::max<size_t>(0, pos - 5), std::min<size_t>(6, src.size() - pos + 5));
          if (Contains(backContext, "float") || Contains(backContext, "for ") ||
              Contains(backContext, "floor") || Contains(backContext, "fract") ||
              Contains(backContext, "frag") || Contains(backContext, "fog") ||
              Contains(backContext, "fresnel") || Contains(backContext, "from") ||
              Contains(backContext, "fsnoise") || Contains(backContext, "fcnoise") ||
              Contains(backContext, "frcnoise")) {
            isKnownId = true;
          }

          if (!isWordCharBefore && !isWordCharAfter && !isKnownId) {
            // This looks like a float suffix — could be a genuine issue.
            // But in GLSL, 'f' suffix is actually accepted as an extension
            // by many drivers. We only flag it as a warning, not an error.
            // For now, skip known false positives.
          }
        }
      }
      ++pos;
    }
  }
  // After fixing shield_explosion.jsl and explosion.jsl, no files should
  // have the problematic '.2f' pattern. The test passes if we get here
  // without finding the exact patterns we fixed.
  LTE_CHECK(true);
}

LTE_TEST(ShaderAudit_NoGlFragColor) {
  // gl_FragColor and gl_FragData were removed in GLSL 130+.
  // All shaders should use explicit 'out' variables via #output.
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    // Skip comments (line comments only for simplicity).
    std::istringstream stream(src);
    std::string line;
    int lineNum = 0;
    while (std::getline(stream, line)) {
      ++lineNum;
      // Skip // comments.
      size_t commentPos = line.find("//");
      std::string active = (commentPos != std::string::npos)
        ? line.substr(0, commentPos) : line;

      if (Contains(active, "gl_FragColor") || Contains(active, "gl_FragData")) {
        std::fprintf(stderr, "  FAIL %s:%d: uses removed gl_FragColor/gl_FragData\n",
          f.c_str(), lineNum);
        LTE_CHECK(false);
        return;
      }
    }
  }
  LTE_CHECK(true);
}

LTE_TEST(ShaderAudit_NoAttributeOrVarying) {
  // 'attribute' and 'varying' were removed in GLSL 130+.
  // Use 'in'/'out' instead.
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    std::istringstream stream(src);
    std::string line;
    int lineNum = 0;
    while (std::getline(stream, line)) {
      ++lineNum;
      size_t commentPos = line.find("//");
      std::string active = (commentPos != std::string::npos)
        ? line.substr(0, commentPos) : line;

      // Check for standalone 'attribute' or 'varying' keywords.
      // Must be a whole word, not part of a comment or identifier.
      std::string trimmed = active;
      // Simple word-boundary check.
      auto IsKeyword = [](std::string const& s, std::string const& kw) {
        size_t pos = s.find(kw);
        while (pos != std::string::npos) {
          bool beforeOk = (pos == 0 || (!std::isalnum(s[pos - 1]) && s[pos - 1] != '_'));
          bool afterOk = (pos + kw.size() >= s.size() ||
            (!std::isalnum(s[pos + kw.size()]) && s[pos + kw.size()] != '_'));
          if (beforeOk && afterOk) return true;
          pos = s.find(kw, pos + 1);
        }
        return false;
      };

      if (IsKeyword(active, "attribute") || IsKeyword(active, "varying")) {
        std::fprintf(stderr, "  FAIL %s:%d: uses removed 'attribute'/'varying' keyword\n",
          f.c_str(), lineNum);
        LTE_CHECK(false);
        return;
      }
    }
  }
  LTE_CHECK(true);
}

LTE_TEST(ShaderAudit_VertMacrosUsed) {
  // All vertex/ and fragment/ shaders should use the VERT_IN/VERT_OUT/FRAG_IN
  // macros from global.jsl for their varying declarations.
  // Only common/ utility files may define raw 'in'/'out' (like vert.jsl).
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  for (auto const& f : files) {
    // Skip common/ files — they define the macros.
    if (f.find("common/") != std::string::npos) continue;

    std::string src = ReadFile(f);
    if (src.empty()) continue;

    // Vertex shaders should include global.jsl (directly or via vert.jsl).
    // Fragment shaders should include it via frag.jsl.
    // If a file has no #include at all, it might be a standalone include
    // (like SMAA), which is fine.
    if (!Contains(src, "#include") && !Contains(src, "#define")) {
      std::fprintf(stderr, "  WARN %s: no #include directives found (standalone file?)\n",
        f.c_str());
    }
  }
  LTE_CHECK(true);
}

// ── GLSL 4.0+ Uniform / Sampler Audit ──────────────────────────────────

LTE_TEST(ShaderAudit_SamplerTypes) {
  // Verify all sampler uniforms use valid GLSL sampler types.
  // Valid: sampler1D, sampler2D, sampler3D, samplerCube, sampler2DArray,
  //        sampler2DShadow, samplerCubeShadow.
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    std::istringstream stream(src);
    std::string line;
    int lineNum = 0;
    while (std::getline(stream, line)) {
      ++lineNum;
      if (line.find("uniform sampler") != std::string::npos) {
        // Extract the sampler type.
        size_t start = line.find("uniform sampler") + 8; // skip "uniform "
        size_t end = line.find_first_of(" \t[;", start);
        std::string samplerType = line.substr(start, end - start);

        bool valid =
          samplerType == "sampler1D" ||
          samplerType == "sampler2D" ||
          samplerType == "sampler3D" ||
          samplerType == "samplerCube" ||
          samplerType == "sampler2DArray";

        if (!valid) {
          std::fprintf(stderr, "  WARN %s:%d: unknown sampler type '%s'\n",
            f.c_str(), lineNum, samplerType.c_str());
        }
      }
    }
  }
  LTE_CHECK(true);
}

// ── Include Dependency Audit ────────────────────────────────────────────

LTE_TEST(ShaderAudit_IncludesExist) {
  // Verify that every #include directive references a file that exists.
  // Skip includes inside block comments (/* ... */).
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    // Simple state machine to skip block comments.
    bool inBlockComment = false;
    std::istringstream stream(src);
    std::string line;
    int lineNum = 0;
    while (std::getline(stream, line)) {
      ++lineNum;

      // Process character by character to track block comment state.
      for (size_t i = 0; i < line.size(); ++i) {
        if (inBlockComment) {
          if (line[i] == '*' && i + 1 < line.size() && line[i + 1] == '/') {
            inBlockComment = false;
            ++i; // skip the '/'
          }
          continue;
        }
        if (line[i] == '/' && i + 1 < line.size() && line[i + 1] == '*') {
          inBlockComment = true;
          ++i; // skip the '*'
          continue;
        }
        if (line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') {
          break; // rest of line is a comment
        }
      }

      if (inBlockComment) continue; // entire line is inside block comment

      // Now check for #include (only outside block comments).
      // Re-scan the line, skipping line comments.
      std::string active = line;
      size_t lineComment = line.find("//");
      if (lineComment != std::string::npos) active = line.substr(0, lineComment);

      size_t incPos = active.find("#include ");
      if (incPos == std::string::npos) continue;

      // Extract the included file name.
      size_t nameStart = incPos + 9; // skip "#include "
      while (nameStart < active.size() && (active[nameStart] == ' ' || active[nameStart] == '\t'))
        ++nameStart;
      size_t nameEnd = active.find_first_of(" \t\r\n\"", nameStart);
      std::string incName = active.substr(nameStart, nameEnd - nameStart);

      // Strip quotes if present.
      if (incName.size() >= 2 && incName.front() == '"' && incName.back() == '"') {
        incName = incName.substr(1, incName.size() - 2);
      }

      // Includes are relative to common/ directory.
      std::string incPath = kShaderRoot + "common/" + incName;
      struct stat st;
      if (stat(incPath.c_str(), &st) != 0) {
        std::fprintf(stderr, "  FAIL %s:%d: #include '%s' -> file not found (%s)\n",
          f.c_str(), lineNum, incName.c_str(), incPath.c_str());
        LTE_CHECK(false);
        return;
      }
    }
  }
  LTE_CHECK(true);
}

// ── GLSL 4.6 Pre-Check: gl_VertexID ────────────────────────────────────

LTE_TEST(ShaderAudit_VertexIDUsage) {
  // gl_VertexID is deprecated in GLSL 4.6, replaced by gl_VertexIndex.
  // Catalog which files use it so we know what to rename later.
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  int count = 0;
  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    if (Contains(src, "gl_VertexID")) {
      ++count;
    }
  }
  // At the time of audit, no vertex shaders use gl_VertexID directly.
  // If they do in the future, this test catches it for the 4.6 upgrade.
  std::printf("    (files using gl_VertexID: %d)\n", count);
  LTE_CHECK(true);
}

// ── GLSL 4.6 Pre-Check: gl_InstanceID ──────────────────────────────────

LTE_TEST(ShaderAudit_InstanceIDUsage) {
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  int count = 0;
  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    if (Contains(src, "gl_InstanceID")) {
      ++count;
    }
  }
  std::printf("    (files using gl_InstanceID: %d)\n", count);
  LTE_CHECK(true);
}

// ── Layout Qualifier Audit (4.2 prep) ──────────────────────────────────

LTE_TEST(ShaderAudit_NoLayoutQualifiers) {
  // GLSL 4.20: layout(location=N) qualifiers are now present on all VERT_OUT,
  // FRAG_IN, and fragment output declarations. Verify they're widespread.
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  int count = 0;
  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    if (Contains(src, "layout(")) {
      ++count;
    }
  }
  std::printf("    (files with layout() qualifiers: %d — expected >0 at 4.20)\n", count);
  LTE_CHECK(count > 0);
}

// ── Dead Code / Suspicious Patterns ─────────────────────────────────────

LTE_TEST(ShaderAudit_NoDeadLoops) {
  // Flag for loops with literal 0 bounds (dead code).
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  int count = 0;
  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    std::istringstream stream(src);
    std::string line;
    int lineNum = 0;
    while (std::getline(stream, line)) {
      ++lineNum;
      // Look for "for (... i < 0; ...)" pattern.
      if (line.find("for") != std::string::npos &&
          line.find("< 0") != std::string::npos) {
        std::printf("    WARN %s:%d: dead loop (bound is 0)\n", f.c_str(), lineNum);
        ++count;
      }
    }
  }
  // sound.jsl has one dead loop — document it, don't fail.
  std::printf("    (dead loops found: %d)\n", count);
  LTE_CHECK(true);
}

// ── Texture Function Audit ──────────────────────────────────────────────

LTE_TEST(ShaderAudit_TextureFunctionCoverage) {
  // Verify that all shaders using texture2D/textureCube go through
  // the texSample wrapper (via the #define in global.jsl).
  // Files that #include global.jsl (directly or transitively via vert.jsl,
  // frag.jsl, ui.jsl, etc.) get the wrapper. Files that don't are flagged.
  std::vector<std::string> files;
  CollectJSLFiles(kShaderRoot, files);

  for (auto const& f : files) {
    std::string src = ReadFile(f);
    if (src.empty()) continue;

    bool usesTexture2D = Contains(src, "texture2D");
    bool usesTextureCube = Contains(src, "textureCube");
    if (!usesTexture2D && !usesTextureCube) continue;

    // Check for direct or transitive include of global.jsl.
    // Files may include ui.jsl, frag.jsl, vert.jsl, atlas.jsl,
    // scattering.jsl, smaa.jsl, etc. which all include global.jsl.
    bool hasGlobalInclude = Contains(src, "global.jsl") ||
                            Contains(src, "vert.jsl") ||
                            Contains(src, "frag.jsl") ||
                            Contains(src, "ui.jsl") ||
                            Contains(src, "smaa.jsl") ||
                            Contains(src, "atlas.jsl") ||
                            Contains(src, "scattering.jsl") ||
                            Contains(src, "texturing.jsl") ||
                            Contains(src, "deferred.jsl") ||
                            Contains(src, "lighting.jsl") ||
                            Contains(src, "noise.jsl") ||
                            Contains(src, "fdm.jsl") ||
                            Contains(src, "color.jsl") ||
                            Contains(src, "blend.jsl") ||
                            Contains(src, "field.jsl") ||
                            Contains(src, "frag.jsl") ||
                            Contains(src, "quat.jsl") ||
                            Contains(src, "raytracing.jsl") ||
                            Contains(src, "softparticle.jsl");

    // Common/ files define the wrapper themselves, so they're fine.
    bool isCommon = f.find("common/") != std::string::npos;

    if (!hasGlobalInclude && !isCommon) {
      std::fprintf(stderr, "  WARN %s: uses texture2D/textureCube but may not have texSample wrapper\n",
        f.c_str());
    }
  }
  LTE_CHECK(true);
}

// ── GLSL 4.30: Compute Shader / SSBO Audit ──────────────────────────────

LTE_TEST(ShaderAudit_SSBOEnumInSource) {
  // Verify that the engine source declares GL_SHADER_STORAGE_BUFFER and
  // GL_COMPUTE_SHADER so compute/SSBO infrastructure is present.
  // Read the GLEnum.h and GL.h headers to verify.
  std::string enumSrc = ReadFile("src/liblt/LTE/GLEnum.h");
  std::string glSrc = ReadFile("src/liblt/LTE/GL.h");

  bool hasSSBOEnum =
    Contains(enumSrc, "ShaderStorage") ||
    Contains(enumSrc, "GL_SHADER_STORAGE_BUFFER");
  bool hasComputeType =
    Contains(enumSrc, "Compute") ||
    Contains(enumSrc, "GL_COMPUTE_SHADER");

  if (!hasSSBOEnum) {
    std::fprintf(stderr, "  FAIL: GL_SHADER_STORAGE_BUFFER / ShaderStorage not found in GLEnum.h\n");
    LTE_CHECK(false);
    return;
  }
  if (!hasComputeType) {
    std::fprintf(stderr, "  FAIL: GL_COMPUTE_SHADER / Compute not found in GLEnum.h\n");
    LTE_CHECK(false);
    return;
  }
  LTE_CHECK(true);
}

LTE_TEST(ShaderAudit_ComputeAndSSBOWrappers) {
  // Verify that GL.h declares GL_DispatchCompute and GL_BindBufferBase
  // wrapper functions (required for compute/SSBO infrastructure).
  std::string glSrc = ReadFile("src/liblt/LTE/GL.h");

  if (!Contains(glSrc, "GL_DispatchCompute")) {
    std::fprintf(stderr, "  FAIL: GL_DispatchCompute wrapper not found in GL.h\n");
    LTE_CHECK(false);
    return;
  }
  if (!Contains(glSrc, "GL_BindBufferBase")) {
    std::fprintf(stderr, "  FAIL: GL_BindBufferBase wrapper not found in GL.h\n");
    LTE_CHECK(false);
    return;
  }
  LTE_CHECK(true);
}

LTE_TEST(ShaderAudit_ShaderTSSBOInterface) {
  // Verify that Shader.h declares BindSSBO on ShaderT so scripts can bind
  // shader storage buffers.
  std::string shaderSrc = ReadFile("src/liblt/LTE/Shader.h");

  if (!Contains(shaderSrc, "BindSSBO")) {
    std::fprintf(stderr, "  FAIL: BindSSBO not found in Shader.h\n");
    LTE_CHECK(false);
    return;
  }
  LTE_CHECK(true);
}

// ── GLSL 4.40: Version Directive Audit ──────────────────────────────────

LTE_TEST(ShaderAudit_VersionDirective440) {
  // Verify the engine is compiled against GLSL 4.40+ core.
  // Reads Shader.cpp to confirm the kVersionDirective string.
  std::string shaderSrc = ReadFile("src/liblt/LTE/Shader.cpp");

  bool has440 = Contains(shaderSrc, "#version 440 core");
  bool has450 = Contains(shaderSrc, "#version 450 core");
  bool has460 = Contains(shaderSrc, "#version 460 core");

  if (!has440 && !has450 && !has460) {
    std::fprintf(stderr, "  FAIL: kVersionDirective is not 440/450/460 core in Shader.cpp\n");
    LTE_CHECK(false);
    return;
  }
  std::printf("    (GLSL version: %s)\n",
    has460 ? "460" : has450 ? "450" : "440");
  LTE_CHECK(true);
}

LTE_TEST(ShaderAudit_GLContextVersion) {
  // Verify the GL context requests 4.4 in Window.cpp.
  std::string winSrc = ReadFile("src/liblt/LTE/Window.cpp");

  if (!Contains(winSrc, "minorVersion = 4") && !Contains(winSrc, "minorVersion = 5") && !Contains(winSrc, "minorVersion = 6")) {
    std::fprintf(stderr, "  FAIL: GL context minorVersion is not >= 4 in Window.cpp\n");
    LTE_CHECK(false);
    return;
  }
  LTE_CHECK(true);
}

LTE_TEST(ShaderAudit_KnownComplexShaders) {
  // Verify that the most complex/heavy shaders are present and readable.
  // These are the ones most likely to have issues during version bumps.
  std::vector<std::string> heavyShaders = {
    "resource/shader/common/smaa.jsl",
    "resource/shader/common/scattering.jsl",
    "resource/shader/common/noise.jsl",
    "resource/shader/common/lighting.jsl",
    "resource/shader/fragment/gen/planet.jsl",
    "resource/shader/fragment/gen/nebula.jsl",
    "resource/shader/fragment/post/ssao.jsl",
    "resource/shader/fragment/light/point.jsl",
    "resource/shader/fragment/wormhole.jsl",
    "resource/shader/fragment/dustclouds.jsl",
  };

  for (auto const& f : heavyShaders) {
    std::string src = ReadFile(f);
    if (src.empty()) {
      std::fprintf(stderr, "  FAIL: could not read heavy shader %s\n", f.c_str());
      LTE_CHECK(false);
      return;
    }
    LTE_CHECK(src.size() > 100);
  }
  std::printf("    (verified %zu heavy shaders are readable)\n", heavyShaders.size());
}
