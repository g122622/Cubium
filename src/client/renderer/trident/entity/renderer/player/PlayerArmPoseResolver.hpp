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

#include "client/renderer/trident/entity/model/player/PlayerModel.hpp"
#include "common/core/Types.hpp"

namespace mc {
class Player;
}

namespace mc::client::renderer::entity::renderer::player {

/**
 * @brief 玩家手臂姿态解析器
 *
 * 将玩家手持物品与使用状态解析为第三人称 PlayerModel 使用的 ArmPose。
 * 对应 MC 1.21.11 AvatarRenderer.getArmPose 与 setModelVisibilities 中的双手协调逻辑。
 *
 * 该类为纯逻辑工具类，不依赖任何渲染层（Vulkan/管线/层渲染器），
 * 可在单元测试中直接调用，避免构造完整 PlayerRenderer 所引入的重依赖。
 */
class PlayerArmPoseResolver {
public:
    /**
     * @brief 解析指定手的手臂姿态
     *
     * 解析流程：
     * 1. 空手 → Empty
     * 2. 已装填的弩（且未挥动）→ CrossbowHold
     * 3. 正在使用物品且使用手匹配时按 UseAction 映射：
     *    Block→Block, Bow→BowAndArrow, Spear/Trident→ThrowSpear,
     *    Crossbow→CrossbowCharge, Spyglass→Spyglass, Brush→Brush,
     *    Bundle 暂降级为 Item（TODO：第三人称 ArmPose 无 EatOrDrink 枚举，未来扩展后应改为返回该姿态）
     * 4. 长矛类物品（ItemTags::SPEARS）→ ThrowSpear
     * 5. 默认持有物品 → Item
     *
     * @param player 玩家实体
     * @param hand 要查询的手（主手或副手）
     * @return 对应的 ArmPose
     */
    [[nodiscard]] static model::player::ArmPose determineArmPose(::mc::Player& player, ::mc::Hand hand);

    /**
     * @brief 双手姿态协调结果
     *
     * leftArmPose / rightArmPose 为已根据玩家主手偏好映射后的模型左右臂姿态。
     */
    struct ArmPosePair {
        model::player::ArmPose leftArmPose;
        model::player::ArmPose rightArmPose;
    };

    /**
     * @brief 解析双手姿态并完成双手协调与主/副手映射
     *
     * 步骤：
     * 1. 分别解析主手与副手的 ArmPose
     * 2. 双手姿态协调：若主手姿态为 BowAndArrow/CrossbowCharge/CrossbowHold，
     *    副手姿态降级为 Empty（副手空）或 Item（副手非空）
     * 3. 根据玩家主手偏好映射到模型右臂/左臂：
     *    右撇子：主手姿态 → 右臂，副手姿态 → 左臂
     *    左撇子：主手姿态 → 左臂，副手姿态 → 右臂
     *
     * @param player 玩家实体
     * @return 已映射到模型左右臂的姿态对
     */
    [[nodiscard]] static ArmPosePair resolveArmPoses(::mc::Player& player);
};

} // namespace mc::client::renderer::entity::renderer::player
