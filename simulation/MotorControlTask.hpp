// simulation/MotorControlTask.hpp
#pragma once

#include "core/IScheduler.hpp"
#include "core/ProcessImage.hpp"

// Адреса выходов — должны совпадать с IEC программой:
//   motorRun   AT %QX0.0  → bool_output[0]  → outputs[0] бит 0
//   lampGreen  AT %QX1.0  → bool_output[8]  → outputs[8] бит 0
//   lampRed    AT %QX2.0  → bool_output[16] → outputs[16] бит 0
//
// IecRawBuffer раскладывает bool_output как плоский массив uint8_t[],
// где каждый элемент = одна IEC BOOL переменная.
// %QX0.0 → bool_output[0], %QX1.0 → bool_output[8], %QX2.0 → bool_output[16]
//
// В ProcessImage.outputs[] эти байты лежат начиная с BOOL_OFFSET=0:
//   outputs[0]  = bool_output[0]  = motorRun
//   outputs[8]  = bool_output[8]  = lampGreen
//   outputs[16] = bool_output[16] = lampRed

namespace MotorTaskIO {
    // Совпадает с MotorIO::BYTE_OUT/BIT_* из MotorSimulation.hpp
    constexpr size_t BYTE_OUT        = 1;
    constexpr size_t BIT_MOTOR_RUN   = 0;  // %QX1.0
    constexpr size_t BIT_LAMP_GREEN  = 1;  // %QX1.1
    constexpr size_t BIT_LAMP_RED    = 2;  // %QX1.2
}
// MotorControlTask — наблюдатель состояния мотора.
// Логика управления полностью перенесена в IEC программу MotorControl.
// Задача только читает выходы ProcessImage и печатает сообщения
// при изменении состояния motorRun.
class MotorControlTask : public ITickable {
public:
    explicit MotorControlTask(ProcessImage& image);
    void tick() override;

private:
    bool readOutput(size_t byteAddr, size_t bitAddr) const;

    ProcessImage& image_;
    bool          prevMotorRun_ = false;
};