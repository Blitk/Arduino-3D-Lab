
#pragma once

#include "emulator/Memory.hpp"

#include <string>

namespace arduino3d::emulator {

class FirmwareLoader {
public:
    // Carrega um Intel HEX validado na Flash do ATmega328P.
    // Retorna false e preenche error se houver falha.
    static bool loadIntelHex(
        const std::string& path,
        Memory& memory,
        std::string* error = nullptr
    );
};

} // namespace arduino3d::emulator
