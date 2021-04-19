///////////////////////////////////////////////////////////////////////////////
// Copyright Lewis Baker, Corentin Jabot
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0.
// (See accompanying file LICENSE or http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////
#include <generator>
#include <string>
#include <type_traits>

#include "check.hpp"

// Check some basic properties of the type at compile-time.

// A generator should be a 'range'
static_assert(std::ranges::range<std::generator<int>>);

// A generator should also be a 'view'
static_assert(std::ranges::view<std::generator<int>>);

// 
static_assert(std::is_same_v<
    std::ranges::range_reference_t<std::generator<const std::string&>>,
    const std::string&>);
static_assert(std::is_same_v<
    std::ranges::range_value_t<std::generator<const std::string&>>,
    std::string>);

void test_default_constructor() {
    std::generator<int> g;
    CHECK(g.begin() == g.end());
}

void test_empty_generator() {
    bool started = false;
    auto makeGen = [&]() -> std::generator<int> {
        started = true;
        co_return;
    };
    auto gen = makeGen();
    CHECK(!started);
    auto it = gen.begin();
    CHECK(started);
    CHECK(it == gen.end());
}

void test_move_constructor() {
    auto g = []() -> std::generator<int> { co_yield 42; }();
    auto g2 = std::move(g);
    auto it = g2.begin();
    CHECK(it != g2.end());
    CHECK(*it == 42);
    ++it;
    CHECK(it == g2.end());
}

void test_range_based_for_loop() {
    std::generator<int> g = []() -> std::generator<int> { co_yield 42; }();
    size_t count = 0;
    for (decltype(auto) x : g) {
        static_assert(std::is_same_v<decltype(x), int>);
        CHECK(x == 42);
        ++count;
    }
    CHECK(count == 1);
}

void test_range_based_for_loop_2() {
    static size_t count;
    count = 0;

    struct X {
        X() { ++count; }
        X(const X&) { ++count; }
        X(X&&) { ++count; }
        ~X() { --count; }
    };

    auto g = []() -> std::generator<X> {
        co_yield X{};
        co_yield X{};
    }();

    size_t elementCount = 0;

    for (decltype(auto) x : g) {
        static_assert(std::is_same_v<decltype(x), X>);

        // 1. temporary in co_yield expression
        // 2. reference value stored in promise
        // 3. iteration variable
        CHECK(count == 3);
        ++elementCount;
    }

    CHECK(count == 0);
    CHECK(elementCount == 2);
}

void test_range_based_for_loop_3() {
    static size_t count;
    count = 0;

    struct X {
        X() { ++count; }
        X(const X&) { ++count; }
        X(X&&) { ++count; }
        ~X() { --count; }
    };

    auto g = []() -> std::generator<const X&> {
        co_yield X{};
        co_yield X{};
    }();

    size_t elementCount = 0;

    for (decltype(auto) x : g) {
        static_assert(std::is_same_v<decltype(x), const X&>);

        // 1. temporary in co_yield expression
        CHECK(count == 1);
        ++elementCount;
    }

    CHECK(count == 0);
    CHECK(elementCount == 2);
}

void test_dereference_iterator_copies_reference() {
    static size_t ctorCount = 0;
    static size_t dtorCount = 0;
    struct X {
        X() { ++ctorCount; }
        X(const X&) { ++ctorCount; }
        X(X&&) { ++ctorCount; }
        ~X() { ++dtorCount; }
    };

    {
        auto g = []() -> std::generator<X> {
            co_yield X{};
        }();

        CHECK(ctorCount == 0);
        CHECK(dtorCount == 0);

        auto it = g.begin();

        CHECK(ctorCount > 0);
        CHECK(dtorCount == 0);

        for (int i = 0; i < 3; ++i) {
            auto beforeCtorCount = ctorCount;
            auto beforeDtorCount = dtorCount;
            {
                decltype(auto) x = *it;
                CHECK(ctorCount == beforeCtorCount + 1);
                CHECK(dtorCount == beforeDtorCount);
            }
            CHECK(ctorCount = beforeCtorCount + 1);
            CHECK(dtorCount == beforeDtorCount + 1);
        }
    }

    CHECK(ctorCount == dtorCount);
}

int main() {
    RUN(test_default_constructor);
    RUN(test_empty_generator);
    RUN(test_move_constructor);
    RUN(test_range_based_for_loop);
    RUN(test_range_based_for_loop_2);
    RUN(test_range_based_for_loop_3);
    RUN(test_dereference_iterator_copies_reference);
    return 0;
}
