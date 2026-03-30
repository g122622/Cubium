#pragma once

#include "../basic/AnimalEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include <memory>

namespace mc {

// Forward declarations
class LivingEntity;

/**
 * @brief 蜜蜂实体
 *
 * 生活在蜂巢和蜂箱中的小型飞行生物。
 *
 * 特性：
 * - 授粉：采集花朵后获得花粉，可用于加速农作物生长
 * - 蜂巢记忆：记住蜂巢位置并返回
 * - 攻击：被攻击后会群起攻击，螫刺后死亡
 * - 飞行：可以飞行
 * - 群体：会召唤其他蜜蜂一起攻击
 *
 * 参考 MC 1.16.5 BeeEntity
 */
class BeeEntity : public AnimalEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    BeeEntity(LegacyEntityType type, EntityId id);
    ~BeeEntity() override = default;

    // 禁止拷贝
    BeeEntity(const BeeEntity&) = delete;
    BeeEntity& operator=(const BeeEntity&) = delete;

    // 允许移动
    BeeEntity(BeeEntity&&) = default;
    BeeEntity& operator=(BeeEntity&&) = default;

    /**
     * @brief 创建蜜蜂实体
     * @param world 世界实例
     * @return 新的蜜蜂实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 花粉状态 ==========

    /**
     * @brief 是否携带花粉
     */
    [[nodiscard]] bool hasNectar() const { return m_hasNectar; }

    /**
     * @brief 设置花粉状态
     */
    void setHasNectar(bool nectar) { m_hasNectar = nectar; }

    /**
     * @brief 是否有螫刺
     * 蜜蜂失去螫刺后无法攻击
     */
    [[nodiscard]] bool hasStung() const { return m_hasStung; }

    /**
     * @brief 设置螫刺状态
     */
    void setHasStung(bool stung) { m_hasStung = stung; }

    // ========== 蜂巢系统 ==========

    /**
     * @brief 设置蜂巢位置
     * @param pos 蜂巢位置
     */
    void setHivePos(const BlockPos& pos);

    /**
     * @brief 获取蜂巢位置
     */
    [[nodiscard]] const BlockPos& getHivePos() const { return m_hivePos; }

    /**
     * @brief 是否有蜂巢
     */
    [[nodiscard]] bool hasHive() const { return m_hasHive; }

    /**
     * @brief 是否正在返回蜂巢
     */
    [[nodiscard]] bool isReturningToHive() const { return m_returningToHive; }

    /**
     * @brief 设置返回蜂巢状态
     */
    void setReturningToHive(bool returning) { m_returningToHive = returning; }

    // ========== 愤怒系统 ==========

    /**
     * @brief 是否愤怒
     */
    [[nodiscard]] bool isAngry() const { return m_angerTime > 0; }

    /**
     * @brief 获取愤怒时间
     */
    [[nodiscard]] i32 getAngerTime() const { return m_angerTime; }

    /**
     * @brief 设置愤怒时间
     */
    void setAngerTime(i32 time) { m_angerTime = time; }

    /**
     * @brief 设置攻击目标
     */
    void setAttackTarget(LivingEntity* target) { m_attackTarget = target; }

    /**
     * @brief 获取攻击目标
     */
    [[nodiscard]] LivingEntity* getAttackTarget() const { return m_attackTarget; }

    /**
     * @brief 是否正在攻击
     */
    [[nodiscard]] bool isAttacking() const { return m_attacking; }

    /**
     * @brief 设置攻击状态
     */
    void setAttacking(bool attacking) { m_attacking = attacking; }

    // ========== 飞行系统 ==========

    /**
     * @brief 获取飞行目标位置
     */
    [[nodiscard]] BlockPos getFlowerPos() const { return m_flowerPos; }

    /**
     * @brief 设置花朵位置
     */
    void setFlowerPos(const BlockPos& pos);

    /**
     * @brief 是否有花朵目标
     */
    [[nodiscard]] bool hasFlower() const { return m_hasFlower; }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 蜜蜂使用花朵繁殖
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
    [[nodiscard]] f32 eyeHeight() const override { return 0.3f; }

    // ========== 生命周期 ==========

    void tick() override;

    // ========== 标记 ==========

    /**
     * @brief 设置攻击目标标记
     * 用于通知其他蜜蜂一起攻击
     */
    void setTargetPlayer(u64 playerId) { m_targetPlayerId = playerId; }

    /**
     * @brief 获取攻击目标玩家ID
     */
    [[nodiscard]] u64 getTargetPlayer() const { return m_targetPlayerId; }

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 花粉状态
    bool m_hasNectar = false;
    bool m_hasStung = false;

    // 蜂巢系统
    BlockPos m_hivePos;
    bool m_hasHive = false;
    bool m_returningToHive = false;

    // 花朵位置
    BlockPos m_flowerPos;
    bool m_hasFlower = false;

    // 愤怒系统
    LivingEntity* m_attackTarget = nullptr;
    i32 m_angerTime = 0;
    bool m_attacking = false;
    u64 m_targetPlayerId = 0;

    // 计时器
    i32 m_underWaterTimer = 0;

    // 常量
    static constexpr i32 MAX_ANGER_TIME = 1200; // 60秒
    static constexpr i32 STING_DAMAGE = 2; // 螫刺伤害
};

} // namespace mc
