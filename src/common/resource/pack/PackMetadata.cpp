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

#include "common/resource/pack/PackMetadata.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <exception>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::resource {

PackMetadata::PackMetadata(i32 packFormat, std::string description)
    : m_packFormat(packFormat)
    , m_description(std::move(description))
{}

Result<PackMetadata> PackMetadata::parse(std::string_view jsonContent)
{
    try {
        auto json = nlohmann::json::parse(jsonContent);

        PackMetadata metadata;

        if (json.contains("pack")) {
            const auto& pack = json["pack"];

            if (pack.contains("pack_format")) {
                metadata.m_packFormat = pack["pack_format"].get<i32>();
            }

            if (pack.contains("description")) {
                metadata.m_description = pack["description"].get<std::string>();
            }
        }

        return metadata;
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::ResourceParseError, std::string("Failed to parse pack.mcmeta: ") + e.what());
    }
}

Result<PackMetadata> PackMetadata::parseFile(std::string_view filePath)
{
    std::ifstream file(std::string(filePath), std::ios::binary);

    if (!file.is_open()) {
        return Error(ErrorCode::FileNotFound, std::string("Cannot open: ") + std::string(filePath));
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return parse(content);
}

bool PackMetadata::isCompatible(i32 minFormat, i32 maxFormat) const noexcept
{
    return m_packFormat >= minFormat && m_packFormat <= maxFormat;
}

} // namespace mc::resource
