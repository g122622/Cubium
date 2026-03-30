#pragma once

#include "AbstractHorseEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 驴实体
 *
 * 可骑乘的马类实体，有更大的背包容量。
 *
 * 特性：
 * - 可骑乘：驯服后可骑乘
 * - 可装备：鞍和箱子
 * - 背包：15格背包
 * - 繁殖：与马繁殖产生骡
 *
 * 参考 MC 1.16.5 DonkeyEntity
 */
class DonkeyEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    DonkeyEntity(LegacyEntityType type, EntityId id);
    ~DonkeyEntity() override = default;

    // 禁止拷贝
    DonkeyEntity(const DonkeyEntity&) = delete;
    DonkeyEntity& operator=(const DonkeyEntity&) = delete;

    // 允许移动
    DonkeyEntity(DonkeyEntity&&) = default;
    DonkeyEntity& operator=(DonkeyEntity&&) = default;

    /**
     * @brief 创建驴实体
     * @param world 世界实例
     * @return 新的驴实体
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
     * 驴有16个槽位：鞍槽 + 15格背包（如果有箱子）
     */
    [[nodiscard]] i32 getInventorySize() const override {
        return m_hasChest ? 16 : 1;
    }

    // ========== 繁殖系统 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     * 与马繁殖产生骡
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.45f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_hasChest = false;
};

} // namespace mc
