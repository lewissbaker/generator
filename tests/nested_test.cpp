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
#include <memory>
#include <exception>
#include <atomic>

#include "check.hpp"

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
    auto get_strings1 = []() -> std::generator<std::string> {
        co_yield "foo";
    };

    auto get_strings2 = [&]() -> std::generator<std::string_view> {
        co_yield std::ranges::elements_of(get_strings1());
        co_yield "bar";
    };

    auto g = get_strings2();
    auto it = g.begin();
    CHECK(it != g.end());
    CHECK(*it == "foo");
    ++it;
    CHECK(it != g.end());
    CHECK(*it == "bar");
    ++it;
    CHECK(it == g.end());
}

struct counting_allocator_base {
    static std::atomic<std::size_t> allocatedCount;
};

std::atomic<std::size_t> counting_allocator_base::allocatedCount{0};

template<typename T>
struct counting_allocator : counting_allocator_base {

    using value_type = T;

    T* allocate(size_t n) {
        size_t size = n * sizeof(T);
        T* p = std::allocator<T>{}.allocate(n);
        counting_allocator_base::allocatedCount += size;
        return p;
    }

    void deallocate(T* p, size_t n) {
        size_t size = n * sizeof(T);
        counting_allocator_base::allocatedCount -= size;
        std::allocator<T>{}.deallocate(p, n);
    }
};


void test_yielding_elements_of_generator_with_different_allocator_type() {
    // TODO: Ideally we'd be able to detect that yielding elements of a nested generator
    // with a different allocator type wasn't wrapping the nested generator in another
    // coroutine, but we can only really check this by inspecting assembly.
    // So for now we just check that it is functional and ignore the runtime
    // aspects of this.
    auto g = []() -> std::generator<int,  int, std::allocator<std::byte>> {
        co_yield std::ranges::elements_of([]() -> std::generator<int, int, counting_allocator<std::byte>> {
            co_yield 42;
        }());
        co_yield 101;
    }();

    auto it = g.begin();
    CHECK(it != g.end());
    CHECK(*it == 42);
    ++it;
    CHECK(it != g.end());
    CHECK(*it == 101);
    ++it;
    CHECK(it == g.end());
}

void test_yielding_elements_of_vector() {
    auto g = []() -> std::generator<int> {
        std::vector<int> v = {2, 4, 6, 8};
        co_yield std::ranges::elements_of(v);
    }();

    auto it = g.begin();
    CHECK(it != g.end());
    CHECK(*it == 2);
    ++it;
    CHECK(it != g.end());
    CHECK(*it == 4);
    ++it;
    CHECK(it != g.end());
    CHECK(*it == 6);
    ++it;
    CHECK(it != g.end());
    CHECK(*it == 8);
    ++it;
    CHECK(it == g.end());
}

template<typename F>
struct scope_guard {
    F f;

    scope_guard(F f) : f(std::move(f)) {}

    scope_guard(scope_guard&&) = delete;
    scope_guard(const scope_guard&) = delete;

    ~scope_guard() {
        f();
    }
};

void test_nested_generator_scopes_exit_innermost_scope_first() {
    std::vector<int> events;
    auto makeGen = [&]() -> std::generator<int> {
        events.push_back(1);
        scope_guard f{[&] { events.push_back(2); }};

        auto nested = [&]() -> std::generator<int> {
            events.push_back(3);
            scope_guard g{[&] { events.push_back(4); }};

            co_yield 42;
        }();

        scope_guard h{[&] { events.push_back(5); }};

        co_yield std::ranges::elements_of(std::move(nested));
    };

    {
        auto gen = makeGen();
        auto it = gen.begin();
        CHECK(*it == 42);
        CHECK((events == std::vector{1, 3}));
    }

    CHECK((events == std::vector{1, 3, 4, 5, 2}));
}

void test_exception_propagating_from_nested_generator() {
    struct my_error : std::exception {};

    auto g = []() -> std::generator<int> {
        try {
            co_yield std::ranges::elements_of([]() -> std::generator<int> {
                co_yield 42;
                throw my_error{};
            }());
            CHECK(false);
        } catch (const my_error&) {

        }

        co_yield 99;
    }();

    auto it = g.begin();
    CHECK(it != g.end());
    CHECK(*it == 42);
    ++it;
    CHECK(it != g.end());
    CHECK(*it == 99);
    ++it;
    CHECK(it == g.end());
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
    RUN(test_yielding_elements_of_empty_generator);
    RUN(test_yielding_elements_of_nested_one_level);
    RUN(test_yielding_elements_of_recursive);
    RUN(test_yielding_elements_of_generator_with_different_value_type);
    RUN(test_yielding_elements_of_generator_with_different_reference_type);
    RUN(test_yielding_elements_of_generator_with_different_allocator_type);
    RUN(test_elementsof_with_allocator_args);
    RUN(test_yielding_elements_of_vector);
    RUN(test_nested_generator_scopes_exit_innermost_scope_first);
    RUN(test_exception_propagating_from_nested_generator);
    return 0;
}
