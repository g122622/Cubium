#pragma once

#include "world/blockentity/BlockEntity.hpp"
#include "world/block/BlockPos.hpp"
#include <vector>
#include <optional>
#include <string>

namespace mc {

class IWorld;
class Player;
class LivingEntity;

namespace blockentity {

/**
 * @brief 潮涌核心方块实体
 *
 * 潮涌核心是一种水下信标类方块，特点：
 * - 需要周围被水包围（3x3x3立方体）
 * - 需要海晶石框架（16-42个方块）激活
 * - 激活后给予附近玩家潮涌能量效果
 * - 框架达到42个方块时睁开眼睛，攻击敌对生物
 *
 * 框架检测逻辑（MC 1.16.5）：
 * - 中心周围3x3x3必须全部是水
 * - 框架位置：5x5x5范围内，距离中心2格的位置
 * - 有效方块：海晶石、海晶石砖、暗海晶石、海晶灯
 *
 * 效果范围：
 * - 16-20个方块：(16/7)*16 = 16格
 * - 21-27个方块：(21/7)*16 = 32格
 * - 28-34个方块：(28/7)*16 = 48格
 * - 35-41个方块：(35/7)*16 = 64格
 * - 42+个方块：(42/7)*16 = 96格
 *
 * 参考: net.minecraft.tileentity.ConduitTileEntity
 */
class ConduitEntity : public BlockEntity {
public:
    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit ConduitEntity(const BlockPos& pos);

    ~ConduitEntity() override = default;

    // ========== 状态查询 ==========

    /**
     * @brief 检查是否激活
     * @return 如果激活返回true
     */
    [[nodiscard]] bool isActive() const { return m_active; }

    /**
     * @brief 检查眼睛是否睁开
     * 眼睛睁开时表示可以攻击敌对生物
     * @return 如果眼睛睁开返回true
     */
    [[nodiscard]] bool isEyeOpen() const { return m_eyeOpen; }

    /**
     * @brief 获取激活旋转角度（客户端渲染用）
     * @param partialTick 部分tick
     * @return 旋转角度（弧度）
     */
    [[nodiscard]] f32 getActiveRotation(f32 partialTick) const;

    /**
     * @brief 获取海晶石框架位置列表
     * @return 框架方块位置列表
     */
    [[nodiscard]] const std::vector<BlockPos>& getPrismarinePositions() const {
        return m_prismarinePositions;
    }

    /**
     * @brief 获取当前攻击目标
     * @return 目标实体，如果没有返回nullptr
     */
    [[nodiscard]] LivingEntity* getTarget() const { return m_target; }

    /**
     * @brief 获取效果范围
     * @return 效果半径（方块数）
     */
    [[nodiscard]] i32 getEffectRange() const;

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const override { return true; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    /**
     * @brief 检查是否应该激活
     * 检测周围水和海晶石框架
     * @param world 世界引用
     * @return 如果应该激活返回true
     */
    [[nodiscard]] bool shouldBeActive(IWorld& world);

    /**
     * @brief 给附近玩家添加潮涌能量效果
     * @param world 世界引用
     */
    void addEffectsToPlayers(IWorld& world);

    /**
     * @brief 攻击附近的敌对生物
     * 需要42个以上的框架方块
     * @param world 世界引用
     */
    void attackMobs(IWorld& world);

    /**
     * @brief 检查位置是否在水中
     * @param world 世界引用
     * @param pos 方块位置
     * @return 如果在水中返回true
     */
    [[nodiscard]] bool isWaterAt(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 检查方块是否为有效的框架方块
     * @param world 世界引用
     * @param pos 方块位置
     * @return 如果是有效框架方块返回true
     */
    [[nodiscard]] bool isValidFrameBlock(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 设置激活状态
     * @param world 世界引用
     * @param active 激活状态
     */
    void setActive(IWorld& world, bool active);

    /**
     * @brief 设置眼睛状态
     * @param eyeOpen 眼睛是否睁开
     */
    void setEyeOpen(bool eyeOpen);

    // ========== 状态数据 ==========

    bool m_active = false;              ///< 是否激活
    bool m_eyeOpen = false;             ///< 眼睛是否睁开
    i32 m_ticksExisted = 0;             ///< 存在时间（tick）
    f32 m_activeRotation = 0.0f;        ///< 激活时的旋转角度
    i64 m_ambientSoundCounter = 0;      ///< 环境音效计数器
    std::vector<BlockPos> m_prismarinePositions; ///< 海晶石框架位置

    // ========== 目标追踪 ==========

    LivingEntity* m_target = nullptr;   ///< 当前攻击目标
    std::optional<std::string> m_targetUuid; ///< 目标的UUID（用于持久化）
};

} // namespace blockentity
} // namespace mc
