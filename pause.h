#pragma once
#ifndef PAUSE_H
#define PAUSE_H

#include <assert.h>

inline void pause(void) {
    #ifdef _MSC_VER
        __debugbreak();
    #else
        assert(0);
    #endif
}

#endif
