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
#include "common/world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class Player;

namespace blockentity {

/**
 * @brief 末影箱方块实体
 *
 * 末影箱存储玩家背包中的末影箱物品栏。
 * 所有末影箱共享同一个物品存储（每个玩家独立）。
 *
 * 特点：
 * - 不存储实际物品（物品在玩家数据中）
 * - 打开动画与普通箱子相同
 * - 爆破抗性高（600）
 *
 * 参考: net.minecraft.tileentity.EnderChestTileEntity
 */
class EnderChestEntity : public BlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit EnderChestEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~EnderChestEntity() override;

    // ========== 打开状态管理 ==========

    /**
     * @brief 获取打开计数
     * @return 当前打开的玩家数量
     */
    [[nodiscard]] i32 getOpenCount() const { return m_openCount; }

    /**
     * @brief 玩家打开末影箱
     * @param player 玩家
     * @return 如果成功打开返回true
     */
    bool openContainer(Player* player);

    /**
     * @brief 玩家关闭末影箱
     * @param player 玩家
     */
    void closeContainer(Player* player);

    /**
     * @brief 检查玩家是否可以访问末影箱
     * @param player 玩家
     * @return 如果可以访问返回true
     */
    [[nodiscard]] bool canPlayerAccess(Player* player) const;

    // ========== 动画支持 ==========

    /**
     * @brief 获取盖子打开角度
     * @return 角度 (0.0 = 关闭, 1.0 = 完全打开)
     */
    [[nodiscard]] f32 getLidAngle() const { return m_lidAngle; }

    /**
     * @brief 获取插值后的盖子角度（用于渲染）
     * @param partialTick 部分tick时间 (0.0 - 1.0)
     * @return 插值后的角度
     */
    [[nodiscard]] f32 getLidAngle(f32 partialTick) const;

    /**
     * @brief 获取上一帧的盖子角度
     * @return 角度
     */
    [[nodiscard]] f32 getPrevLidAngle() const { return m_prevLidAngle; }

    /**
     * @brief 更新盖子动画
     * @param partialTick 部分tick时间
     */
    void updateLidAnimation(f32 partialTick);

    // ========== 方块事件 ==========

    /**
     * @brief 处理方块事件（客户端动画同步）
     * @param id 事件ID（1 = 开合动画）
     * @param type 事件参数（>0 表示打开，0 表示关闭）
     * @return 如果事件被处理返回true
     */
    [[nodiscard]] bool triggerEvent(i32 id, i32 type) override;

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return true; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    f32 m_lidAngle = 0.0f;     ///< 当前盖子角度 (0-1)
    f32 m_prevLidAngle = 0.0f; ///< 上一帧盖子角度
    i32 m_openCount = 0;       ///< 打开计数
    i32 m_ticksSinceSync = 0;  ///< 同步计数器
};

} // namespace blockentity
} // namespace mc
