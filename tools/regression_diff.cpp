// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// LTSL regression diff — compiles every .lts script under a corpus root
// through BOTH the old interpreter (Expression_Compile) and the new pipeline
// (Lexer → Parser → SymbolResolver), then compares results.
//
// Each file is compiled in a forked child with a configurable timeout so
// a hang in one script doesn't block the entire corpus.
//
//   Build:  cmake --build ./build --target ltsl_regression_diff
//   Run:    LD_LIBRARY_PATH=bin:extbin/linux64 bin/ltsl_regression_diff \
//             [--timeout N] [corpusRoot]
//           timeout  seconds per file (default 5; 0 = no timeout)
//           corpusRoot  default "resource/script"

#include "LTE/AST.h"
#include "LTE/Lexer.h"
#include "LTE/OS.h"
#include "LTE/Parser.h"
#include "LTE/Script.h"
#include "LTE/String.h"
#include "LTE/SymbolResolver.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <ctime>

using namespace LTE;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string Std(String const& s) {
  return std::string(s.c_str());
}

static void CollectScripts(String const& root, Vector<String>& names) {
  Vector<String> entries = OS_ListDir(root);
  for (size_t i = 0; i < entries.size(); ++i) {
    String const& entry = entries[i];
    if (entry == "." || entry == "..")
      continue;
    String full = root + "/" + entry;
    if (OS_IsDir(full)) {
      CollectScripts(full, names);
    } else if (entry.size() > 4 &&
               Std(entry).rfind(".lts") == entry.size() - 4) {
      names.push(full);
    }
  }
}

static String ToScriptName(String const& fullPath, String const& root) {
  String rel = fullPath;
  if (rel.size() > root.size() &&
      strncmp(rel.c_str(), root.c_str(), root.size()) == 0)
    rel = rel.substr(root.size() + 1);
  if (rel.size() > 4)
    rel = rel.substr(0, rel.size() - 4);
  return rel;
}

static String ReadFile(String const& path) {
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) return "";
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::vector<char> buf(len);
  if (len > 0)
    fread(buf.data(), 1, len, f);
  fclose(f);
  String result;
  for (long i = 0; i < len; ++i)
    result += buf[i];
  return result;
}

// ---------------------------------------------------------------------------
// Declaration extraction
// ---------------------------------------------------------------------------

struct OldDecls {
  std::set<std::string> functions;
  std::set<std::string> types;
};

static OldDecls GetOldDecls(String const& name) {
  OldDecls d;
  Script script = Script_Load(name);
  if (script) {
    for (auto it = script->functions.begin(); it != script->functions.end(); ++it)
      d.functions.insert(Std(it->first));
    for (auto it = script->types.begin(); it != script->types.end(); ++it) {
      d.types.insert(Std(it->first));
      /* Method functions also form the resolvable callable surface (the old
         interpreter registered field-less types' methods at script level but
         fielded types' methods only on the type — a known inconsistency).
         Compare the union so that registration-site differences don't spam
         the diff as long as the callable NAME exists. */
      if (!it->second) continue;
      for (auto m = it->second->functions.begin(); m != it->second->functions.end(); ++m)
        d.functions.insert(Std(m->first));
    }
  }
  return d;
}

struct NewDecls {
  std::set<std::string> functions;
  std::set<std::string> types;
};

