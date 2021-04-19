///////////////////////////////////////////////////////////////////////////////
// Copyright Lewis Baker, Corentin Jabot
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0.
// (See accompanying file LICENSE or http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////
#ifndef CHECK_HPP_INCLUDED
#define CHECK_HPP_INCLUDED

#pragma once

#include <cstdio>
#include <cstdlib>

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define CHECK(X) \
    do { \
        if (X) ; else { \
            ::std::puts("FAIL: " __FILE__ "(" STRINGIFY(__LINE__) ") '" #X "' was not true"); \
            ::std::abort(); \
        } \
    } while (false)

#define RUN(X) \
    do { \
        std::puts("-> " #X); \
        std::fflush(stdout); \
        X(); \
    } while (false)

#endif
