#pragma once

#include <map>
#include <thread>
#include <mutex>
#include <unordered_map>

#include "com/com_include.h"

#define REF_CREATE(ptr, type) refTracker()->create(ptr, type, typeid(*ptr).name())
#define REF_BUMP(ptr, count)  refTracker()->ref(ptr, typeid(*ptr).name(), count)
#define REF_DESTROY(ptr)      refTracker()->destroy(ptr)

namespace dxvk {

  class RefTracker {
  public:
    struct Ref {
        const char *name = nullptr;
        std::thread::id create_thread;
        unsigned short type  = 0;
        ULONG ref_count      = 0;
        ULONG unknown_origin = 0;
    };
    using RefMap = std::unordered_map<void*, Ref>;

    RefTracker();
    ~RefTracker();

    void create(void *ptr, int type, const char *name);
    void destroy(void *ptr);
    void ref(void *ptr, const char *type, ULONG refs);

    void dump() const;
    
    void move_to(RefTracker &tgt);

  private:
    RefTracker(const RefTracker &) = delete;
    RefTracker &operator=(RefTracker &) = delete;

    mutable std::mutex mtx;
    RefMap     rm;
  };


  std::shared_ptr<RefTracker> &refTracker();
  void linkRefTracker(std::shared_ptr<RefTracker> &tracker);

}
