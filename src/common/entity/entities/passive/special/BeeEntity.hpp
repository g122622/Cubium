/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

#pragma once

#include "../../../../core/Types.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../core/DataParameter.hpp"
#include "../../../interfaces/IAngerable.hpp"
#include "../../../interfaces/IFlyingAnimal.hpp"
#include "../basic/AnimalEntity.hpp"
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
class BeeEntity : public AnimalEntity, public entity::IFlyingAnimal, public entity::IAngerable {
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
     *
     * MC 1.16.5: return this.getBeeFlag(8)
     * 从 DataParameter 读取，支持网络同步
     */
    [[nodiscard]] bool hasNectar() const;

    /**
     * @brief 设置花粉状态
     *
     * MC 1.16.5: setBeeFlag(8, nectar)
     * 通过 DataParameter 同步到客户端
     */
    void setHasNectar(bool nectar);

    /**
     * @brief 是否有螫刺
     * 蜜蜂失去螫刺后无法攻击
     *
     * MC 1.16.5: return this.getBeeFlag(4)
     * 从 DataParameter 读取，支持网络同步
     */
    [[nodiscard]] bool hasStung() const;

    /**
     * @brief 设置螫刺状态
     *
     * MC 1.16.5: setBeeFlag(4, stung)
     * 通过 DataParameter 同步到客户端
     */
    void setHasStung(bool stung);

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

    // ========== 飞行系统 (IFlyingAnimal接口) ==========

    /**
     * @brief 是否正在飞行 (IFlyingAnimal接口实现)
     */
    [[nodiscard]] bool isFlying() const override { return m_flying; }

    /**
     * @brief 设置飞行状态 (IFlyingAnimal接口实现)
     */
    void setFlying(bool flying) override { m_flying = flying; }

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

    // ========== 数据同步 ==========

    /**
     * @brief 注册同步数据参数
     *
     * MC 1.16.5: BeeEntity.registerData()
     * 注册 DATA_FLAGS 和 ANGER_TIME 数据参数
     */
    void registerData() override;

    // ========== IAngerable 接口实现 ==========

    /**
     * @brief 设置攻击目标 (IAngerable)
     */
    void setAttackTarget(LivingEntity* target) override { m_attackTarget = target; }

    /**
     * @brief 获取攻击目标 (IAngerable)
     */
    [[nodiscard]] LivingEntity* getAttackTarget() const override { return m_attackTarget; }

    /**
     * @brief 设置复仇目标 (IAngerable)
     *
     * MC 1.16.5: 设置愤怒目标和愤怒时间
     */
    void setRevengeTarget(LivingEntity* target) override;

    /**
     * @brief 获取复仇目标 (IAngerable)
     */
    [[nodiscard]] LivingEntity* getRevengeTarget() const override;

    /**
     * @brief 获取复仇计时器 (IAngerable)
     */
    [[nodiscard]] i32 getRevengeTimer() const override { return m_revengeTimer; }

    /**
     * @brief 是否愤怒 (IAngerable)
     *
     * MC 1.16.5: return this.getAngerTime() > 0
     */
    [[nodiscard]] bool isAngry() const override { return getAngerTime() > 0; }

    /**
     * @brief 设置愤怒状态 (IAngerable)
     *
     * MC 1.16.5: 设置愤怒时间为默认值
     */
    void setAngry(bool angry) override;

    /**
     * @brief 获取愤怒时间 (IAngerable)
     *
     * MC 1.16.5: return this.dataManager.get(ANGER_TIME)
     */
    [[nodiscard]] i32 getAngerTime() const override;

    /**
     * @brief 设置愤怒时间 (IAngerable)
     *
     * MC 1.16.5: this.dataManager.set(ANGER_TIME, time)
     */
    void setAngerTime(i32 time) override;

    /**
     * @brief 更新愤怒计时器 (IAngerable)
     *
     * 每tick调用，递减愤怒时间
     */
    void updateAnger() override;

    /**
     * @brief 是否正在攻击
     */
    [[nodiscard]] bool isAttacking() const { return m_attacking; }

    /**
     * @brief 设置攻击状态
     */
    void setAttacking(bool attacking) { m_attacking = attacking; }

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

    /**
     * @brief 获取水下计时器
     * MC 1.16.5: 用于追踪蜜蜂在水中的时间
     */
    [[nodiscard]] i32 getUnderWaterTimer() const { return m_underWaterTimer; }

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // ========== MC 1.16.5 数据参数 ==========
    // DATA_FLAGS (i8): 位标志
    //   - bit 2 (0x02): NEAR_TARGET - 接近攻击目标
    //   - bit 3 (0x04): HAS_STUNG - 已螫刺
    //   - bit 4 (0x08): HAS_NECTAR - 携带花蜜
    static entity::DataParameter<i8> DATA_FLAGS_PARAM;
    // ANGER_TIME (i32): 愤怒时间（ticks）
    static entity::DataParameter<i32> ANGER_TIME_PARAM;

    // 数据参数标志位
    static constexpr i8 FLAG_NEAR_TARGET = 2; // bit 1: 接近攻击目标
    static constexpr i8 FLAG_HAS_STUNG = 4;   // bit 2: 已螫刺
    static constexpr i8 FLAG_HAS_NECTAR = 8;  // bit 3: 携带花蜜

    // ========== 花粉状态 ==========
    bool m_hasNectar = false;
    bool m_hasStung = false;

    // ========== 蜂巢系统 ==========
    BlockPos m_hivePos;
    bool m_hasHive = false;
    bool m_returningToHive = false;

    // ========== 花朵位置 ==========
    BlockPos m_flowerPos;
    bool m_hasFlower = false;

    // ========== 愤怒系统 ==========
    LivingEntity* m_attackTarget = nullptr;
    std::optional<u64> m_revengeTargetId;
    i32 m_revengeTimer = 0;
    i32 m_angerTime = 0; // 本地缓存，从 DataParameter 同步
    bool m_attacking = false;
    u64 m_targetPlayerId = 0;

    // ========== 飞行状态 ==========
    bool m_flying = false;

    // ========== 计时器 ==========
    i32 m_underWaterTimer = 0;
    i32 m_timeSinceSting = 0;  ///< 蛰刺后经过的 tick 数

    // ========== 常量 ==========
    static constexpr i32 MAX_ANGER_TIME = 1200; // 60秒
    static constexpr i32 STING_DAMAGE = 2;      // 螫刺伤害

    // ========== 私有辅助方法 ==========

    /**
     * @brief 获取数据参数标志位
     * @param flag 标志位掩码
     * @return 标志位是否设置
     */
    [[nodiscard]] bool getBeeFlag(i8 flag) const;

    /**
     * @brief 设置数据参数标志位
     * @param flag 标志位掩码
     * @param value 是否设置
     */
    void setBeeFlag(i8 flag, bool value);
};

} // namespace mc
