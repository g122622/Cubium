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

#include "NoiseSettings.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include "common/world/block/Block.hpp"
#include <array>
#include <memory>
#include <vector>

#include <cstddef>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class BlockState;
namespace world::gen::density {
class DensityFunction;
}
namespace world::gen::surface {
class SurfaceRule;
}
} // namespace mc

namespace mc {

/// noise_router 15 字段在 NoiseRouter 构造函数中的固定顺序（与 JSON 键名一致，下划线转 camel）。
/// DimensionSettings::m_routerDfs 与 NoiseSettingsLoader 按此顺序填充。
enum class RouterSlot : u8 {
    Barrier,
    FluidLevelFloodedness,
    FluidLevelSpread,
    Lava,
    Temperature,
    Vegetation,
    Continents,
    Erosion,
    Depth,
    Ridges,
    PreliminarySurfaceLevel,
    FinalDensity,
    VeinToggle,
    VeinRidged,
    VeinGap,
    Count ///< = 15
};

inline constexpr size_t ROUTER_SLOT_COUNT = static_cast<size_t>(RouterSlot::Count);

/**
 * @brief 维度类型标识
 *
 * 明确标识维度类型，用于选择正确的 NoiseRouter 和 SurfaceRules。
 * 用于选择正确的 NoiseRouter 和 SurfaceRules。
 */
enum class DimensionKind : u8 {
    Overworld,       ///< 主世界
    LargeBiomes,     ///< 主世界（大型生物群系）
    Amplified,       ///< 主世界（放大化）
    Nether,          ///< 下界
    End,             ///< 末地
    Caves,           ///< 洞穴预设
    FloatingIslands, ///< 浮岛预设
    Flat             ///< 超平坦
};

/**
 * @brief 维度生成设置
 *
 * 包含维度级别的生成配置（对齐 MC 1.21.11 NoiseGeneratorSettings）。
 * 使用 BlockState* 替代固定 BlockId，支持动态方块注册。
 */
struct DimensionSettings {
    NoiseSettings noise;
    const BlockState* defaultBlock = nullptr; ///< 默认方块（石头等）
    const BlockState* defaultFluid = nullptr; ///< 默认流体（水/熔岩）
    i32 seaLevel = world::SEA_LEVEL;
    DimensionKind dimensionKind = DimensionKind::Overworld; ///< 维度类型标识
    bool largeBiomes = false;                               ///< 是否使用大型生物群系预设
    bool oreVeinsEnabled = true;                            ///< 是否启用矿脉生成（主世界=true，下界/末地=false）
    bool disableMobGeneration = false;                      ///< 是否禁用生物生成（末地=true）

    /**
     * @brief 出生点气候目标参数列表
     *
     * MC 1.21.11: NoiseGeneratorSettings.spawnTarget
     * 用于 Climate.Sampler.findSpawnPosition() 在气候空间中径向搜索最佳出生点。
     *
     * - 主世界 / 大型生物群系 / 放大化：由 OverworldBiomeBuilder.spawnTarget() 提供
     *   （2 个 ParameterPoint，depth=0，weirdness 以 ±0.16 分割）
     * - 下界 / 末地 / 洞穴 / 浮岛 / 超平坦：空列表（沿用 (0,0) 区块作为出生点）
     *
     * 在 NoiseChunk.cachedClimateSampler 中传给 Climate::Sampler，
     * 同时由 RandomState::create 时设置到 m_sampler 上以供出生点查找使用。
     */
    std::vector<world::biome::climate::ParameterPoint> spawnTarget;

    // === 数据驱动扩展字段（noise_settings JSON 加载后填充）===

    /**
     * @brief 来源 noise_settings 资源位置
     *
     * 数据驱动唯一路径标识：RandomState::create 据此查 NoiseSettingsRegistry 取完整
     * DimensionSettings（含 m_routerDfs 模板 + m_surfaceRule），经 NoiseBindingVisitor 绑定。
     * 7 个 C++ 静态预设（overworld/large_biomes/amplified/nether/end/caves/floating_islands）
     * 均挂规范 RL；flat() 不挂（超平坦走 FlatChunkGenerator，不经 create）。
     * 空（默认）表示非数据驱动预设（仅 flat()），若误传入 create() 将断言失败。
     */
    resource::ResourceLocation m_noiseSettingsId;

