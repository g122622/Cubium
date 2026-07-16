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
#include "common/world/gen/settings/DimensionSettings.hpp"

#include <unordered_map>

namespace mc::world::gen::settings {

/**
 * @brief noise_settings 注册表（MC 1.21.11 worldgen/noise_settings）
 *
 * name → DimensionSettings。NoiseSettingsLoader 从数据包加载 7 个 noise_settings JSON，
 * 经 DimensionSettings::fromJson 解析（noise 4 尺寸字段 + 15 DF 路由模板 + surface_rule +
 * spawn_target + 标量字段）后注册到本表。RandomState::create 查本表走数据驱动唯一路径。
 *
 * 进程级单例。DimensionSettings 含 shared_ptr 字段（routerDfs/surfaceRule），可拷贝共享模板。
 */
class NoiseSettingsRegistry {
public:
    static NoiseSettingsRegistry& instance();

    /**
     * @brief 注册 noise_settings
     *
     * NoiseSettingsLoader 解析完一个 noise_settings JSON 后调用。
     *
     * @param name 资源位置（如 "minecraft:overworld"）
     * @param settings 维度设置（m_noiseSettingsId 应已设为 name）
     */
    void registerSettings(const resource::ResourceLocation& name, DimensionSettings settings);

    /**
     * @brief 按名取维度设置
     * @return 维度设置指针，或 nullptr（未注册）
     */
    [[nodiscard]] const DimensionSettings* get(const resource::ResourceLocation& name) const;

    /** 是否已注册 */
    [[nodiscard]] bool has(const resource::ResourceLocation& name) const;

    /** 清空注册表（数据驱动加载前调用） */
    void clear() noexcept;

    /**
     * @brief 标记数据驱动加载状态
     *
     * true=已由 NoiseSettingsLoader 注入；false=重置。
     */
    void markLoadedFromDatapack(bool loaded) noexcept;

    /** 是否已由数据驱动加载 */
    [[nodiscard]] bool isLoadedFromDatapack() const noexcept;

private:
    NoiseSettingsRegistry() = default;
    ~NoiseSettingsRegistry() = default;
    NoiseSettingsRegistry(const NoiseSettingsRegistry&) = delete;
    NoiseSettingsRegistry& operator=(const NoiseSettingsRegistry&) = delete;

    std::unordered_map<resource::ResourceLocation, DimensionSettings> m_settings;
    bool m_loadedFromDatapack = false;
};

} // namespace mc::world::gen::settings
