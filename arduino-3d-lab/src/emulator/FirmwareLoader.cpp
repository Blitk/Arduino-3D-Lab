
#include "emulator/FirmwareLoader.hpp"

#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace arduino3d::emulator {
namespace {

bool fail(std::string* error, const std::string& message)
{
    if (error) {
        *error = message;
    }
    return false;
}

int hexDigit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool parseByte(const std::string& line, std::size_t pos, uint8_t& out)
{
    if (pos + 1 >= line.size()) return false;

    const int hi = hexDigit(line[pos]);
    const int lo = hexDigit(line[pos + 1]);

    if (hi < 0 || lo < 0) return false;

    out = static_cast<uint8_t>((hi << 4) | lo);
    return true;
}

} // namespace

bool FirmwareLoader::loadIntelHex(
    const std::string& path,
    Memory& memory,
    std::string* error)
{
    std::ifstream file(path);
    if (!file) {
        return fail(error, "Não foi possível abrir o firmware: " + path);
    }

    std::vector<uint8_t> image(Memory::FlashSize, 0xFF);
    std::vector<bool> written(Memory::FlashSize, false);

    uint32_t addressBase = 0;
    bool eofSeen = false;
    bool dataSeen = false;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;

        // Compatibilidade com arquivos Intel HEX no formato CRLF (Windows).
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) continue;

        const auto lineError = [&](const std::string& message) {
            return fail(
                error,
                "Intel HEX inválido na linha " +
                    std::to_string(lineNumber) + ": " + message
            );
        };

        if (eofSeen) {
            return lineError("conteúdo encontrado após o registro EOF");
        }

        if (line[0] != ':' || line.size() < 11 ||
            ((line.size() - 1) % 2) != 0) {
            return lineError("estrutura do registro inválida");
        }

        uint8_t count = 0;
        uint8_t addrHi = 0;
        uint8_t addrLo = 0;
        uint8_t type = 0;

        if (!parseByte(line, 1, count) ||
            !parseByte(line, 3, addrHi) ||
            !parseByte(line, 5, addrLo) ||
            !parseByte(line, 7, type)) {
            return lineError("campo hexadecimal inválido");
        }

        const std::size_t expectedLength =
            1 + 2 * (static_cast<std::size_t>(count) + 5);

        if (line.size() != expectedLength) {
            return lineError("tamanho não corresponde ao campo de dados");
        }

        const uint16_t address = static_cast<uint16_t>(
            (static_cast<uint16_t>(addrHi) << 8) | addrLo
        );

        std::vector<uint8_t> data;
        data.reserve(count);

        uint8_t checksumSum = static_cast<uint8_t>(
            count + addrHi + addrLo + type
        );

        for (std::size_t i = 0; i < count; ++i) {
            uint8_t value = 0;
            if (!parseByte(line, 9 + i * 2, value)) {
                return lineError("byte de dados inválido");
            }
            data.push_back(value);
            checksumSum = static_cast<uint8_t>(checksumSum + value);
        }

        uint8_t checksum = 0;
        if (!parseByte(line, 9 + count * 2, checksum)) {
            return lineError("checksum inválido");
        }
        checksumSum = static_cast<uint8_t>(checksumSum + checksum);

        if (checksumSum != 0) {
            return lineError("falha na verificação do checksum");
        }

        switch (type) {
        case 0x00: { // Data
            const uint64_t start =
                static_cast<uint64_t>(addressBase) + address;
            const uint64_t end = start + count;

            if (end > Memory::FlashSize) {
                return lineError("dados ultrapassam o limite da Flash");
            }

            for (std::size_t i = 0; i < data.size(); ++i) {
                const auto index = static_cast<std::size_t>(start) + i;

                if (written[index] && image[index] != data[i]) {
                    return lineError("registros sobrepostos com dados conflitantes");
                }

                image[index] = data[i];
                written[index] = true;
            }

            dataSeen = dataSeen || !data.empty();
            break;
        }

        case 0x01: // EOF
            if (count != 0 || address != 0) {
                return lineError("registro EOF malformado");
            }
            eofSeen = true;
            break;

        case 0x04: // Extended Linear Address
            if (count != 2 || address != 0) {
                return lineError("registro de endereço estendido malformado");
            }

            addressBase =
                (static_cast<uint32_t>(data[0]) << 24) |
                (static_cast<uint32_t>(data[1]) << 16);

            if (addressBase != 0) {
                return lineError(
                    "endereço estendido fora do espaço de Flash suportado"
                );
            }
            break;

        default:
            return lineError(
                "tipo de registro não suportado: " + std::to_string(type)
            );
        }
    }

    if (file.bad()) {
        return fail(error, "Erro de leitura ao acessar o firmware: " + path);
    }

    if (!eofSeen) {
        return fail(error, "Intel HEX inválido: registro EOF ausente");
    }

    if (!dataSeen) {
        return fail(error, "Intel HEX inválido: nenhum byte de programa encontrado");
    }

    try {
        memory.loadProgram(image);
    } catch (const std::exception& ex) {
        return fail(
            error,
            std::string("Falha ao carregar a Flash do emulador: ") + ex.what()
        );
    }

    if (error) error->clear();
    return true;
}

} // namespace arduino3d::emulator
