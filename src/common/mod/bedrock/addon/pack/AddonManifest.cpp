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

#include "common/mod/bedrock/addon/pack/AddonManifest.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/pack/AddonDependency.hpp"
#include "common/mod/bedrock/addon/pack/AddonModule.hpp"
#include "common/mod/bedrock/addon/pack/PackVersion.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::mod::bedrock::addon {

namespace {

PackVersion parseVersion(const nlohmann::json& j)
{
    if (!j.is_array()) {
        return PackVersion{};
    }
    std::vector<i32> parts;
    for (const auto& part : j) {
        if (part.is_number_integer()) {
            parts.push_back(part.get<i32>());
        }
    }
    return PackVersion::fromVector(parts);
}

AddonModule parseModule(const nlohmann::json& j)
{
    AddonModule module;

    // 解析类型
    if (j.contains("type") && j["type"].is_string()) {
        module.type = AddonModule::parseType(j["type"].get<std::string>());
    }

    // 解析UUID
    if (j.contains("uuid") && j["uuid"].is_string()) {
        module.uuid = j["uuid"].get<std::string>();
    }

    // 解析版本
    if (j.contains("version")) {
        module.version = parseVersion(j["version"]);
    }

    // 解析名称（仅脚本模块）
    if (j.contains("name") && j["name"].is_string()) {
        module.name = j["name"].get<std::string>();
    }

    // 解析入口点
    if (j.contains("entry") && j["entry"].is_string()) {
        module.entry = j["entry"].get<std::string>();
    }

    // 解析语言
    if (j.contains("language") && j["language"].is_string()) {
        module.language = j["language"].get<std::string>();
    }

    return module;
}

} // namespace

Result<AddonManifest> AddonManifest::parse(const std::string& json)
{
    try {
        auto j = nlohmann::json::parse(json);
        AddonManifest manifest;

        // 解析格式版本
        if (j.contains("format_version")) {
            if (j["format_version"].is_number_integer()) {
                manifest.formatVersion = j["format_version"].get<i32>();
            } else if (j["format_version"].is_string()) {
                // 尝试解析字符串格式的版本号
                manifest.formatVersion = std::stoi(j["format_version"].get<std::string>());
            }
        }

        // 只支持格式版本2
        if (manifest.formatVersion != 2) {
            return Error(ErrorCode::InvalidData,
                "Unsupported manifest format version: " + std::to_string(manifest.formatVersion));
        }

        // 解析头部
        if (!j.contains("header") || !j["header"].is_object()) {
            return Error(ErrorCode::InvalidData, "Manifest missing header");
        }

        const auto& header = j["header"];
        if (header.contains("name") && header["name"].is_string()) {
            manifest.header.name = header["name"].get<std::string>();
        }
        if (header.contains("description") && header["description"].is_string()) {
            manifest.header.description = header["description"].get<std::string>();
        }
        if (header.contains("uuid") && header["uuid"].is_string()) {
            manifest.header.uuid = header["uuid"].get<std::string>();
        }
        if (header.contains("version")) {
            manifest.header.version = parseVersion(header["version"]);
        }
        if (header.contains("min_engine_version")) {
            manifest.header.minEngineVersion = parseVersion(header["min_engine_version"]);
        }

        // 解析模块
        if (j.contains("modules") && j["modules"].is_array()) {
            for (const auto& moduleJson : j["modules"]) {
                manifest.modules.push_back(parseModule(moduleJson));
            }
        }

        // 解析依赖
        if (j.contains("dependencies") && j["dependencies"].is_array()) {
            for (const auto& depJson : j["dependencies"]) {
                manifest.dependencies.push_back(AddonDependency::fromJson(depJson));
            }
        }

        // 解析能力
        if (j.contains("capabilities") && j["capabilities"].is_array()) {
            for (const auto& capJson : j["capabilities"]) {
                if (capJson.is_string()) {
                    manifest.capabilities.push_back(capJson.get<std::string>());
                }
            }
        }

        return manifest;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Parse error: ") + e.what());
    }
}

Result<AddonManifest> AddonManifest::loadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return Error(ErrorCode::FileNotFound, "Cannot open manifest file: " + path);
    }

    try {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        auto result = parse(content);
        if (result.failed()) {
            return Error(
                result.error().code(), "Failed to parse manifest from " + path + ": " + result.error().message());
        }
        return result;
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileReadFailed, std::string("Failed to read manifest file: ") + e.what());
    }
}

bool AddonManifest::hasScriptModule() const noexcept
{
    return std::any_of(modules.begin(), modules.end(), [](const AddonModule& module) {
        return module.type == AddonModuleType::Script;
    });
}

std::vector<AddonModule> AddonManifest::getScriptModules() const
{
    std::vector<AddonModule> scriptModules;
    std::copy_if(modules.begin(), modules.end(), std::back_inserter(scriptModules), [](const AddonModule& module) {
        return module.type == AddonModuleType::Script;
    });
    return scriptModules;
}

bool AddonManifest::hasCapability(const std::string& cap) const noexcept
{
    return std::find(capabilities.begin(), capabilities.end(), cap) != capabilities.end();
}

} // namespace mc::mod::bedrock::addon
