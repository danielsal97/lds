#include <iostream>
#include <sys/inotify.h>
#include <unistd.h>
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

int main()
{
    std::cout << "\n🔍 Testing inotify directly...\n";

    std::string testDir = "/tmp/inotify_test";
    fs::create_directories(testDir);
    std::cout << "📁 Created: " << testDir << "\n";

    // Direct inotify test
    int fd = inotify_init();
    if (fd == -1) {
        std::cerr << "❌ inotify_init failed\n";
        return 1;
    }
    std::cout << "✅ inotify_init: fd=" << fd << "\n";

    int wd = inotify_add_watch(fd, testDir.c_str(), IN_CREATE | IN_DELETE);
    if (wd == -1) {
        std::cerr << "❌ inotify_add_watch failed\n";
        perror("Error");
        return 1;
    }
    std::cout << "✅ inotify_add_watch: wd=" << wd << "\n";

    std::cout << "\n⏳ Waiting for file events (10 seconds)...\n";
    std::cout << "Copy a file to: " << testDir << "\n\n";

    // Read events in a thread
    std::thread reader([fd]() {
        char buf[4096];
        while (true) {
            int n = read(fd, buf, sizeof(buf));
            if (n > 0) {
                std::cout << "🔔 Event detected! Bytes read: " << n << "\n";
            }
        }
    });
    reader.detach();

    // Wait
    std::this_thread::sleep_for(std::chrono::seconds(10));

    close(fd);
    fs::remove_all(testDir);
    std::cout << "\n✅ Test complete\n";

    return 0;
}
