#ifndef Attribute_Task_h__
#define Attribute_Task_h__

#include "Common.h"

template <class T>
struct Attribute_Task : public T {
  using SelfType = Attribute_Task;
  ATTRIBUTE_COMMON(task)
  Task task;

  Attribute_Task() = default;

  Task const& GetTask() const override {
    return task;
  }

  bool HasTask() const override {
    return true;
  }
};

#endif
