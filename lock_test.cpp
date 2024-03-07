
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <functional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <benchmark/benchmark.h>
#include <boost/asio.hpp>

#include <random>

#include "reader_writer_lock.h"
#include "recursive_shared_mutex.h"


// total number of threads
size_t total_number_of_threads;
// Between 20-80% of the total number of threads.
size_t number_of_writers;
// Remaining number of threads.
size_t number_of_readers;

// Cycles that readers are going to hold the lock. between 2-100 cycles
constexpr size_t reading_cycles = 5;
// Duration that readers are going to hold the lock. between 5-100 milliseconds
constexpr size_t reading_step_duration = 2;

// Cycles that writers are going to hold the lock, between 2-100 cycles
constexpr size_t writing_cycles = 5;
// Duration that writers are going to hold the lock. between 5-100 milliseconds
constexpr size_t writing_step_duration = 2;

reader_writer_lock global_reader_writer_lock;
recursive_shared_mutex global_recursive_shared_mutex;


void calculate_parameters(size_t threads) {
  total_number_of_threads = threads;
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> d( total_number_of_threads * 0.2 , total_number_of_threads * 0.8);

  number_of_writers = d(rng);
  number_of_readers = total_number_of_threads - number_of_writers;
}

template <typename LockType>
void reader(LockType& main_lock) {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> d;

  bool release_required = main_lock.start_read();
  for(int i = 0; i < reading_cycles; i++) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(reading_step_duration));
  }
  bool recurse = !((bool) std::uniform_int_distribution<std::mt19937::result_type>(0, 10)(rng) % 4); // ~30%
  if (recurse) {
    reader(main_lock);
  }
  if (release_required) main_lock.end_read();
}

template <typename LockType>
void writer(LockType& main_lock) {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> d;

  bool release_required = main_lock.start_write();
  for(int i = 0; i < writing_cycles; i++) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(writing_step_duration));
  }
  bool recurse = !((bool) std::uniform_int_distribution<std::mt19937::result_type>(0, 10)(rng) % 4); // ~20%
  if (recurse) {
    bool which = std::uniform_int_distribution<std::mt19937::result_type>(0, 10)(rng) % 2;
    if (which) {
      writer(main_lock);
    }
    else {
      reader(main_lock);
    }
  }
  if (release_required) main_lock.end_write();
}

template <typename LockType>
void RUN_TEST(LockType& main_lock) {
  std::vector<boost::thread> threads;
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> d(0, 10);

  int reader_count = 0;
  int writer_count = 0;
  while(reader_count  < number_of_readers
        || writer_count  < number_of_writers ) {
    bool which_one = d(rng) % 2;
    if(which_one) {
      threads.push_back(boost::thread(reader<LockType>, std::ref(main_lock))); reader_count++;
    }
    else {
      threads.push_back(boost::thread(writer<LockType>, std::ref(main_lock))); writer_count++;
    }
  }

  std::for_each(threads.begin(), threads.end(), [&] (boost::thread& thread) {
    if (thread.joinable()) thread.join();
  });
}


#define RUN_BENCHMARK(NAME, ITERATION)                                             \
BENCHMARK(NAME)->Iterations(ITERATION)->Unit(benchmark::kMicrosecond);             \
BENCHMARK(NAME)->Iterations(ITERATION * 2)->Unit(benchmark::kMicrosecond);         \
BENCHMARK(NAME)->Iterations(ITERATION * 4)->Unit(benchmark::kMicrosecond);


/////////////////////////////////////////////////////////////////////////////////////
// Single thread lock / unlock benchmark
constexpr size_t WARM_UP_ITER = 50;
constexpr size_t WARM_UP_SLEEP_STEP = 30;

static void reader_writer_lock_warmup(benchmark::State& state) {
  reader_writer_lock rw_lock;
  boost::asio::io_service io_service;
  boost::asio::deadline_timer timer(io_service);

  for (auto _ : state) {
    rw_lock.start_write();
    timer.expires_from_now(boost::posix_time::milliseconds(WARM_UP_SLEEP_STEP));
    timer.wait();
    rw_lock.end_write();
  }
}
BENCHMARK(reader_writer_lock_warmup)->Iterations(WARM_UP_ITER);

static void recursive_shared_mutex_warmup(benchmark::State& state) {
  recursive_shared_mutex rc_mutex;
  boost::asio::io_service io_service;
  boost::asio::deadline_timer timer(io_service);

  for (auto _ : state) {
    rc_mutex.lock();
    timer.expires_from_now(boost::posix_time::milliseconds(WARM_UP_SLEEP_STEP));
    timer.wait();
    rc_mutex.unlock();
  }
}
BENCHMARK(recursive_shared_mutex_warmup)->Iterations(WARM_UP_ITER);

