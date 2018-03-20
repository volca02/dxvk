#pragma once

#include <atomic>

#include "com_include.h"

#include "../log/log.h"
#include "../util_string.h"
#include "../util_reftracker.h"

#define COM_QUERY_IFACE(riid, ppvObject, Iface) \
  do {                                          \
    if (riid == __uuidof(Iface)) {              \
      this->AddRef();                           \
      *ppvObject = static_cast<Iface*>(this);   \
      return S_OK;                              \
    }                                           \
  } while (0)
  
namespace dxvk {
  
  template<typename... Base>
  class ComObject : public Base... {
    
  public:
    
    virtual ~ComObject() {
      REF_DESTROY(this);
    }

    ComObject() {
      REF_CREATE(this, 2);
    }

    ULONG STDMETHODCALLTYPE AddRef() {
      REF_BUMP(this, m_refCount + 1);
      return ++m_refCount;
    }
    
    ULONG STDMETHODCALLTYPE Release() {
      ULONG refCount = --m_refCount;
      if (refCount == 0) {
        delete this;
      } else {
        REF_BUMP(this, refCount);
      }
      return refCount;
    }
    
  private:
    
    std::atomic<ULONG> m_refCount = { 0ul };
    
  };
  
}
