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

#include "PackListBase.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/core/settings/ResourcePackListOption.hpp"
#include "common/resource/PackType.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace mc::resource {

/**
 * @brief 客户端资源包列表管理器
 *
 * 管理多个客户端资源包，默认使用 PackType::ClientResources。
 * 支持文件夹和 ZIP 格式的资源包。
 * 实现类似 Minecraft 1.16.5 的资源加载优先级系统：
 * 高优先级的资源包后加载，其资源会覆盖低优先级的同名资源。
 * 查找资源时，先从高优先级资源包查找。
 */
class PackRepository : public PackListBase {
public:
    PackRepository()
        : PackListBase(PackType::ClientResources)
    {}

    ~PackRepository() override = default;

    PackRepository(const PackRepository&) = delete;
    PackRepository& operator=(const PackRepository&) = delete;
    PackRepository(PackRepository&&) = delete;
    PackRepository& operator=(PackRepository&&) = delete;

    using PackListBase::getResourceNamespaces;
    using PackListBase::hasResource;
    using PackListBase::listResources;
    using PackListBase::readResource;
    using PackListBase::readTextResource;

    [[nodiscard]] bool hasResource(std::string_view resourcePath) const
    {
        return PackListBase::hasResource(PackType::ClientResources, resourcePath);
    }

    [[nodiscard]] Result<std::vector<u8>> readResource(std::string_view resourcePath) const
    {
        return PackListBase::readResource(PackType::ClientResources, resourcePath);
    }

    [[nodiscard]] Result<std::string> readTextResource(std::string_view resourcePath) const
    {
        return PackListBase::readTextResource(PackType::ClientResources, resourcePath);
    }

    [[nodiscard]] Result<std::vector<std::string>> listResources(
        std::string_view directory, std::string_view extension) const
    {
        return PackListBase::listResources(PackType::ClientResources, directory, extension);
    }

    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces() const
    {
        return PackListBase::getResourceNamespaces(PackType::ClientResources);
    }

    void loadFromSettings(const ResourcePackListOption& settings);
    void saveToSettings(ResourcePackListOption& settings) const;

protected:
    void onPackListChanged() override;
};

} // namespace mc::resource

namespace mc {
using PackRepository = resource::PackRepository;
} // namespace mc
