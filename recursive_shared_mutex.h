#include <atomic>
#include <cstdint>
#include <limits>
#include <unordered_map>

#include <boost/thread/shared_mutex.hpp>

class recursive_shared_mutex
{
public:
    recursive_shared_mutex();

    // delete copy/move construct/assign operations
    recursive_shared_mutex(const recursive_shared_mutex&)            = delete;
    recursive_shared_mutex(recursive_shared_mutex&&)                 = delete;
    recursive_shared_mutex& operator=(const recursive_shared_mutex&) = delete;
    recursive_shared_mutex& operator=(recursive_shared_mutex&&)      = delete;


    [[clang::always_inline]] bool start_read() noexcept;
    [[clang::always_inline]] void end_read() noexcept;
    [[clang::always_inline]] bool start_write() noexcept;
    [[clang::always_inline]] void end_write() noexcept;

    // Lockable
    void lock();
    bool try_lock();
    void unlock();

    // SharedLockable
    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();

private:
    using slot_t = std::uint32_t;
    using access_counter_t = std::uint32_t;

    static constexpr access_counter_t access_counter_max = std::numeric_limits<access_counter_t>::max();
    static constexpr access_counter_t depth_mask = access_counter_max >> 1;
    static constexpr access_counter_t write_bit = access_counter_max - depth_mask;

    // what the slot id will be for the next instance of recursive_shared_mutex
    static std::atomic<slot_t> shared_slot_counter;

    // keeps track of read/write depth and write mode per instance of recursive_shared_mutex
    static thread_local std::unordered_map<slot_t, access_counter_t> tlocal_access_per_mutex;

    boost::shared_mutex m_rw_mutex; // we use boost::shared_mutex since it is guaranteed fair, unlike std::shared_mutex
    const slot_t m_slot; // this is an ID number used per-thread to identify whether already held (enables recursion)
};