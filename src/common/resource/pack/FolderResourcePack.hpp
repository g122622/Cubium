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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/pack/PackMetadata.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace mc::resource {

/**
 * @brief 文件夹资源包实现
 *
 * 从文件系统目录读取资源
 * 目录结构应符合Minecraft资源包格式
 */
class FolderResourcePack : public IResourcePack {
public:
    explicit FolderResourcePack(std::string rootPath);
    ~FolderResourcePack() override = default;

    // 移动构造和赋值
    FolderResourcePack(FolderResourcePack&& other) noexcept = default;
    FolderResourcePack& operator=(FolderResourcePack&& other) noexcept = default;

    // 禁止拷贝（由于资源包持有文件系统资源）
    FolderResourcePack(const FolderResourcePack&) = delete;
    FolderResourcePack& operator=(const FolderResourcePack&) = delete;

    // IResourcePack接口实现
    [[nodiscard]] Result<void> initialize() override;
    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }
    [[nodiscard]] bool hasResource(PackType type, std::string_view resourcePath) const override;
    [[nodiscard]] Result<std::vector<u8>> readResource(PackType type, std::string_view resourcePath) const override;
    [[nodiscard]] Result<std::vector<std::string>> listResources(
        PackType type, std::string_view directory, std::string_view extension) const override;
    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces(PackType type) const override;
    [[nodiscard]] std::string name() const override { return m_name; }

    // 内部 API — 仅供诊断/管理代码使用
    [[nodiscard]] const std::string& rootPath() const { return m_rootPath; }

private:
    std::string m_rootPath;
    std::string m_name;
    PackMetadata m_metadata;

    // 资源路径索引：在 initialize() 时一次性扫描包根构建，存放所有常规文件
    // 相对于包根的规范化路径（正斜杠），如 "data/minecraft/loot_tables/blocks/stone.json"。
    // listResources/hasResource/getResourceNamespaces 全部基于此内存索引查询，
    // 避免每次调用都重新递归遍历磁盘目录树（与 ZipResourcePack::m_entries 对称）。
    std::unordered_set<std::string> m_entries;

    // 构造类型目录前缀的完整路径（typeDir + "/" + path）并词法归一化，用于索引前缀匹配
    [[nodiscard]] static std::string _makeTypedPath(PackType type, std::string_view path);
};

} // namespace mc::resource

namespace mc {
using FolderResourcePack = resource::FolderResourcePack;
} // namespace mc
