#pragma once

#include "AbstractHorseEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>
#include <vector>

namespace mc {

/**
 * @brief 羊驼实体
 *
 * 可驯服、可携带物品的马类实体，可组成商队。
 *
 * 特性：
 * - 可驯服：通过骑乘驯服
 * - 可装备：地毯装饰
 * - 背包：3-15格背包（取决于强度）
 * - 商队：可跟随前方的羊驼
 * - 吐口水：攻击时向目标吐口水
 * - 繁殖：使用干草块繁殖
 * - 变种：4种颜色变种
 *
 * 参考 MC 1.16.5 LlamaEntity
 */
class LlamaEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 羊驼颜色
     */
    enum class LlamaColor : u8 {
        Creamy = 0,    // 奶油色
        White = 1,     // 白色
        Brown = 2,     // 棕色
        Gray = 3       // 灰色
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    LlamaEntity(LegacyEntityType type, EntityId id);
    ~LlamaEntity() override = default;

    // 禁止拷贝
    LlamaEntity(const LlamaEntity&) = delete;
    LlamaEntity& operator=(const LlamaEntity&) = delete;

    // 允许移动
    LlamaEntity(LlamaEntity&&) = default;
    LlamaEntity& operator=(LlamaEntity&&) = default;

    /**
     * @brief 创建羊驼实体
     * @param world 世界实例
     * @return 新的羊驼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 外观系统 ==========

    /**
     * @brief 获取羊驼颜色
     */
    [[nodiscard]] LlamaColor getColor() const { return m_color; }

    /**
     * @brief 设置羊驼颜色
     */
    void setColor(LlamaColor color) { m_color = color; }

    /**
     * @brief 随机设置外观
     */
    void randomizeAppearance();

    // ========== 骑乘系统 ==========

    /**
     * @brief 检查玩家是否可以骑乘
     * 羊驼可以骑乘但不能控制方向
     */
    [[nodiscard]] bool canBeRiddenBy(Player* player) const;

    /**
     * @brief 是否可以装备鞍
     * 羊驼不能装备鞍
     */
    [[nodiscard]] bool canEquipSaddle() const { return false; }

    // ========== 背包系统 ==========

    /**
     * @brief 是否有箱子
     */
    [[nodiscard]] bool hasChest() const { return m_hasChest; }

    /**
     * @brief 设置箱子状态
     */
    void setChest(bool chest) { m_hasChest = chest; }

    /**
     * @brief 获取背包大小
     * 根据强度决定背包大小：3 + 3 * strength
     */
    [[nodiscard]] i32 getInventoryColumns() const;

    /**
     * @brief 获取装备栏大小
     */
    [[nodiscard]] i32 getInventorySize() const override;

    /**
     * @brief 获取强度
     * 强度影响背包大小
     */
    [[nodiscard]] i32 getStrength() const { return m_strength; }

    // ========== 装饰系统 ==========

    /**
     * @brief 获取地毯颜色
     * 返回-1表示没有地毯
     */
    [[nodiscard]] i32 getCarpetColor() const { return m_carpetColor; }

    /**
     * @brief 设置地毯颜色
     */
    void setCarpetColor(i32 color) { m_carpetColor = color; }

    // ========== 商队系统 ==========

    /**
     * @brief 是否在商队中
     */
    [[nodiscard]] bool isInCaravan() const { return m_inCaravan; }

    /**
     * @brief 设置商队状态
     */
    void setInCaravan(bool inCaravan) { m_inCaravan = inCaravan; }

    /**
     * @brief 获取商队领袖
     */
    [[nodiscard]] LlamaEntity* getCaravanLeader() const { return m_caravanLeader; }

    /**
     * @brief 设置商队领袖
     */
    void setCaravanLeader(LlamaEntity* leader) { m_caravanLeader = leader; }

    // ========== 攻击系统 ==========

    /**
     * @brief 是否正在吐口水
     */
    [[nodiscard]] bool isSpitting() const { return m_spitting; }

    /**
     * @brief 设置吐口水状态
     */
    void setSpitting(bool spitting) { m_spitting = spitting; }

    // ========== 繁殖系统 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 使用干草块繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否可用于驯服
     * 羊驼不响应特定驯服物品
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.77f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 外观
    LlamaColor m_color = LlamaColor::Creamy;

    // 背包
    bool m_hasChest = false;
    i32 m_strength = 1;  // 1-5

    // 装饰
    i32 m_carpetColor = -1;  // -1 表示无地毯

    // 商队
    bool m_inCaravan = false;
    LlamaEntity* m_caravanLeader = nullptr;

    // 攻击
    bool m_spitting = false;
    i32 m_spitCooldown = 0;
};

} // namespace mc
