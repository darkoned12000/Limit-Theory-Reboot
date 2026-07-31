// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// LTSL API database generator. Links against liblt.so, iterates the engine's
// reflection registries (Function_GetList / Type_GetList) and writes a single
// api-database.json consumed by the LTSL language server. Because the data is
// generated from the engine itself it can never drift from the runtime.
//
//   Build:  python3 configure.py build  (target: ltsl_api_dump -> bin/ltsl_api_dump)
//   Run:    LD_LIBRARY_PATH=bin:extbin/linux64 bin/ltsl_api_dump [out.json]
//
// The JSON schema matches the LSP implementation guide
// (docs/LTSL-LSP-IMPLEMENTATION-GUIDE.md):
//   functions[]: name, signature, returnType, parameters[], documentation, source
//   types[]:     name, description, fields[], methods[], documentation

#include "LTE/Field.h"
#include "LTE/Function.h"
#include "LTE/Type.h"
#include "LTE/Vector.h"

#include <cstdio>
#include <string>

static std::string JsonEscape(String const& s) {
  std::string out;
  for (char const* p = s.c_str(); *p; ++p) {
    unsigned char c = (unsigned char)*p;
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if (c < 0x20) {
          char buf[8];
          snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out += (char)c;
        }
    }
  }
  return out;
}

static std::string TypeRef(Type const& t) {
  if (!t)
    return "void";
  return JsonEscape(t->GetAliasName());
}

static void EmitFunction(FILE* out, String const& displayName, Function const& fn) {
  fprintf(out, "    {\n");
  fprintf(out, "      \"name\": \"%s\",\n", JsonEscape(displayName).c_str());
  fprintf(out, "      \"signature\": \"%s\",\n",
    JsonEscape(fn->GetSignature()).c_str());
  fprintf(out, "      \"returnType\": \"%s\",\n", TypeRef(fn->returnType).c_str());
  fprintf(out, "      \"parameters\": [");
  for (uint p = 0; p < fn->paramCount; ++p) {
    fprintf(out, "%s\n        {\"name\": \"%s\", \"type\": \"%s\"}",
      p ? "," : "",
      JsonEscape(fn->params[p].name).c_str(),
      TypeRef(fn->params[p].type).c_str());
  }
  fprintf(out, "%s\n      ],\n", fn->paramCount ? "" : "");
  fprintf(out, "      \"documentation\": \"%s\",\n",
    JsonEscape(fn->description).c_str());
  fprintf(out, "      \"source\": \"\"\n");
  fprintf(out, "    }");
}

struct FunctionDumpContext {
  FILE* out;
  uint emitted;
};

static void EmitFunctionByName(
  void* user, String const& name, Vector<Function> const& functions)
{
  FunctionDumpContext* ctx = (FunctionDumpContext*)user;
  for (uint i = 0; i < functions.size(); ++i) {
    Function const& fn = functions[i];
    if (!fn || !fn->returnType)
      continue;
    if (ctx->emitted++)
      fprintf(ctx->out, ",\n");
    EmitFunction(ctx->out, name, fn);
  }
}

int main(int argc, char** argv) {
  LTE_Initialize();

  std::string const outPath =
    argc > 1 ? argv[1] : "api-database.json";

  FILE* out = fopen(outPath.c_str(), "w");
  if (!out) {
    fprintf(stderr, "ltsl_api_dump: cannot open '%s'\n", outPath.c_str());
    return 1;
  }

  uint emittedFunctions = 0;
  fprintf(out, "{\n  \"functions\": [\n");
  FunctionDumpContext ctx = { out, 0 };
  Function_ForEach(&ctx, EmitFunctionByName);
  emittedFunctions = ctx.emitted;
  fprintf(out, "\n  ],\n");

  uint emittedTypes = 0;
  fprintf(out, "  \"types\": [\n");
  Vector<Type> const& types = Type_GetList();
  for (uint i = 0; i < types.size(); ++i) {
    Type const& t = types[i];
    if (!t)
      continue;
    if (emittedTypes++)
      fprintf(out, ",\n");

    fprintf(out, "    {\n");
    fprintf(out, "      \"name\": \"%s\",\n", JsonEscape(t->GetAliasName()).c_str());
    fprintf(out, "      \"description\": \"%s\",\n",
      t->base ? JsonEscape(t->base->GetAliasName()).c_str() : "");

    fprintf(out, "      \"fields\": [");
    Vector<Field> const& fields = t->GetFields();
    for (uint f = 0; f < fields.size(); ++f) {
      fprintf(out, "%s\n        {\"name\": \"%s\", \"type\": \"%s\"}",
        f ? "," : "",
        JsonEscape(fields[f].name).c_str(),
        TypeRef(fields[f].type).c_str());
    }
    fprintf(out, "%s\n      ],\n", fields.size() ? "" : "");

    fprintf(out, "      \"methods\": [");
    Vector<Function> const& methods = t->GetFunctions();
    for (uint m = 0; m < methods.size(); ++m) {
      if (m)
        fprintf(out, ",\n");
      else
        fprintf(out, "\n");
      EmitFunction(out, methods[m]->name, methods[m]);
    }
    fprintf(out, "%s\n      ],\n", methods.size() ? "" : "");

    fprintf(out, "      \"documentation\": \"\"\n");
    fprintf(out, "    }");
  }

  fprintf(out, "\n  ]\n}\n");
  fclose(out);

  printf("ltsl_api_dump: wrote %s (%u functions, %u types)\n",
    outPath.c_str(), emittedFunctions, emittedTypes);
  return 0;
}
