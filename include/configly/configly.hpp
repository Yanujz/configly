#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>
#include <array>
#include <utility>
#include <cassert>
#include <cstring>

namespace detail {
    struct any_type {
        template<typename T>
        constexpr operator T() const noexcept;
    };

    template<typename T, typename... Args>
    constexpr auto is_brace_constructible(int) -> decltype(T{std::declval<Args>()...}, std::true_type{});
    
    template<typename T, typename... Args>
    constexpr std::false_type is_brace_constructible(...);

    template<typename T, typename... Args>
    constexpr bool can_construct = decltype(is_brace_constructible<T, Args...>(0))::value;

    template<typename T, std::size_t... Is>
    constexpr std::size_t count_fields_impl(std::index_sequence<Is...>) {
        if constexpr (can_construct<T, decltype(Is, any_type{})...>) {
            return sizeof...(Is);
        } else {
            return 0;
        }
    }

    template<typename T, std::size_t N = 0>
    constexpr std::size_t count_fields() {
        if constexpr (N > 64) {
            return 0; // Sanity limit
        } else if constexpr (count_fields_impl<T>(std::make_index_sequence<N>{}) == N) {
            return N;
        } else {
            return count_fields<T, N + 1>();
        }
    }
    
    template<typename T>
    constexpr std::size_t default_max_callbacks() {
        constexpr std::size_t detected = count_fields<T>();
        return detected > 0 ? detected : 16;
    }
}

template<typename T, size_t MaxCallbacks = detail::default_max_callbacks<T>()>
class Configly
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "Config struct T must be trivially copyable.");
    static_assert(MaxCallbacks > 0 && MaxCallbacks <= 64,
                  "MaxCallbacks must be between 1 and 64");

