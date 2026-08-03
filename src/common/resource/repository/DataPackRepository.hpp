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
#include "common/resource/PackType.hpp"

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

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

    /**
     * @brief 当前列表中没有任何已初始化的数据包时，从默认游戏目录注入原版数据包。
     *
     * 世界生成已 100% 数据驱动（noise_settings / density_function / world_preset 等均从数据包
     * 加载，注册表无硬编码兜底）。若服务端扫描到的数据包目录为空（测试用临时游戏根目录、
     * 或真实游戏目录尚未放入任何数据包），各 worldgen loader 会无条件 clear() 注册表后加载
     * 0 条目，使 RandomState::create 因找不到 "minecraft:overworld" 等条目而断言失败。
     *
     * 原版 Minecraft 始终将 vanilla 数据包作为内置包加载，不依赖用户目录是否放置。本方法
     * 镜像该语义：列表为空时从 GameDirectory::defaultDirectory().dataPacksDir() 下的 "Vanilla"
     * 子目录注入原版数据包。列表非空（用户已放置自定义数据包）时不干预，保留用户覆盖权。
     *
     * @param vanillaDataPackDir 原版数据包目录（由调用方从默认游戏目录推导）
     * @return 注入的包数量（0 或 1）；目录缺失或列表已有包则返回 0
     */
    inline size_t ensureVanillaBuiltinPack(const std::filesystem::path& vanillaDataPackDir)
    {
        // 列表非空 = 用户已放置自定义数据包，保留用户覆盖权，不强制注入原版包。
        if (!getEnabledPackInfos().empty()) {
            return 0;
        }

        std::error_code ec;
        if (!std::filesystem::exists(vanillaDataPackDir, ec)) {
            return 0;
        }

        const i32 priority = static_cast<i32>(packCount());
        auto result = addPack(vanillaDataPackDir, /*enabled=*/true, priority);
        return result.success() && result.value().initialized ? 1u : 0u;
    }
};

} // namespace mc::resource

namespace mc {
using DataPackRepository = resource::DataPackRepository;
} // namespace mc
