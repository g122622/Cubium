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

#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/pack/AddonManifest.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <utility>

namespace mc::mod::bedrock::addon {

BehaviorPack::BehaviorPack(std::string path, AddonManifest manifest)
    : m_path(std::move(path))
    , m_manifest(std::move(manifest))
{}

const std::string& BehaviorPack::path() const
{
    return m_path;
}

const AddonManifest& BehaviorPack::manifest() const
{
    return m_manifest;
}

const std::string& BehaviorPack::uuid() const
{
    return m_manifest.header.uuid;
}

const std::string& BehaviorPack::name() const
{
    return m_manifest.header.name;
}

bool BehaviorPack::isEnabled() const
{
    return m_enabled;
}

void BehaviorPack::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

i32 BehaviorPack::priority() const
{
    return m_priority;
}

void BehaviorPack::setPriority(i32 priority)
{
    m_priority = priority;
}

Result<std::string> BehaviorPack::readScriptFile(const std::string& relativePath) const
{
    std::filesystem::path fullPath = std::filesystem::path(m_path) / relativePath;

    if (!std::filesystem::exists(fullPath)) {
        return Error(ErrorCode::FileNotFound, "Script file not found: " + fullPath.string());
    }

    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, "Cannot open script file: " + fullPath.string());
    }

    try {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return content;
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileReadFailed, std::string("Failed to read script file: ") + e.what());
    }
}

} // namespace mc::mod::bedrock::addon
