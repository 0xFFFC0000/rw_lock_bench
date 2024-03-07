#include <boost/chrono/duration.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/detail/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <queue>
#include <set>
#include <unordered_set>
#include <utility>
#include <functional>


  struct reader_writer_lock {

    reader_writer_lock() noexcept :
    wait_queue() {};

    bool start_read() noexcept {
      if (have_write() || have_read()) {
        return false;
      }
      lock_reader();
      return true;
    }

    void end_read() noexcept {
      unlock_reader();
    }

    bool start_write() noexcept {
      if (have_write()) {
        return false;
      }
      lock_and_change(boost::this_thread::get_id());
      return true;
    }

    void end_write() noexcept {
      unlock_writer();
    }

    bool have_write() noexcept {
      boost::mutex::scoped_lock lock(internal_mutex);
      return have_write(lock);
    }

    bool have_read() noexcept {
      boost::mutex::scoped_lock lock(internal_mutex);
      return have_read(lock);
    }

    ~reader_writer_lock() = default;
    reader_writer_lock(reader_writer_lock&) = delete;
    reader_writer_lock operator=(reader_writer_lock&) = delete;

  private:

    enum reader_writer_kind {
      WRITER = 0,
      READER = 1
    };

    [[clang::always_inline]]
    bool have_read(boost::mutex::scoped_lock& lock) noexcept {
      return readers.find(boost::this_thread::get_id()) != readers.end();
    }

    [[clang::always_inline]]
    bool have_write(boost::mutex::scoped_lock& lock) noexcept {
      if (writer_id == boost::this_thread::get_id()) {
        return true;
      }
      return false;
    }

    //  internal_mutex should be locked when calling this.
    [[clang::always_inline]]
    void wake_up() {

      if (wait_queue.front().first == reader_writer_kind::WRITER) { // writer
        auto thread_to_wake_up = wait_queue.front(); wait_queue.pop();
        thread_to_wake_up.second.get().notify_one();
        return;
      }

      while (wait_queue.size() && wait_queue.front().first == reader_writer_kind::READER) { // readers
        auto thread_to_wake_up = wait_queue.front(); wait_queue.pop();
        thread_to_wake_up.second.get().notify_one();
      }

      return;
    }

    [[clang::always_inline]]
    void unlock_writer() noexcept {
      boost::mutex::scoped_lock lock(internal_mutex);
      rw_mutex.unlock();
      writer_id = boost::thread::id();
      if (wait_queue.size()) {
          wake_up();
      }
    }

    [[clang::always_inline]]
    void unlock_reader() noexcept {
      boost::mutex::scoped_lock lock(internal_mutex);
      rw_mutex.unlock_shared();
      readers.erase(boost::this_thread::get_id());
      if (!readers.size() && wait_queue.size()) {
        wake_up();
      }
    }

    [[clang::always_inline]]
    void wait_in_queue(const reader_writer_kind& kind, boost::mutex::scoped_lock& lock) {
      boost::condition_variable wait_condition;
      wait_queue.push(std::make_pair(std::reference_wrapper(kind), std::reference_wrapper(wait_condition)));
      wait_condition.wait(lock);
    }

    [[clang::always_inline]]
    void entrance(const reader_writer_kind& kind) {
      boost::mutex::scoped_lock lock(internal_mutex);
      if (wait_queue.size()) {
        wait_in_queue(kind, lock);
      }
    }

    [[clang::always_inline]]
    void lock_reader() noexcept {
      entrance(reader_writer_kind::READER);
      do {
        boost::mutex::scoped_lock lock(internal_mutex);
        if (!rw_mutex.try_lock_shared()) {
          wait_in_queue(reader_writer_kind::READER, lock);
          continue;
        }
        readers.insert(boost::this_thread::get_id());
        return;
      } while(true);
    }

    [[clang::always_inline]]
    void lock_and_change(const boost::thread::id& new_owner) noexcept {
      entrance(reader_writer_kind::WRITER);
      do {
        boost::mutex::scoped_lock lock(internal_mutex);
        if (!rw_mutex.try_lock()) {
          wait_in_queue(reader_writer_kind::WRITER, lock);
          continue;
        }
        writer_id = new_owner;
        return;
      } while(true);
    }

    boost::mutex internal_mutex; // keep the data in RWLock consistent.
    boost::shared_mutex rw_mutex;
    std::set<boost::thread::id> readers;
    boost::thread::id writer_id;
    typedef std::pair<std::reference_wrapper<const reader_writer_kind>, std::reference_wrapper<boost::condition_variable>> waiting_thread;
    std::queue<waiting_thread> wait_queue;
  };
