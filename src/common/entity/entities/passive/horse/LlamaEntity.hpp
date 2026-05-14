#pragma once

#include "AbstractChestedHorseEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 羊驼实体
 *
 * 对齐 1.16.5 `LlamaEntity` 的基础层次。当前先把箱子马类层抽出来，
 * 并保留强度、地毯颜色、商队和吐口水相关状态。
 */
class LlamaEntity : public AbstractChestedHorseEntity {
public:
    /**
     * @brief 羊驼颜色
     */
    enum class LlamaColor : u8 { Creamy = 0, White = 1, Brown = 2, Gray = 3 };

    /**
     * @brief 构造羊驼实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    LlamaEntity(LegacyEntityType type, EntityId id);
    ~LlamaEntity() override = default;

    LlamaEntity(const LlamaEntity&) = delete;
    LlamaEntity& operator=(const LlamaEntity&) = delete;
    LlamaEntity(LlamaEntity&&) = default;
    LlamaEntity& operator=(LlamaEntity&&) = default;

    /**
     * @brief 创建羊驼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 获取羊驼颜色
     */
    [[nodiscard]] LlamaColor getColor() const { return m_color; }

    /**
     * @brief 设置羊驼颜色
     */
    void setColor(LlamaColor color) { m_color = color; }

    /**
     * @brief 随机设置颜色和强度
     */
    void randomizeAppearance();

    /**
     * @brief 羊驼可被骑乘但不能控制方向
     */
    [[nodiscard]] bool canBeRiddenBy(Player* player) const;

    /**
     * @brief 羊驼不能装备鞍
     */
    [[nodiscard]] bool canEquipSaddle() const { return false; }

    /**
     * @brief 返回背包列数
     *
     * 对齐 vanilla，等于 strength。
     */
    [[nodiscard]] i32 getInventoryColumns() const override;

    /**
     * @brief 获取强度
     */
    [[nodiscard]] i32 getStrength() const { return m_strength; }

    /**
     * @brief 设置强度
     */
    void setStrength(i32 strength);

    /**
     * @brief 获取地毯颜色，-1 表示没有地毯
     */
    [[nodiscard]] i32 getCarpetColor() const { return m_carpetColor; }

    /**
     * @brief 设置地毯颜色
     */
    void setCarpetColor(i32 color) { m_carpetColor = color; }

    /**
     * @brief 当前是否处于商队中
     */
    [[nodiscard]] bool isInCaravan() const { return m_inCaravan; }

    /**
     * @brief 设置商队状态
     */
    void setInCaravan(bool inCaravan) { m_inCaravan = inCaravan; }

    /**
     * @brief 获取商队领头羊驼
     */
    [[nodiscard]] LlamaEntity* getCaravanLeader() const { return m_caravanLeader; }

    /**
     * @brief 设置商队领头羊驼
     */
    void setCaravanLeader(LlamaEntity* leader) { m_caravanLeader = leader; }

    /**
     * @brief 当前是否正在吐口水
     */
    [[nodiscard]] bool isSpitting() const { return m_spitting; }

    /**
     * @brief 设置吐口水状态
     */
    void setSpitting(bool spitting) { m_spitting = spitting; }

    /**
     * @brief 羊驼使用干草块繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 羊驼不依赖专门驯服食物
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.77f; }

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    LlamaColor m_color = LlamaColor::Creamy;
    i32 m_strength = 1;
    i32 m_carpetColor = -1;
    bool m_inCaravan = false;
    LlamaEntity* m_caravanLeader = nullptr;
    bool m_spitting = false;
    i32 m_spitCooldown = 0;
};

} // namespace mc
