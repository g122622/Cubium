#pragma once

#include "../basic/AnimalEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include <memory>

namespace mc {

/**
 * @brief 海龟实体
 *
 * 生活在海洋和沙滩的大型被动生物。
 *
 * 特性：
 * - 出生地记忆：海龟会记住出生位置并返回产卵
 * - 产卵：在沙滩上产卵，孵化出小海龟
 * - 游泳：擅长游泳，陆地上缓慢
 * - 婴儿：小海龟受到攻击会害怕
 * - 天敌：僵尸、劫掠者等会攻击海龟蛋
 *
 * 参考 MC 1.16.5 TurtleEntity
 */
class TurtleEntity : public AnimalEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    TurtleEntity(LegacyEntityType type, EntityId id);
    ~TurtleEntity() override = default;

    // 禁止拷贝
    TurtleEntity(const TurtleEntity&) = delete;
    TurtleEntity& operator=(const TurtleEntity&) = delete;

    // 允许移动
    TurtleEntity(TurtleEntity&&) = default;
    TurtleEntity& operator=(TurtleEntity&&) = default;

    /**
     * @brief 创建海龟实体
     * @param world 世界实例
     * @return 新的海龟实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 出生地系统 ==========

    /**
     * @brief 设置出生位置
     * @param pos 出生位置
     *
     * 小海龟孵化后会记住这个位置
     */
    void setHomePos(const BlockPos& pos);

    /**
     * @brief 获取出生位置
     */
    [[nodiscard]] const BlockPos& getHomePos() const { return m_homePos; }

    /**
     * @brief 检查是否有出生位置
     */
    [[nodiscard]] bool hasHomePos() const { return m_hasHomePos; }

    // ========== 产卵状态 ==========

    /**
     * @brief 是否正在产卵
     */
    [[nodiscard]] bool isLayingEgg() const { return m_layingEgg; }

    /**
     * @brief 设置产卵状态
     */
    void setLayingEgg(bool laying) { m_layingEgg = laying; }

    /**
     * @brief 是否有蛋
     */
    [[nodiscard]] bool hasEgg() const { return m_hasEgg; }

    /**
     * @brief 设置是否有蛋
     */
    void setHasEgg(bool hasEgg) { m_hasEgg = hasEgg; }

    // ========== 行进状态 ==========

    /**
     * @brief 是否正在前往出生地
     */
    [[nodiscard]] bool isGoingHome() const { return m_goingHome; }

    /**
     * @brief 设置前往出生地状态
     */
    void setGoingHome(bool going) { m_goingHome = going; }

    /**
     * @brief 是否正在旅行（去海里）
     */
    [[nodiscard]] bool isTravelling() const { return m_travelling; }

    /**
     * @brief 设置旅行状态
     */
    void setTravelling(bool travelling) { m_travelling = travelling; }

    // ========== 水陆状态 ==========

    /**
     * @brief 是否在水中
     */
    [[nodiscard]] bool isInWater() const;

    /**
     * @brief 是否在陆地上
     */
    [[nodiscard]] bool isOnLand() const { return !isInWater(); }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 海龟使用海草繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.2f : 0.4f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 出生位置
    BlockPos m_homePos;
    bool m_hasHomePos = false;

    // 产卵状态
    bool m_layingEgg = false;
    bool m_hasEgg = false;

    // 行进状态
    bool m_goingHome = false;
    bool m_travelling = false;

    // 产卵计时器
    i32 m_layEggTimer = 0;
    static constexpr i32 LAY_EGG_DURATION = 200; // 10秒
};

} // namespace mc
