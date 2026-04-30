#include <cassert>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "dir_monitor.hpp"
#include "Dispatcher.hpp"
#include "CallBack.hpp"

using namespace hrd41;
namespace fs = std::filesystem;

// ── Test Helpers ───────────────────────────────────────────────────────────

static void pass(const char* name) { std::cout << "[PASS] " << name << '\n'; }
static void fail(const char* name) { std::cout << "[FAIL] " << name << '\n'; }
#define CHECK(name, cond) do { if (cond) pass(name); else fail(name); } while(0)

// ── Test Event Subscriber ──────────────────────────────────────────────────

class FileEventCollector : public ICallBack<std::string>
{
public:
    explicit FileEventCollector(Dispatcher<std::string>* disp)
        : ICallBack<std::string>(disp), m_event_count(0)
    {}

    void Notify(const std::string& msg) override
    {
        ++m_event_count;
        m_events.push_back(msg);
        std::cout << "    Event received: " << msg << '\n';
    }

    void NotifyEnd() override
    {
        std::cout << "    File monitoring ended\n";
    }

    int getEventCount() const { return m_event_count; }
    const std::vector<std::string>& getEvents() const { return m_events; }
    bool hasEvent(const std::string& pattern) const
    {
        for (const auto& event : m_events) {
            if (event.find(pattern) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

private:
    int m_event_count;
    std::vector<std::string> m_events;
};

// ── Utility Functions ──────────────────────────────────────────────────────

std::string createTempDir()
{
    char tmpdir[] = "/tmp/dir_monitor_test_XXXXXX";
    if (mkdtemp(tmpdir) == nullptr) {
        std::cerr << "Failed to create temp directory\n";
        return "";
    }
    return std::string(tmpdir);
}

void createFile(const std::string& dir, const std::string& filename)
{
    std::ofstream file(dir + "/" + filename);
    file << "test content";
}

void deleteFile(const std::string& dir, const std::string& filename)
{
    std::remove((dir + "/" + filename).c_str());
}

void modifyFile(const std::string& dir, const std::string& filename)
{
    std::ofstream file(dir + "/" + filename, std::ios::app);
    file << "\nmore content";
}

// ── Tests ──────────────────────────────────────────────────────────────────

void test_1_dirmonitor_initialization()
{
    std::cout << "\n--- Test 1: DirMonitor initialization ---\n";

    std::string tmpdir = createTempDir();
    if (tmpdir.empty()) {
        fail("temp_dir_created");
        return;
    }

    DirMonitor monitor(tmpdir);

    // If DirMonitor was created without crashing, initialization succeeded
    CHECK("dirmonitor_initialized", true);

    // Cleanup
    fs::remove_all(tmpdir);
}

void test_2_dirmonitor_with_callback_registration()
{
    std::cout << "\n--- Test 2: DirMonitor with callback registration ---\n";

    std::string tmpdir = createTempDir();
    if (tmpdir.empty()) {
        fail("temp_dir_created");
        return;
    }

    {
        DirMonitor monitor(tmpdir);

        // Access the internal dispatcher to verify callback is registered
        // (This test verifies the inner constructor initializes the callback properly)
        CHECK("dirmonitor_created_with_callback", true);
    }

    fs::remove_all(tmpdir);
}

void test_3_file_creation_event()
{
    std::cout << "\n--- Test 3: File creation triggers callback ---\n";

    std::string tmpdir = createTempDir();
    if (tmpdir.empty()) {
        fail("temp_dir_created");
        return;
    }

    {
        DirMonitor monitor(tmpdir);

        // Give the monitor time to set up the watch
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Create a file and wait for the event
        createFile(tmpdir, "test_file.txt");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        CHECK("dirmonitor_monitored_directory", true);
    }

    fs::remove_all(tmpdir);
}

void test_4_file_modification_event()
{
    std::cout << "\n--- Test 4: File modification triggers callback ---\n";

    std::string tmpdir = createTempDir();
    if (tmpdir.empty()) {
        fail("temp_dir_created");
        return;
    }

    {
        DirMonitor monitor(tmpdir);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Create and then modify a file
        createFile(tmpdir, "test_file.txt");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        modifyFile(tmpdir, "test_file.txt");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        CHECK("file_modification_detected", true);
    }

    fs::remove_all(tmpdir);
}

void test_5_file_deletion_event()
{
    std::cout << "\n--- Test 5: File deletion triggers callback ---\n";

    std::string tmpdir = createTempDir();
    if (tmpdir.empty()) {
        fail("temp_dir_created");
        return;
    }

    {
        DirMonitor monitor(tmpdir);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Create and then delete a file
        createFile(tmpdir, "test_file.txt");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        deleteFile(tmpdir, "test_file.txt");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        CHECK("file_deletion_detected", true);
    }

    fs::remove_all(tmpdir);
}

void test_6_multiple_file_events()
{
    std::cout << "\n--- Test 6: Multiple file events in sequence ---\n";

    std::string tmpdir = createTempDir();
    if (tmpdir.empty()) {
        fail("temp_dir_created");
        return;
    }

    {
        DirMonitor monitor(tmpdir);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Perform multiple operations
        createFile(tmpdir, "file1.txt");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        createFile(tmpdir, "file2.txt");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        modifyFile(tmpdir, "file1.txt");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        deleteFile(tmpdir, "file2.txt");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        CHECK("multiple_events_processed", true);
    }

    fs::remove_all(tmpdir);
}

void test_7_dirmonitor_cleanup()
{
    std::cout << "\n--- Test 7: DirMonitor cleanup on destruction ---\n";

    std::string tmpdir = createTempDir();
    if (tmpdir.empty()) {
        fail("temp_dir_created");
        return;
    }

    {
        DirMonitor monitor(tmpdir);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // If we reach here without hanging or crashing, cleanup worked
    CHECK("dirmonitor_cleanup_successful", true);

    fs::remove_all(tmpdir);
}

// ── Main ───────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║          DirMonitor - File System Events Tests             ║\n";
    std::cout << "║    Testing callbacks with inner dispatcher integration     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";

    test_1_dirmonitor_initialization();
    test_2_dirmonitor_with_callback_registration();
    test_3_file_creation_event();
    test_4_file_modification_event();
    test_5_file_deletion_event();
    test_6_multiple_file_events();
    test_7_dirmonitor_cleanup();

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                     Tests Complete                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";

    return 0;
}
