// simulation/MotorSimulation.cpp
#include "MotorSimulation.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

MotorSimulation::MotorSimulation(IoBuffer& ioBuffer) : ioBuffer_(ioBuffer) {}
MotorSimulation::~MotorSimulation() { stop(); }

void MotorSimulation::start() {
    running_ = true;
    thread_  = std::thread(&MotorSimulation::run, this);
}

void MotorSimulation::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void MotorSimulation::run() {
    using namespace std::chrono_literals;

    struct Event {
        int64_t     timeMs;
        const char* description;
        bool        btnStart, btnStop, fault;
    };

    const Event events[] = {
        {    0, "--- Система запущена ---",         false, false, false},
        { 2000, ">>> Нажата кнопка СТАРТ",          true,  false, false},
        { 2200, "    Кнопка СТАРТ отпущена",        false, false, false},
        { 5000, "!!! Сработал аварийный датчик",    false, false, true },
        { 6000, "    Авария сброшена",              false, false, false},
        { 8000, ">>> Нажата кнопка СТАРТ (повтор)", true,  false, false},
        { 8200, "    Кнопка СТАРТ отпущена",        false, false, false},
        {11000, ">>> Нажата кнопка СТОП",           false, true,  false},
        {11200, "    Кнопка СТОП отпущена",         false, false, false},
        {13000, "--- Завершение симуляции ---",     false, false, false},
    };

    const auto startTime = std::chrono::steady_clock::now();
    size_t nextEvent = 0;
    std::cout << "\n[Simulation] поток запущен\n\n";

    //ioBuffer_.setOutputByte(MotorIO::IDX_MOTOR_RUN, 0u);

    while (running_) {
        const auto now = std::chrono::steady_clock::now();
        const int64_t elapsedMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - startTime).count();

        while (nextEvent < std::size(events) &&
               elapsedMs >= events[nextEvent].timeMs) {
            const auto& ev = events[nextEvent];
            std::cout << "\n[Simulation] t=" << std::setw(5)
                      << ev.timeMs << "ms  " << ev.description << "\n";
            // Писать каждый сигнал в отдельный байт ProcessImage.inputs[N]
            // Matiec: %IX0.0 → bool_input[0] → один байт, значение 0 или 1
            ioBuffer_.setBoolInput(MotorIO::BYTE_IN, MotorIO::BIT_BTN_START, ev.btnStart);
            ioBuffer_.setBoolInput(MotorIO::BYTE_IN, MotorIO::BIT_BTN_STOP,  ev.btnStop);
            ioBuffer_.setBoolInput(MotorIO::BYTE_IN, MotorIO::BIT_FAULT,     ev.fault);
            ++nextEvent;
        }

        if (nextEvent >= std::size(events)) { running_ = false; break; }

        //printState();
        std::this_thread::sleep_for(500ms);
    }
    std::cout << "\n[Simulation] поток завершён\n";
}

void MotorSimulation::printState() {
    const bool motorRun  = ioBuffer_.getBoolOutput(MotorIO::BYTE_OUT, MotorIO::BIT_MOTOR_RUN);
    const bool lampGreen = ioBuffer_.getBoolOutput(MotorIO::BYTE_OUT, MotorIO::BIT_LAMP_GREEN);
    const bool lampRed   = ioBuffer_.getBoolOutput(MotorIO::BYTE_OUT, MotorIO::BIT_LAMP_RED);
    std::cout << "[Outputs]  Motor=" << (motorRun  ? "ON " : "off")
              << "  Green=" << (lampGreen ? "ON " : "off")
              << "  Red="   << (lampRed   ? "ON " : "off") << "\n";
}
