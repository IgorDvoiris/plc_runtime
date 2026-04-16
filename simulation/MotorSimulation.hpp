// simulation/MotorSimulation.hpp
#pragma once

#include "core/IIoSystem.hpp"
#include "core/IoBuffer.hpp"
#include <thread>
#include <atomic>

// Маппинг сигналов на адреса Process Image.
// Симулятор знает о семантике — core не знает.
namespace MotorIO {
    // bool_input[byteAddr][bitAddr]
    // Входы все в байте 0
    constexpr size_t BYTE_IN        = 0;
    constexpr size_t BIT_BTN_START  = 0;   // %IX0.0 → bool_input[0][0]
    constexpr size_t BIT_BTN_STOP   = 1;   // %IX0.1 → bool_input[0][1]
    constexpr size_t BIT_FAULT      = 2;   // %IX0.2 → bool_input[0][2]

    // bool_output[byteAddr][bitAddr]
    // Выходы все в байте 1
    constexpr size_t BYTE_OUT        = 1;
    constexpr size_t BIT_MOTOR_RUN   = 0;  // %QX1.0 → bool_output[1][0]
    constexpr size_t BIT_LAMP_GREEN  = 1;  // %QX1.1 → bool_output[1][1]
    constexpr size_t BIT_LAMP_RED    = 2;  // %QX1.2 → bool_output[1][2]
}

class MotorSimulation : public IIoSystem {
public:
    explicit MotorSimulation(IoBuffer& ioBuffer);
    ~MotorSimulation();

    void start() override;
    void stop()  override;

    bool isRunning() const { return running_.load(); }

private:
    void run();
    void printState();

    IoBuffer&         ioBuffer_;
    std::thread       thread_;
    std::atomic<bool> running_{false};
};
