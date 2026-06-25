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

namespace mc::resource {

/**
 * @brief 数据包列表管理器
 *
 * 专门管理服务端数据包，固定使用 PackType::ServerData。
 * 数据包资源路径统一相对于 data/ 根目录。
 */
class DataPackRepository : public PackListBase {
public:
    DataPackRepository()
        : PackListBase(PackType::ServerData)
    {}

    ~DataPackRepository() override = default;

    DataPackRepository(const DataPackRepository&) = delete;
    DataPackRepository& operator=(const DataPackRepository&) = delete;
    DataPackRepository(DataPackRepository&&) = delete;
    DataPackRepository& operator=(DataPackRepository&&) = delete;

    using PackListBase::getResourceNamespaces;
    using PackListBase::hasResource;
    using PackListBase::listResources;
    using PackListBase::listResourceStacks;
    using PackListBase::readAllResourceVersions;
    using PackListBase::readResource;
    using PackListBase::readTextResource;

    [[nodiscard]] bool hasResource(std::string_view resourcePath) const
    {
        return PackListBase::hasResource(PackType::ServerData, resourcePath);
    }

    [[nodiscard]] Result<std::vector<u8>> readResource(std::string_view resourcePath) const
    {
        return PackListBase::readResource(PackType::ServerData, resourcePath);
    }

    [[nodiscard]] Result<std::string> readTextResource(std::string_view resourcePath) const
    {
        return PackListBase::readTextResource(PackType::ServerData, resourcePath);
    }

    [[nodiscard]] Result<std::vector<std::string>> listResources(
        std::string_view directory, std::string_view extension) const
    {
        return PackListBase::listResources(PackType::ServerData, directory, extension);
    }

    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces() const
    {
        return PackListBase::getResourceNamespaces(PackType::ServerData);
    }

    [[nodiscard]] Result<std::vector<ResourceVersion>> readAllResourceVersions(std::string_view resourcePath) const
    {
        return PackListBase::readAllResourceVersions(PackType::ServerData, resourcePath);
    }

    [[nodiscard]] Result<std::map<std::string, std::vector<ResourceVersion>>> listResourceStacks(
        std::string_view directory, std::string_view extension) const
    {
        return PackListBase::listResourceStacks(PackType::ServerData, directory, extension);
    }
};

} // namespace mc::resource

namespace mc {
using DataPackRepository = resource::DataPackRepository;
} // namespace mc