/////////////////////////////////////////////////////////////////////////////////////
// Single thread lock / unlock benchmark
constexpr size_t SINGLE_THREAD_ITER = 500;
constexpr size_t SINGLE_THREAD_SLEEP_STEP = 10;

static void reader_writer_lock_single_thread(benchmark::State& state) {
  reader_writer_lock rw_lock;
  boost::asio::io_service io_service;
  boost::asio::deadline_timer timer(io_service);

  for (auto _ : state) {
    rw_lock.start_write();
    timer.expires_from_now(boost::posix_time::milliseconds(SINGLE_THREAD_SLEEP_STEP));
    timer.wait();
    rw_lock.end_write();
  }
}
RUN_BENCHMARK(reader_writer_lock_single_thread, SINGLE_THREAD_ITER);


static void recursive_shared_mutex_single_thread(benchmark::State& state) {
  recursive_shared_mutex rc_mutex;
  boost::asio::io_service io_service;
  boost::asio::deadline_timer timer(io_service);

  for (auto _ : state) {
    rc_mutex.lock();
    timer.expires_from_now(boost::posix_time::milliseconds(SINGLE_THREAD_SLEEP_STEP));
    timer.wait();
    rc_mutex.unlock();
  }
}
RUN_BENCHMARK(recursive_shared_mutex_single_thread, SINGLE_THREAD_ITER);

/////////////////////////////////////////////////////////////////////////////////////
// Multithread lock / unlock benchmark
constexpr size_t RANDOMIZED_READER_WRITER_ITER = 10;

static void randomized_reader_writer_lock_10(benchmark::State& state) {
  calculate_parameters(100);

  for (auto _ : state) {
    RUN_TEST(global_reader_writer_lock);
  }
}
RUN_BENCHMARK(randomized_reader_writer_lock_10, RANDOMIZED_READER_WRITER_ITER);

static void randomized_recursive_shared_mutex_10(benchmark::State& state) {
  calculate_parameters(10);

  for (auto _ : state) {
    RUN_TEST(global_recursive_shared_mutex);
  }
}
RUN_BENCHMARK(randomized_recursive_shared_mutex_10, RANDOMIZED_READER_WRITER_ITER);

/////////////////////////////////////////////////////////////////////////////////////
// Multithread lock / unlock benchmark

static void randomized_reader_writer_lock_100(benchmark::State& state) {
  calculate_parameters(100);

  for (auto _ : state) {
    RUN_TEST(global_reader_writer_lock);
  }
}
RUN_BENCHMARK(randomized_reader_writer_lock_100, RANDOMIZED_READER_WRITER_ITER);

static void randomized_recursive_shared_mutex_100(benchmark::State& state) {
  calculate_parameters(100);

  for (auto _ : state) {
    RUN_TEST(global_recursive_shared_mutex);
  }
}
RUN_BENCHMARK(randomized_recursive_shared_mutex_100, RANDOMIZED_READER_WRITER_ITER);

/////////////////////////////////////////////////////////////////////////////////////
// Multithread lock / unlock benchmark

static void randomized_reader_writer_lock_1000(benchmark::State& state) {
  calculate_parameters(1000);

  for (auto _ : state) {
    RUN_TEST(global_reader_writer_lock);
  }
}
RUN_BENCHMARK(randomized_reader_writer_lock_1000, RANDOMIZED_READER_WRITER_ITER);

static void randomized_recursive_shared_mutex_1000(benchmark::State& state) {
  calculate_parameters(1000);

  for (auto _ : state) {
    RUN_TEST(global_recursive_shared_mutex);
  }
}
RUN_BENCHMARK(randomized_recursive_shared_mutex_1000, RANDOMIZED_READER_WRITER_ITER);

/////////////////////////////////////////////////////////////////////////////////////
// Multithread lock / unlock benchmark

static void randomized_reader_writer_lock_5000(benchmark::State& state) {
  calculate_parameters(5000);

  for (auto _ : state) {
    RUN_TEST(global_reader_writer_lock);
  }
}
RUN_BENCHMARK(randomized_reader_writer_lock_5000, RANDOMIZED_READER_WRITER_ITER);

static void randomized_recursive_shared_mutex_5000(benchmark::State& state) {
  calculate_parameters(5000);

  for (auto _ : state) {
    RUN_TEST(global_recursive_shared_mutex);
  }
}
RUN_BENCHMARK(randomized_recursive_shared_mutex_5000, RANDOMIZED_READER_WRITER_ITER);


BENCHMARK_MAIN();