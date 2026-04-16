#pragma once
#include "IPlugin.hpp"
#include <string>
#include <vector>

class PluginLoader {
public:
    ~PluginLoader();
    IPlugin* load(const std::string& path, SystemBus& bus);
    void unloadAll();

private:
    struct LoadedPlugin {
        void*    handle;
        IPlugin* instance;
        using DestroyFn = void(*)(IPlugin*);
        DestroyFn destroy;
    };
    std::vector<LoadedPlugin> plugins_;
};
