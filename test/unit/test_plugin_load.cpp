#include <iostream>
#include <dlfcn.h>

int main()
{
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║      Direct Plugin Loading via dlopen()            ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";

    std::cout << "📦 Loading plugin: ./bin/sample_plugin.so\n";
    void* handle = dlopen("./bin/sample_plugin.so", RTLD_LAZY);

    if (!handle) {
        std::cerr << "❌ Failed to load: " << dlerror() << "\n";
        return 1;
    }

    std::cout << "✅ Plugin loaded successfully\n\n";

    std::cout << "════════════════════════════════════════════════════\n";
    std::cout << "Plugin is now active (constructor already ran)\n";
    std::cout << "════════════════════════════════════════════════════\n\n";

    std::cout << "🧹 Unloading plugin...\n";
    dlclose(handle);

    std::cout << "\n✅ Plugin unloaded (destructor already ran)\n\n";

    return 0;
}
