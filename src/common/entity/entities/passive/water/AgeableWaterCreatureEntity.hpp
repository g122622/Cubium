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

#include "common/core/Types.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/entities/passive/water/WaterMobEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 可成长水生生物基类
 *
 * 对齐 vanilla 1.21.11 net.minecraft.world.entity.animal.AgeableWaterCreature：
 *   abstract class AgeableWaterCreature extends AgeableMob
 * Dolphin、Squid、GlowSquid 在 vanilla 均经此类获得水行为 + 年龄系统（幼体/成体）。
 *
 * 本项目类树对齐说明：vanilla 中 WaterAnimal（鱼类，不可成长）与 AgeableWaterCreature
 * （可成长）是 PathfinderMob 下的两个兄弟分支。本项目 WaterMobEntity 对应 WaterAnimal
 * 分支（提供水行为，AbstractFishEntity 经此），本类在其之上叠加年龄系统——受限于 C++
 * 单一继承（AgeableEntity 与 WaterMobEntity 均派生自 CreatureEntity，多重继承会构成菱形），
 * 无法直接复用 Entity 形态的 AgeableEntity，故本类自带与 AgeableEntity 等价的年龄状态与
 * 方法（常量/算法严格对齐 AgeableEntity 与 vanilla AgeableMob），并复用其静态常量与
 * getSpeedUpSecondsWhenFeeding 工具。
 *
 * 同步字段：vanilla AgeableMob.DATA_BABY(Boolean,id16)。本项目 WaterMobEntity 无自有字段，
 * 本类首字段即 id16，故 DATA_BABY 落 id16，子类字段续接 17、18…，与 vanilla 完全一致。
 */
class AgeableWaterCreatureEntity : public WaterMobEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    AgeableWaterCreatureEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~AgeableWaterCreatureEntity() override = default;

    /// 本类继承链标识（parent = WaterMobEntity::classInfo()）。见 Entity::classInfo()。
    // vanilla 1.21.11 AgeableWaterCreature 经 AgeableMob，本类在 id16 注册 DATA_BABY。
    static const entity::EntityClassInfo& classInfo();

    // 禁止拷贝
    AgeableWaterCreatureEntity(const AgeableWaterCreatureEntity&) = delete;
    AgeableWaterCreatureEntity& operator=(const AgeableWaterCreatureEntity&) = delete;

    // 允许移动
    AgeableWaterCreatureEntity(AgeableWaterCreatureEntity&&) = delete;
    AgeableWaterCreatureEntity& operator=(AgeableWaterCreatureEntity&&) = delete;

    // ========== 年龄系统（对齐 AgeableEntity / vanilla AgeableMob） ==========

    /**
     * @brief 获取年龄（负数=幼体，0或正数=成体）
     */
    [[nodiscard]] i32 getGrowingAge() const { return m_growingAge; }

    /**
     * @brief 设置年龄
     */
    void setGrowingAge(i32 age);

    /**
     * @brief 是否为幼体
     */
    [[nodiscard]] bool isChild() const override { return m_growingAge < 0; }

    /// 同步字段 id 访问器（测试/诊断用，抗字段 id 偏移）
    [[nodiscard]] static u16 getBabyParamId() { return DATA_BABY_PARAM.id(); }

    /**
     * @brief 设置为幼体/成体
     */
    void setChild(bool child);

    /**
     * @brief 成长（增加年龄）
     * @param seconds 成长的秒数
     */
    void ageUp(i32 seconds);

    /**
     * @brief 添加年龄（可用于加速成长）
     */
    void addGrowingAge(i32 amount);

    // ========== 生命周期 ==========

    void tick() override;

    // ========== NBT 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

protected:
    // ========== 同步数据参数注册 ==========
    void registerData() override;

    /**
     * @brief 年龄更新（每 tick 调用）
     */
    void updateAge();

    /**
     * @brief 幼体变成成体时调用
     */
    virtual void onGrowUp() {}

    // ========== 常量（对齐 AgeableEntity::BABY_AGE / MAX_AGE） ==========

    static constexpr i32 BABY_AGE = AgeableEntity::BABY_AGE; // 幼体起始年龄（-24000，20分钟）
    static constexpr i32 MAX_AGE = AgeableEntity::MAX_AGE;   // 成体年龄（0）

private:
    i32 m_growingAge = 0; // 年龄（负数=幼体）

    // ========== 同步数据参数（vanilla 1.21.11 AgeableMob.DATA_BABY，见 registerData） ==========
    static entity::DataParameter<bool> DATA_BABY_PARAM; // id16，幼体状态同步镜像
};

} // namespace mc