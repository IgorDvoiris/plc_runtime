// simulation/MotorSimulation.cpp
#include "SimulationPlugin.hpp"
//#include "GpioSimulator.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <iostream>
#include <chrono>

std::atomic<bool> g_running(false);
void gpioSimulationThreadFunc (std::shared_ptr<GpioSimulator> sim, std::atomic<bool>& running);

void SimulationPlugin::init(SystemBus& bus) {
    bus.registerService<ISimulation>(this);

    sim_ = std::make_shared<GpioSimulator>();

    std::cout << "[Simulation] init\n";
}

void SimulationPlugin::start()
{ 
    if (g_running) {
        return;
    }

    g_running  = true;
    thread_  = std::thread(&SimulationPlugin::run, this);

    std::cout << "[Simulation] start\n";
}

void SimulationPlugin::stop() { 
    g_running = false;

    if (thread_.joinable()) {
        thread_.join();
    }

    std::cout << "[Simulation] stop\n";
}

void SimulationPlugin::run() {
    gpioSimulationThreadFunc (sim_, std::ref(g_running));
}

static uint16_t makeChannel(bool isOutput, DirectAddressSize sz, uint16_t byte, uint8_t bit = 0) {
    return (static_cast<uint16_t>(isOutput) << 15) |
           (static_cast<uint16_t>(sz) << 13) | (byte << 3) | bit;
}

void gpioSimulationThreadFunc(std::shared_ptr<GpioSimulator> sim, std::atomic<bool>& running) {
    // isOutput: false=%I, true=%Q
    const uint16_t CH_START   = makeChannel(false, DirectAddressSize::BIT,   0, 0); // %IX0.0
    const uint16_t CH_PRESSURE= makeChannel(false, DirectAddressSize::WORD,  1);    // %IW1
    const uint16_t CH_PUMP_ON = makeChannel(true,  DirectAddressSize::BIT,   0, 0); // %QX0.0
    const uint16_t CH_SETPOINT= makeChannel(true,  DirectAddressSize::WORD,  2);    // %QW2

    auto start_time = std::chrono::steady_clock::now();
    bool button_released = false;

    while (running) {
        // 1. Кнопка Start (первые 2 секунды)
        auto now = std::chrono::steady_clock::now();
        if (!button_released) {
            if (now - start_time < std::chrono::seconds(2)) {
                sim->setInput(CH_START, Value::fromBool(true));
            } else {
                sim->setInput(CH_START, Value::fromBool(false));
                button_released = true;
            }
        }

        // 2. Ждём обновления выходов от ПЛК
        sim->waitForOutputChange(std::chrono::milliseconds(200));

        // 3. Читаем ВЫХОДЫ (теперь они в отдельной мапе)
        Value pump = sim->getOutput(CH_PUMP_ON, DirectAddressSize::BIT);
        Value sp   = sim->getOutput(CH_SETPOINT, DirectAddressSize::WORD);
        std::cout << "[SIM] Pump: " << (pump.asBool() ? "ON" : "OFF")
                  << " | SP: " << sp.asInt() << std::endl;

        // 4. Физика: давление растёт при работе насоса
        if (pump.asBool()) {
            int16_t p = sim->read(CH_PRESSURE, DirectAddressSize::WORD).asInt();
            if (p < 2000) sim->setInput(CH_PRESSURE, Value::fromInt(p + 10));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

extern "C" {
    PLUGIN_EXPORT IPlugin* createPlugin()              { return new SimulationPlugin(); }
    PLUGIN_EXPORT void     destroyPlugin(IPlugin* p)   { delete p; }
}