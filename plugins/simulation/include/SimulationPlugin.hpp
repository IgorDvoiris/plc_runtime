#pragma once
#include "core/ISimulation.hpp"
#include "GpioSimulator.hpp"
#include "core/IPlugin.hpp"
#include "core/SystemBus.hpp"
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

#ifdef __GNUC__
#  define PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#  define PLUGIN_EXPORT
#endif

extern "C" {
    PLUGIN_EXPORT IPlugin* createPlugin();
    PLUGIN_EXPORT void     destroyPlugin(IPlugin* plugin);
}

class SimulationPlugin final : public IPlugin, public ISimulation {
public:
    void init(SystemBus& bus)   override;
    void start()                override;
    void stop()                 override;
    void run()                  override;
    const char* name()    const override { return "Simulation"; }
    
    SimulationPlugin() { sim_ = std::make_shared<GpioSimulator>(); }

    // ── Реализация ISimulation ─────────────────────────────
    Value read(uint16_t ch, uint8_t size) override {
        return sim_->read(ch, static_cast<DirectAddressSize>(size));
    }
    void write(uint16_t ch, uint8_t size, const Value& val) override {
        sim_->write(ch, static_cast<DirectAddressSize>(size), val);
    }

private:
    std::thread       thread_;
    std::shared_ptr<GpioSimulator> sim_;
};
