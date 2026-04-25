// direct_address_parser.hpp
#pragma once
#include "../compiler/ast.hpp"
#include "../platform/io_driver.hpp"
#include <string>
#include <stdexcept>
#include <cctype>

// Парсер строки вида %IX0.0, %QW1, %MD0
class DirectAddressParser {
public:
    static DirectAddress parse(const std::string& s) {
        DirectAddress addr{};

        if (s.empty() || s[0] != '%')
            throw std::runtime_error("Direct address must start with '%'");

        size_t pos = 1;

        // ── Префикс ───────────────────────────────────────────
        if (pos >= s.size())
            throw std::runtime_error("Missing prefix in: " + s);

        switch (s[pos++]) {
            case 'I': addr.prefix = DirectAddressPrefix::INPUT;  break;
            case 'Q': addr.prefix = DirectAddressPrefix::OUTPUT; break;
            case 'M': addr.prefix = DirectAddressPrefix::MEMORY; break;
            default:
                throw std::runtime_error(
                    "Unknown prefix '" + std::string(1, s[pos-1]) +
                    "' in: " + s);
        }

        // ── Размер (опционально, default = X для BOOL, W для INT) ─
        if (pos < s.size() &&
            (s[pos] == 'X' || s[pos] == 'B' ||
             s[pos] == 'W' || s[pos] == 'D')) {
            switch (s[pos++]) {
                case 'X': addr.size = DirectAddressSize::BIT;   break;
                case 'B': addr.size = DirectAddressSize::BYTE;  break;
                case 'W': addr.size = DirectAddressSize::WORD;  break;
                case 'D': addr.size = DirectAddressSize::DWORD; break;
            }
        } else {
            // Если размер не указан — по умолчанию WORD
            addr.size = DirectAddressSize::WORD;
        }

        // ── Адрес байта ───────────────────────────────────────
        if (pos >= s.size() ||
            !std::isdigit(static_cast<unsigned char>(s[pos])))
            throw std::runtime_error("Missing byte address in: " + s);

        addr.byteAddr = 0;
        while (pos < s.size() &&
               std::isdigit(static_cast<unsigned char>(s[pos]))) {
            addr.byteAddr = static_cast<uint16_t>(
                addr.byteAddr * 10 + (s[pos++] - '0'));
        }

        // ── Адрес бита (опционально) ──────────────────────────
        addr.hasBit = false;
        if (pos < s.size() && s[pos] == '.') {
            ++pos;
            if (pos >= s.size() ||
                !std::isdigit(static_cast<unsigned char>(s[pos])))
                throw std::runtime_error("Missing bit address in: " + s);
            addr.bitAddr = static_cast<uint8_t>(s[pos++] - '0');
            addr.hasBit  = true;
        }

        // ── Валидация соответствия типа и размера ─────────────
        if (addr.size == DirectAddressSize::BIT && !addr.hasBit) {
            // %IX0 без .bit — подразумеваем .0
            addr.bitAddr = 0;
            addr.hasBit  = true;
        }

        return addr;
    }

    // Вычислить IODirection из префикса
    static IODirection toIODirection(DirectAddressPrefix pfx) {
        switch (pfx) {
            case DirectAddressPrefix::INPUT:  return IODirection::INPUT;
            case DirectAddressPrefix::OUTPUT: return IODirection::OUTPUT;
            case DirectAddressPrefix::MEMORY: return IODirection::INOUT;
        }
        return IODirection::INPUT;
    }

    // Вычислить ValueType из размера
    static ValueType toValueType(DirectAddressSize sz) {
        switch (sz) {
            case DirectAddressSize::BIT:   return ValueType::BOOL;
            case DirectAddressSize::BYTE:  return ValueType::INT;
            case DirectAddressSize::WORD:  return ValueType::INT;
            case DirectAddressSize::DWORD: return ValueType::DINT;
        }
        return ValueType::DINT;
    }
};