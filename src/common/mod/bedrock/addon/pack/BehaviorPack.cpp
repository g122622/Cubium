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

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <utility>

namespace mc::mod::bedrock::addon {
namespace {

Result<std::filesystem::path> _resolveContainedPath(const std::string& packPath, const std::string& resource)
{
    namespace fs = std::filesystem;
    const fs::path relative(resource);
    if (resource.empty() || relative.is_absolute() || relative.has_root_name()) {
        return Error(ErrorCode::InvalidArgument, "Pack resource path must be relative: " + resource);
    }

    // 先解析链接和父目录，再验证真实路径归属，单纯检查字符串前缀不能覆盖链接跳转。
    std::error_code error;
    const auto root = fs::weakly_canonical(fs::path(packPath), error);
    if (error) {
        return Error(ErrorCode::FileReadFailed, "Cannot resolve pack directory: " + error.message());
    }

    const auto full = fs::weakly_canonical(root / relative, error);
    if (error) {
        return Error(ErrorCode::FileReadFailed, "Cannot resolve pack resource: " + error.message());
    }

    const auto contained = full.lexically_relative(root);
    if (contained.empty() || contained.is_absolute() || *contained.begin() == "..") {
        return Error(ErrorCode::InvalidArgument, "Resource path escapes pack directory: " + resource);
    }

    return full;
}

Result<std::filesystem::path> _resolveResource(const std::string& packPath, const std::string& resource)
{
    auto full = _resolveContainedPath(packPath, resource);
    if (full.failed()) {
        return full.error();
    }
    std::error_code error;
    if (!std::filesystem::is_regular_file(full.value(), error)) {
        return Error(ErrorCode::FileNotFound, "Pack resource is not a regular file: " + full.value().string());
    }

    return full;
}

} // namespace

Result<std::vector<std::string>> BehaviorPack::listResources(
    const std::string& directory, const std::string& suffix) const
{
    auto resolved = _resolveContainedPath(m_path, directory);
    if (resolved.failed()) {
        return resolved.error();
    }
    std::error_code error;
    if (!std::filesystem::exists(resolved.value(), error)) {
        if (error) {
            return Error(ErrorCode::FileReadFailed, "Cannot inspect pack directory: " + error.message());
        }
        return std::vector<std::string>{};
    }
    if (!std::filesystem::is_directory(resolved.value(), error)) {
        return Error(ErrorCode::InvalidData, "Pack content path is not a directory: " + directory);
    }

    std::vector<std::string> files;
    const auto root = std::filesystem::weakly_canonical(std::filesystem::path(m_path), error);
    if (error) {
        return Error(ErrorCode::FileReadFailed, "Cannot resolve pack root: " + error.message());
    }
    auto iterator = std::filesystem::recursive_directory_iterator(resolved.value(), error);
    const auto end = std::filesystem::recursive_directory_iterator();
    while (iterator != end && !error) {
        const auto& entry = *iterator;
        if (entry.is_regular_file(error) && entry.path().generic_string().ends_with(suffix)) {
            const auto relative = entry.path().lexically_relative(root);
            auto contained = _resolveResource(m_path, relative.generic_string());
            if (contained.failed()) {
                return contained.error();
            }
            files.push_back(relative.generic_string());
        }
        iterator.increment(error);
    }
    if (error) {
        return Error(ErrorCode::FileReadFailed, "Cannot enumerate pack resources: " + error.message());
    }
    std::sort(files.begin(), files.end());
    return files;
}

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
    auto resolved = _resolveResource(m_path, relativePath);
    if (resolved.failed()) {
        return resolved.error();
    }

    const auto& fullPath = resolved.value();

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

Result<std::vector<u8>> BehaviorPack::readResource(const std::string& relativePath) const
{
    auto resolved = _resolveResource(m_path, relativePath);
    if (resolved.failed()) {
        return resolved.error();
    }

    const auto& fullPath = resolved.value();

    if (!std::filesystem::exists(fullPath)) {
        return Error(ErrorCode::FileNotFound, "Resource not found: " + fullPath.string());
    }

    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, "Cannot open resource: " + fullPath.string());
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize < 0) {
        return Error(ErrorCode::FileReadFailed, "Cannot determine resource size: " + fullPath.string());
    }

    if (fileSize == 0) {
        return std::vector<u8>{};
    }

    file.seekg(0, std::ios::beg);
    std::vector<u8> data(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(data.data()), fileSize)) {
        return Error(ErrorCode::FileReadFailed, "Failed to read resource: " + fullPath.string());
    }

    return data;
}

} // namespace mc::mod::bedrock::addon
