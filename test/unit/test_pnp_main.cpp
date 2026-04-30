#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include "pnp.hpp"

namespace fs = std::filesystem;
using namespace hrd41;

int main()
{
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║     PNP - Plugin Monitor (Manual Drag & Drop)      ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";

    std::string pluginDir = "/tmp/pnp_plugins";
    fs::create_directories(pluginDir);

    std::cout << "📁 Plugin Directory: " << pluginDir << "\n";
    std::cout << "🎯 Copy/paste .so files here to load them\n\n";

    try {
        std::cout << "🔍 Starting PNP monitor...\n";
        PNP pnp(pluginDir);
        std::cout << "✅ PNP monitoring active\n\n";

        // Give inotify thread time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        std::cout << "════════════════════════════════════════════════════\n";
        std::cout << "Waiting for plugins...\n";
        std::cout << "(Press Ctrl+C to exit)\n";
        std::cout << "════════════════════════════════════════════════════\n\n";

        // Keep monitoring indefinitely
        while (true) {
            std::cout << "⏳ Monitoring... (check " << pluginDir << " for plugins)\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
