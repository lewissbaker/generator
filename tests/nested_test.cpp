///////////////////////////////////////////////////////////////////////////////
// Copyright Lewis Baker, Corentin Jabot
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0.
// (See accompanying file LICENSE or http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////
#include <generator>
#include <string>
#include <string_view>
#include <vector>
#include "check.hpp"

void test_yielding_elements_of_default_constructed_generator() {
    bool started = false;
    bool finished = false;
    auto makeGen = [&]() -> std::generator<int> {
        started = true;
        co_yield std::ranges::elements_of(std::generator<int>{});
        finished = true;
    };

    auto gen = makeGen();
    CHECK(!started);
    CHECK(!finished);
    auto it = gen.begin();
    CHECK(started);
    CHECK(finished);
    CHECK(it == gen.end());
}

void test_yielding_elements_of_empty_generator() {
    bool started1 = false;
    bool started2 = false;
    bool finished = false;
    auto makeGen = [&]() -> std::generator<int> {
        started1 = true;
        co_yield std::ranges::elements_of([&]() -> std::generator<int> {
            started2 = true;
            co_return;
        }());
        finished = true;
    };

    auto gen = makeGen();
    CHECK(!started1);
    CHECK(!started2);
    CHECK(!finished);
    auto it = gen.begin();
    CHECK(started1);
    CHECK(started2);
    CHECK(finished);
    CHECK(it == gen.end());
}

void test_yielding_elements_of_nested_one_level() {
    int checkpoint = 0;
    auto makeGen = [&]() -> std::generator<int> {
        checkpoint = 1;
        co_yield 1;
        checkpoint = 2;
        co_yield std::ranges::elements_of([&]() -> std::generator<int> {
            checkpoint = 3;
            co_yield 2;
            checkpoint = 4;
        }());
        checkpoint = 5;
        co_yield 3;
        checkpoint = 6;
    };

    auto gen = makeGen();
    CHECK(checkpoint == 0);
    auto it = gen.begin();
    CHECK(checkpoint == 1);
    CHECK(it != gen.end());
    CHECK(*it == 1);
    ++it;
    CHECK(checkpoint == 3);
    CHECK(it != gen.end());
    CHECK(*it == 2);
    ++it;
    CHECK(checkpoint == 5);
    CHECK(it != gen.end());
    CHECK(*it == 3);
    ++it;
    CHECK(checkpoint == 6);
    CHECK(it == gen.end());
}

void test_yielding_elements_of_recursive() {
    auto makeGen = [](auto& makeGen, int depth) -> std::generator<int> {
        co_yield depth;
        if (depth > 0) {
            co_yield std::ranges::elements_of(makeGen(makeGen, depth - 1));
            co_yield -depth;
        }
    };

    auto gen = makeGen(makeGen, 3);
    auto it = gen.begin();
    CHECK(it != gen.end());
    CHECK(*it == 3);
    ++it;
    CHECK(it != gen.end());
    CHECK(*it == 2);
    ++it;
    CHECK(it != gen.end());
    CHECK(*it == 1);
    ++it;
    CHECK(it != gen.end());
    CHECK(*it == 0);
    ++it;
    CHECK(it != gen.end());
    CHECK(*it == -1);
    ++it;
    CHECK(it != gen.end());
    CHECK(*it == -2);
    ++it;
    CHECK(it != gen.end());
    CHECK(*it == -3);
    ++it;
    CHECK(it == gen.end());
}

void test_yielding_elements_of_generator_with_different_value_type() {
    auto strings = [](int x) -> std::generator<std::string_view, std::string> {
        co_yield std::to_string(x);

        // This should still perform O(1) nested suspend/resume operations even
        // though the value_type is different.
        // Note that there's not really a way to test this property, though.
        co_yield std::ranges::elements_of([]() -> std::generator<std::string_view, std::string_view> {
            co_yield "foo";
            co_yield "bar";
        }());

        co_yield std::to_string(x + 1);
    }(42);

    auto it = strings.begin();
    CHECK(it != strings.end());
    CHECK(*it == "42");
    ++it;
    CHECK(it != strings.end());
    CHECK(*it == "foo");
    ++it;
    CHECK(it != strings.end());
    CHECK(*it == "bar");
    ++it;
    CHECK(it != strings.end());
    CHECK(*it == "43");
    ++it;
    CHECK(it == strings.end());
}

void test_yielding_elements_of_generator_with_different_reference_type() {
    // TODO
}

void test_yielding_elements_of_generator_with_different_allocator_type() {
    // TODO
}

void test_yielding_elements_of_vector() {
    // TODO
}

void test_nested_generator_scopes_exit_innermost_scope_first() {
    // TODO
}

void test_elementsof_with_allocator_args() {
    std::vector<int> v;
    auto with_alloc = [&] (std::allocator_arg_t, std::allocator<std::byte> a) -> std::generator<int> {
            co_yield 42;
            //co_yield std::ranges::elements_of(v);
            co_yield std::ranges::elements_of(v, a);
    };
    for(auto && i : with_alloc(std::allocator_arg, {})){

    }
}

int main() {
    RUN(test_yielding_elements_of_default_constructed_generator);
    RUN(test_yielding_elements_of_empty_generator);
    RUN(test_yielding_elements_of_nested_one_level);
    RUN(test_yielding_elements_of_recursive);
    RUN(test_yielding_elements_of_generator_with_different_value_type);
    RUN(test_yielding_elements_of_generator_with_different_reference_type);
    RUN(test_yielding_elements_of_generator_with_different_allocator_type);
    RUN(test_elementsof_with_allocator_args);
    RUN(test_yielding_elements_of_vector);
    RUN(test_nested_generator_scopes_exit_innermost_scope_first);
    return 0;
}
