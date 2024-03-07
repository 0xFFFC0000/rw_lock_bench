
#include "recursive_shared_mutex.h"

#include <cassert>

// Initialize static member shared_slot_counter
std::atomic<recursive_shared_mutex::slot_t> recursive_shared_mutex::shared_slot_counter{0};

// Initialize static member tlocal_access_per_mutex
thread_local
    std::unordered_map<recursive_shared_mutex::slot_t, recursive_shared_mutex::access_counter_t>
        recursive_shared_mutex::tlocal_access_per_mutex{};


recursive_shared_mutex::recursive_shared_mutex(): m_rw_mutex{}, m_slot{shared_slot_counter++}
{}

void recursive_shared_mutex::lock()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];
    assert(0 == access || access & write_bit); // cannot upgrade from read->write

    if (!access)
        m_rw_mutex.lock();

    access = (access + 1) | write_bit;
}

bool recursive_shared_mutex::try_lock()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];
    assert(0 == access || access & write_bit); // cannot upgrade from read->write

    if (access || m_rw_mutex.try_lock())
    {
        access = (access + 1) | write_bit;
        return true;
    }

    if (!access)
        tlocal_access_per_mutex.erase(m_slot);

    return false;
}

void recursive_shared_mutex::unlock()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];
    assert(access & depth_mask); // write depth must be non zero

    const bool still_held = --access & depth_mask;
    if (!still_held)
    {
        m_rw_mutex.unlock();
        tlocal_access_per_mutex.erase(m_slot);
    }
}

void recursive_shared_mutex::lock_shared()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];

    if (!(access++))
        m_rw_mutex.lock_shared();
}

bool recursive_shared_mutex::try_lock_shared()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];

    if (access || m_rw_mutex.try_lock_shared())
    {
        ++access;
        return true;
    }

    if (!access)
        tlocal_access_per_mutex.erase(m_slot);

    return false;
}

void recursive_shared_mutex::unlock_shared()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];
    assert(access & depth_mask); // read depth must be non zero

    const bool still_held = --access & depth_mask;
    if (!still_held)
    {
        m_rw_mutex.unlock_shared();
        tlocal_access_per_mutex.erase(m_slot);
    }
}

[[clang::always_inline]]
bool recursive_shared_mutex::start_read() noexcept {
    this->lock_shared();
    return true;
}

[[clang::always_inline]]
void recursive_shared_mutex::end_read() noexcept {
    this->unlock_shared();
}

[[clang::always_inline]]
bool recursive_shared_mutex::start_write() noexcept {
    this->lock();
    return true;
}


[[clang::always_inline]]
void recursive_shared_mutex::end_write() noexcept {
    this->unlock();
}