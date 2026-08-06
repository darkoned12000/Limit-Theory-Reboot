#include "Thread.h"

#include "AutoPtr.h"
#include "Job.h"
#include "Lock.h"

#include <atomic>
#include <chrono>
#include <thread>
#include "LTE/FunctionBind.h"

namespace {
  Lock GetThreadLock() {
    /* NOTE : Locks cannot be static destructed. */
    static Lock* lock = new Lock(Lock_Create());
    return *lock;
  }

  struct ThreadImpl : public ThreadT {
    Job job;
    std::thread thread;
    std::atomic<bool> finished;
    bool waited;

    ThreadImpl(Job const& job) :
      job(job),
      finished(false),
      waited(false)
    {
      ScopedLock lock(GetThreadLock());
      job->OnBegin();
      thread = std::thread(&ThreadImpl::Run, this);
    }

    ~ThreadImpl() override {
      if (thread.joinable()) {
        if (!finished.load(std::memory_order_acquire))
          thread.detach();
        else
          thread.join();
      }
      job->OnEnd();
    }

    Job GetJob() const override {
      return job;
    }

    bool IsFinished() const override {
      return finished.load(std::memory_order_acquire);
    }

    void Run() {
      job->OnRun(UINT_MAX);
      /* Publish the result (written by OnRun via the job's returnValue)
         before marking the thread finished, so a consumer that observes
         IsFinished() == true is guaranteed to also observe the result. */
      finished.store(true, std::memory_order_release);
    }

    void Terminate() override {
      ScopedLock lock(GetThreadLock());
      if (thread.joinable())
        thread.detach();
    }

    void Wait() override {
      if (!waited) {
        if (thread.joinable())
          thread.join();
        waited = true;
      }
    }
  };
}

Thread Thread_Create(Job const& job) {
  return new ThreadImpl(job);
}

void Thread_SleepMS(uint const& ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
static Function const Thread_SleepMS_Registration = Function_Bind(
  "Thread_SleepMS",
  "None",
  &Thread_SleepMS,
  "ms");



void Thread_SleepUS(uint const& us) {
  std::this_thread::sleep_for(std::chrono::microseconds(us));
}
static Function const Thread_SleepUS_Registration = Function_Bind(
  "Thread_SleepUS",
  "None",
  &Thread_SleepUS,
  "us");


