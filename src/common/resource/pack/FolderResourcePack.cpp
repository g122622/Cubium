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

#include "common/resource/pack/FolderResourcePack.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/PackMetadata.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace mc::trace;

namespace fs = std::filesystem;

namespace mc::resource {

namespace {
std::string _getDirectoryName(const std::string& path)
{
    fs::path p(path);
    return p.filename().string();
}
} // namespace

FolderResourcePack::FolderResourcePack(std::string rootPath)
    : m_rootPath(std::move(rootPath))
    , m_name(_getDirectoryName(m_rootPath))
{}

Result<void> FolderResourcePack::initialize()
{
    // 检查根目录是否存在
    if (!fs::exists(m_rootPath)) {
        return Error(ErrorCode::ResourcePackNotFound, std::string("Resource pack not found: ") + m_rootPath);
    }

    if (!fs::is_directory(m_rootPath)) {
        return Error(
            ErrorCode::ResourcePackInvalid, std::string("Resource pack path is not a directory: ") + m_rootPath);
    }

    // 读取pack.mcmeta
    std::string mcmetaPath = m_rootPath + "/pack.mcmeta";

    if (fs::exists(mcmetaPath)) {
        auto result = PackMetadata::parseFile(mcmetaPath);
        if (result.success()) {
            m_metadata = result.value();
        } else {
            // pack.mcmeta存在但解析失败，继续但记录警告
            m_metadata = PackMetadata();
        }
    }

    // 一次性扫描包根，构建资源路径索引（assets/ 与 data/ 下的所有常规文件）。
    // 此后 listResources/hasResource/getResourceNamespaces 全部基于该内存索引查询，
    // 避免每次调用都重新递归遍历磁盘目录树。扫描失败视为致命错误（无法提供资源列举能力）。
    try {
        MC_TRACE_SCOPED_EVENT(TraceEvents.IO.Resource, "FolderResourcePack::initialize::IndexResources");

        m_entries.clear();
        const fs::path root(m_rootPath);
        const std::string rootStr = root.string();
        MC_ASSERT_RELEASE(!rootStr.empty());

        // 方向1：用字符串前缀裁剪替代 fs::relative。recursive_directory_iterator 产出的
        // entry.path() 必然以“rootStr + 一个分隔符”为字面前缀（迭代器只遍历 root 子树，
        // 不解析符号链接、不含 .. /.），故裁掉该前缀即得相对路径，与 fs::relative 结果等价，
        // 但避免了每个文件一次 fs::relative 内部的路径绝对化/规范化系统调用——后者会构造大量
        // 临时 fs::path 对象，是本阶段内存峰值（实测 IndexResources 期间工作集 +125MB）的主因。
        //
        // 前缀长度取 rootStr.size() + 1（多裁一个分隔符），保证裁到分隔符边界，
        // 不会把 rootX 误当作 root 的同级前缀（如 "packs/foo" vs "packs/foobar"）。
        const size_t prefixLen = rootStr.size() + 1;

        // 方向3：遍历逻辑放入独立作用域，使 recursive_directory_iterator 及其内部缓存的
        // 目录句柄/WIN32_FIND_DATA/递归栈在 m_entries 构建完成后立即析构，把遍历期间驻留的
        // 堆归还给 CRT，降低本阶段结束后的常驻内存。
        {
            for (const auto& entry : fs::recursive_directory_iterator(root)) {
                if (!entry.is_regular_file()) {
                    continue;
                }

                const std::string fullStr = entry.path().string();
                MC_ASSERT_RELEASE(fullStr.size() > prefixLen && fullStr.compare(0, rootStr.size(), rootStr) == 0);

                // 相对于包根的路径，统一为正斜杠
                std::string relativePath = fullStr.substr(prefixLen);
                for (char& c : relativePath) {
                    if (c == '\\') {
                        c = '/';
                    }
                }
                m_entries.insert(std::move(relativePath));
            }
        }
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::OperationFailed, std::string("Failed to index resource pack: ") + e.what());
    }

    // 记录索引规模，便于核查内存占用与扫描覆盖范围
    MC_TRACE_INSTANT_EVENT(TraceEvents.IO.Resource,
        "FolderResourcePack::initialize::IndexResources::Done",
        "pack",
        m_name,
        "entries",
        static_cast<i64>(m_entries.size()));

    return Result<void>::ok();
}

