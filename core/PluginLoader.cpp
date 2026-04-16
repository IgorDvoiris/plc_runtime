#include "PluginLoader.hpp"
#include <dlfcn.h>
#include <stdexcept>
#include <iostream>

using CreateFn  = IPlugin*(*)();
using DestroyFn = void(*)(IPlugin*);

PluginLoader::~PluginLoader() { unloadAll(); }

IPlugin* PluginLoader::load(const std::string& path, SystemBus& bus) {
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle)
        throw std::runtime_error(std::string("dlopen: ") + dlerror());

    auto* createFn  = reinterpret_cast<CreateFn> (dlsym(handle, "createPlugin"));
    auto* destroyFn = reinterpret_cast<DestroyFn>(dlsym(handle, "destroyPlugin"));
    if (!createFn || !destroyFn) {
        dlclose(handle);
        throw std::runtime_error(std::string("dlsym: ") + dlerror());
    }

    IPlugin* plugin = createFn();
    if (!plugin) { dlclose(handle); throw std::runtime_error("createPlugin() = null"); }

    plugin->init(bus);
    plugins_.push_back({handle, plugin, destroyFn});
    std::cout << "[PluginLoader] loaded '" << plugin->name() << "'\n";
    return plugin;
}

void PluginLoader::unloadAll() {
    for (auto it = plugins_.rbegin(); it != plugins_.rend(); ++it) {
        it->instance->stop();
        it->destroy(it->instance);
        dlclose(it->handle);
        std::cout << "[PluginLoader] unloaded\n";
    }
    plugins_.clear();
}