    /**
     * @brief 数据驱动 noise_router 15 字段密度函数模板（未绑定噪声叶子）
     *
     * 由 NoiseSettingsLoader 从 noise_settings JSON 的 noise_router 对象解析，
     * 每字段是 DF Holder（字符串 RL → 查 DensityFunctionRegistry；内联对象 → TypeRegistry；
     * 裸数字 → Constant）。噪声叶子（noise/shifted_noise/.../old_blended_noise）解析期存
     * UnboundNoiseLeaf 占位，由 RandomState::create/createRouterCopy 经 NoiseBindingVisitor
     * 替换为真实叶子（getOrCreateNoise(name) name-hash）。
     *
     * shared_ptr 使得 DimensionSettings 可拷贝且 15 槽位可被多个 RandomState 共享模板。
     * 数组索引对应 RouterSlot 枚举顺序。原版 7 个 noise_settings JSON 均提供全部 15 字段，
     * 故加载后数组必填；RandomState::create 无 dimensionKind 兜底（数据驱动为唯一路径）。
     */
    std::array<std::shared_ptr<world::gen::density::DensityFunction>, ROUTER_SLOT_COUNT> m_routerDfs{};

    /**
     * @brief 数据驱动 surface_rule 根节点
     *
     * 由 SurfaceRuleDeserializer 从 noise_settings JSON 的 surface_rule 字段解析。
     * 数据驱动为唯一路径：RandomState::create 断言此字段非空（所有 noise_settings 必须提供 surface_rule）。
     */
    std::shared_ptr<world::gen::surface::SurfaceRule> m_surfaceRule;

    // === 预设 ===

    /**
     * @brief 主世界设置
     */
    static DimensionSettings overworld() noexcept;

    /**
     * @brief 大型生物群系设置
     */
    static DimensionSettings largeBiomesPreset() noexcept;

    /**
     * @brief 放大化设置
     */
    static DimensionSettings amplified() noexcept;

    /**
     * @brief 下界设置
     */
    static DimensionSettings nether() noexcept;

    /**
     * @brief 末地设置
     */
    static DimensionSettings end() noexcept;

    /**
     * @brief 洞穴预设设置
     */
    static DimensionSettings caves() noexcept;

    /**
     * @brief 浮岛预设设置
     */
    static DimensionSettings floatingIslands() noexcept;

    /**
     * @brief 平坦世界设置（占位用，FlatChunkGenerator 不使用噪声生成）
     */
    static DimensionSettings flat() noexcept;

    // === 数据驱动加载 ===

    /**
     * @brief 从 noise_settings JSON 解析 DimensionSettings（MC 1.21.11）
     *
     * 顶层 11 字段：
     * - noise → NoiseSettings 4 尺寸字段（min_y/height/size_horizontal/size_vertical），
     *   其余（scaling/slides/densityFactor/densityOffset 等）由 RL 推导的 dimensionKind 从
     *   C++ 静态预设补全（用户决策：JSON 只提供 4 尺寸字段）
     * - default_block/default_fluid → BlockStateParser
     * - noise_router → 15 DF Holder（字符串 RL / 内联对象 / 裸数字），调 DensityFunctionLoader
     * - surface_rule → SurfaceRuleDeserializer
     * - spawn_target → ParameterPointCodec
     * - sea_level/disable_mob_generation/aquifers_enabled/ore_veins_enabled/legacy_random_source → 直接字段
     *
     * @param root noise_settings JSON 根对象
     * @param id 本 noise_settings 的资源位置（写入 m_noiseSettingsId）
     * @return DimensionSettings，或错误
     */
    [[nodiscard]] static Result<DimensionSettings> fromJson(
        const nlohmann::json& root, const resource::ResourceLocation& id);
};

} // namespace mc
