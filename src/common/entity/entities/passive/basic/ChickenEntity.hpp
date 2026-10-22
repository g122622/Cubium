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

#include <memory>
#include <optional>

#include "AnimalEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/IChunk.hpp"

namespace mc {

/**
 * @brief 鸡变种（对齐 vanilla ChickenVariants 注册表，registry id 即数据包加载顺序）
 *
 * - Temperate(0)：温带（默认），产蛋 minecraft:egg
 * - Warm(1)：温暖，产蛋 minecraft:brown_egg
 * - Cold(2)：寒冷（cold 模型），产蛋 minecraft:blue_egg
 */
enum class ChickenVariant : i32 {
    Temperate = 0, // 温带（默认）
    Warm = 1,      // 温暖
    Cold = 2,      // 寒冷
};

/**
 * @brief 鸡实体
 *
 * 会下蛋的被动动物，用种子繁殖。
 * 特性：不会受到摔落伤害，有翅膀拍打动画。
 */
class ChickenEntity : public AnimalEntity {
public:
    /// DATA_VARIANT_ID 的 wire serializerId（vanilla EntityDataSerializers.CHICKEN_VARIANT == 27）
    static constexpr i32 VANILLA_CHICKEN_VARIANT_SERIALIZER_ID = 27;
    /// 默认变种（vanilla ChickenVariants.DEFAULT == TEMPERATE）
    static constexpr ChickenVariant DEFAULT_VARIANT = ChickenVariant::Temperate;

    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

    ChickenEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~ChickenEntity() noexcept override = default;

    /**
     * @brief 实体工厂方法
     *
     * 用于 EntityRegistry 注册
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 下蛋 ==========

    /**
     * @brief 获取下蛋计时器
     * @return 到下次下蛋的时间（tick）
     */
    [[nodiscard]] i32 getEggTimer() const { return m_eggTimer; }

    /**
     * @brief 重置下蛋计时器
     */
    void resetEggTimer();

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 声音 ==========

    /**
     * @brief 获取声音音量
     */
    [[nodiscard]] f32 getSoundVolume() const override { return 1.0f; }

    /**
     * @brief 获取环境音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取脚步声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getStepSound() const;

    // ========== 生命周期 ==========

    void tick() override;

    /** @brief 保存产蛋倒计时与鸡骑士状态。 */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /** @brief 恢复产蛋倒计时与鸡骑士状态。 */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    // ========== 翅膀动画 ==========

    /**
     * @brief 获取翅膀旋转角度（用于渲染）
     */
    [[nodiscard]] f32 getWingRotation() const { return m_wingRotation; }

    /**
     * @brief 获取上一帧翅膀旋转角度
     */
    [[nodiscard]] f32 getPrevWingRotation() const { return m_prevWingRotation; }

    /**
     * @brief 是否是鸡骑士
     * 鸡骑士模式下不下蛋
     */
    [[nodiscard]] bool isChickenJockey() const { return m_chickenJockey; }

    /**
     * @brief 设置是否是鸡骑士
     */
    void setChickenJockey(bool jockey) { m_chickenJockey = jockey; }

    // ========== 变种 ==========

    /**
     * @brief 获取变种
     *
     * 对齐 vanilla Chicken.getVariant：值由 DATA_VARIANT_ID 同步数据承载
     * （wire serializerId 27 = CHICKEN_VARIANT，值为注册表 id）。
     */
    [[nodiscard]] ChickenVariant getVariant() const
    {
        return static_cast<ChickenVariant>(m_dataManager.get(DATA_VARIANT_ID_PARAM).registryId);
    }

    /**
     * @brief 设置变种
     */
    void setVariant(ChickenVariant variant);

    /**
     * @brief 获取 DATA_VARIANT_ID 的参数 id（测试/诊断用）
     */
    [[nodiscard]] static u16 getVariantParamId() { return DATA_VARIANT_ID_PARAM.id(); }

    /**
     * @brief 按群系为位置选择变种（对齐 vanilla VariantUtils.selectVariantToSpawn）
     *
     * 群系属于 #spawns_warm_variant_farm_animals → Warm；
     * 属于 #spawns_cold_variant_farm_animals → Cold；否则 Temperate。
     */
    [[nodiscard]] static ChickenVariant selectVariantForPosition(const IWorld* world, const BlockPos& pos);

    /**
     * @brief 生成初始化：按生成位置的农畜变种群系标签选择变种
     *
     * 对齐 vanilla Chicken.finalizeSpawn 的 VariantUtils.selectVariantToSpawn 分支。
     */
    void finalizeSpawn(IWorld& world,
        const entity::combat::DifficultyInstance& difficulty,
        world::spawn::SpawnReason spawnReason) override;

    /**
     * @brief 该变种对应的产蛋物品（对齐数据包 gameplay/chicken_lay 战利品表）
     */
    [[nodiscard]] static const Item* eggItemForVariant(ChickenVariant variant);

    /**
     * @brief 按当前变种生成产蛋物品堆（数量 1）
     */
    [[nodiscard]] ItemStack getEggItemStack() const;

    /// 同步数据参数：DATA_VARIANT_ID（wire 27 = CHICKEN_VARIANT，对齐 vanilla Chicken.defineSynchedData）
    static entity::DataParameter<entity::HolderVariantValue> DATA_VARIANT_ID_PARAM;

    // DataParameter 的 id 由 registerData 沿继承链分配；基类构造期 vtable 指向基类，
    // 故派生类构造函数须显式调用本方法（WolfEntity / FrogEntity 同范式）。
    void registerData() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    // ========== 脚步声 ==========

    /**
     * @brief 播放脚步声
     */
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.4f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.7f; }

private:
    i32 m_eggTimer = 0; // 下蛋计时器

    // 翅膀动画
    f32 m_wingRotation = 0.0f;
    f32 m_prevWingRotation = 0.0f;
    f32 m_wingRotDelta = 1.0f;

    // 鸡骑士标记
    bool m_chickenJockey = false;

    static constexpr i32 EGG_TIME_MIN = 6000;  // 最小下蛋时间（5分钟）
    static constexpr i32 EGG_TIME_MAX = 12000; // 最大下蛋时间（10分钟）
};

} // namespace mc