std::string FolderResourcePack::_makeTypedPath(PackType type, std::string_view path)
{
    std::string result(std::string(resource::packTypeDirectoryName(type)) + "/" + std::string(path));
    // 统一为正斜杠，与索引中的路径格式一致
    for (char& c : result) {
        if (c == '\\') {
            c = '/';
        }
    }
    return result;
}

bool FolderResourcePack::hasResource(PackType type, std::string_view resourcePath) const
{
    // 基于内存索引查询，避免每次 fs::exists 系统调用
    const std::string typedPath = _makeTypedPath(type, resourcePath);
    return m_entries.find(typedPath) != m_entries.end();
}

Result<std::vector<u8>> FolderResourcePack::readResource(PackType type, std::string_view resourcePath) const
{
    const std::string fullPath =
        _normalize_path(std::string(resource::packTypeDirectoryName(type)) + "/" + std::string(resourcePath));

    if (!fs::exists(fullPath)) {
        return Error(ErrorCode::ResourceNotFound,
            std::string("Resource not found in folder pack: ") + std::string(m_name) +
                ", name: " + std::string(resourcePath));
    }

    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, std::string("Cannot open resource: ") + fullPath);
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<u8> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    if (!file) {
        return Error(ErrorCode::FileReadFailed, std::string("Failed to read resource: ") + fullPath);
    }

    return data;
}

Result<std::vector<std::string>> FolderResourcePack::listResources(
    PackType type, std::string_view directory, std::string_view extension) const
{
    std::vector<std::string> resources;
    std::string normalizedDir = _makeTypedPath(type, directory);

    // 确保目录前缀以斜杠结尾，用于前缀匹配
    if (!normalizedDir.empty() && normalizedDir.back() != '/') {
        normalizedDir += '/';
    }

    // 类型目录前缀，如 "assets/" 或 "data/"
    // 返回的路径应相对于类型目录根（如 "minecraft/loot_tables/blocks/stone.json"），
    // 与 ZipResourcePack 保持一致
    std::string typePrefix = std::string(resource::packTypeDirectoryName(type)) + "/";

    // 扩展名归一化：剥离前导点号后比较，兼容 "json" 与 ".json" 两种调用约定
    // （部分调用方传 "json" 无点，部分传 ".json" 有点）。保留原磁盘遍历版本的语义。
    std::string checkExt(extension);
    if (!checkExt.empty() && checkExt[0] == '.') {
        checkExt = checkExt.substr(1);
    }

    // 基于内存索引做前缀匹配，避免重新递归遍历磁盘目录树
    for (const auto& path : m_entries) {
        // 检查是否在指定目录下
        if (path.size() <= normalizedDir.size() || path.substr(0, normalizedDir.size()) != normalizedDir) {
            continue;
        }

        // 相对于类型目录根的路径
        std::string relativePath = path.substr(typePrefix.size());

        if (checkExt.empty()) {
            resources.push_back(relativePath);
            continue;
        }

        // 取文件扩展名（不含点）比较
        size_t dotPos = relativePath.rfind('.');
        if (dotPos == std::string::npos) {
            continue;
        }
        std::string fileExt = relativePath.substr(dotPos + 1);
        if (fileExt == checkExt) {
            resources.push_back(relativePath);
        }
    }

    std::sort(resources.begin(), resources.end());
    return resources;
}

Result<std::vector<std::string>> FolderResourcePack::getResourceNamespaces(PackType type) const
{
    std::string typeDir(resource::packTypeDirectoryName(type));
    std::string prefix = typeDir + "/";

    // 基于内存索引提取类型目录下的第一级子目录作为命名空间
    std::unordered_set<std::string> namespaces;

    for (const auto& path : m_entries) {
        if (path.size() > prefix.size() && path.substr(0, prefix.size()) == prefix) {
            std::string rest = path.substr(prefix.size());
            size_t slashPos = rest.find('/');
            if (slashPos != std::string::npos) {
                namespaces.insert(rest.substr(0, slashPos));
            }
        }
    }

    std::vector<std::string> result(namespaces.begin(), namespaces.end());
    std::sort(result.begin(), result.end());
    return result;
}

std::string FolderResourcePack::_normalize_path(std::string_view resourcePath) const
{
    std::string path(resourcePath);

    // 确保使用正斜杠
    for (char& c : path) {
        if (c == '\\') c = '/';
    }

    // 移除前导斜杠
    while (!path.empty() && path[0] == '/') {
        path = path.substr(1);
    }

    return m_rootPath + "/" + path;
}

} // namespace mc::resource
