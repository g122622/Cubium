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

#include "common/mod/bedrock/addon/pack/AddonDependency.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/pack/PackVersion.hpp"

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::mod::bedrock::addon {

AddonDependency AddonDependency::fromJson(const nlohmann::json& j)
{
    AddonDependency dep;

    // 解析UUID
    if (j.contains("uuid") && j["uuid"].is_string()) {
        dep.uuid = j["uuid"].get<std::string>();
    }

    // 解析版本
    if (j.contains("version") && j["version"].is_array()) {
        std::vector<i32> versionParts;
        for (const auto& part : j["version"]) {
            if (part.is_number_integer()) {
                versionParts.push_back(part.get<i32>());
            }
        }
        dep.version = PackVersion::fromVector(versionParts);
    }

    return dep;
}

} // namespace mc::mod::bedrock::addon
