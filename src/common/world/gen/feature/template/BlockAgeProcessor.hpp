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

// ============================================================================
// 注意：本文件不在 CMakeLists.txt 编译列表中，仅供参考。
// BlockAgeProcessor 的实际实现在 Template.hpp/cpp 中。
// 修改处理器逻辑时，务必修改 Template.hpp/cpp，而非仅修改本文件。
// ============================================================================

#pragma once

#include "StructureProcessor.hpp"
#include "common/core/Types.hpp"

namespace mc {

class BlockState;
class Block;

namespace math {
class Random;
}

namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 方块老化结构处理器（苔藓化处理器）
 *
 * 根据苔藓概率随机将石砖相关方块替换为苔藓化或裂变版本。
 * 用于村庄等结构的老化效果。
 *
 * 处理规则（对齐 MC 1.21.11 BlockAgeProcessor）：
 * - 石砖/石头/錾刻石砖 -> 50%不替换，否则随机替换为裂纹石砖/石砖楼梯/苔藓石砖/苔藓石砖楼梯
 * - 任意楼梯 -> 50%不替换，否则随机替换为苔藓石砖楼梯（保留属性）/苔藓石砖台阶/石台阶/石砖台阶
 * - 任意台阶 -> mossiness概率替换为苔藓石砖台阶（保留属性）
 * - 任意墙壁 -> mossiness概率替换为苔藓石砖墙（保留属性）
 * - 黑曜石 -> 15%替换为哭泣黑曜石
 */
class BlockAgeProcessor : public StructureProcessor {
public:
    /**
     * @brief 构造方块老化处理器
     * @param mossiness 苔藓化概率 (0.0 - 1.0)
     */
    explicit BlockAgeProcessor(f32 mossiness);

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<BlockAgeProcessor>(m_mossiness);
    }

    [[nodiscard]] f32 mossiness() const { return m_mossiness; }

private:
    f32 m_mossiness;

    /// 尝试替换完整石砖方块（石砖、石头、錾刻石砖）
    [[nodiscard]] const BlockState* _maybeReplaceFullStoneBlock(math::Random& rng);

    /// 尝试替换楼梯方块，使用 withPropertiesOf 保留原朝向/半部分属性
    [[nodiscard]] const BlockState* _maybeReplaceStairs(const BlockState& state, math::Random& rng);

    /// 尝试替换台阶方块，使用 withPropertiesOf 保留原类型属性
    [[nodiscard]] const BlockState* _maybeReplaceSlab(const BlockState& state, math::Random& rng);

    /// 尝试替换墙壁方块，使用 withPropertiesOf 保留原连接属性
    [[nodiscard]] const BlockState* _maybeReplaceWall(const BlockState& state, math::Random& rng);

    /// 尝试替换黑曜石为哭泣黑曜石
    [[nodiscard]] static const BlockState* _maybeReplaceObsidian(math::Random& rng);

    /// 生成随机朝向的楼梯状态（随机水平朝向 + 随机上半/下半）
    [[nodiscard]] static const BlockState& _getRandomFacingStairs(math::Random& rng, const Block& stairsBlock);

    /// 根据 mossiness 概率从两组候选中随机选择
    [[nodiscard]] const BlockState* _getRandomBlock(
        math::Random& rng, const BlockState* const nonMossy[], const BlockState* const mossy[]);

    /// 从选项数组中随机选取一个非空元素
    [[nodiscard]] static const BlockState* _pickRandomNonNull(
        math::Random& rng, const BlockState* const options[], size_t count);
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
