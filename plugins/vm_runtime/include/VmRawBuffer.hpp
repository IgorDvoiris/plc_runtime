// plugins/iec_runtime/include/IecRawBuffer.hpp
#pragma once
#include "core/ProcessImage.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
// IecRawBuffer — типизированный адаптер поверх ProcessImage.
//
// Matiec использует плоские массивы (IEC_BOOL = uint8_t, 1 байт на переменную).
// %IX0.0 → bool_input[0], %IX0.1 → bool_input[1], %IX1.0 → bool_input[8].
//
// Раскладка в ProcessImage.inputs[]:
//   [0  .. 63 ] BOOL  (64 переменных)
//   [64 .. 95 ] BYTE  (32 переменных)
//   [96 .. 127] INT   (16 x 2 байта)
//   [128.. 159] DINT  (8  x 4 байта)
//   [160.. 191] LINT  (4  x 8 байт)

class VmRawBuffer {
public:
    using IEC_BOOL  = uint8_t;
    using IEC_BYTE  = uint8_t;
    using IEC_INT   = int16_t;
    using IEC_DINT  = int32_t;
    using IEC_LINT  = int64_t;

    static constexpr size_t BOOL_BUF_SIZE = 64;
    static constexpr size_t BYTE_BUF_SIZE = 32;
    static constexpr size_t INT_BUF_SIZE  = 16;
    static constexpr size_t DINT_BUF_SIZE = 8;
    static constexpr size_t LINT_BUF_SIZE = 4;

    static constexpr size_t BOOL_OFFSET  = 0;
    static constexpr size_t BYTE_OFFSET  = BOOL_OFFSET  + BOOL_BUF_SIZE;
    static constexpr size_t INT_OFFSET   = BYTE_OFFSET  + BYTE_BUF_SIZE;
    static constexpr size_t DINT_OFFSET  = INT_OFFSET   + INT_BUF_SIZE  * sizeof(IEC_INT);
    static constexpr size_t LINT_OFFSET  = DINT_OFFSET  + DINT_BUF_SIZE * sizeof(IEC_DINT);
    static constexpr size_t TOTAL_BYTES  = LINT_OFFSET  + LINT_BUF_SIZE * sizeof(IEC_LINT);

    static_assert(TOTAL_BYTES <= PROCESS_IMAGE_SIZE, "IecRawBuffer exceeds ProcessImage");

    explicit VmRawBuffer(ProcessImage& image) : image_(image) {}

    IEC_BOOL* boolInputData()  { return reinterpret_cast<IEC_BOOL*>(image_.inputs  + BOOL_OFFSET); }
    IEC_BOOL* boolOutputData() { return reinterpret_cast<IEC_BOOL*>(image_.outputs + BOOL_OFFSET); }
    IEC_BYTE* byteInputData()  { return reinterpret_cast<IEC_BYTE*>(image_.inputs  + BYTE_OFFSET); }
    IEC_BYTE* byteOutputData() { return reinterpret_cast<IEC_BYTE*>(image_.outputs + BYTE_OFFSET); }
    IEC_INT*  intInputData()   { return reinterpret_cast<IEC_INT* >(image_.inputs  + INT_OFFSET);  }
    IEC_INT*  intOutputData()  { return reinterpret_cast<IEC_INT* >(image_.outputs + INT_OFFSET);  }
    IEC_DINT* dintInputData()  { return reinterpret_cast<IEC_DINT*>(image_.inputs  + DINT_OFFSET); }
    IEC_DINT* dintOutputData() { return reinterpret_cast<IEC_DINT*>(image_.outputs + DINT_OFFSET); }
    IEC_LINT* lintInputData()  { return reinterpret_cast<IEC_LINT*>(image_.inputs  + LINT_OFFSET); }
    IEC_LINT* lintOutputData() { return reinterpret_cast<IEC_LINT*>(image_.outputs + LINT_OFFSET); }

    const IEC_BOOL* boolInputData()  const { return reinterpret_cast<const IEC_BOOL*>(image_.inputs  + BOOL_OFFSET); }
    const IEC_BOOL* boolOutputData() const { return reinterpret_cast<const IEC_BOOL*>(image_.outputs + BOOL_OFFSET); }
    const IEC_BYTE* byteInputData()  const { return reinterpret_cast<const IEC_BYTE*>(image_.inputs  + BYTE_OFFSET); }
    const IEC_BYTE* byteOutputData() const { return reinterpret_cast<const IEC_BYTE*>(image_.outputs + BYTE_OFFSET); }
    const IEC_INT*  intInputData()   const { return reinterpret_cast<const IEC_INT* >(image_.inputs  + INT_OFFSET);  }
    const IEC_INT*  intOutputData()  const { return reinterpret_cast<const IEC_INT* >(image_.outputs + INT_OFFSET);  }
    const IEC_DINT* dintInputData()  const { return reinterpret_cast<const IEC_DINT*>(image_.inputs  + DINT_OFFSET); }
    const IEC_DINT* dintOutputData() const { return reinterpret_cast<const IEC_DINT*>(image_.outputs + DINT_OFFSET); }
    const IEC_LINT* lintInputData()  const { return reinterpret_cast<const IEC_LINT*>(image_.inputs  + LINT_OFFSET); }
    const IEC_LINT* lintOutputData() const { return reinterpret_cast<const IEC_LINT*>(image_.outputs + LINT_OFFSET); }

    ProcessImage& image() { return image_; }

private:
    ProcessImage& image_;
};
