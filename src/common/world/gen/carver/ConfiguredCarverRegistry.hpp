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

#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

// 前向声明：ConfiguredCarverBase 定义于 world/carver/WorldCarver.hpp，
// 无 name/id 字段，故由本注册表外部用 ResourceLocation 索引。
class ConfiguredCarverBase;

/**
 * @brief 配置化雕刻器注册表
 *
 * 运行时 worldgen 注册表中的 configured_carver 注册表。
 * 从数据包 JSON 加载所有 ConfiguredCarverBase，按 ResourceLocation 索引。
 *
 * ConfiguredCarverBase 本身无 name/id（仅 carve/shouldCarve 接口），
 * 故标识由本注册表外部维护（id 取自 configured_carver JSON 文件名）。
 */
class ConfiguredCarverRegistry {
public:
    static ConfiguredCarverRegistry& instance();

    /**
     * @brief 注册配置化雕刻器
     * @param carver 雕刻器（转移所有权）
     * @param id 雕刻器的 ResourceLocation（对应 configured_carver JSON 文件名）
     */
    void registerCarver(std::unique_ptr<ConfiguredCarverBase> carver, ResourceLocation id);

    /**
     * @brief 按 ResourceLocation 查找配置化雕刻器
     * @return 雕刻器指针，未注册返回 nullptr
     */
    [[nodiscard]] const ConfiguredCarverBase* get(const ResourceLocation& id) const noexcept;

    /**
     * @brief 是否包含指定 id
     */
    [[nodiscard]] bool has(const ResourceLocation& id) const noexcept;

    /**
     * @brief 获取所有已注册雕刻器的数量
     */
    [[nodiscard]] size_t size() const noexcept { return m_ownedCarvers.size(); }

    /**
     * @brief 清除所有雕刻器
     */
    void clear();

private:
    ConfiguredCarverRegistry() = default;
    ~ConfiguredCarverRegistry() = default;
    ConfiguredCarverRegistry(const ConfiguredCarverRegistry&) = delete;
    ConfiguredCarverRegistry& operator=(const ConfiguredCarverRegistry&) = delete;

    std::vector<std::unique_ptr<ConfiguredCarverBase>> m_ownedCarvers;
    std::unordered_map<ResourceLocation, const ConfiguredCarverBase*> m_carversById;
};

} // namespace mc
