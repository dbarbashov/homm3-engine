#pragma once
#include "mousemgr.h"
#include "kb.h"

// A server decision can arrive while the original route walker has hidden
// the pointer. Temporarily release that outer hide only for the modal UI.
class ScopedPresentationPointer {
    int hidden;
public:
    ScopedPresentationPointer():hidden(g_mouseManager->m_hideCount){g_mouseManager->showPointer(true);}
    ~ScopedPresentationPointer(){if(hidden){g_mouseManager->hidePointer();g_mouseManager->m_hideCount=hidden;}}
    ScopedPresentationPointer(const ScopedPresentationPointer&)=delete;
    ScopedPresentationPointer& operator=(const ScopedPresentationPointer&)=delete;
};
