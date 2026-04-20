// serializer.hpp
#pragma once
#include "../compiler/codegen.hpp"
#include <string>

#pragma pack(push, 1)
struct PlcBcHeader {
    char     magic[4];
    uint16_t version;
    uint16_t numPOUs;
    uint32_t dataSegOff;
    uint32_t codeSegOff;
    uint32_t symTabOff;
    uint8_t  reserved[8];
};
#pragma pack(pop)

class Serializer {
public:
    static void             save(const PlcBytecodeFile& file,
                                 const std::string& path);
    static PlcBytecodeFile  load(const std::string& path);
};
