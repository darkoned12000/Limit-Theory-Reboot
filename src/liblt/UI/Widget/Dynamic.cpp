#include "../Widgets.h"

#include "LTE/AutoClass.h"
#include "LTE/Pool.h"
#include "LTE/HashMap.h"

#include "UI/Widget.h"

#include "LTE/Debug.h"

namespace {
  using ChildMapT = HashMap<HashT, Widget>;

  AutoClassDerived(WidgetDynamic, WidgetComponentT,
    ChildMapT, childrenMap)
    DERIVED_TYPE_EX(WidgetDynamic)
    POOLED_TYPE

    WidgetDynamic() = default;

    void PreUpdate(Widget const& self) override {
      for (size_t i = 0; i < self->children.size(); ++i)
        self->children[i]->visited = false;

      /* Dynamically map new children. */ {
        Vector<Widget> newChildren;
        self->CreateChildren(newChildren);

        for (size_t i = 0; i < newChildren.size(); ++i) {
          HashT hash = newChildren[i]->GetHash();

          Widget& child = childrenMap[hash];
          if (child)
            child->visited = true;
          else {
            child = newChildren[i];
            self->AddChild(child);
          }
        }
      }

      /* Delete orphaned or invalidated children. */ {
        for (int i = 0; i < static_cast<int>(self->children.size()); ++i) {
          Widget const& child = self->children[i];
          if (!child->visited) {
            childrenMap.erase(child->GetHash());
            child->Clear();
            self->children.eraseIndex(i);
            i--;
          }
        }

        for (size_t i = 0; i < self->children.size(); ++i)
          childrenMap[self->children[i]->GetHash()] = self->children[i];
      }
    }
  };
}

DefineFunction(Widget_Dynamic) {
  args.widget->Add(new WidgetDynamic());
  return args.widget;
} FunctionAlias(Widget_Dynamic, Dynamic);
