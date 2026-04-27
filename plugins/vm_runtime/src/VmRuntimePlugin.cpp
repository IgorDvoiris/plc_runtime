// plugins/vm_runtime/src/VmRuntimePlugin.cpp
#include "IVmRuntime.hpp"
#include "VmProgram.hpp"
#include "core/IPlugin.hpp"
#include "core/SystemBus.hpp"
#include "core/ITaskManager.hpp"
#include <iostream>
#include <vector>
#include <memory>

#ifdef __GNUC__
#  define PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#  define PLUGIN_EXPORT
#endif

class VmRuntimePlugin final : public IPlugin, public IVmRuntime {
public:
    explicit VmRuntimePlugin() {}

    void init(SystemBus& bus) override {
        bus_ = &bus;
        bus.registerService<IVmRuntime>(this);
        std::cout << "[VmRuntime] init\n";
    }
    void start() override {
        std::cout << "[VmRuntime] start, programs=" << programs_.size() << "\n";
    }
    void stop() override {
        programs_.clear();
        std::cout << "[VmRuntime] stop\n";
    }
    const char* name() const override { return "VmRuntime"; }

    bool loadProgram(const std::string& soPath,
                     const std::string& instanceName,
                     const std::string& taskName) override {
        auto prog = std::make_unique<VmProgram>(instanceName);
        if (!prog->load(soPath)) return false;
        if (bus_) {
            try {
                bus_->getService<ITaskManager>()->addToTask(taskName, prog.get());
                std::cout << "[VmRuntime] '" << instanceName
                          << "' -> task '" << taskName << "'\n";
            } catch (const std::exception& e) {
                std::cerr << "[VmRuntime] addToTask: " << e.what() << "\n";
                return false;
            }
        }
        programs_.push_back(std::move(prog));
        return true;
    }

    void   unloadAll()          override { programs_.clear(); }
    size_t programCount() const override { return programs_.size(); }

private:
    std::vector<std::unique_ptr<VmProgram>> programs_;
    SystemBus* bus_ = nullptr;
};

extern "C" {
    PLUGIN_EXPORT IPlugin* createPlugin() {
        return new VmRuntimePlugin();
    }

    PLUGIN_EXPORT void destroyPlugin(IPlugin* p) { delete p; }
}
