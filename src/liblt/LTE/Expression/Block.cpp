#include "../Expressions.h"

#include "LTE/AutoClass.h"
#include "LTE/Environment.h"
#include "LTE/Pool.h"
#include "LTE/StringList.h"
#include "LTE/Vector.h"

#include "LTE/Debug.h"

namespace {
  AutoClassDerived(ExpressionBlock, ExpressionT,
    Vector<Expression>, expressions,
    Array<Type>, types,
    Array<Type>, locals)
    DERIVED_TYPE_EX(ExpressionBlock)
    POOLED_TYPE

    ExpressionBlock() = default;

    ExpressionBlock(
        Vector<Expression> const& expressions,
        Vector<Type> const& locals) :
      expressions(expressions)
    {
      this->types.resize(expressions.size());
      this->locals.resize(locals.size());
      for (size_t i = 0; i < expressions.size(); ++i)
        this->types[i] = expressions[i]->GetType();
      for (size_t i = 0; i < locals.size(); ++i)
        this->locals[i] = locals[i];
    }

    String Emit(Vector<String>& context) const override {
      context.push("{");
      Vector<String> localContext;
      String value;
      for (size_t i = 0; i < expressions.size(); ++i) {
        value = expressions[i]->Emit(localContext);
        if (value.size())
          localContext.push(value + ";");
      }

      for (size_t i = 0; i < localContext.size(); ++i)
        context.push("  " + localContext[i]);
      context.push("}");
      return value;
    }

    void Evaluate(void* returnValue, Environment& env) const override {
      uint base = env.registers.size();
      for (size_t i = 0; i < expressions.size(); ++i) {
        if (i + 1 == expressions.size())
          expressions[i]->Evaluate(returnValue, env);
        else {
          if (types[i]->allocate) {
            void* lv = env.Allocate(types[i]);
            expressions[i]->Evaluate(lv, env);
            env.Free(types[i], lv);
          } else {
            expressions[i]->Evaluate(0, env);
          }
        }

        /* A `return` anywhere in the block (or a nested block) sets this
           flag; stop evaluating subsequent expressions but still run the
           local-destruction loop below so registers stay balanced. Locals
           declared after the `return` were never pushed, so the loop must
           only destruct the registers that this block actually pushed. */
        if (env.returnSignal)
          break;
      }

      /* Destruct all locals that were constructed in this scope. */ {
        uint count = env.registers.size() - base;
        if (count > locals.size())
          count = locals.size();
        for (size_t i = 0; i < count; ++i) {
          Type const& local = locals[count - (i + 1)];
          if (local)
            env.Free(local, env.registers.back());
          env.registers.pop();
        }
      }
    }

    Type GetType() const override {
      return expressions[expressions.size() - 1]->GetType();
    }

    bool IsConstant(CompileEnvironment& env) const override {
      for (size_t i = 0; i < expressions.size(); ++i)
        if (!expressions[i]->IsConstant(env))
          return false;
      return true;
    }
  };
}

namespace LTE {
  Expression Expression_Block(
    Vector<Expression> const& expressions,
    Vector<Type> const& locals)
  {
    return new ExpressionBlock(expressions, locals);
  }

  Expression Expression_Block(
    StringList const& list,
    CompileEnvironment& env,
    uint startIndex)
  {
    Vector<Expression> expressions;
    Vector<String> localNames;

    for (size_t i = startIndex; i < list->GetSize(); ++i) {
      Expression e = Expression_Compile(list->Get(i), env, &localNames);
      if (e)
        expressions.push(e);
    }

    Vector<Type> locals;
    for (size_t i = 0; i < localNames.size(); ++i) {
      String const& name = localNames[i];
      Variable const& var = env.Get(name);
      locals.push(var.reference ? nullptr : var.type);
      env.Free(name);
    }

    if (locals.size())
      return Expression_Block(expressions, locals);

    if (expressions.size() == 0)
      return nullptr;
    else if (expressions.size() == 1)
      return expressions[0];
    else
      return Expression_Block(expressions, locals);
  }
}
