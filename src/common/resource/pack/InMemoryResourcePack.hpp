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
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::resource {

/**
 * @brief 内存资源包
 *
 * 从内存中提供资源，用于内置资源（如原版基础模型）。
 * 优先级最高，始终加载。
 */
class InMemoryResourcePack : public IResourcePack {
public:
    /**
     * @brief 构造函数
     * @param name 资源包名称
     */
    explicit InMemoryResourcePack(std::string name);

    /**
     * @brief 析构函数
     */
    ~InMemoryResourcePack() override = default;

    /**
     * @brief 添加资源（统一入口）
     * @param type 资源包类型
     * @param path 资源路径（如 "minecraft/models/block/cube_all.json"）
     * @param content 资源内容
     */
    void addResource(PackType type, std::string_view path, std::vector<u8> content);

    // 便捷方法 — 委托给 addResource
    void addClientResource(std::string path, std::string content);
    void addServerDataResource(std::string path, std::string content);

    /**
     * @brief 添加二进制资源
     * @param path 资源路径
     * @param data 资源数据
     */
    void addClientResource(std::string path, std::vector<u8> data);
    void addServerDataResource(std::string path, std::vector<u8> data);

    /**
     * @brief 添加目录条目（用于 listResources）
     * @param type 资源类型
     * @param directory 目录路径（相对于类型根目录）
     */
    void addDirectory(PackType type, std::string directory);

    // IResourcePack 接口实现

    [[nodiscard]] Result<void> initialize() override;
    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }
    [[nodiscard]] bool hasResource(PackType type, std::string_view resourcePath) const override;
    [[nodiscard]] Result<std::vector<u8>> readResource(PackType type, std::string_view resourcePath) const override;
    [[nodiscard]] Result<std::vector<std::string>> listResources(
        PackType type, std::string_view directory, std::string_view extension) const override;
    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces(PackType type) const override;
    [[nodiscard]] std::string name() const override { return m_name; }

private:
    std::string m_name;
    PackMetadata m_metadata;
    std::unordered_map<std::string, std::vector<u8>> m_resources;
    std::unordered_set<std::string> m_directories; // 用于 listResources

    /**
     * @brief 规范化路径
     *
     * 将路径中的反斜杠转换为正斜杠，移除前导斜杠
     *
     * @param path 原始路径
     * @return 规范化后的路径
     */
    [[nodiscard]] static std::string _normalizePath(std::string_view path);

    /**
     * @brief 构造带类型前缀的路径
     *
     * 将资源类型目录前缀添加到路径前，并规范化路径
     *
     * @param type 资源类型
     * @param path 资源路径
     * @return 带类型前缀的规范化路径
     */
    [[nodiscard]] static std::string _makeTypedPath(PackType type, std::string_view path);

    /**
     * @brief 为资源路径自动添加所有父目录条目
     *
     * @param normalizedPath 已规范化的资源路径
     */
    void _addDirectoryEntries(const std::string& normalizedPath);
};

} // namespace mc::resource

namespace mc {
using InMemoryResourcePack = resource::InMemoryResourcePack;
} // namespace mc