public:
    static Configly& instance() {
        static Configly i;
        return i;
    }

    Configly(const Configly&) = delete;
    Configly& operator=(const Configly&) = delete;
    Configly(Configly&&) = delete;
    Configly& operator=(Configly&&) = delete;

    void setDefault(const T& defaultConfig) {
        m_defaultConfig = defaultConfig;

        // init both buffers with seq = 0 (even → stable) and same data
        m_buffers[0].seq.store(0, std::memory_order_relaxed);
        m_buffers[0].data = defaultConfig;

        m_buffers[1].seq.store(0, std::memory_order_relaxed);
        m_buffers[1].data = defaultConfig;

        m_activeIndex.store(0, std::memory_order_release);
    }

    [[nodiscard]] const T& getDefault() const {
        return m_defaultConfig;
    }

    /**
     * @brief Atomically retrieves a consistent snapshot of the entire current configuration.
     */
    void getAll(T& outConfig) const {
        for (;;) {
            int idx = m_activeIndex.load(std::memory_order_acquire);
            const Buffer& buf = m_buffers[idx];

            // read start sequence
            std::uint64_t start = buf.seq.load(std::memory_order_acquire);
            if (start & 1u) {
                // writer in progress on this buffer
                continue;
            }

            // copy data
            T temp = buf.data;

            // make sure we see any writes to data before re-reading seq
            std::atomic_thread_fence(std::memory_order_acquire);

            // read end sequence
            std::uint64_t end = buf.seq.load(std::memory_order_acquire);

            // success iff same and even
            if (start == end && !(end & 1u)) {
                outConfig = temp;
                return;
            }
            // else retry
        }
    }

    template<typename MemberPtr>
    [[nodiscard]] auto get(MemberPtr member) const {
        T tempConfig;
        getAll(tempConfig);
        return tempConfig.*member;
    }
    
    /**
     * @brief Atomically updates the entire configuration from an in-memory struct.
     */
    void update(const T& new_config) {
        // serialize writers
        while (m_writeLock.test_and_set(std::memory_order_acquire)) {
            // spin
        }

        int active_idx = m_activeIndex.load(std::memory_order_relaxed);
        int inactive_idx = 1 - active_idx;

        T old_config = m_buffers[active_idx].data;

        // write to inactive buffer using seq protocol
        Buffer& target = m_buffers[inactive_idx];

        // start write: make seq odd
        std::uint64_t seq = target.seq.load(std::memory_order_relaxed);
        target.seq.store(seq + 1, std::memory_order_release);   // odd → writer active

        // actual data write
        target.data = new_config;

        // end write: make seq even
        target.seq.store(seq + 2, std::memory_order_release);   // even → stable

        // publish
        m_activeIndex.store(inactive_idx, std::memory_order_release);

        m_writeLock.clear(std::memory_order_release);

        // callbacks based on old vs new
        triggerCallbacksForChanges(old_config, m_buffers[inactive_idx].data);
    }

    /**
     * @brief Set a specific member value and trigger its callback if registered.
     */
    template<typename MemberPtr, typename ValueType>
    void set(MemberPtr member, ValueType&& value) {
        static_assert(std::is_member_object_pointer<MemberPtr>::value,
                      "Member pointer required");

        while (m_writeLock.test_and_set(std::memory_order_acquire)) {
            // spin
        }

        int active_idx = m_activeIndex.load(std::memory_order_relaxed);
        int inactive_idx = 1 - active_idx;

        auto oldValue = m_buffers[active_idx].data.*member;

        // start write on inactive
        Buffer& target = m_buffers[inactive_idx];
        std::uint64_t seq = target.seq.load(std::memory_order_relaxed);
        target.seq.store(seq + 1, std::memory_order_release);   // odd

        // copy current config then modify field
        target.data = m_buffers[active_idx].data;
        target.data.*member = std::forward<ValueType>(value);

        // end write
        target.seq.store(seq + 2, std::memory_order_release);   // even

        // publish
        m_activeIndex.store(inactive_idx, std::memory_order_release);

        m_writeLock.clear(std::memory_order_release);

        // trigger callback if actually changed
        if (!(oldValue == (m_buffers[inactive_idx].data.*member))) {
            const size_t offset = calculateOffset(member);
            triggerCallback(offset, &(m_buffers[inactive_idx].data.*member));
        }
    }

    template<typename MemberPtr>
    Configly& onChange(
        MemberPtr member,
        void (*user_callback)(const std::remove_reference_t<decltype(std::declval<T>().*member)>&, void*),
        void* user_context = nullptr) {
        
        static_assert(std::is_member_object_pointer<MemberPtr>::value,
                      "Member pointer required");

        using MemberType = std::remove_reference_t<decltype(std::declval<T>().*member)>;

        const size_t offset = calculateOffset(member);
        assert(offset < sizeof(T) && "Invalid member offset");

        for (size_t i = 0; i < m_callbackCount; ++i) {
            if (m_callbacks[i].memberOffset == offset) {
                m_callbacks[i].originalCallback = reinterpret_cast<void*>(user_callback);
                m_callbacks[i].userData = user_context;
                m_callbacks[i].thunk = &callbackThunk<MemberType>;
                return *this;
            }
        }

        assert(m_callbackCount < MaxCallbacks && "Exceeded maximum number of callbacks!");
        if (m_callbackCount >= MaxCallbacks) {
            return *this;
        }

        auto& slot = m_callbacks[m_callbackCount];
        slot.memberOffset = offset;
        slot.originalCallback = reinterpret_cast<void*>(user_callback);
        slot.userData = user_context;
        slot.memberSize = sizeof(m_defaultConfig.*member);
        slot.thunk = &callbackThunk<MemberType>;
        
        ++m_callbackCount;
        
        return *this;
    }

    template<typename MemberPtr>
    void removeCallback(MemberPtr member) {
        const size_t offset = calculateOffset(member);
        for (size_t i = 0; i < m_callbackCount; ++i) {
            if (m_callbacks[i].memberOffset == offset) {
                m_callbacks[i].thunk = nullptr;
                return;
            }
        }
    }

    void setSaveFunction(bool (*fn)(const T&)) { 
        m_saveFunction = fn; 
    }
    
    void setLoadFunction(bool (*fn)(T&)) { 
        m_loadUserConfig = fn; 
    }

    [[nodiscard]] bool save() const {
        if (!m_saveFunction) return false;
        T tempConfig;
        getAll(tempConfig);
        return m_saveFunction(tempConfig);
    }

    [[nodiscard]] bool load() {
        if (!m_loadUserConfig) return false;
        T tempConfig;
        if (m_loadUserConfig(tempConfig)) {
            update(tempConfig);
            return true;
        }
        return false;
    }

    void restoreDefaults() {
        update(m_defaultConfig);
    }

    template<typename MemberPtr>
    void restoreDefault(MemberPtr member) {
        set(member, m_defaultConfig.*member);
    }

