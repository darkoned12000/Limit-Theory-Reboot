#!/usr/bin/env python3
# Copyright (C) 2025  darkoned12000
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the ltheory-old-test modernization effort (Revamp Work).
# See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
#
# Regenerates src/liblt/LTE/DeclareFunction.h.
#
# The header defines the DeclareFunctionN / DeclareFunctionArgBindN macro
# families used by every script-API binding file. Run this script whenever a
# binding signature changes, then rebuild:
#
#   python3 script/meta/DeclareFunction.py [output_path] [--check]

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import *


def resolve_output():
  """Default output path is repo/src/liblt/LTE/DeclareFunction.h."""
  if len(sys.argv) > 1 and not sys.argv[1].startswith('--'):
    return sys.argv[1]
  here = os.path.dirname(os.path.abspath(__file__))
  return os.path.normpath(os.path.join(here, '..', '..', 'src', 'liblt', 'LTE',
                                       'DeclareFunction.h'))


def call_assign(params):
  """Return the RT-returning and void-returning CallAndAssign overloads.

  The void-returning overload never writes 'out'; the zero-arg overloads never
  read 'in' (and the zero-arg void overload reads neither). Mark those
  structurally-unused parameters [[maybe_unused]] so -Wunused-parameter stays
  clean for every instantiation.
  """
  template_args = ', '.join(['class RT'] + ['class T%i' % i for i in range(params)])
  fn_args = ', '.join(['T%i const&' % i for i in range(params)])
  call_args = ', '.join(['*(T%i*)in[%i]' % (i, i) for i in range(params)])

  in_attr = '[[maybe_unused]] ' if params == 0 else ''

  return [
    'template <%s>' % template_args,
    'inline void CallAndAssign(%svoid** in, void* out, RT (*fn)(%s)) {' %
      (in_attr, fn_args),
    '  *(RT*)out = fn(%s);' % call_args,
    '}',
    '',
    'template <%s>' % ', '.join(['class T%i' % i for i in range(params)]),
    'inline void CallAndAssign(%svoid** in, [[maybe_unused]] void* out, void (*fn)(%s)) {' %
      (in_attr, fn_args),
    '  fn(%s);' % call_args,
    '}',
    ''
  ]


def infer_metadata(params):
  """Return the Infer_MetaData overloads (metadata inference for bindings)."""
  if params == 0:
    return [
      'template <class RT>',
      'void Infer_MetaData(Function const& type, RT (*fn)()) {}',
      ''
    ]

  template_args = ', '.join(['class RT'] + ['class T%i' % i for i in range(params)])
  fn_args = ', '.join(['T%i const&' % i for i in range(params)])

  code = [
    'template <%s>' % template_args,
    'void Infer_MetaData(Function const& type, RT (*fn)(%s)) {' % fn_args,
    '  type->params = new Parameter[%i];' % params
  ]
  code += ['  Mutable(type->params[%i]).type = Type_Get<T%i>();' % (i, i)
           for i in range(params)]
  code += ['}', '']
  return code


def binding_body(params, macro_args):
  """Return the body of the DeclareFunctionN macro."""
  body = ['inline uint Name##_ParamCount() { return %i; }' % params]

  if params > 0:
    body += [
      'inline char const* Name##_ParamName(uint i) {',
      '  char const* const table[] = {%s};' %
        ', '.join(['#N%i' % i for i in range(params)]),
      '  return table[i];',
      '}'
    ]
  else:
    body += [
      'inline char const* Name##_ParamName([[maybe_unused]] uint i) { return 0; }'
    ]

  body += ['typedef ReturnType Name##_ReturnType;']
  body += ['typedef T%d Name##_ParamType%d;' % (i, i) for i in range(params)]
  body += ['']

  if params > 0:
    body += ['struct Name##_ArgRefs {']
    body += ['  T%d const& N%d;' % (i, i) for i in range(params)]
    body += [
      '  Name##_ArgRefs(%s) : %s {}' %
        (', '.join(['T%d const& N%d' % (i, i) for i in range(params)]),
         ', '.join(['N%d(N%d)' % (i, i) for i in range(params)])),
      '};',
      ''
    ]
    body += ['LT_API ReturnType Name(Name##_ArgRefs const&);']
    for n in ('Name', 'Name##_ExplicitCall'):
      body += [
        'inline ReturnType %s(%s) {' %
          (n, ', '.join(['T%d const& N%d' % (i, i) for i in range(params)])),
        '  return Name(Name##_ArgRefs(%s));' % ', '.join(['N%d' % i for i in range(params)]),
        '}'
      ]
  else:
    body += ['typedef int Name##_ArgRefs;']
    body += [
      'LT_API ReturnType Name(Name##_ArgRefs const&);',
      'inline ReturnType Name() { return Name(0); }',
      'inline ReturnType Name##_ExplicitCall() { return Name(0); }'
    ]

  body += [
    'inline void Name##_Call(void** in, void* out) {',
    '  CallAndAssign(in, out, Name##_ExplicitCall);',
    '}'
  ]
  return body


