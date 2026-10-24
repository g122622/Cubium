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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/GameMasterBlock.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"

#include <memory>
#include <string>

namespace mc {

class BlockItemUseContext;
class IWorld;
class Player;

namespace blocks {

/**
 * @brief 拼图方块
 *
 * 用于结构生成的拼图系统。
 *
 * 状态属性：
 * - ORIENTATION: 方向 (DOWN_EAST, DOWN_NORTH, ..., UP_WEST)
 */
class JigsawBlock : public Block, public GameMasterBlock {
public:
    explicit JigsawBlock(const BlockProperties& properties);
    ~JigsawBlock() override = default;

    // ========== 标记接口 ==========

    [[nodiscard]] bool isGameMaster() const noexcept override
    {
        return true;
    }

    // ========== 放置 ==========

    /**
     * @brief 放置时按点击面与玩家朝向推导 ORIENTATION（对齐 vanilla JigsawBlock.getStateForPlacement）
     *
     * 点击面为竖直轴时，front 取点击面、top 取玩家水平朝向的反方向；否则 front 取点击面、top 取 Up。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 方块实体 ==========

    /// 拼图方块带方块实体（保存 name/target/pool/joint/final_state/优先级）
    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    /// 创建拼图方块实体
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 交互 ==========

    [[nodiscard]] BlockActionResult useWithoutItem(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        const BlockRaycastResult& hit) override;

    // ========== 结构生成辅助 ==========

    /**
     * @brief 判断两个拼图方块连接点是否可以对接（对齐 vanilla JigsawBlock.canAttach）
     *
     * 条件：前朝向互为反方向；连接类型为 rollable，或两侧 top 朝向相同；且前者的 target
     * 等于后者的 name。
     *
     * @param sourceOrientation 源连接点朝向与名称信息
     * @param sourceTarget 源连接点的 target 名称
     * @param targetOrientation 目标连接点朝向
     * @param targetName 目标连接点的 name 名称
     * @param sourceJointRollable 源连接点是否为 rollable（否则为 aligned）
     * @return 是否可以对接
     */
    [[nodiscard]] static bool canAttach(world::gen::jigsaw::JigsawOrientation sourceOrientation,
        const std::string& sourceTarget,
        world::gen::jigsaw::JigsawOrientation targetOrientation,
        const std::string& targetName,
        bool sourceJointRollable);

    /**
     * @brief 获取拼图方块的 front 朝向（ORIENTATION 的主方向，对齐 vanilla JigsawBlock.getFrontFacing）
     */
    [[nodiscard]] static Direction getFrontFacing(const BlockState& state);

    /**
     * @brief 获取拼图方块的 top 朝向（ORIENTATION 的旋转方向，对齐 vanilla JigsawBlock.getTopFacing）
     */
    [[nodiscard]] static Direction getTopFacing(const BlockState& state);
};

} // namespace blocks
} // namespace mc
