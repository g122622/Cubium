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

#include "ConfiguredFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {

// 前向声明
class WorldGenRegion;
class Block;
class BlockTag;

namespace world::gen::feature {

/**
 * @brief spring_feature 配置
 *
 * 对应 MC 1.21.11 SpringConfiguration{state, requiresBlockBelow, rockCount, holeCount, validBlocks}。
 * state 是 FluidState（数据包 JSON 形如 {"Name":"minecraft:water","Properties":{"falling":"true"}}），
 * place 时调 FluidState::getBlockState()（= MC createLegacyBlock）转回方块状态放置。
 * validBlocks 为 HolderSet<Block>：方块列表或 #tag，运行时 is 语义判断。
 */
struct SpringConfig {
    /// 流体状态（如 water/lava，可带 falling）。
    const fluid::FluidState* state = nullptr;
    bool requiresBlockBelow = true;
    i32 rockCount = 4;
    i32 holeCount = 1;
    /// valid_blocks 显式方块列表（与 validTag 至少其一非空）。
    std::vector<const Block*> validBlocks;
    /// valid_blocks 为 #tag 时的标签（nullptr 表示用列表）。
    const BlockTag* validTag = nullptr;

    SpringConfig() = default;
};

/**
 * @brief 流体泉特征（spring_feature）
 *
 * 忠实复刻 MC 1.21.11 SpringFeature：
 * - origin 上方须为 validBlocks，否则 return false；
 * - requiresBlockBelow 时，origin 下方也须为 validBlocks，否则 return false；
 * - origin 自身须为空气或 validBlocks，否则 return false；
 * - 统计水平四邻(W/E/N/S)+下方(D)中 validBlocks 数 j 与空气数 k，
 *   j==rockCount && k==holeCount 时在 origin 放置流体状态并返回 true。
 *
 * scheduleTick 调用省略（项目无 scheduleTick API；流体放置后流动由后续 tick 自然触发）。
 * 装饰阶段 FluidSprings。
 */
class ConfiguredSpringFeature : public ConfiguredFeatureBase {
public:
    explicit ConfiguredSpringFeature(std::unique_ptr<SpringConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::FluidSprings; }

private:
    std::unique_ptr<SpringConfig> m_config;
    std::string m_name;
};

} // namespace world::gen::feature
} // namespace mc
