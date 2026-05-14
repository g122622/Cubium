#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

namespace mc::client {

inline std::filesystem::path resolveShaderPath(std::string_view shaderFileName)
{
    namespace fs = std::filesystem;

    const fs::path shaderName(shaderFileName);
    std::vector<fs::path> candidates;
    candidates.reserve(24);

    auto addCandidate = [&](const fs::path& candidate) {
        if (candidate.empty()) {
            return;
        }

        for (const auto& existing : candidates) {
            if (existing == candidate) {
                return;
            }
        }

        candidates.push_back(candidate);
    };

    auto addCandidatesFromBase = [&](const fs::path& base) {
        addCandidate(base / "build" / "shaders" / shaderName);
        addCandidate(base / "shaders" / shaderName);
        addCandidate(base / "bin" / "shaders" / shaderName);
    };

    fs::path currentPath = fs::current_path();
    for (int depth = 0; depth < 6; ++depth) {
        addCandidatesFromBase(currentPath);

        if (!currentPath.has_parent_path()) {
            break;
        }

        const fs::path parentPath = currentPath.parent_path();
        if (parentPath == currentPath) {
            break;
        }

        currentPath = parentPath;
    }

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return fs::weakly_canonical(candidate);
        }
    }

    return {};
}

} // namespace mc::client
