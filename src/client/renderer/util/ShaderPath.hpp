/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

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
