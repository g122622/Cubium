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

#include "../../Block.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <utility>
#include <vector>

namespace mc {

class IWorld;
class BlockState;
class ItemStack;

namespace blocks {

/**
 * @brief 被感染方块基类（虫蚀方块/怪物蛋方块）
 *
 * 外观与普通方块相同，但被破坏时会生成蠹虫。
 * 蠹虫可以藏入这些方块中。
 *
 * 有 6 种虫蚀方块变体：
 * - INFESTED_STONE (虫蚀石头)
 * - INFESTED_COBBLESTONE (虫蚀圆石)
 * - INFESTED_STONE_BRICKS (虫蚀石砖)
 * - INFESTED_MOSSY_STONE_BRICKS (虫蚀苔藓石砖)
 * - INFESTED_CRACKED_STONE_BRICKS (虫蚀裂纹石砖)
 * - INFESTED_CHISELED_STONE_BRICKS (虫蚀錾制石砖)
 */
class InfestedBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param hostBlock 被感染的方块ID
     * @param properties 方块属性
     */
    InfestedBlock(u32 hostBlock, const BlockProperties& properties);
    ~InfestedBlock() override = default;

    /**
     * @brief 方块被破坏后的额外生成处理
     *
     * 当虫蚀方块被破坏时，根据游戏规则和精准采集附魔决定是否生成蠹虫。
     * 如果 doTileDrops 游戏规则为 true 且工具没有精准采集附魔，则生成蠹虫。
     *
     * 参考: net.minecraft.block.InfestedBlock.spawnAfterBreak
     */
    void spawnAfterBreak(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const ItemStack* tool,
        bool dropExp) const override;

    // ========== 掉落 ==========

    [[nodiscard]] u32 getHostBlock() const { return m_hostBlock; }

    // ========== 静态方法 ==========

    /**
     * @brief 检查方块状态是否可以被虫蚀
     * @param state 要检查的方块状态
     * @return 如果此方块有对应的虫蚀版本，返回 true
     */
    [[nodiscard]] static bool canContainSilverfish(const BlockState& state);

    /**
     * @brief 将普通方块转换为虫蚀方块
     * @param block 普通方块指针
     * @return 对应的虫蚀方块的默认状态，如果不存在则返回 nullptr
     */
    [[nodiscard]] static const BlockState* infest(const Block& block);

    /**
     * @brief 注册虫蚀方块映射
     * @param hostBlock 原版方块ID
     * @param infestedBlock 虫蚀方块ID
     *
     * 此方法在 VanillaBlocks 初始化时调用，用于建立映射关系。
     */
    static void registerInfestedBlock(u32 hostBlock, u32 infestedBlock);

    /**
     * @brief 初始化映射表
     *
     * 必须在使用 canContainSilverfish 或 infest 前调用。
     * 由 VanillaBlocks::initialize() 自动调用。
     */
    static void initializeMappings();

private:
    /// 被感染的方块ID
    u32 m_hostBlock;

    /// 原版方块到虫蚀方块的映射表
    static std::vector<std::pair<u32, u32>> s_hostToInfestedMap;

    /// 映射表是否已初始化
    static bool s_mappingsInitialized;
};

} // namespace blocks
} // namespace mc
