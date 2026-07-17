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
#include "common/world/gen/settings/FlatLevelGeneratorSettings.hpp"

#include <unordered_map>

namespace mc::world::gen::settings {

/**
 * @brief flat_level_generator_preset 注册表（MC 1.21.11 worldgen/flat_level_generator_preset）
 *
 * name → FlatLevelGeneratorSettings。FlatLevelGeneratorPresetLoader 从数据包加载 9 个
 * flat_level_generator_preset JSON（classic_flat/the_void/tunnelers_dream/water_world/
 * snowy_kingdom/desert/overworld/bottomless_pit/redstone_ready），经
 * FlatLevelGeneratorSettings::fromJson 解析（biome RL→BiomeId + layers + features/lakes +
 * structure_overrides）后注册到本表。ServerDimensionManager flat 分支查本表走数据驱动路径。
 *
 * 进程级单例。FlatLevelGeneratorSettings 可拷贝（持 vector + 裸 BlockState* 指针，方块注册表
 * 进程期存活）。
 */
class FlatLevelGeneratorPresetRegistry {
public:
    static FlatLevelGeneratorPresetRegistry& instance();

    /**
     * @brief 注册 flat 预设
     *
     * FlatLevelGeneratorPresetLoader 解析完一个 JSON 后调用。
     *
     * @param name 资源位置（如 "minecraft:classic_flat"）
     * @param settings 平坦世界生成设置
     */
    void registerPreset(const resource::ResourceLocation& name, FlatLevelGeneratorSettings settings);

    /**
     * @brief 按名取平坦世界生成设置
     * @return 设置指针，或 nullptr（未注册）
     */
    [[nodiscard]] const FlatLevelGeneratorSettings* get(const resource::ResourceLocation& name) const;

    /** 是否已注册 */
    [[nodiscard]] bool has(const resource::ResourceLocation& name) const;

    /** 清空注册表（数据驱动加载前调用） */
    void clear() noexcept;

    /**
     * @brief 标记数据驱动加载状态
     *
     * true=已由 FlatLevelGeneratorPresetLoader 注入；false=重置。
     */
    void markLoadedFromDatapack(bool loaded) noexcept;

    /** 是否已由数据驱动加载 */
    [[nodiscard]] bool isLoadedFromDatapack() const noexcept;

private:
    FlatLevelGeneratorPresetRegistry() = default;
    ~FlatLevelGeneratorPresetRegistry() = default;
    FlatLevelGeneratorPresetRegistry(const FlatLevelGeneratorPresetRegistry&) = delete;
    FlatLevelGeneratorPresetRegistry& operator=(const FlatLevelGeneratorPresetRegistry&) = delete;

    std::unordered_map<resource::ResourceLocation, FlatLevelGeneratorSettings> m_presets;
    bool m_loadedFromDatapack = false;
};

} // namespace mc::world::gen::settings