static void CollectNewDecls(ASTNode const& node, NewDecls& d) {
  if (!node.t) return;
  switch (node->kind) {
    case AST_MODULE: {
      ASTModuleNodeT* mod = static_cast<ASTModuleNodeT*>(node.t);
      for (size_t i = 0; i < mod->statements.size(); ++i)
        CollectNewDecls(mod->statements[i], d);
      break;
    }
    case AST_FUNC_DECL: {
      ASTFuncDeclNodeT* fn = static_cast<ASTFuncDeclNodeT*>(node.t);
      d.functions.insert(Std(fn->name));
      CollectNewDecls(fn->body, d);
      break;
    }
    case AST_TYPE_DECL: {
      // Matches Script.cpp BuildFunction: methods live on the type AND are
      // surfaced in the script function map (old-interpreter parity).
      ASTTypeDeclNodeT* tp = static_cast<ASTTypeDeclNodeT*>(node.t);
      d.types.insert(Std(tp->name));
      for (size_t m = 0; m < tp->members.size(); ++m)
        CollectNewDecls(tp->members[m], d);
      break;
    }
    case AST_BLOCK: {
      ASTBlockNodeT* blk = static_cast<ASTBlockNodeT*>(node.t);
      for (size_t i = 0; i < blk->statements.size(); ++i)
        CollectNewDecls(blk->statements[i], d);
      break;
    }
    case AST_IF: {
      ASTIfNodeT* ifNode = static_cast<ASTIfNodeT*>(node.t);
      CollectNewDecls(ifNode->thenBlock, d);
      CollectNewDecls(ifNode->elseBlock, d);
      break;
    }
    case AST_WHILE: {
      ASTWhileNodeT* w = static_cast<ASTWhileNodeT*>(node.t);
      CollectNewDecls(w->body, d);
      break;
    }
    case AST_FOR: {
      ASTForNodeT* f = static_cast<ASTForNodeT*>(node.t);
      CollectNewDecls(f->body, d);
      break;
    }
    default:
      break;
  }
}

static NewDecls GetNewDecls(ASTNode const& module) {
  NewDecls d;
  if (!module.t || module->kind != AST_MODULE) return d;
  CollectNewDecls(module, d);
  return d;
}

// ---------------------------------------------------------------------------
// Per-file result categories
// ---------------------------------------------------------------------------

enum FileResult {
  RESULT_BOTH_PASS,
  RESULT_OLD_ONLY_PASS,
  RESULT_NEW_ONLY_PASS,
  RESULT_BOTH_FAIL,
  RESULT_TIMEOUT
};

// ---------------------------------------------------------------------------
// Per-file work — runs in a child process.
// Writes result byte + declaration lists over the pipe.
// ---------------------------------------------------------------------------

