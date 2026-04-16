// simulation/MotorControlTask.cpp
#include "MotorControlTask.hpp"
#include <iostream>

MotorControlTask::MotorControlTask(ProcessImage& image) : image_(image) {}

bool MotorControlTask::readOutput(size_t byteAddr, size_t bitAddr) const {
    const size_t index = byteAddr * 8 + bitAddr;
    if (index >= PROCESS_IMAGE_SIZE) return false;
    return image_.outputs[index] != 0;
}

void MotorControlTask::tick() {
    const bool motorRun  = readOutput(MotorTaskIO::BYTE_OUT, MotorTaskIO::BIT_MOTOR_RUN);
    const bool lampGreen = readOutput(MotorTaskIO::BYTE_OUT, MotorTaskIO::BIT_LAMP_GREEN);
    const bool lampRed   = readOutput(MotorTaskIO::BYTE_OUT, MotorTaskIO::BIT_LAMP_RED);

    if (motorRun != prevMotorRun_) {
        if (motorRun)
            std::cout << "[MotorControlTask] *** Мотор ЗАПУЩЕН ***"
                      << "  Green=" << (lampGreen ? "ON" : "off")
                      << "  Red="   << (lampRed   ? "ON" : "off") << "\n";
        else
            std::cout << "[MotorControlTask] --- Мотор ОСТАНОВЛЕН ---"
                      << "  Green=" << (lampGreen ? "ON" : "off")
                      << "  Red="   << (lampRed   ? "ON" : "off") << "\n";
        prevMotorRun_ = motorRun;
    }
}