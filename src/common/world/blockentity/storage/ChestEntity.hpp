#pragma once

#include "world/blockentity/core/LockableBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "world/blockentity/storage/DoubleSidedInventory.hpp"
#include <memory>

namespace mc {

class World;
class ChestBlock;

namespace blockentity {

/**
 * @brief 箱子方块实体
 *
 * 存储27格物品，支持：
 * - 单箱/双箱模式
 * - 打开计数和盖子动画
 * - 红石比较器信号
 * - 锁定功能
 *
 * 参考: net.minecraft.tileentity.ChestTileEntity
 *
 * 盖子动画:
 * - m_lidAngle 从0.0到1.0表示打开程度
 * - 每tick更新，使用插值实现平滑动画
 *
 * 双箱合并:
 * - 当两个箱子相邻放置时自动合并
 * - 使用ChestType属性标识LEFT/RIGHT/SINGLE
 * - 打开时创建DoubleSidedInventory包装两个箱子
 */
class ChestEntity : public LockableBlockEntity {
public:
    /// 箱子容量（27格）
    static constexpr i32 CHEST_SIZE = 27;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit ChestEntity(const BlockPos& pos);

    /**
     * @brief 构造函数（指定类型）
     * @param type 方块实体类型
     * @param pos 方块位置
     */
    ChestEntity(BlockEntityType type, const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~ChestEntity() override;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return CHEST_SIZE; }

    // ========== 箱子特有接口 ==========

    /**
     * @brief 检查是否是双箱的一部分
     * @param world 世界引用
     * @return 如果连接到另一个箱子返回true
     */
    [[nodiscard]] bool isDoubleChest(World& world) const;

    /**
     * @brief 获取相邻箱子（如果是双箱）
     * @param world 世界引用
     * @return 相邻箱子实体指针，如果不是双箱返回nullptr
     */
    [[nodiscard]] ChestEntity* getConnectedChest(World& world) const;

    /**
     * @brief 获取合并后的双箱背包
     * @param world 世界引用
     * @return 双箱背包，如果是单箱返回nullptr
     */
    [[nodiscard]] std::unique_ptr<DoubleSidedInventory> getDoubleInventory(World& world);

    /**
     * @brief 获取打开计数
     * @return 当前打开的玩家数量
     */
    [[nodiscard]] i32 getOpenCount() const { return m_openCount; }

    /**
     * @brief 玩家打开箱子
     */
    void openContainer() override;

    /**
     * @brief 玩家关闭箱子
     */
    void closeContainer() override;

    /**
     * @brief 计算红石比较器信号
     * @param world 世界引用
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getComparatorSignal(IWorld& world) const;

    // ========== 动画支持 ==========

    /**
     * @brief 获取盖子打开角度
     * @return 角度 (0.0 = 关闭, 1.0 = 完全打开)
     */
    [[nodiscard]] f32 getLidAngle() const { return m_lidAngle; }

    /**
     * @brief 获取上一帧的盖子角度（用于插值）
     * @return 角度
     */
    [[nodiscard]] f32 getPrevLidAngle() const { return m_prevLidAngle; }

    /**
     * @brief 更新盖子动画
     * @param partialTick 部分tick时间
     */
    void updateLidAnimation(f32 partialTick);

    // ========== Tick 更新 ==========

    void tick(World& world) override;
    [[nodiscard]] bool needsTick() const override { return true; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    [[nodiscard]] String getDefaultName() const override { return "container.chest"; }

    /**
     * @brief 广播打开/关闭事件
     * @param world 世界引用
     * @param open true=打开, false=关闭
     */
    void broadcastChestState(World& world, bool open);

    /**
     * @brief 播放音效
     * @param world 世界引用
     * @param open true=打开音效, false=关闭音效
     */
    void playSound(World& world, bool open);

private:
    SimpleInventory m_inventory;   ///< 27格物品存储
    f32 m_lidAngle = 0.0f;          ///< 当前盖子角度 (0-1)
    f32 m_prevLidAngle = 0.0f;      ///< 上一帧盖子角度
    i32 m_openCount = 0;            ///< 打开计数
    i32 m_ticksSinceSync = 0;       ///< 同步计数器
};

} // namespace blockentity
} // namespace mc
