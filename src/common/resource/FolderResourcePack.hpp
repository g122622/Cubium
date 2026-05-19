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

#include "IResourcePack.hpp"
#include <filesystem>

namespace mc {

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

    // IResourcePack接口实现
    [[nodiscard]] Result<void> initialize() override;
    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }
    [[nodiscard]] bool hasResource(std::string_view resourcePath) const override;
    [[nodiscard]] Result<std::vector<u8>> readResource(std::string_view resourcePath) const override;
    [[nodiscard]] Result<std::vector<std::string>> listResources(
        std::string_view directory, std::string_view extension = "") const override;
    [[nodiscard]] std::string name() const override { return m_name; }

    // 获取根路径
    [[nodiscard]] const std::string& rootPath() const { return m_rootPath; }

private:
    std::string m_rootPath;
    std::string m_name;
    PackMetadata m_metadata;

    // 规范化路径
    [[nodiscard]] std::string normalizePath(std::string_view resourcePath) const;
};

} // namespace mc