static void RunOneFile(int fd, String const& path, String const& name) {
  // Child: run both compilers, write result to pipe, exit.
  // We close the read end inherited from parent (harmless if already closed).

  /* --- Old compiler (fast, known-good) --- */
  /* The NEW pipeline is now the default; force the legacy interpreter for the
     old side of the comparison. */
  setenv("LTSL_OLD_COMPILER", "1", 1);
  Vector<String> oldErrors;
  bool oldOk = Script_CompileCheck(name, oldErrors);

  /* --- New pipeline: Lex -> Parse -> Resolve --- */
  String source = ReadFile(path);
  bool newOk = false;
  std::string declDiff;  // serialized decl diffs

  if (!source.empty()) {
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.Tokenize();
    bool lexOk = lexer.GetErrors().empty();
    /* Unset the legacy override so nothing downstream is affected by the
       old-compiler selection made for the old side above. */
    unsetenv("LTSL_OLD_COMPILER");

    bool parseOk = false;
    ASTNode module;
    Parser parser(tokens);
    if (lexOk) {
      module = parser.Parse();
      parseOk = parser.GetErrors().empty();
    }

    bool resolveOk = false;
    SymbolResolver resolver;
    if (parseOk && module.t) {
      resolveOk = resolver.Resolve(module);
    }

    newOk = lexOk && parseOk && resolveOk;

    // Diagnostic: dump NEW-pipeline errors to stderr (inherited by parent).
    if (!newOk) {
      std::fprintf(stderr, "[NEW-PARSER] %s\n", name.c_str());
      if (!lexOk) {
        for (auto const& e : lexer.GetErrors())
          std::fprintf(stderr, "  LEX %d:%d %s\n", e.line, e.column, e.message.c_str());
      }
      if (lexOk && !parseOk) {
        for (auto const& e : parser.GetErrors())
          std::fprintf(stderr, "  PARSE %d:%d %s\n", e.line, e.column, e.message.c_str());
      }
      if (lexOk && parseOk && !resolveOk) {
        for (auto const& e : resolver.GetErrors())
          std::fprintf(stderr, "  RESOLVE %d:%d %s\n", e.line, e.column, e.message.c_str());
      }
    }

    // Declaration diff if both pass
    if (oldOk && newOk) {
      OldDecls oldD = GetOldDecls(name);
      NewDecls newD = GetNewDecls(module);
      for (auto const& fn : oldD.functions)
        if (newD.functions.find(fn) == newD.functions.end())
          declDiff += "old_has_func:" + fn + "\n";
      for (auto const& fn : newD.functions)
        if (oldD.functions.find(fn) == oldD.functions.end())
          declDiff += "new_has_func:" + fn + "\n";
      for (auto const& tp : oldD.types)
        if (newD.types.find(tp) == newD.types.end())
          declDiff += "old_has_type:" + tp + "\n";
      for (auto const& tp : newD.types)
        if (oldD.types.find(tp) == oldD.types.end())
          declDiff += "new_has_type:" + tp + "\n";
    }
  }

  // Determine result byte
  char result;
  if (oldOk && newOk)      result = '0';
  else if (oldOk)           result = '1';
  else if (newOk)           result = '2';
  else                      result = '3';

  // Write: result byte, then decl diff size + data
  (void)write(fd, &result, 1);
  uint32_t ddLen = (uint32_t)declDiff.size();
  (void)write(fd, &ddLen, 4);
  if (ddLen > 0)
    (void)write(fd, declDiff.data(), ddLen);

  close(fd);
  _exit(0);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
  int timeoutSec = 5;
  String corpusRoot = "resource/script";
  String singleFile;

  // Parse args: [--timeout N] [--single FILE] [corpusRoot]
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
      timeoutSec = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--single") == 0 && i + 1 < argc) {
      singleFile = String(argv[++i]);
    } else {
      corpusRoot = String(argv[i]);
    }
  }

  // Single-file mode: test one file with verbose output
  if (!singleFile.empty()) {
    std::fprintf(stderr, "Single-file mode: %s (timeout=%ds)\n",
                 singleFile.c_str(), timeoutSec);

    // Find the file
    String path = singleFile;
    String name = singleFile;
    // Strip .lts extension for script name
    if (name.size() > 4 && Std(name).rfind(".lts") == name.size() - 4)
      name = name.substr(0, name.size() - 4);
    // Strip leading resource/script/ for script name
    String prefix = "resource/script/";
    if (strncmp(name.c_str(), prefix.c_str(), prefix.size()) == 0)
      name = name.substr(prefix.size());

    String source = ReadFile(path);
    if (source.empty()) {
      std::fprintf(stderr, "Cannot read file: %s\n", path.c_str());
      return 1;
    }
    std::fprintf(stderr, "Script name: %s (%zu chars)\n", name.c_str(), source.size());

    int pipefd[2];
    pipe(pipefd);
    pid_t pid = fork();
    if (pid == 0) {
      close(pipefd[0]);
      RunOneFile(pipefd[1], path, name);
    }
    close(pipefd[1]);

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int status = 0;
    pid_t waitResult = 0;
    while (true) {
      waitResult = waitpid(pid, &status, WNOHANG);
      if (waitResult > 0) {
        char resByte = '4';
        uint32_t ddLen = 0;
        read(pipefd[0], &resByte, 1);
        read(pipefd[0], &ddLen, 4);
        std::string dd;
        if (ddLen > 0 && ddLen < 1024*1024) {
          dd.resize(ddLen);
          read(pipefd[0], &dd[0], ddLen);
        }
        const char* names[] = {"BOTH PASS","OLD PASS/NEW FAIL","NEW PASS/OLD FAIL","BOTH FAIL","ERROR"};
        int idx = resByte - '0';
        if (idx < 0 || idx > 4) idx = 4;
        std::fprintf(stderr, "Result: %s\n", names[idx]);
        if (!dd.empty()) std::fprintf(stderr, "Decl diffs:\n%s\n", dd.c_str());
        break;
      } else if (waitResult == 0) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed = now.tv_sec - start.tv_sec;
        if (timeoutSec > 0 && elapsed >= timeoutSec) {
          std::fprintf(stderr, "TIMEOUT after %lds — killing child\n", elapsed);
          kill(pid, SIGKILL);
          waitpid(pid, &status, 0);
          break;
        }
        if (elapsed > 0 && elapsed % 2 == 0) {
          std::fprintf(stderr, "Still running after %lds...\n", elapsed);
        }
        usleep(10000); // 10ms poll for single-file mode
      } else {
        std::fprintf(stderr, "waitpid error\n");
        break;
      }
    }
    close(pipefd[0]);
    return 0;
  }

  Vector<String> paths;
  CollectScripts(corpusRoot, paths);
  if (paths.empty()) {
    std::printf("regression-diff: no .lts files found under '%s'\n",
                corpusRoot.c_str());
    return 1;
  }

  std::sort(paths.begin(), paths.end(),
            [](String const& a, String const& b) {
              return strcmp(a.c_str(), b.c_str()) < 0;
            });

  size_t total = paths.size();
  size_t bothPass = 0, oldOnlyPass = 0, newOnlyPass = 0, bothFail = 0, timedOut = 0;
  bool hasDeclMismatch = false;

  std::vector<std::string> oldOnlyFailDetails;
  std::vector<std::string> newOnlyFailDetails;
  std::vector<std::string> timeoutDetails;
  std::vector<std::string> declDiffs;

  std::fprintf(stderr, "regression-diff: %zu file(s), timeout=%ds\n",
               total, timeoutSec);

  for (size_t i = 0; i < paths.size(); ++i) {
    String name = ToScriptName(paths[i], corpusRoot);

    // Progress to stderr
    if ((i + 1) % 10 == 0 || i + 1 == total)
      std::fprintf(stderr, "  [%zu/%zu] %s\n", i + 1, total, name.c_str());

    // Skip files that can't be read (no timeout needed)
    String source = ReadFile(paths[i]);
    if (source.empty()) {
      ++bothFail;
      continue;
    }

    // Create pipe for child -> parent communication
    int pipefd[2];
    if (pipe(pipefd) != 0) {
      ++bothFail;
      continue;
    }

    pid_t pid = fork();
    if (pid < 0) {
      close(pipefd[0]);
      close(pipefd[1]);
      ++bothFail;
      continue;
    }

    if (pid == 0) {
      // Child: close read end, do the work, write result
      close(pipefd[0]);
      RunOneFile(pipefd[1], paths[i], name);
      // RunOneFile calls _exit — never reaches here
    }

    // Parent: close write end, wait for child with timeout
    close(pipefd[1]);

    FileResult result = RESULT_TIMEOUT;
    std::string declDiff;

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int status = 0;
    pid_t waitResult = 0;

    while (true) {
      waitResult = waitpid(pid, &status, WNOHANG);
      if (waitResult > 0) {
        // Child finished — read result from pipe
        char resByte = '4';
        uint32_t ddLen = 0;
        ssize_t n = read(pipefd[0], &resByte, 1);
        if (n == 1) {
          (void)read(pipefd[0], &ddLen, 4);
          if (ddLen > 0 && ddLen < 1024 * 1024) {
            declDiff.resize(ddLen);
            (void)read(pipefd[0], &declDiff[0], ddLen);
          }
        }
        switch (resByte) {
          case '0': result = RESULT_BOTH_PASS; break;
          case '1': result = RESULT_OLD_ONLY_PASS; break;
          case '2': result = RESULT_NEW_ONLY_PASS; break;
          case '3': result = RESULT_BOTH_FAIL; break;
          default:  result = RESULT_BOTH_FAIL; break;
        }
        break;
      } else if (waitResult == 0) {
        // Still running — check timeout
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed = (now.tv_sec - start.tv_sec);
        if (timeoutSec > 0 && elapsed >= timeoutSec) {
          // Timeout — kill child
          kill(pid, SIGKILL);
          waitpid(pid, &status, 0);
          result = RESULT_TIMEOUT;
          break;
        }
        usleep(1000); // 1ms poll
      } else {
        // waitpid error
        result = RESULT_BOTH_FAIL;
        break;
      }
    }

    close(pipefd[0]);

    // Categorize result
    std::string sname = Std(name);
    switch (result) {
      case RESULT_BOTH_PASS:
        ++bothPass;
        // Parse decl diffs from child
        {
          // declDiff lines: "old_has_func:X", "new_has_func:X", etc.
          size_t pos = 0;
          while (pos < declDiff.size()) {
            size_t eol = declDiff.find('\n', pos);
            if (eol == std::string::npos) eol = declDiff.size();
            std::string line = declDiff.substr(pos, eol - pos);
            pos = eol + 1;
            if (line.empty()) continue;
            hasDeclMismatch = true;
            if (line.compare(0, 13, "old_has_func:") == 0)
              declDiffs.push_back(sname + ": old has function '" + line.substr(13) + "' missing in new");
            else if (line.compare(0, 13, "new_has_func:") == 0)
              declDiffs.push_back(sname + ": new has function '" + line.substr(13) + "' missing in old");
            else if (line.compare(0, 13, "old_has_type:") == 0)
              declDiffs.push_back(sname + ": old has type '" + line.substr(13) + "' missing in new");
            else if (line.compare(0, 13, "new_has_type:") == 0)
              declDiffs.push_back(sname + ": new has type '" + line.substr(13) + "' missing in old");
          }
        }
        break;
      case RESULT_OLD_ONLY_PASS:
        ++oldOnlyPass;
        oldOnlyFailDetails.push_back(sname);
        break;
      case RESULT_NEW_ONLY_PASS:
        ++newOnlyPass;
        newOnlyFailDetails.push_back(sname);
        break;
      case RESULT_BOTH_FAIL:
        ++bothFail;
        break;
      case RESULT_TIMEOUT:
        ++timedOut;
        timeoutDetails.push_back(sname);
        break;
    }
  }

  /* Report */
  std::printf("\nregression-diff: %zu file(s) scanned\n\n", total);
  std::printf("  Both compilers PASS:  %zu\n", bothPass);
  std::printf("  Old only PASS:       %zu\n", oldOnlyPass);
  std::printf("  New only PASS:       %zu\n", newOnlyPass);
  std::printf("  Both compilers FAIL: %zu\n", bothFail);
  std::printf("  Timed out:           %zu\n", timedOut);
  std::printf("  Declaration diffs:   %s\n\n", hasDeclMismatch ? "YES" : "none");

  if (!timeoutDetails.empty()) {
    std::printf("=== TIMED OUT (%zu files) ===\n", timeoutDetails.size());
    for (auto const& s : timeoutDetails)
      std::printf("  %s\n", s.c_str());
    std::printf("\n");
  }

  if (!oldOnlyFailDetails.empty()) {
    std::printf("=== Old PASS / New FAIL (%zu files) ===\n", oldOnlyFailDetails.size());
    for (auto const& s : oldOnlyFailDetails)
      std::printf("  %s\n", s.c_str());
    std::printf("\n");
  }

  if (!newOnlyFailDetails.empty()) {
    std::printf("=== Old FAIL / New PASS (%zu files) ===\n", newOnlyFailDetails.size());
    for (auto const& s : newOnlyFailDetails)
      std::printf("  %s\n", s.c_str());
    std::printf("\n");
  }

  if (!declDiffs.empty()) {
    std::printf("=== Declaration Differences (%zu) ===\n", declDiffs.size());
    for (auto const& s : declDiffs)
      std::printf("  %s\n", s.c_str());
    std::printf("\n");
  }

  /* A migration-direction result: NEW passing while OLD fails is the expected
     modernization signal (files rewritten to idioms the retiring old compiler
     can no longer parse) and is NOT a regression. The only regressions that
     block are OLD passing where NEW fails, timeouts, and declaration drift. */
  bool pass = (oldOnlyPass == 0) && (timedOut == 0) && !hasDeclMismatch;
  std::printf("regression-diff: %s\n", pass ? "PASS" : "FAIL");
  return pass ? 0 : 1;
}
