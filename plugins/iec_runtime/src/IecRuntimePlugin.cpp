// plugins/iec_runtime/src/IecRuntimePlugin.cpp
#include "IIecRuntime.hpp"
#include "IecProgram.hpp"
#include "IecRawBuffer.hpp"
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

namespace { ProcessImage* g_processImage = nullptr; }

class IecRuntimePlugin final : public IPlugin, public IIecRuntime {
public:
    explicit IecRuntimePlugin(ProcessImage& pi)
        : rawBuf_(pi) {}

    void init(SystemBus& bus) override {
        bus_ = &bus;
        bus.registerService<IIecRuntime>(this);
        std::cout << "[IecRuntime] init\n";
    }
    void start() override {
        std::cout << "[IecRuntime] start, programs=" << programs_.size() << "\n";
    }
    void stop() override {
        programs_.clear();
        std::cout << "[IecRuntime] stop\n";
    }
    const char* name() const override { return "IecRuntime"; }

    bool loadProgram(const std::string& soPath,
                     const std::string& instanceName,
                     const std::string& taskName) override {
        auto prog = std::make_unique<IecProgram>(instanceName, rawBuf_);
        if (!prog->load(soPath)) return false;
        if (bus_) {
            try {
                bus_->getService<ITaskManager>()->addToTask(taskName, prog.get());
                std::cout << "[IecRuntime] '" << instanceName
                          << "' -> task '" << taskName << "'\n";
            } catch (const std::exception& e) {
                std::cerr << "[IecRuntime] addToTask: " << e.what() << "\n";
                return false;
            }
        }
        programs_.push_back(std::move(prog));
        return true;
    }

    void   unloadAll()          override { programs_.clear(); }
    size_t programCount() const override { return programs_.size(); }

private:
    IecRawBuffer rawBuf_;
    std::vector<std::unique_ptr<IecProgram>> programs_;
    SystemBus* bus_ = nullptr;
};

extern "C" {
    PLUGIN_EXPORT void setProcessImage(ProcessImage* pi) { g_processImage = pi; }

    PLUGIN_EXPORT IPlugin* createPlugin() {
        if (!g_processImage) {
            std::cerr << "[IecRuntime] setProcessImage() not called\n";
            return nullptr;
        }
        return new IecRuntimePlugin(*g_processImage);
    }

    PLUGIN_EXPORT void destroyPlugin(IPlugin* p) { delete p; }
}
