#pragma once

#include "world/blockentity/BlockEntity.hpp"
#include "world/block/BlockPos.hpp"
#include <array>

namespace mc {

class World;

namespace blockentity {

/**
 * @brief 附魔台方块实体
 *
 * 附魔台是交互类方块实体，负责：
 * - 检测周围书架
 * - 计算附魔力量
 * - 管理书本动画
 *
 * 附魔力量计算：
 * - 有效书架必须在附魔台周围2格范围内
 * - 书架与附魔台之间必须有空气
 * - 每个有效书架增加0.5附魔力量（最大15书架 = 7.5力量）
 *
 * 参考: net.minecraft.tileentity.EnchantingTableTileEntity
 */
class EnchantingTableEntity : public BlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit EnchantingTableEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~EnchantingTableEntity() override = default;

    // ========== 方块实体接口 ==========

    /**
     * @brief 每tick更新
     * @param world 所在世界
     */
    void tick(World& world) override;

    /**
     * @brief 检查是否需要tick
     * @return 附魔台总是返回false（不需要tick）
     */
    [[nodiscard]] bool needsTick() const override {
        return false;
    }

    /**
     * @brief 从JSON加载数据
     * @param data JSON数据
     * @return 是否成功
     */
    bool load(const nlohmann::json& data) override;

    /**
     * @brief 保存数据到JSON
     * @param data 输出JSON数据
     */
    void save(nlohmann::json& data) const override;

    /**
     * @brief 创建副本
     * @return 副本的unique_ptr
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== 附魔力量 ==========

    /**
     * @brief 获取附魔力量
     * @return 附魔力量（0-15）
     */
    [[nodiscard]] i32 getEnchantPower() const { return m_enchantPower; }

    /**
     * @brief 重新计算附魔力量
     * @param world 世界
     */
    void recalculateEnchantPower(World& world);

    // ========== 自定义名称 ==========

    /**
     * @brief 获取自定义名称
     * @return 自定义名称
     */
    [[nodiscard]] String getCustomName() const override { return m_customName; }

    /**
     * @brief 设置自定义名称
     * @param name 名称
     */
    void setCustomName(const String& name) override;

    // ========== 动画 ==========

    /**
     * @brief 获取书本翻转角度
     * @return 角度（弧度）
     */
    [[nodiscard]] f32 getBookRotation() const { return m_bookRotation; }

    /**
     * @brief 获取书本翻开角度
     * @return 角度（0-1，0=关闭，1=完全打开）
     */
    [[nodiscard]] f32 getBookOpenAmount() const { return m_bookOpen; }

    /**
     * @brief 获取书本翻页角度
     * @return 角度（弧度）
     */
    [[nodiscard]] f32 getBookPageAngle() const { return m_bookPageAngle; }

    /**
     * @brief 更新书本动画
     * @param world 世界
     * @param dt Delta时间（秒）
     */
    void updateAnimation(World& world, f32 dt);

private:
    /**
     * @brief 检查位置是否有有效书架
     * @param world 世界
     * @param bookshelfPos 书架位置
     * @param tablePos 附魔台位置
     * @return 如果有效返回true
     */
    [[nodiscard]] static bool isValidBookshelf(World& world,
                                                const BlockPos& bookshelfPos,
                                                const BlockPos& tablePos);

    /// 附魔力量（0-15，每个书架+1，书架最大15个）
    i32 m_enchantPower = 0;

    /// 自定义名称（铁砧重命名）
    String m_customName;

    // ========== 动画状态 ==========
    /// 书本翻转角度（0-2π）
    f32 m_bookRotation = 0.0f;

    /// 上一次翻转角度
    f32 m_prevBookRotation = 0.0f;

    /// 书本翻开程度（0=关闭，1=打开）
    f32 m_bookOpen = 0.0f;

    /// 上一次翻开程度
    f32 m_prevBookOpen = 0.0f;

    /// 书页角度
    f32 m_bookPageAngle = 0.0f;

    /// 上一次书页角度
    f32 m_prevBookPageAngle = 0.0f;

    /// 动画时间
    f32 m_time = 0.0f;

    /// 随机翻转速度
    f32 m_flipSpeed = 0.0f;

    /// 动画随机种子
    i32 m_randomSeed = 0;
};

} // namespace blockentity
} // namespace mc
