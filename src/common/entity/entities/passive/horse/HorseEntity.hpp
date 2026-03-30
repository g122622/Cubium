#pragma once

#include "AbstractHorseEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 马实体
 *
 * 最常见的马类实体，有多种颜色和花纹。
 *
 * 特性：
 * - 可骑乘：驯服后可骑乘
 * - 可装备：鞍和马铠
 * - 跳跃：可蓄力跳跃
 * - 加速：使用胡萝卜钓竿加速
 * - 繁殖：使用金苹果/金胡萝卜繁殖
 * - 变种：7种基础颜色 × 5种花纹 = 35种变体
 *
 * 参考 MC 1.16.5 HorseEntity
 */
class HorseEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 马的颜色
     */
    enum class HorseColor : u8 {
        White = 0,      // 白色
        Creamy = 1,     // 奶油色
        Chestnut = 2,   // 栗色
        Brown = 3,      // 棕色
        Black = 4,      // 黑色
        Gray = 5,       // 灰色
        DarkBrown = 6   // 深棕色
    };

    /**
     * @brief 马的花纹
     */
    enum class HorseMarking : u8 {
        None = 0,           // 无花纹
        White = 1,          // 白色斑点
        WhiteField = 2,     // 白色区域
        WhiteDots = 3,      // 白色点状
        BlackDots = 4       // 黑色点状
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    HorseEntity(LegacyEntityType type, EntityId id);
    ~HorseEntity() override = default;

    // 禁止拷贝
    HorseEntity(const HorseEntity&) = delete;
    HorseEntity& operator=(const HorseEntity&) = delete;

    // 允许移动
    HorseEntity(HorseEntity&&) = default;
    HorseEntity& operator=(HorseEntity&&) = default;

    /**
     * @brief 创建马实体
     * @param world 世界实例
     * @return 新的马实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 外观系统 ==========

    /**
     * @brief 获取马的颜色
     */
    [[nodiscard]] HorseColor getColor() const { return m_color; }

    /**
     * @brief 设置马的颜色
     */
    void setColor(HorseColor color) { m_color = color; }

    /**
     * @brief 获取马的花纹
     */
    [[nodiscard]] HorseMarking getMarking() const { return m_marking; }

    /**
     * @brief 设置马的花纹
     */
    void setMarking(HorseMarking marking) { m_marking = marking; }

    /**
     * @brief 随机设置外观
     */
    void randomizeAppearance();

    // ========== 驯服系统 ==========

    /**
     * @brief 检查物品是否可用于驯服
     * 马不响应特定驯服物品，需要骑乘来驯服
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否可用于繁殖
     * 使用金苹果或金胡萝卜
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取装备栏大小
     * 马有2个槽位：鞍和马铠
     */
    [[nodiscard]] i32 getInventorySize() const override { return 2; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.53f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 外观
    HorseColor m_color = HorseColor::White;
    HorseMarking m_marking = HorseMarking::None;

    // 驯服相关
    i32 m_rearingCounter = 0;   // 前腿站立计数器
    bool m_isRearing = false;   // 是否正在前腿站立
};

} // namespace mc
