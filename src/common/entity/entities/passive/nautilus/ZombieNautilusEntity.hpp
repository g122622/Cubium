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

#include "AbstractNautilusEntity.hpp"
#include "ZombieNautilusVariant.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <memory>
#include <optional>

namespace mc {

// 前向声明
class AnimalEntity;
class DamageSource;
class ItemStack;
class Player;

/**
 * @brief 僵尸鹦鹉螺实体
 *
 * 对应 MC 1.21.11 net.minecraft.world.entity.animal.nautilus.ZombieNautilus。
 *
 * 与 NautilusEntity 的区别：
 * - 不可繁殖（spawnBaby 返回 nullptr）
 * - 永远不是幼体（isChild 恒为 false）
 * - 亡灵生物：阳光下燃烧（需装备鹦鹉螺铠甲防护）
 * - 移动速度 1.1（活体鹦鹉螺为 1.0）
 * - 音效仅有水下/陆地变体（无幼体变体）
 * - 根据生成生物群系选择气候变体（温带/寒冷/温暖）
 *
 * 变体选择规则（参考 MC 1.21.11 ZombieNautilusVariants + BiomeTags）：
 * - 温暖海洋/暖水深海 → Warm
 * - 冻洋/冻洋深海/冷水海洋/冷水深海 → Cold
 * - 其他海洋 → Temperate（默认）
 *
 * 阳光防护：sunProtectionSlot 返回 EquipmentSlot::Body，即鹦鹉螺铠甲槽可替代亡灵燃烧。
 */
class ZombieNautilusEntity : public AbstractNautilusEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    explicit ZombieNautilusEntity(EntityInstanceId id);

    ~ZombieNautilusEntity() override = default;

    // 禁止拷贝
    ZombieNautilusEntity(const ZombieNautilusEntity&) = delete;
    ZombieNautilusEntity& operator=(const ZombieNautilusEntity&) = delete;

    // 禁止移动
    ZombieNautilusEntity(ZombieNautilusEntity&&) = delete;
    ZombieNautilusEntity& operator=(ZombieNautilusEntity&&) = delete;

    /**
     * @brief 工厂方法
     * @param world 世界指针（未使用，实体 ID 由 EntityManager 分配）
     * @return 新创建的 ZombieNautilusEntity 实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 变体系统 ==========

    /**
     * @brief 获取气候变体
     */
    [[nodiscard]] ZombieNautilusVariant getVariant() const { return m_variant; }

    /**
     * @brief 设置气候变体
     */
    void setVariant(ZombieNautilusVariant variant) { m_variant = variant; }

    // ========== 繁殖系统 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     *
     * 僵尸鹦鹉螺不能繁殖，恒返回 false
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override
    {
        MC_UNUSED(itemStack);
        return false;
    }

    /**
     * @brief 生成幼体
     *
     * 僵尸鹦鹉螺不能繁殖，返回 nullptr
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override
    {
        MC_UNUSED(partner);
        return nullptr;
    }

    // ========== 物品判断 ==========

    /**
     * @brief 检查物品是否为驯服物品
     *
     * 僵尸鹦鹉螺不可驯服，恒返回 false
     */
    [[nodiscard]] bool isTamingItem(const ItemStack& itemStack) const override
    {
        MC_UNUSED(itemStack);
        return false;
    }

    // ========== 亡灵特性 ==========

    /**
     * @brief 阳光防护装备槽位
     *
     * 对应 MC 1.21.11 ZombieNautilus.sunProtectionSlot()：返回 EquipmentSlot::Body
     * 鹦鹉螺铠甲槽中的物品可替代亡灵在阳光下燃烧
     */
    [[nodiscard]] EquipmentSlot sunProtectionSlot() const override { return EquipmentSlot::Body; }

    // ========== 音效 ==========

    /**
     * @brief 环境音效
     *
     * 2 路分支：水下/陆地（无幼体变体）
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 受伤音效
     *
     * 2 路分支：水下/陆地
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 死亡音效
     *
     * 2 路分支：水下/陆地
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 冲刺音效
     *
     * 2 路分支：水下/陆地
     */
    [[nodiscard]] std::optional<ResourceLocation> getDashSound() const override;

    /**
     * @brief 冲刺就绪音效
     *
     * 2 路分支：水下/陆地
     */
    [[nodiscard]] std::optional<ResourceLocation> getDashReadySound() const override;

    /**
     * @brief 进食音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getEatSound() const override;

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 完成生成
     *
     * 对应 MC 1.21.11 ZombieNautilus.finalizeSpawn()：
     * 根据生成位置的生物群系选择气候变体
     */
    void finalizeSpawn(IWorld& world,
        const entity::combat::DifficultyInstance& difficulty,
        world::spawn::SpawnReason spawnReason) override;

    // ========== NBT 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

protected:
    /**
     * @brief 注册同步数据参数
     *
     * 注册变体相关同步数据参数
     */
    void registerData() override;

    /**
     * @brief 注册属性
     *
     * 僵尸鹦鹉螺属性：
     * - MAX_HEALTH: 15.0（继承自父类）
     * - MOVEMENT_SPEED: 1.1（比活体鹦鹉螺快 0.1）
     */
    void registerAttributes() override;

protected:
    /// 本类继承链标识（parent = AbstractNautilusEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

private:
    // ========== 成员变量 ==========

    /// 气候变体（默认温带）
    ZombieNautilusVariant m_variant = ZombieNautilusVariant::Temperate;

    /**
     * @brief 根据当前位置的生物群系选择默认变体
     *
     * 对应 MC 1.21.11 VariantUtils.selectVariantToSpawn()：
     * - 暖水海洋/暖水深海 → Warm
     * - 冻洋/冻洋深海/冷水海洋/冷水深海 → Cold
     * - 其他 → Temperate
     */
    [[nodiscard]] ZombieNautilusVariant selectVariantForBiome() const;
};

} // namespace mc
