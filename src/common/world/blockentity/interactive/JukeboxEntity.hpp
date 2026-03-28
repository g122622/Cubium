#pragma once

#include "world/blockentity/ContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 唱片机方块实体
 *
 * 唱片机用于播放音乐唱片，特点：
 * - 1个槽位存放唱片
 * - 播放音乐时发射红石信号
 * - 可以被漏斗提取唱片
 *
 * 参考: net.minecraft.tileentity.JukeboxTileEntity
 */
class JukeboxEntity : public ContainerBlockEntity {
public:
    /// 唱片机只有1个槽位
    static constexpr i32 SLOT_RECORD = 0;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit JukeboxEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~JukeboxEntity() override;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return 1; }

    // ========== 唱片机接口 ==========

    /**
     * @brief 获取唱片
     * @return 唱片物品
     */
    [[nodiscard]] ItemStack getRecord() const;

    /**
     * @brief 设置唱片
     * @param record 唱片物品
     */
    void setRecord(const ItemStack& record);

    /**
     * @brief 检查是否有唱片
     * @return 如果有唱片返回true
     */
    [[nodiscard]] bool hasRecord() const;

    /**
     * @brief 开始播放唱片
     * @param world 世界引用
     */
    void startPlaying(IWorld& world);

    /**
     * @brief 停止播放唱片
     * @param world 世界引用
     */
    void stopPlaying(IWorld& world);

    /**
     * @brief 检查是否正在播放
     * @return 如果正在播放返回true
     */
    [[nodiscard]] bool isPlaying() const { return m_isPlaying; }

    /**
     * @brief 获取红石信号强度
     * @return 信号强度（有唱片时返回唱片ID，无唱片返回0）
     */
    [[nodiscard]] i32 getComparatorSignal() const;

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const override { return true; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    SimpleInventory m_inventory;    ///< 1格物品存储
    bool m_isPlaying = false;       ///< 是否正在播放
    i32 m_recordId = 0;             ///< 当前播放的唱片ID
};

} // namespace blockentity
} // namespace mc