def arg_bind(params, macro_args):
  """Return the body of the DeclareFunctionArgBindN macro."""
  bind = ['DeclareFunction%d(Name, ReturnType, %s)' %
    (2 * params, ', '.join(['T%d, N%d' % (i, i) for i in range(params)]))]

  bind += [
    'AutoClass(Name##_Args, %s) ' %
      ', '.join(['T%d, N%d' % (i, i) for i in range(params)]),
    '  Name##_Args() {}',
    '  Name##_Args(Name##_ArgRefs const& args) : %s {}' %
      ', '.join(['N%d(args.N%d)' % (i, i) for i in range(params)]),
    '};'
  ]
  bind += [
    'inline ReturnType Name(Name##_Args const& args) {',
    '  return Name(Name##_ArgRefs(%s));' % ', '.join(['args.N%d' % i for i in range(params)]),
    '}'
  ]
  return bind


def main():
  max_params = 12
  code = []
  code += include('AutoClass')
  code += include('Function')
  code += include('Type')
  code += ['']

  code += [
    '#define _DeclareFunction(Name, ReturnType, %s, x, ...) \\'
      % ', '.join(['_%d' % i for i in range(2 * max_params)]),
    '  MACRO_IDENTITY(DeclareFunction##x(Name, ReturnType, %s))'
      % ', '.join(['_%d' % i for i in range(2 * max_params)]), '',
    '#define DeclareFunction(Name, ReturnType, ...) \\',
    '  MACRO_IDENTITY(_DeclareFunction(Name, ReturnType, __VA_ARGS__, %s))'
      % ', '.join(['%d' % (2 * max_params - i) for i in range(2 * max_params)]), '',

    '#define _DeclareFunctionArgBind(Name, ReturnType, %s, x, ...) \\'
      % ', '.join(['_%d' % i for i in range(2 * max_params)]),
    '  MACRO_IDENTITY(DeclareFunctionArgBind##x(Name, ReturnType, %s))'
      % ', '.join(['_%d' % i for i in range(2 * max_params)]), '',
    '#define DeclareFunctionArgBind(Name, ReturnType, ...) \\',
    '  MACRO_IDENTITY(_DeclareFunctionArgBind(Name, ReturnType, __VA_ARGS__, %s))'
      % ', '.join(['%d' % (2 * max_params - i) for i in range(2 * max_params)]), '',

    '#define DeclareFunctionNoParams(Name, ReturnType) \\',
    '  MACRO_IDENTITY(DeclareFunction0(Name, ReturnType))',

    '#define DefineFunction(Name) RegisterFunction(Name) Name##_ReturnType Name(Name##_ArgRefs const& args)', ''
  ]

  register = [
    'Function Name##_GetMetadata() {',
    '  static Function fn;',
    '  if (!fn) {',
    '    fn = Function_Create(#Name);',
    '    fn->description = "None";',
    '    fn->call = Name##_Call;',
    '    fn->paramCount = Name##_ParamCount();',
    '    fn->params = nullptr;',
    '    fn->returnType = Type_Get<Name##_ReturnType>();',
    '    Infer_MetaData(fn, &Name##_ExplicitCall);',
    '    for (uint i = 0; i < fn->paramCount; ++i)',
    '      Mutable(fn->params[i]).name = Name##_ParamName(i);',
    '  }',
    '  return fn;',
    '}',
    'static Function Name##_Metadata = Name##_GetMetadata();'
  ]
  code += macro('RegisterFunction', ['Name'], register)

  for params in range(0, max_params + 1):
    macro_args = ['Name', 'ReturnType']
    if params > 0:
      macro_args += [('%d' % i, 'N%d' % i) for i in range(params)]
      macro_args = ['Name', 'ReturnType'] + ['T%d' % i for i in range(params)] + \
                   ['N%d' % i for i in range(params)]

    code += call_assign(params, macro_args)
    code += infer_metadata(params)
    code += macro('DeclareFunction%d' % (2 * params), macro_args,
                  binding_body(params, macro_args))

    if params > 0:
      code += macro('DeclareFunctionArgBind%d' % (2 * params), macro_args,
                    arg_bind(params, macro_args))

  code = includeGuard(code, 'LTE_DeclareFunction')
  code = ['/* DeclareFunction.py ~ Automatically-generated code */', ''] + code

  output = resolve_output()
  contents = '\n'.join(code) + '\n'

  if '--check' in sys.argv:
    if os.path.exists(output) and open(output).read() == contents:
      print('DeclareFunction.h is up to date.')
      return 0
    print('DeclareFunction.h is out of date; run script/meta/DeclareFunction.py',
          file=sys.stderr)
    return 1

  with open(output, 'w') as f:
    f.write(contents)
  print('Wrote %s' % output)
  return 0


if __name__ == '__main__':
  sys.exit(main())
