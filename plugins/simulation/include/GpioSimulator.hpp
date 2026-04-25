#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <core/value.hpp>"
#include "compiler/direct_address_parser.hpp"

class GpioSimulator {
public:
    // Вызывается из IOMapper::readInputs() → читает только %I
    Value read(uint16_t channel, DirectAddressSize size) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = input_state_.find(channel);
        return (it != input_state_.end()) ? it->second : defaultValue(size);
    }

    // Вызывается из IOMapper::writeOutputs() → пишет только в %Q
    void write(uint16_t channel, DirectAddressSize size, const Value& val) {
        std::lock_guard<std::mutex> lock(mutex_);
        output_state_[channel] = val;
        cv_.notify_all(); // Будим поток симуляции: выходы ПЛК обновились
    }

    // ── API для потока симуляции ─────────────────────────────
    void setInput(uint16_t channel, const Value& val) {
        std::lock_guard<std::mutex> lock(mutex_);
        input_state_[channel] = val;
        // notify не нужен: ПЛК читает входы только в начале такта
    }

    // Чтение физических выходов ПЛК из потока симуляции
    Value getOutput(uint16_t channel, DirectAddressSize size) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = output_state_.find(channel);
        return (it != output_state_.end()) ? it->second : defaultValue(size);
    }

    // Безопасное ожидание изменения выходов
    void waitForOutputChange(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, timeout);
    }

private:
    static Value defaultValue(DirectAddressSize sz) {
        switch (sz) {
            case DirectAddressSize::BIT:   return Value::fromBool(false);
            case DirectAddressSize::WORD:  return Value::fromInt(0);
            case DirectAddressSize::DWORD: return Value::fromDInt(0);
            default:                       return Value::fromDInt(0);
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::unordered_map<uint16_t, Value> input_state_;  // строго %I
    std::unordered_map<uint16_t, Value> output_state_; // строго %Q
};