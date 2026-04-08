#pragma once

#include "../../../core/AgeableEntity.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

// 前向声明
class ItemStack;

/**
 * @brief 动物实体基类
 *
 * 可繁殖的动物实体基类，支持喂食、繁殖、跟随父母等行为。
 * 猪、牛、羊、鸡等动物继承此类。
 *
 * 参考 MC 1.16.5 AnimalEntity
 */
class AnimalEntity : public AgeableEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AnimalEntity(LegacyEntityType type, EntityId id);
    ~AnimalEntity() override = default;

    // 禁止拷贝
    AnimalEntity(const AnimalEntity&) = delete;
    AnimalEntity& operator=(const AnimalEntity&) = delete;

    // 允许移动
    AnimalEntity(AnimalEntity&&) = default;
    AnimalEntity& operator=(AnimalEntity&&) = default;

    // ========== 繁殖系统 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * @param itemStack 物品堆
     * @return 是否可以用于繁殖
     *
     * 子类应该重写此方法来定义特定的繁殖物品
     */
    [[nodiscard]] virtual bool isBreedingItem(const ItemStack& itemStack) const;

    /**
     * @brief 检查是否可以与另一动物交配
     * @param other 另一个动物
     * @return 是否可以交配
     */
    [[nodiscard]] virtual bool canMateWith(const AnimalEntity& other) const;

    /**
     * @brief 生成幼体
     * @param partner 交配伙伴
     * @return 生成的幼体实体
     *
     * 子类必须重写此方法来创建特定类型的幼体
     */
    virtual std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) = 0;

    /**
     * @brief 玩家交互
     * @param player 玩家
     * @param hand 手（主手/副手）
     * @return 是否成功交互
     */
    // TODO: bool interact(PlayerEntity& player, Hand hand);

    // ========== 爱心状态 ==========

    /**
     * @brief 获取爱心计时器
     */
    [[nodiscard]] i32 getInLove() const { return m_inLoveTimer; }

    /**
     * @brief 设置爱心计时器
     */
    void setInLove(i32 timer) { m_inLoveTimer = timer; }

    /**
     * @brief 获取喂食玩家的ID
     */
    [[nodiscard]] u64 getLoveCause() const { return m_loveCause; }

    /**
     * @brief 设置喂食玩家
     */
    void setLoveCause(u64 playerId) { m_loveCause = playerId; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    /**
     * @brief 注册 AI 目标
     *
     * 子类应该调用此方法来注册基础动物行为：
     * - SwimGoal (优先级 0)
     * - PanicGoal (优先级 1)
     * - BreedGoal (优先级 2)
     * - TemptGoal (优先级 3)
     * - FollowParentGoal (优先级 4)
     * - WaterAvoidingRandomWalkingGoal (优先级 5)
     * - LookAtGoal (优先级 6)
     * - LookRandomlyGoal (优先级 7)
     */
    void registerGoals() override;

    /**
     * @brief 注册属性
     *
     * 注册动物的基础属性。子类应该调用此方法然后覆盖特定属性值。
     * MC 1.16.5 动物默认属性：
     * - MAX_HEALTH: 10.0
     * - MOVEMENT_SPEED: 0.2
     */
    void registerAttributes() override;

    /**
     * @brief 更新爱心状态
     */
    void updateInLove();

    /**
     * @brief 重置爱心状态
     */
    void resetInLove();

private:
    i32 m_inLoveTimer = 0;    // 爱心动画计时器
    u64 m_loveCause = 0;       // 使其进入爱心状态的玩家ID

    static constexpr i32 IN_LOVE_DURATION = 600; // 爱心状态持续时间（30秒）
};

} // namespace mc
