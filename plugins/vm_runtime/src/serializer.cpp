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

    // ── Data segment ─────────────────────────────────────────
    hdr.dataSegOff = static_cast<uint32_t>(ofs.tellp());
    for (auto& pou : file.pous) {
        uint16_t varCount = static_cast<uint16_t>(pou.vars.size());
        ofs.write(reinterpret_cast<char*>(&varCount), 2);

        for (auto& v : pou.vars) {
            // Тип переменной
            // [type:1][initVal:4][nameLen:1][name:N][hasAddr:1]([addr:5])

            uint8_t type = static_cast<uint8_t>(v.type);
            ofs.write(reinterpret_cast<char*>(&type), 1);

            // Начальное значение (4 байта)
            uint32_t initVal = 0;
            if (v.type == IECType::BOOL)
                initVal = std::get<bool>(v.initialValue) ? 1u : 0u;
            else if (v.type == IECType::INT)
                initVal = static_cast<uint32_t>(
                    static_cast<uint16_t>(
                        std::get<int16_t>(v.initialValue)));
            else if (v.type == IECType::DINT)
                initVal = static_cast<uint32_t>(
                    std::get<int32_t>(v.initialValue));
            else if (v.type == IECType::REAL) {
                float fv = std::get<float>(v.initialValue);
                std::memcpy(&initVal, &fv, 4);
            }
            ofs.write(reinterpret_cast<char*>(&initVal), 4);

            // ── Имя переменной ────────────────────────────────
            // Формат: [nameLen:1][name:nameLen]
            uint8_t nameLen = static_cast<uint8_t>(
                v.name.size() > 255 ? 255 : v.name.size());
            ofs.write(reinterpret_cast<char*>(&nameLen), 1);
            ofs.write(v.name.c_str(), nameLen);

            // Прямой адрес
            uint8_t hasAddr = v.hasDirectAddress ? 1u : 0u;
            ofs.write(reinterpret_cast<char*>(&hasAddr), 1);

            if (v.hasDirectAddress) {
                // Компактная запись: [prefix:1][size:1][byteAddr:2][bitAddr:1][hasBit:1]
                uint8_t prefix  = static_cast<uint8_t>(v.directAddress.prefix);
                uint8_t sz      = static_cast<uint8_t>(v.directAddress.size);
                uint16_t bAddr  = v.directAddress.byteAddr;
                uint8_t  bitA   = v.directAddress.bitAddr;
                uint8_t  hasBit = v.directAddress.hasBit ? 1u : 0u;

                ofs.write(reinterpret_cast<char*>(&prefix), 1);
                ofs.write(reinterpret_cast<char*>(&sz),     1);
                ofs.write(reinterpret_cast<char*>(&bAddr),  2);
                ofs.write(reinterpret_cast<char*>(&bitA),   1);
                ofs.write(reinterpret_cast<char*>(&hasBit), 1);
            }
        }
    }

    // ── Code segment ─────────────────────────────────────────
    hdr.codeSegOff = static_cast<uint32_t>(ofs.tellp());
    for (auto& pou : file.pous) {
        uint8_t nameLen = static_cast<uint8_t>(
            pou.name.size() > 255 ? 255 : pou.name.size());
        ofs.write(reinterpret_cast<char*>(&nameLen), 1);
        ofs.write(pou.name.c_str(), nameLen);

        uint32_t codeSize = static_cast<uint32_t>(pou.code.size());
        ofs.write(reinterpret_cast<char*>(&codeSize), 4);
        ofs.write(reinterpret_cast<const char*>(
                      pou.code.data()), codeSize);
    }

    // ── Symbol table ─────────────────────────────────────────
    hdr.symTabOff = static_cast<uint32_t>(ofs.tellp());
    for (auto& sym : file.symbolTable) {
        uint16_t len = static_cast<uint16_t>(sym.size());
        ofs.write(reinterpret_cast<char*>(&len), 2);
        ofs.write(sym.c_str(), len);
    }

    // Перезаписываем заголовок с актуальными offset'ами
    ofs.seekp(0);
    ofs.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));
}

