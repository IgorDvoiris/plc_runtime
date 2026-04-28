// plugins/gpio_simulator/src/GpioSimulatorPlugin.cpp
#include "core/IPlugin.hpp"
#include "core/SystemBus.hpp"
#include "core/IScheduler.hpp"
#include "core/ITaskManager.hpp"
#include "core/IGpioDriverProvider.hpp"
#include "IGpioSimulator.hpp"
#include "SimulatedGpioDriver.hpp"
#include <iostream>
#include <memory>
#include <chrono>

#ifdef __GNUC__
#  define PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#  define PLUGIN_EXPORT
#endif

class GpioSimulatorPlugin final
    : public IPlugin
    , public IGpioSimulator
    , public IGpioDriverProvider
    , public ITickable
{
public:
    void init(SystemBus& bus) override {
        bus.registerService<IGpioSimulator>(this);
        bus.registerService<IGpioDriverProvider>(this);
        std::cout << "[GpioSim] init: services registered\n";

        // Опционально: подключиться к task'у для tick-based сценариев
        try {
            auto* tm = bus.getService<ITaskManager>();
            tm->createTask("GpioSimTask", 50);   // 50ms — достаточно для большинства сценариев
            tm->addToTask("GpioSimTask", this);
        } catch (const std::exception& e) {
            std::cout << "[GpioSim] task not bound: " << e.what() << "\n";
        }
    }

    void start() override { std::cout << "[GpioSim] start\n"; }
    void stop()  override {
        scenario_.reset();
        std::cout << "[GpioSim] stop\n";
    }
    const char* name() const override { return "GpioSimulator"; }

    // ── IGpioSimulator ────────────────────────────────────────
    void setScenario(std::unique_ptr<IGpioScenario> s) override {
        // Заменяем атомарно: сначала старый отвязываем, потом устанавливаем новый
        driver_.setScenario(nullptr);
        scenario_ = std::move(s);
        driver_.setScenario(scenario_.get());
        if (scenario_)
            std::cout << "[GpioSim] scenario active: " << scenario_->name() << "\n";
    }
    IGpioScenario* scenario() const override { return scenario_.get(); }

    // ── IGpioDriverProvider ───────────────────────────────────
    IIODriver* gpioDriver() override { return &driver_; }

    // ── ITickable (для time-based сценариев) ─────────────────
    void tick() override {
        if (!scenario_) return;
        const auto nowMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        scenario_->tick(nowMs);
    }

private:
    SimulatedGpioDriver               driver_;
    std::unique_ptr<IGpioScenario>    scenario_;
};

extern "C" {
    PLUGIN_EXPORT IPlugin* createPlugin()            { return new GpioSimulatorPlugin(); }
    PLUGIN_EXPORT void     destroyPlugin(IPlugin* p) { delete p; }
}