#include <gtest/gtest.h>
#include <configly/configly.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

// --- Struttura di test ---
struct TestConfig {
    uint32_t a;
    int b;
    bool c;

    // Necessario per i confronti nei test
    bool operator==(const TestConfig& other) const {
        return a == other.a && b == other.b && c == other.c;
    }
};

// --- Callback di test ---
std::atomic<int> callback_value = 0;
void testCallback(const int& newValue, void* userData) {
    callback_value.store(newValue);
}


// --- Test Suite per le Funzioni Base ---
class ConfiglyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Resetta lo stato prima di ogni test
        Configly<TestConfig>::instance().restoreDefaults();
        callback_value = 0;
    }
    
    Configly<TestConfig>& config = Configly<TestConfig>::instance();
    const TestConfig defaultConfig = {10, -20, false};

    ConfiglyTest() {
        config.setDefault(defaultConfig);
    }
};

TEST_F(ConfiglyTest, Initialization) {
    TestConfig current;
    config.getAll(current);
    ASSERT_EQ(current, defaultConfig);
}

TEST_F(ConfiglyTest, GetAndSet) {
    ASSERT_EQ(config.get(&TestConfig::a), 10);
    config.set(&TestConfig::a, 99);
    ASSERT_EQ(config.get(&TestConfig::a), 99);
}

TEST_F(ConfiglyTest, RestoreDefaults) {
    config.set(&TestConfig::a, 123);
    config.set(&TestConfig::b, 456);
    config.set(&TestConfig::c, true);
    ASSERT_NE(config.get(&TestConfig::a), defaultConfig.a);

    config.restoreDefaults();
    TestConfig current;
    config.getAll(current);
    ASSERT_EQ(current, defaultConfig);
}

TEST_F(ConfiglyTest, CallbackOnSet) {
    (void)config.onChange(&TestConfig::b, &testCallback);
    config.set(&TestConfig::b, 777);
    // Aspetta un istante per essere sicuro che il callback venga eseguito in un contesto multithread
    std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
    ASSERT_EQ(callback_value.load(), 777);
}

// --- Test Suite per la Concorrenza ---

struct ConcurrencyConfig {
    uint64_t val1;
    uint64_t val2;
    uint64_t val3;
};

TEST(ConfiglyConcurrencyTest, NoTornReads) {
    auto& config = Configly<ConcurrencyConfig>::instance();
    config.setDefault({0, 0, 0});

    std::atomic<bool> stop_signal = false;
    std::atomic<int> torn_reads_count = 0;

    // --- Thread Scrittore ---
    std::thread writer_thread([&]() {
        uint64_t i = 1;
        while (!stop_signal.load()) {
            config.update({i, i, i});
            i++;
        }
    });

    // --- Thread Lettori ---
    std::vector<std::thread> reader_threads;
    const int num_readers = 4;
    for (int i = 0; i < num_readers; ++i) {
        reader_threads.emplace_back([&]() {
            while (!stop_signal.load()) {
                ConcurrencyConfig current;
                config.getAll(current);
                if (current.val1 != current.val2 || current.val1 != current.val3) {
                    torn_reads_count++;
                }
            }
        });
    }

    // Lascia i thread in esecuzione per un po'
    std::this_thread::sleep_for(std::chrono::seconds(2));
    stop_signal.store(true);

    // Attendi la terminazione di tutti i thread
    writer_thread.join();
    for (auto& t : reader_threads) {
        t.join();
    }
    
    // L'ASSERT FONDAMENTALE: non ci devono essere state letture corrotte.
    ASSERT_EQ(torn_reads_count.load(), 0);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}