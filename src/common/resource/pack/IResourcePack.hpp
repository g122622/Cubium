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
#include "common/resource/pack/PackMetadata.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mc::resource {

/**
 * @brief 资源包抽象接口
 *
 * 提供统一的资源读取接口，支持文件夹和 ZIP 资源包。
 * 所有资源访问都必须显式指定 PackType，禁止再使用默认客户端资源语义。
 */
class IResourcePack {
public:
    virtual ~IResourcePack() = default;

    // 初始化资源包
    [[nodiscard]] virtual Result<void> initialize() = 0;

    // 获取元数据
    [[nodiscard]] virtual const PackMetadata& metadata() const = 0;

    // 检查指定类型的资源是否存在
    [[nodiscard]] virtual bool hasResource(PackType type, std::string_view resourcePath) const = 0;

    // 读取指定类型的资源内容
    [[nodiscard]] virtual Result<std::vector<u8>> readResource(PackType type, std::string_view resourcePath) const = 0;

    // 读取指定类型的文本资源
    [[nodiscard]] virtual Result<std::string> readTextResource(PackType type, std::string_view resourcePath) const;

    // 列出指定类型目录下的所有资源
    [[nodiscard]] virtual Result<std::vector<std::string>> listResources(
        PackType type, std::string_view directory, std::string_view extension) const = 0;

    // 获取指定类型的所有命名空间
    [[nodiscard]] virtual Result<std::vector<std::string>> getResourceNamespaces(PackType type) const = 0;

    // 获取资源包路径/名称
    [[nodiscard]] virtual std::string name() const = 0;
};

using ResourcePackPtr = std::shared_ptr<IResourcePack>;

} // namespace mc::resource

namespace mc {
using IResourcePack = resource::IResourcePack;
using ResourcePackPtr = resource::ResourcePackPtr;
} // namespace mc