PlcBytecodeFile Serializer::load(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open: " + path);

    PlcBcHeader hdr{};
    ifs.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (std::memcmp(hdr.magic, "PLCB", 4) != 0)
        throw std::runtime_error("Invalid PLCB magic");

    PlcBytecodeFile file;

    // ── Data segment ─────────────────────────────────────────
    ifs.seekg(hdr.dataSegOff);
    for (uint16_t p = 0; p < hdr.numPOUs; ++p) {
        POUBytecode pou;

        uint16_t varCount = 0;
        ifs.read(reinterpret_cast<char*>(&varCount), 2);

        for (uint16_t i = 0; i < varCount; ++i) {
            VarDecl v{};

            // Тип
            uint8_t type = 0;
            ifs.read(reinterpret_cast<char*>(&type), 1);
            v.type = static_cast<IECType>(type);

            // Начальное значение
            uint32_t initVal = 0;
            ifs.read(reinterpret_cast<char*>(&initVal), 4);
            switch (v.type) {
                case IECType::BOOL:
                    v.initialValue = initVal != 0u;
                    break;
                case IECType::INT:
                    v.initialValue = static_cast<int16_t>(initVal);
                    break;
                case IECType::DINT:
                    v.initialValue = static_cast<int32_t>(initVal);
                    break;
                case IECType::REAL: {
                    float fv;
                    std::memcpy(&fv, &initVal, 4);
                    v.initialValue = fv;
                    break;
                }
                default:
                    v.initialValue = int32_t{0};
            }

            // ── Имя переменной ────────────────────────────────
            uint8_t nameLen = 0;
            ifs.read(reinterpret_cast<char*>(&nameLen), 1);
            v.name.resize(nameLen);
            if (nameLen > 0)
                ifs.read(&v.name[0], nameLen);

            uint8_t hasAddr = 0;
            ifs.read(reinterpret_cast<char*>(&hasAddr), 1);
            v.hasDirectAddress = (hasAddr != 0);

            if (v.hasDirectAddress) {
                uint8_t  prefix = 0, sz = 0, bitA = 0, hasBit = 0;
                uint16_t bAddr  = 0;
                ifs.read(reinterpret_cast<char*>(&prefix), 1);
                ifs.read(reinterpret_cast<char*>(&sz),     1);
                ifs.read(reinterpret_cast<char*>(&bAddr),  2);
                ifs.read(reinterpret_cast<char*>(&bitA),   1);
                ifs.read(reinterpret_cast<char*>(&hasBit), 1);

                v.directAddress.prefix   = static_cast<DirectAddressPrefix>(prefix);
                v.directAddress.size     = static_cast<DirectAddressSize>(sz);
                v.directAddress.byteAddr = bAddr;
                v.directAddress.bitAddr  = bitA;
                v.directAddress.hasBit   = (hasBit != 0);
            }

            v.index = i;
            v.kind  = VarDecl::Kind::LOCAL;

            // ── Заполняем varMap ──────────────────────────────
            pou.varMap[v.name] = i;
            pou.vars.push_back(std::move(v));
        }

        file.pous.push_back(std::move(pou));
    }

    // ── Code segment ─────────────────────────────────────────
    ifs.seekg(hdr.codeSegOff);
    for (uint16_t p = 0; p < hdr.numPOUs; ++p) {
        uint8_t nameLen = 0;
        ifs.read(reinterpret_cast<char*>(&nameLen), 1);
        file.pous[p].name.resize(nameLen);
        if (nameLen > 0)
            ifs.read(&file.pous[p].name[0], nameLen);

        uint32_t codeSize = 0;
        ifs.read(reinterpret_cast<char*>(&codeSize), 4);
        file.pous[p].code.resize(codeSize);
        ifs.read(reinterpret_cast<char*>(
                     file.pous[p].code.data()), codeSize);
    }

    // ── Symbol table ─────────────────────────────────────────
    ifs.seekg(hdr.symTabOff);
    while (ifs.peek() != EOF) {
        uint16_t len = 0;
        if (!ifs.read(reinterpret_cast<char*>(&len), 2)) break;
        std::string sym(len, '\0');
        if (len > 0)
            ifs.read(&sym[0], len);
        file.symbolTable.push_back(std::move(sym));
    }

    return file;
}