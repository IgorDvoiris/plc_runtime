// serializer.cpp
#include "serializer.hpp"
#include <fstream>
#include <cstring>
#include <stdexcept>

void Serializer::save(const PlcBytecodeFile& file,
                      const std::string& path)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) throw std::runtime_error("Cannot open for write: " + path);

    PlcBcHeader hdr{};
    std::memcpy(hdr.magic, "PLCB", 4);
    hdr.version = 0x0100;
    hdr.numPOUs = static_cast<uint16_t>(file.pous.size());
    ofs.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));

    hdr.dataSegOff = static_cast<uint32_t>(ofs.tellp());
    for (auto& pou : file.pous) {
        uint16_t varCount = static_cast<uint16_t>(pou.vars.size());
        ofs.write(reinterpret_cast<char*>(&varCount), 2);
        for (auto& v : pou.vars) {
            uint8_t  type    = static_cast<uint8_t>(v.type);
            uint32_t initVal = 0;
            if      (v.type == IECType::BOOL)
                initVal = std::get<bool>(v.initialValue) ? 1u : 0u;
            else if (v.type == IECType::INT)
                initVal = static_cast<uint16_t>(
                    std::get<int16_t>(v.initialValue));
            else if (v.type == IECType::DINT)
                initVal = static_cast<uint32_t>(
                    std::get<int32_t>(v.initialValue));
            else if (v.type == IECType::REAL) {
                float fv = std::get<float>(v.initialValue);
                std::memcpy(&initVal, &fv, 4);
            }
            ofs.write(reinterpret_cast<char*>(&type),    1);
            ofs.write(reinterpret_cast<char*>(&initVal), 4);
        }
    }

    hdr.codeSegOff = static_cast<uint32_t>(ofs.tellp());
    for (auto& pou : file.pous) {
        uint8_t nameLen = static_cast<uint8_t>(pou.name.size());
        ofs.write(reinterpret_cast<const char*>(&nameLen), 1);
        ofs.write(pou.name.c_str(), nameLen);
        uint32_t codeSize = static_cast<uint32_t>(pou.code.size());
        ofs.write(reinterpret_cast<char*>(&codeSize), 4);
        ofs.write(reinterpret_cast<const char*>(
                      pou.code.data()), codeSize);
    }

    hdr.symTabOff = static_cast<uint32_t>(ofs.tellp());
    for (auto& sym : file.symbolTable) {
        uint16_t len = static_cast<uint16_t>(sym.size());
        ofs.write(reinterpret_cast<char*>(&len), 2);
        ofs.write(sym.c_str(), len);
    }

    ofs.seekp(0);
    ofs.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));
}

PlcBytecodeFile Serializer::load(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open: " + path);

    PlcBcHeader hdr{};
    ifs.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (std::memcmp(hdr.magic, "PLCB", 4) != 0)
        throw std::runtime_error("Invalid PLCB magic");

    PlcBytecodeFile file;

    ifs.seekg(hdr.dataSegOff);
    for (int p = 0; p < hdr.numPOUs; ++p) {
        POUBytecode pou;
        uint16_t varCount = 0;
        ifs.read(reinterpret_cast<char*>(&varCount), 2);
        for (uint16_t i = 0; i < varCount; ++i) {
            VarDecl v{};
            uint8_t  type;
            uint32_t initVal;
            ifs.read(reinterpret_cast<char*>(&type),    1);
            ifs.read(reinterpret_cast<char*>(&initVal), 4);
            v.type  = static_cast<IECType>(type);
            v.index = i;
            switch (v.type) {
                case IECType::BOOL:  v.initialValue = initVal != 0u; break;
                case IECType::INT:   v.initialValue = static_cast<int16_t>(initVal); break;
                case IECType::DINT:  v.initialValue = static_cast<int32_t>(initVal); break;
                case IECType::REAL: {
                    float fv;
                    std::memcpy(&fv, &initVal, 4);
                    v.initialValue = fv;
                    break;
                }
                default: v.initialValue = int32_t{0};
            }
            pou.vars.push_back(v);
        }
        file.pous.push_back(std::move(pou));
    }

    ifs.seekg(hdr.codeSegOff);
    for (int p = 0; p < hdr.numPOUs; ++p) {
        uint8_t nameLen;
        ifs.read(reinterpret_cast<char*>(&nameLen), 1);
        file.pous[static_cast<size_t>(p)].name.resize(nameLen);
        ifs.read(&file.pous[static_cast<size_t>(p)].name[0], nameLen);
        uint32_t codeSize;
        ifs.read(reinterpret_cast<char*>(&codeSize), 4);
        file.pous[static_cast<size_t>(p)].code.resize(codeSize);
        ifs.read(reinterpret_cast<char*>(
            file.pous[static_cast<size_t>(p)].code.data()), codeSize);
    }

    return file;
}
