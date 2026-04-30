#include <iostream>
#include "../include/factory.hpp"
#include "../include/singelton.hpp"

namespace hrd41 {

class SamplePlugin {
public:
    static void main() {
        std::cout << "⚡ [SamplePlugin] Main trigger executed!\n";
    }
};

// Plugin factory: Factory<ReturnType, KeyType, ArgsType>
using PluginFactory = Factory<std::function<void()>, std::string, std::nullptr_t>;

} // namespace hrd41

__attribute__((constructor))
void init_plugin() {
    std::cout << "⚙️  [PluginInitializer] Starting...\n";

    auto factory = hrd41::Singleton<hrd41::PluginFactory>::GetInstance();
    factory->Add("main", [](std::nullptr_t) {
        return std::make_shared<std::function<void()>>(&hrd41::SamplePlugin::main);
    });

    std::nullptr_t args = nullptr;
    auto func = factory->Create("main", args);
    (*func)();

    std::cout << "✅ [PluginInitializer] Complete\n";
}

__attribute__((destructor))
void cleanup_plugin() {
    std::cout << "⚙️  [PluginInitializer] Cleanup...\n";
    std::cout << "✅ [PluginInitializer] Done\n";
}
