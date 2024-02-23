#include <atomic>
#include <cstdint>
#include <messagecache/ring_buffer.hpp>

#include <benchmark/benchmark.h>

#include <iostream>
#include <thread>


static void pinThread(int cpu)
{
    if(cpu < 0) {
        return;
    }
    ::cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if(::pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
        std::perror("pthread_setaffinity_rp");
        std::exit(EXIT_FAILURE);
    }
}

constexpr auto cpu1 = 1;
constexpr auto cpu2 = 2;


void try_alloc(benchmark::State& state)
{
    using buf_type = messagecache::ring_buffer<131072>;
    using counter_type = std::int_fast64_t;

    buf_type fifo;

    std::atomic_flag finished;

    auto t1 = std::jthread([&] {
        pinThread(cpu1);
        for(auto i = counter_type{};; ++i) {
            auto slot = fifo.try_alloc(16);
            benchmark::DoNotOptimize(slot);

            if(finished.test(std::memory_order_relaxed)) {
                break;
            }
        }
    });

    auto value = counter_type{};
    pinThread(cpu2);
    for(auto _ : state) {

        auto slot = fifo.try_alloc(16);
        benchmark::DoNotOptimize(slot);

        ++value;
    }
    state.counters["ops/sec (1T)"] =
        benchmark::Counter(double(value), benchmark::Counter::kIsRate);

    finished.test_and_set();
}


BENCHMARK(try_alloc);

BENCHMARK_MAIN();