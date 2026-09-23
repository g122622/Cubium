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

#include "common/core/Types.hpp"
#include <string>

namespace mc::world::storage::reader::java {

/// 默认生物群系 ID（未知时回退）
constexpr BiomeId UnknownBiome = 0;

/**
 * @brief Java 版生物群系名称→内部 BiomeId 映射器
 *
 * Java 1.18+ 存档的群系 palette 存的是**名称字符串**（如
 * "minecraft:old_growth_birch_forest"），须转译成项目内部的 BiomeId。
 *
 * 本类不自建名称对照表，而是委托 biome::JavaBiomeRegistryIdMap——那张表由
 * BiomeRegistry 的真实内容 + 1.18 旧名别名表推导，与网络层/存档层用的是同一份权威
 * 数据。本类曾经手写维护一张名称表，因两张表互不同步而产生系统性缺陷：65 个群系里
 * 15 个错漏（7 个完全缺失、8 个被映射到语义相近但错误的群系，例如
 * meadow→Plains、dripstone_caves→TheEnd），且未命中时静默回退，外部存档读进来后
 * 群系整片错位却没有任何报错。委托权威表后此类漂移不再可能发生。
 *
 * 使用前提：biome::JavaBiomeRegistryIdMap::instance().initialize() 已执行
 * （其在 BiomeRegistry::initialize() 之后即可调用，生产路径由 RegistryBootstrap /
 * ClientApplicationBootstrap 完成）。未初始化时本类返回 UnknownBiome 并记 warn，
 * 以便把"初始化缺失"与"名称确有对应"两种情况区分开。
 */
class JavaBiomeMapper {
public:
    JavaBiomeMapper() = default;

    /**
     * @brief 从 Java 版生物群系名称映射到内部 BiomeId
     *
     * 接受带或不带 "minecraft:" 前缀的写法，也接受 1.18 旧名（内部经别名表归一化）。
     *
     * @param biomeName Java 版生物群系名称（如 "minecraft:old_growth_birch_forest"）
     * @return 内部 BiomeId；无对应群系或映射表未建立时返回 UnknownBiome 并记 warn
     */
    BiomeId mapBiome(const std::string& biomeName);

    /**
     * @brief 从 Java 版数值 ID 映射到内部 BiomeId
     *
     * 仅适用于 Java 1.16.5 及更早的存档：那时的数值 id 与项目内部 id 一致。
     * 1.18+ 的 palette 存名称不存数值，故不经过本入口。
     *
     * @param numericId Java 版生物群系数值 ID
     * @return 内部 BiomeId
     */
    BiomeId mapBiome(i32 numericId);
};

} // namespace mc::world::storage::reader::java
