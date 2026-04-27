// io_driver.hpp
#pragma once
#include <core/io/value.hpp>
#include <core/io/IOAddress.hpp>
#include <string>
#include <cstdint>

// Направление привязки
enum class IODirection : uint8_t {
    INPUT  = 0,   // физика → VM (читаем перед циклом)
    OUTPUT = 1,   // VM → физика (пишем после цикла)
    INOUT  = 2,   // двунаправленный
};

// Хэш для использования в unordered_map
struct IOAddressHash {
    size_t operator()(const IOAddress& a) const {
        size_t h = a.bus;
        h = h * 31 + a.device;
        h = h * 31 + a.channel;
        return h;
    }
};