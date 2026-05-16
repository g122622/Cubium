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
#include "../MonsterEntity.hpp"
#include <memory>

namespace mc {

// 前向声明
class BreakDoorGoal;

/**
 * @brief 僵尸实体
 *
 * 最常见的亡灵怪物。
 *
 * 特性：
 * - 攻击：近战攻击玩家和村民
 * - 燃烧：在阳光下燃烧
 * - 增援：被攻击时有概率召唤增援
 * - 感染：杀死村民会将其转化为僵尸村民
 * - 变种：可转化为溺尸
 * - 破门：可以破坏木门
 *
 * 参考 MC 1.16.5 ZombieEntity
 */
class ZombieEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    ZombieEntity(LegacyEntityType type, EntityId id);
    ~ZombieEntity() override = default;

    // 禁止拷贝
    ZombieEntity(const ZombieEntity&) = delete;
    ZombieEntity& operator=(const ZombieEntity&) = delete;

    // 允许移动
    ZombieEntity(ZombieEntity&&) = default;
    ZombieEntity& operator=(ZombieEntity&&) = default;

    /**
     * @brief 创建僵尸实体
     * @param world 世界实例
     * @return 新的僵尸实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     * 参考 MC 1.16.5 ZombieEntity.getAmbientSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     * 参考 MC 1.16.5 ZombieEntity.getHurtSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     * 参考 MC 1.16.5 ZombieEntity.getDeathSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取脚步声音效
     * 参考 MC 1.16.5 ZombieEntity.getStepSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getStepSound() const;

    // ========== 脚步声 ==========

    /**
     * @brief 播放脚步声
     * MC 1.16.5: ZombieEntity.playStepSound() 使用固定的脚步声
     */
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;

    // ========== 破门能力 ==========

    /**
     * @brief 是否可以破门
     * 参考 MC 1.16.5 ZombieEntity.canBreakDoors()
     */
    [[nodiscard]] bool canBreakDoors() const { return m_canBreakDoors; }

    /**
     * @brief 设置破门能力
     * 参考 MC 1.16.5 ZombieEntity.setBreakDoorsAItask()
     */
    void setBreakDoorsAbility(bool canBreak);

    // ========== 溺水转化 ==========

    /**
     * @brief 是否正在转化为溺尸
     */
    [[nodiscard]] bool isConverting() const { return m_converting; }

    /**
     * @brief 获取转化时间
     */
    [[nodiscard]] i32 getConversionTime() const { return m_conversionTime; }

    /**
     * @brief 开始溺水转化
     * 参考 MC 1.16.5 ZombieEntity.startDrowning()
     */
    void startDrowning(i32 conversionTime);

    /**
     * @brief 是否应该溺水转化
     * 参考 MC 1.16.5 ZombieEntity.shouldDrown()
     */
    [[nodiscard]] virtual bool shouldDrown() const { return true; }

    /**
     * @brief 转化为溺尸
     *
     * 参考 MC 1.16.5 ZombieEntity.onDrowned()
     *
     * 将当前僵尸转化为溺尸：
     * 1. 创建新的 DrownedEntity
     * 2. 复制位置、旋转、生命值、装备、婴儿状态、自定义名称、持久化状态
     * 3. 清空原僵尸装备（防止死亡掉落）
     * 4. 播放转化音效和事件
     * 5. 移除原僵尸
     */
    virtual void convertToDrowned();

    // ========== 婴儿状态 ==========

    /**
     * @brief 是否是婴儿僵尸
     */
    [[nodiscard]] bool isBaby() const { return m_isBaby; }

    /**
     * @brief 设置婴儿状态
     */
    void setBaby(bool baby);

    // ========== 增援系统 ==========

    /**
     * @brief 是否可以召唤增援
     */
    [[nodiscard]] bool canSummonReinforcements() const;

    /**
     * @brief 尝试召唤增援
     * 参考 MC 1.16.5 ZombieEntity.attackEntityFrom()
     */
    void trySummonReinforcements();

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return m_isBaby ? 0.93f : 1.74f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return m_isBaby ? 0.3f : 0.6f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return m_isBaby ? 0.975f : 1.95f; }

    // ========== 伤害处理 ==========

    /**
     * @brief 受到伤害时的处理（包含增援逻辑）
     * 参考 MC 1.16.5 ZombieEntity.attackEntityFrom()
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 作为生物攻击实体
     *
     * 重写以实现燃烧传递逻辑。
     * MC 1.16.5: 燃烧的僵尸攻击时，有概率点燃目标。
     *
     * @param target 目标生物
     * @return 是否攻击成功
     */
    bool attackEntityAsMob(LivingEntity& target) override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 破门能力
    bool m_canBreakDoors = false;
    BreakDoorGoal* m_breakDoorGoal = nullptr;

    // 溺水转化
    bool m_converting = false;
    i32 m_conversionTime = 0;
    i32 m_inWaterTime = 0; // 在水中的时间

    // 婴儿状态
    bool m_isBaby = false;

    // 常量
    static constexpr i32 CONVERSION_DURATION = 300;     // 15秒转化时间 (300 ticks)
    static constexpr i32 IN_WATER_TIME_THRESHOLD = 600; // 水下30秒开始转化
    static constexpr f32 BABY_SPEED_BOOST = 0.5f;       // 婴儿速度加成 50%

    /**
     * @brief 更新溺水转化
     */
    void updateDrowning();
};

} // namespace mc
