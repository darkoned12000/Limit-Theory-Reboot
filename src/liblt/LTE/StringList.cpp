// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "StringList.h"
#include "Location.h"
#include "Tokenizer.h"

#include <iostream>

namespace {
  String const kWhitespace = " \t\n\r";
  String const kTab = " ";
  String const kDelim = " ";
  const char kScopeOpen = '(';
  const char kScopeClose = ')';

  StringList StringList_ParseLine(String const& line, uint32_t lineNum) {
    Vector<StringList> elements;
    size_t i = 0;

    while (i < line.size()) {
      uint level = 0;
      String token;
      bool literal = false;
      bool escaped = false;

      /* Remove whitespace padding. */ {
        while (i < line.size() && kWhitespace.contains(line[i]))
          i++;
      }

      while (i < line.size()) {
        char c = line[i++];
        if (c == '\"' && !escaped)
          literal = !literal;

        if (escaped)
          escaped = false;

        if (literal) {
          if (c == '\\')
            escaped = true;
        } else {
          if (c == kScopeOpen)
            level++;
          if (c == kScopeClose) {
            level--;
            if (level == 0)
              break;
          }

          if (level == 0 && kDelim.contains(c))
            break;
        }

        token += c;
      }

      if (token.size()) {
        if (token.front() == kScopeOpen) {
          elements.push(StringList_ParseLine(token.substr(1), lineNum));
        } else
          elements.push(new StringListAtom(token, lineNum));
      }
    }

    return new StringListList(elements);
  }

  StringList StringList_ParseBlock(Tokenizer& tokenizer, uint indent, uint32_t& currentLine) {
    Vector<StringList> elements;
    Vector<StringList> current;

    while (tokenizer.HasMore()) {
      current.clear();
      size_t cursor = tokenizer.GetCursor();
      String line = tokenizer.ReadLine();
      if (line.containsOnly(kWhitespace)) {
        currentLine++;
        continue;
      }

      uint thisIndent = Tokenizer::GetIndent(line, kTab);

      /* Comment line — skip it and its indented block. Strip here at the
         source rather than relying on the head-`#` checks in
         Expression_Compile: comment text containing a binary operator (e.g.
         `# * foo`) was rewritten into `(* # foo)` before the `#` check ran,
         so the comment leaked into the parser as code and produced spurious
         diagnostics (ltsl-hardening.md §5.1). A `#` line comments everything
         to end of line, and — matching the original Expression_Compile `#`
         block semantics — every deeper-indented line beneath it too (e.g.
         `# desc "X"` dead-blocks in the original apps). Lines still count so
         diagnostics on following statements keep their file line numbers. */ {
        if (thisIndent < line.size() && line[thisIndent] == '#') {
          uint commentIndent = thisIndent;
          currentLine++;
          while (tokenizer.HasMore()) {
            size_t cursor2 = tokenizer.GetCursor();
            String next = tokenizer.ReadLine();
            if (next.containsOnly(kWhitespace)) {
              currentLine++;
              continue;
            }
            uint nextIndent = Tokenizer::GetIndent(next, kTab);
            if (nextIndent > commentIndent) {
              currentLine++;
              continue;
            }
            tokenizer.SetCursor(cursor2);
            break;
          }
          continue;
        }
      }

      if (thisIndent < indent) {
        tokenizer.SetCursor(cursor);
        break;
      }

      currentLine++;

      /* Parse the current line. */ {
        StringList list = StringList_ParseLine(line, currentLine);
        StringListList* l = (StringListList*)list.t;
        for (size_t i = 0; i < l->elements.size(); ++i)
          current.push(l->elements[i]);
      }

      /* Parse inner block. */ {
        size_t cursor2 = tokenizer.GetCursor();
        String nextLine = tokenizer.ReadLine();
        uint nextIndent = Tokenizer::GetIndent(nextLine, kTab);
        tokenizer.SetCursor(cursor2);

        if (nextIndent > indent) {
          StringList list = StringList_ParseBlock(tokenizer, nextIndent, currentLine);
          StringListList* l = (StringListList*)list.t;
          for (size_t i = 0; i < l->elements.size(); ++i)
            current.push(l->elements[i]);
        }
      }

      if (current.size() == 1 && current[0]->IsAtom())
        elements.push(current[0]);
      else
        elements.push(new StringListList(current));
    }
    return new StringListList(elements);
  }
}

StringList StringList_Create(String const& source) {
  Tokenizer tokenizer(source);
  uint32_t currentLine = 0;
  return StringList_ParseBlock(tokenizer, 0, currentLine);
}

StringList StringList_Load(Location const& location) {
  return StringList_Create(location->ReadAscii());
}

void StringList_Print(StringList const& list) {
  if (list->IsAtom())
    std::cout << ((StringListAtom*)list.t)->value;
  else {
    StringListList* l = (StringListList*)list.t;
    std::cout << "(";
    for (size_t i = 0; i < l->elements.size(); ++i) {
      if (i) std::cout << " ";
      StringList_Print(l->elements[i]);
    }
    std::cout << ")";
  }
}