private:
    struct Buffer {
        std::atomic<std::uint64_t> seq;
        T data;
    };

    Configly()
        : m_activeIndex(0),
          m_callbacks{},
          m_callbackCount(0)
    {
        m_writeLock.clear(std::memory_order_relaxed);

        // make buffers initially valid
        m_buffers[0].seq.store(0, std::memory_order_relaxed);
        m_buffers[1].seq.store(0, std::memory_order_relaxed);
    }

    ~Configly() = default;

    template<typename MemberPtr>
    [[nodiscard]] constexpr size_t calculateOffset(MemberPtr member) const {
        const T temp_object{};
        return reinterpret_cast<const char*>(&(temp_object.*member))
               - reinterpret_cast<const char*>(&temp_object);
    }

    void triggerCallback(size_t offset, const void* newValue) {
        for (size_t i = 0; i < m_callbackCount; ++i) {
            if (m_callbacks[i].memberOffset == offset && m_callbacks[i].thunk) {
                m_callbacks[i].thunk(newValue, &m_callbacks[i]);
                return;
            }
        }
    }

    void triggerCallbacksForChanges(const T& oldConfig, const T& newConfig) {
        for (size_t i = 0; i < m_callbackCount; ++i) {
            auto& slot = m_callbacks[i];
            if (slot.thunk) {
                const char* oldPtr = reinterpret_cast<const char*>(&oldConfig) + slot.memberOffset;
                const char* newPtr = reinterpret_cast<const char*>(&newConfig) + slot.memberOffset;
                
                if (std::memcmp(oldPtr, newPtr, slot.memberSize) != 0) {
                    slot.thunk(newPtr, &slot);
                }
            }
        }
    }

    // --- Double-Buffering Members (now w/ seq) ---
    alignas(64) Buffer m_buffers[2];
    alignas(64) std::atomic<int> m_activeIndex;
    std::atomic_flag m_writeLock = ATOMIC_FLAG_INIT;

    // --- Other Members ---
    T m_defaultConfig;

    struct CallbackSlot {
        size_t memberOffset = 0;
        size_t memberSize = 0;
        void (*thunk)(const void*, void*) = nullptr;
        void* originalCallback = nullptr;
        void* userData = nullptr;
    };
    
    std::array<CallbackSlot, MaxCallbacks> m_callbacks;
    size_t m_callbackCount;
    
    template<typename MemberType>
    static void callbackThunk(const void* newValue, void* slotPtr) {
        CallbackSlot* currentSlot = static_cast<CallbackSlot*>(slotPtr);
        const MemberType& typedValue = *static_cast<const MemberType*>(newValue);
        auto original_fn = reinterpret_cast<void (*)(const MemberType&, void*)>(
            currentSlot->originalCallback);
        original_fn(typedValue, currentSlot->userData);
    }

    bool (*m_saveFunction)(const T&) = nullptr;
    bool (*m_loadUserConfig)(T&) = nullptr;
};
