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
 */

#pragma once

#include "WorldPreset.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <unordered_map>

namespace mc::world::gen::settings {

/**
 * @brief world_preset 注册表（MC 1.21.11 worldgen/world_preset）
 *
 * name → WorldPreset。WorldPresetLoader 从数据包加载 6 个 world_preset JSON
 * （normal/flat/large_biomes/amplified/debug_all_block_states/single_biome_surface），
 * 经 WorldPreset::fromJson 解析（dimensions map + generator.type + biome_source + settings）
 * 后注册到本表。ServerDimensionManager 按 DimensionId 映射维度键查表装配三维度。
 *
 * 进程级单例。WorldPreset 可拷贝（持 unordered_map + 可拷贝的 WorldPresetDimension）。
 */
class WorldPresetRegistry {
public:
    static WorldPresetRegistry& instance();

    /**
     * @brief 注册世界预设
     *
     * WorldPresetLoader 解析完一个 JSON 后调用。
     *
     * @param name 资源位置（如 "minecraft:default"，对应 normal.json）
     * @param preset 世界预设
     */
    void registerPreset(const resource::ResourceLocation& name, WorldPreset preset);

    /**
     * @brief 按名取世界预设
     * @return 预设指针，或 nullptr（未注册）
     */
    [[nodiscard]] const WorldPreset* get(const resource::ResourceLocation& name) const;

    /** 是否已注册 */
    [[nodiscard]] bool has(const resource::ResourceLocation& name) const;

    /** 清空注册表（数据驱动加载前调用） */
    void clear() noexcept;

    /**
     * @brief 标记数据驱动加载状态
     *
     * true=已由 WorldPresetLoader 注入；false=重置。
     */
    void markLoadedFromDatapack(bool loaded) noexcept;

    /** 是否已由数据驱动加载 */
    [[nodiscard]] bool isLoadedFromDatapack() const noexcept;

private:
    WorldPresetRegistry() = default;
    ~WorldPresetRegistry() = default;
    WorldPresetRegistry(const WorldPresetRegistry&) = delete;
    WorldPresetRegistry& operator=(const WorldPresetRegistry&) = delete;

    std::unordered_map<resource::ResourceLocation, WorldPreset> m_presets;
    bool m_loadedFromDatapack = false;
};

} // namespace mc::world::gen::settings
