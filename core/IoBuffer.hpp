// core/IoBuffer.hpp
#pragma once

#include "ProcessImage.hpp"
#include <mutex>
#include <cstring>

// IoBuffer — двойной буфер на границе IO System / IEC Runtime.
//
// Модель хранения соответствует Matiec/OpenPLC glueVars:
//   bool_input[byte][bit]  → ProcessImage.inputs[byte * 8 + bit]
//   bool_output[byte][bit] → ProcessImage.outputs[byte * 8 + bit]
//
// Каждая IEC_BOOL переменная занимает ОДИН БАЙТ (0 или 1).
// Это соответствует тому как Matiec хранит IEC_BOOL в памяти.
//
// IO System thread:
//   setBoolInput()   — записать IEC_BOOL вход
//   getBoolOutput()  — прочитать IEC_BOOL выход
//
// tick-loop:
//   copyInputsToProcessImage()    — shadow → PI (начало цикла)
//   copyOutputsFromProcessImage() — PI → shadow (конец цикла)

class IoBuffer {
public:
    // ── IO System API ─────────────────────────────────────────────────
    //
    // Параметры соответствуют адресу IEC 61131-3:
    //   byteAddr — первый индекс в glueVars: bool_input[byteAddr][bitAddr]
    //   bitAddr  — второй индекс в glueVars: bool_input[byteAddr][bitAddr]
    //
    // Пример: %IX0.0 → setInputBool(0, 0, value)
    //         %QX1.0 → getBoolOutput(1, 0)

    void setBoolInput(size_t byteAddr, size_t bitAddr, bool value) {
        const size_t index = byteAddr * 8 + bitAddr;
        std::lock_guard<std::mutex> lock(inputMutex_);
        if (index >= PROCESS_IMAGE_SIZE) return;
        inputShadow_[index] = value ? 1u : 0u;
    }

    bool getBoolInput(size_t byteAddr, size_t bitAddr) {
        const size_t index = byteAddr * 8 + bitAddr;
        return inputShadow_[index];
    }

    bool getBoolOutput(size_t byteAddr, size_t bitAddr) const {
        const size_t index = byteAddr * 8 + bitAddr;
        std::lock_guard<std::mutex> lock(outputMutex_);
        if (index >= PROCESS_IMAGE_SIZE) return false;
        return outputShadow_[index] != 0;
    }

    // ── tick-loop API ─────────────────────────────────────────────────

    void copyInputsToProcessImage(ProcessImage& image) {
        std::lock_guard<std::mutex> lock(inputMutex_);
        std::memcpy(image.inputs, inputShadow_, PROCESS_IMAGE_SIZE);
    }

    void copyOutputsFromProcessImage(const ProcessImage& image) {
        std::lock_guard<std::mutex> lock(outputMutex_);
        std::memcpy(outputShadow_, image.outputs, PROCESS_IMAGE_SIZE);
    }

private:
    uint8_t inputShadow_ [PROCESS_IMAGE_SIZE] = {};
    uint8_t outputShadow_[PROCESS_IMAGE_SIZE] = {};

    mutable std::mutex inputMutex_;
    mutable std::mutex outputMutex_;
};