// core/ProcessImage.hpp
#pragma once

#include <cstdint>
#include <cstring>

// Process Image — байтовый снимок входов/выходов/памяти.
//
// IEC 61131-3: Process Image — безымянный буфер.
// Семантика адресов определяется пользовательской программой.
//
// Доступен только из tick-loop потока — без синхронизации.
// Синхронизация с IO потоком — через IoBuffer.

static constexpr size_t PROCESS_IMAGE_SIZE = 256;

struct ProcessImage {
    uint8_t inputs [PROCESS_IMAGE_SIZE] = {};  // %I*
    uint8_t outputs[PROCESS_IMAGE_SIZE] = {};  // %Q*
    uint8_t memory [PROCESS_IMAGE_SIZE] = {};  // %M*
};
