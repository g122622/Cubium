#pragma once

#include "world/blockentity/core/LockableBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 潜影盒方块实体
 *
 * 潜影盒是一种特殊容器，特点：
 * - 27格物品存储
 * - 被破坏时保留物品（不掉落）
 * - 可以被锁定（需要正确名称的物品打开）
 * - 打开时有动画效果
 *
 * 参考: net.minecraft.tileentity.ShulkerBoxTileEntity
 */
class ShulkerBoxEntity : public LockableBlockEntity {
public:
    /// 潜影盒容量（27格）
    static constexpr i32 SHULKER_BOX_SIZE = 27;

    /// 潜影盒打开状态枚举
    enum class AnimationStatus : u8 {
        Closed = 0,     ///< 关闭状态
        Opening = 1,    ///< 打开中
        Opened = 2,     ///< 已打开
        Closing = 3     ///< 关闭中
    };

    // ========== 构造函数 ==========

    using ContainerBlockEntity::openContainer;
    using ContainerBlockEntity::closeContainer;

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit ShulkerBoxEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~ShulkerBoxEntity() override;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return SHULKER_BOX_SIZE; }

    // ========== 潜影盒特有接口 ==========

    /**
     * @brief 获取动画状态
     * @return 动画状态
     */
    [[nodiscard]] AnimationStatus getAnimationStatus() const { return m_animationStatus; }

    /**
     * @brief 获取打开进度（0.0 - 1.0）
     * @return 打开进度
     */
    [[nodiscard]] f32 getProgress(f32 partialTick) const;

    /**
     * @brief 玩家打开潜影盒
     * @param player 玩家（可为nullptr）
     */
    void openContainer(Player* player) override;

    /**
     * @brief 玩家关闭潜影盒
     * @param player 玩家（可为nullptr）
     */
    void closeContainer(Player* player) override;

    /**
     * @brief 检查玩家是否可以打开
     * @param world 世界引用
     * @return 如果可以打开返回true
     */
    [[nodiscard]] bool canOpen(IWorld& world) const;

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const override { return true; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    [[nodiscard]] String getDefaultName() const override { return "container.shulkerBox"; }

private:
    /**
     * @brief 更新动画状态
     * @param partialTick 部分tick时间
     */
    void updateAnimation(f32 partialTick);

    /**
     * @brief 检查是否可以打开（内部使用）
     * @param world 世界引用
     * @return 如果可以打开返回true
     */
    [[nodiscard]] bool checkCanOpen(IWorld& world) const;

    /**
     * @brief 推动碰撞的实体
     *
     * MC 1.16.5: 当潜影盒打开/关闭时，推动附近实体。
     * 参考: ShulkerBoxTileEntity.moveCollidedEntities()
     *
     * @param world 世界引用
     * @param facing 潜影盒朝向（缓存）
     */
    void moveCollidedEntities(IWorld& world, Direction facing);

    /**
     * @brief 缓存潜影盒朝向
     * @param world 世界引用
     */
    void cacheFacing(IWorld& world);

    SimpleInventory m_inventory;       ///< 27格物品存储
    AnimationStatus m_animationStatus = AnimationStatus::Closed;  ///< 动画状态
    f32 m_progress = 0.0f;             ///< 打开进度 (0.0 - 1.0)
    f32 m_prevProgress = 0.0f;         ///< 上一帧打开进度
    i32 m_openCount = 0;               ///< 打开计数
    Direction m_cachedFacing = Direction::None;  ///< 缓存的朝向（避免每帧查询）
};

} // namespace blockentity
} // namespace mc
