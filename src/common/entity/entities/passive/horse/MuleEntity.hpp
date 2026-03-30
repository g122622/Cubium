#pragma once

#include "AbstractHorseEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 骡实体
 *
 * 马和驴的杂交后代，不能繁殖但可以装备箱子。
 *
 * 特性：
 * - 可骑乘：驯服后可骑乘
 * - 可装备：鞍和箱子
 * - 背包：15格背包
 * - 不育：无法繁殖
 * - 属性：比驴更强壮
 *
 * 参考 MC 1.16.5 MuleEntity
 */
class MuleEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    MuleEntity(LegacyEntityType type, EntityId id);
    ~MuleEntity() override = default;

    // 禁止拷贝
    MuleEntity(const MuleEntity&) = delete;
    MuleEntity& operator=(const MuleEntity&) = delete;

    // 允许移动
    MuleEntity(MuleEntity&&) = default;
    MuleEntity& operator=(MuleEntity&&) = default;

    /**
     * @brief 创建骡实体
     * @param world 世界实例
     * @return 新的骡实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

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
     * @brief 获取装备栏大小
     * 骡有16个槽位：鞍槽 + 15格背包（如果有箱子）
     */
    [[nodiscard]] i32 getInventorySize() const override {
        return m_hasChest ? 16 : 1;
    }

    // ========== 繁殖系统 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 骡不能繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override {
        (void)itemStack;
        return false;  // 骡不能繁殖
    }

    /**
     * @brief 生成幼体
     * 骡不能繁殖，返回nullptr
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override {
        (void)partner;
        return nullptr;  // 骡不能繁殖
    }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.5f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_hasChest = false;
};

} // namespace mc
