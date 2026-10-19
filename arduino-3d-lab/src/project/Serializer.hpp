#pragma once

#include "project/Project.hpp"

#include <filesystem>
#include <string>

namespace arduino3d::project {

class Serializer {
public:
    [[nodiscard]] static Json toJson(const Project& project);

    // Só modifica destination quando o JSON inteiro for válido.
    [[nodiscard]] static bool fromJson(
        const Json& json,
        Project& destination,
        std::string* error = nullptr
    );

    [[nodiscard]] static bool save(
        const Project& project,
        const std::filesystem::path& file,
        std::string* error = nullptr
    );

    [[nodiscard]] static bool load(
        const std::filesystem::path& file,
        Project& destination,
        std::string* error = nullptr
    );
};

} // namespace arduino3d::project
