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

#include "AbstractGroupFishEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <optional>

namespace mc {

namespace resource {
class ResourceLocation;
}

/**
 * @brief 鲑鱼实体
 *
 * 鲑鱼属于群游鱼类，最大群体大小固定为 5。
 * 拥有三种体型（small/medium/large），影响碰撞箱缩放，生成时按权重随机决定。
 */
class SalmonEntity : public AbstractGroupFishEntity {
public:
    /**
     * @brief 鲑鱼体型
     *
     * 对齐 vanilla 1.21.11 Salmon.Variant：id 用于同步数据，boundingBoxScale 用于
     * 碰撞箱缩放（small 0.5 / medium 1.0 / large 1.5），DEFAULT = Medium。
     */
    enum class Variant : i32 {
        Small = 0,  ///< 小型（缩放 0.5）
        Medium = 1, ///< 中型（缩放 1.0，默认）
        Large = 2   ///< 大型（缩放 1.5）
    };

    /// 由 id 取体型（越界钳制到 Large，对齐 vanilla ByIdMap CLAMP）
    [[nodiscard]] static Variant variantById(i32 id) noexcept
    {
        if (id <= 0) {
            return Variant::Small;
        }
        if (id >= 2) {
            return Variant::Large;
        }
        return Variant::Medium;
    }

    /// 体型的碰撞箱缩放系数
    [[nodiscard]] static f32 variantScale(Variant variant) noexcept
    {
        switch (variant) {
            case Variant::Small:
                return 0.5f;
            case Variant::Large:
                return 1.5f;
            case Variant::Medium:
            default:
                return 1.0f;
        }
    }

    /**
     * @brief 构造鲑鱼实体
     * @param id 实体 ID
     * @param registry 实体注册表（ECS）
     */
    SalmonEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~SalmonEntity() override = default;

    SalmonEntity(const SalmonEntity&) = delete;
    SalmonEntity& operator=(const SalmonEntity&) = delete;
    SalmonEntity(SalmonEntity&&) noexcept = delete;
    SalmonEntity& operator=(SalmonEntity&&) noexcept = delete;

    /// 本类继承链标识（parent = AbstractGroupFishEntity::classInfo()）。见 Entity::classInfo()。
    // vanilla 1.21.11 Salmon 自带 DATA_TYPE@17(Int，体型 small/medium/large=0/1/2)，
    // 见 registerData。项目体型业务联动暂未实现，占位 id17 对齐 vanilla 字段表上限。
    static const entity::EntityClassInfo& classInfo();

    /**
     * @brief 创建鲑鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 获取装取该鲑鱼后得到的鱼桶
     * @return salmon_bucket（对齐 Java Salmon.getBucketItemStack = SALMON_BUCKET）
     *
     * override AbstractFishEntity::getBucketItemStack，使 bucketMobPickup 装入对应鱼桶。
     */
    [[nodiscard]] ItemStack getBucketItemStack() const override;

    // ========== 体型 ==========

    /// 获取当前体型
    [[nodiscard]] Variant getVariant() const { return variantById(m_dataManager.get<i32>(DATA_TYPE_PARAM)); }

    /// 设置体型（写入同步数据参数）
    void setVariant(Variant variant) { m_dataManager.set(DATA_TYPE_PARAM, static_cast<i32>(variant)); }

    /// 生成时随机体型所用的权重分母（对齐 vanilla WeightedList：small 30 / medium 50 / large 15）
    static constexpr i32 VARIANT_SMALL_WEIGHT = 30;
    static constexpr i32 VARIANT_MEDIUM_WEIGHT = 50;
    static constexpr i32 VARIANT_LARGE_WEIGHT = 15;
    static constexpr i32 VARIANT_TOTAL_WEIGHT =
        VARIANT_SMALL_WEIGHT + VARIANT_MEDIUM_WEIGHT + VARIANT_LARGE_WEIGHT;

    /**
     * @brief 碰撞箱宽度（按体型缩放）
     *
     * 对齐 vanilla Salmon.getDefaultDimensions = super.scale(getSalmonScale())：
     * 基础尺寸取 EntityType 登记值（0.7），再乘体型缩放系数。
     */
    [[nodiscard]] f32 width() const override;

    /// 碰撞箱高度（按体型缩放）
    [[nodiscard]] f32 height() const override;

    /// 生成时按权重随机决定体型（对齐 vanilla Salmon.finalizeSpawn）
    void finalizeSpawn(IWorld& world,
        const entity::combat::DifficultyInstance& difficulty,
        world::spawn::SpawnReason spawnReason) override;

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    /**
     * @brief vanilla 鲑鱼最大群体大小为 5
     */
    [[nodiscard]] i32 getMaxGroupSize() const override { return 5; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.1f; }

    /**
     * @brief 获取扑腾声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getFlopSound() const override;

    /**
     * @brief 获取环境声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

protected:
    void registerAttributes() override;

    // ========== 同步数据注册 ==========
    // 派生类构造函数须显式调用 registerData()（C++ 基类构造期虚函数不派发，参考 AbstractFishEntity）。
    void registerData() override;

private:
    // ========== 同步数据参数（vanilla 1.21.11 Salmon.DATA_TYPE，见 registerData） ==========
    // id17 DATA_TYPE（体型 small/medium/large=0/1/2），默认 1 = Medium。
    static entity::DataParameter<i32> DATA_TYPE_PARAM;
};

} // namespace mc
