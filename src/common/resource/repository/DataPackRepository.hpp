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
     * @brief 把原版数据包作为最低优先级的底层包注入列表。
     *
     * 世界生成已 100% 数据驱动（noise_settings / density_function / world_preset 等均从数据包
     * 加载，注册表无硬编码兜底），而原版 Minecraft 始终把 vanilla 数据包当作内置底层包，
     * 用户数据包叠加在它之上。本方法镜像该语义：
     *
     *   - 无论用户目录下有没有数据包，都注入原版包；
     *   - 原版包的优先级固定低于所有扫描到的用户包（扫描从 0 开始编号），
     *     保证用户包能覆盖同名资源，而未被覆盖的资源仍由原版提供。
     *
     * 只注入不叠加会导致用户一放自定义数据包，worldgen 注册表就整体为空
     * （`RandomState::create` 找不到 "minecraft:overworld" 而断言失败）。
     *
     * @param vanillaDataPackDir 原版数据包目录（由调用方从默认游戏目录推导）
     * @return 注入成功的包数量（0 或 1）；目录缺失或初始化失败返回 0
     */
    inline size_t ensureVanillaBuiltinPack(const std::filesystem::path& vanillaDataPackDir)
    {
        std::error_code ec;
        if (!std::filesystem::exists(vanillaDataPackDir, ec)) {
            return 0;
        }

        // 内置包在所有用户包之下：scanDirectory 从 0 开始给用户包编号，故取 -1。
        constexpr i32 kBuiltinVanillaPriority = -1;
        auto result = addPack(vanillaDataPackDir, /*enabled=*/true, kBuiltinVanillaPriority);
        return result.success() && result.value().initialized ? 1u : 0u;
    }
};

} // namespace mc::resource

namespace mc {
using DataPackRepository = resource::DataPackRepository;
} // namespace mc
