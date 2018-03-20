#include <memory>
#include <cxxabi.h>

#include "util_reftracker.h"

#include "log/log.h"
#include "util_string.h"

namespace dxvk {
  static std::shared_ptr<RefTracker> s_refTracker(new RefTracker);

  struct ExitDump {
    ExitDump() {}
    ~ExitDump() {
       s_refTracker->dump();
    }
  };

  static ExitDump exitDump;

  RefTracker::RefTracker() {
  }

  RefTracker::~RefTracker() {
  }

  void RefTracker::create(void *ptr, int type, const char *name) {
      auto tid = std::this_thread::get_id();
      std::lock_guard<std::mutex> sl(mtx);
      auto &rec = rm[ptr];
      rec.name = name;
      rec.type = type;
      rec.create_thread = tid;
      rec.ref_count = 0;
      rec.unknown_origin = 0;
  }

  void RefTracker::ref(void *ptr, const char *name, ULONG refs) {
      std::lock_guard<std::mutex> sl(mtx);
      auto &rec = rm[ptr];
      rec.ref_count = refs;
      // this checks if we in fact saw the creation of this object
      if (!rec.name) {
          rec.unknown_origin = 1;
          rec.name = name;
      }
  }

  void RefTracker::destroy(void *ptr) {
      std::lock_guard<std::mutex> sl(mtx);
      rm.erase(ptr);
  }

  void RefTracker::dump() const {
    std::lock_guard<std::mutex> sl(mtx);
    if (rm.empty())
        return;

    Logger::info("=== REACHABLE OBJECT LIST ===");
    for (const auto &ptr : rm) {
      int status = -4;
      std::unique_ptr<char, void(*)(void*)> res {
          abi::__cxa_demangle(ptr.second.name, NULL, NULL, &status),
              std::free
              };

      Logger::info(
        str::format("R ptr=", str::hex(ptr.first),
                    " name='", status ? "UNKNOWN" : res.get(), "'"
                    " type=", ptr.second.type,
                    " ref_count=", ptr.second.ref_count,
                    " unknown=", ptr.second.unknown_origin));
    }
    Logger::info("=== REACHABLE OBJECT LIST END ===");
  }

  void RefTracker::move_to(RefTracker &target) {
      Logger::info("=== REF TRACKER MOVE ===");
      std::lock_guard<std::mutex> slt(target.mtx);
      std::lock_guard<std::mutex> sl(mtx);

      for (const auto &ptr : rm) {
          auto it = target.rm.find(ptr.first);
          if (it == target.rm.end()) {
              // missing records are added
              target.rm[ptr.first] = ptr.second;
          } else {
              it->second.name = ptr.second.name;
              it->second.unknown_origin = 0; // fixed
          }
      }
      rm.clear();
      Logger::info("=== REF TRACKER MOVE END ===");
  }


  std::shared_ptr<RefTracker> &refTracker() {
      return s_refTracker;
  }

  void linkRefTracker(std::shared_ptr<RefTracker> &tracker) {
      if (tracker == s_refTracker)
          return;

      s_refTracker->move_to(*tracker);
      s_refTracker = tracker;
  }

}
