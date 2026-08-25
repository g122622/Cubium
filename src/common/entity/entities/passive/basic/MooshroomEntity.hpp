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
#include "../../../../item/core/ActionResult.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../interfaces/IShearable.hpp"
#include "CowEntity.hpp"
#include "common/core/Result.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

namespace entity::effect {
class EffectInstance;
}

namespace blocks {
class FlowerBlock;
}

/**
 * @brief 哞菇实体
 *
 * 长着蘑菇的牛，只生成在蘑菇岛生物群系。
 *
 * 特性：
 * - 两种皮肤：红色哞菇、棕色哞菇
 * - 蘑菇繁殖：被雷击后红色哞菇变为棕色
 * - 剪毛：使用剪刀获得蘑菇并变成普通牛
 * - 碗交互：使用空碗获得蘑菇汤（或迷之炖菜）
 * - 棕色哞菇：喂食花朵后可产出迷之炖菜
 * - 繁殖：与普通牛相同
 */
class MooshroomEntity : public CowEntity, public entity::IShearable {
public:
    /**
     * @brief 哞菇类型
     */
    enum class MooshroomType : u8 {
        Red = 0,  // 红色哞菇
        Brown = 1 // 棕色哞菇
    };

    /**
     * @brief 构造函数
     * @param id 实体ID
     * @param registry 实体注册表
     */
    MooshroomEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~MooshroomEntity() override = default;

    // 禁止拷贝
    MooshroomEntity(const MooshroomEntity&) = delete;
    MooshroomEntity& operator=(const MooshroomEntity&) = delete;

    // 允许移动
    MooshroomEntity(MooshroomEntity&&) = delete;
    MooshroomEntity& operator=(MooshroomEntity&&) = delete;

    /**
     * @brief 创建哞菇实体
     * @param world 世界实例
     * @return 新的哞菇实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 类型 ==========

    /**
     * @brief 获取哞菇类型
     */
    [[nodiscard]] MooshroomType getMooshroomType() const { return m_mooshroomType; }

    /**
     * @brief 设置哞菇类型
     */
    void setMooshroomType(MooshroomType type) { m_mooshroomType = type; }

    /**
     * @brief 是否是红色哞菇
     */
    [[nodiscard]] bool isRed() const { return m_mooshroomType == MooshroomType::Red; }

    /**
     * @brief 是否是棕色哞菇
     */
    [[nodiscard]] bool isBrown() const { return m_mooshroomType == MooshroomType::Brown; }

    // ========== IShearable接口实现 ==========

    /**
     * @brief 检查是否可以被剪毛 (IShearable接口实现)
     * @return 如果有蘑菇返回true
     */
    [[nodiscard]] bool isShearable() const override { return true; }

    /**
     * @brief 剪毛 (IShearable接口实现)
     * @param player 执行剪毛的玩家
     * @return 获得的蘑菇物品
     */
    std::vector<ItemStack> shear(Player* player = nullptr) override;

    // ========== 玩家交互 ==========

    /**
     * @brief 实体交互
     *
     * 处理以下交互：
     * 1. 空碗 → 蘑菇汤/迷之炖菜
     * 2. 棕色哞菇 + 花朵 → 存储迷之炖菜效果
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    // ========== 迷之炖菜效果 ==========

    /**
     * @brief 是否存储了迷之炖菜效果
     */
    [[nodiscard]] bool hasStewEffect() const { return m_stewEffectType.has_value(); }

    /**
     * @brief 获取存储的迷之炖菜效果类型
     * @return 效果类型，如果没有则返回 std::nullopt
     */
    [[nodiscard]] std::optional<entity::effect::EffectType> getStewEffectType() const { return m_stewEffectType; }

    /**
     * @brief 获取存储的迷之炖菜效果持续时间（秒）
     */
    [[nodiscard]] i32 getStewEffectDuration() const { return m_stewEffectDuration; }

    /**
     * @brief 设置迷之炖菜效果（棕色哞菇被喂食花朵时调用）
     * @param type 效果类型
     * @param duration 持续时间（秒）
     */
    void setStewEffect(entity::effect::EffectType type, i32 duration);

    /**
     * @brief 清除迷之炖菜效果（被取汤后调用）
     */
    void clearStewEffect();

    // ========== 繁殖 ==========

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 寻路权重 ==========

    /**
     * @brief 获取路径权重
     *
     * 哞菇偏好菌丝：站在菌丝上返回10.0f，否则返回亮度相关值。
     */
    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

    // ========== 雷击 ==========

    /**
     * @brief 被雷击时触发
     *
     * 对齐 vanilla MushroomCow#thunderHit（MushroomCow.java:76-83）：不调 super.thunderHit
     * （不受伤、不引燃），仅红↔棕变色 + 播放转换音效。wiki tech_闪电束.txt#转化生物：哞菇被闪电
     * 击中仅变色不受伤害。 Cubium 基类 Entity::onStruckByLightning 默认 hurt(5)+引燃，此处不调基类
     * 以避免哞菇被闪电误伤（修复前 _damageEntities 外层无条件 hurt 致哞菇 HP 10→5 的偏差）。
     */
    void onStruckByLightning(entity::LightningBoltEntity* lightning) override;

    // ========== NBT序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    /**
     * @brief 获取环境音效
     *
     * 哞菇复用牛的环境音。
     * sounds.json 中无 entity.mooshroom.ambient，仅有 convert/eat/milk/shear，
     * 故不能走默认 makeSoundEventId("ambient")（会拼接出 mooshroom.ambient）。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

private:
    // 哞菇类型
    MooshroomType m_mooshroomType = MooshroomType::Red;

    // 迷之炖菜效果（棕色哞菇用）
    // 当棕色哞菇被喂食花朵时，存储花朵对应的效果；
    // 当玩家用空碗右键时，效果转移到迷之炖菜中并清除此存储。
    std::optional<entity::effect::EffectType> m_stewEffectType;
    i32 m_stewEffectDuration = 0; // 持续时间（秒），瞬间效果也为原始秒值

    /**
     * @brief 从物品获取花朵的迷之炖菜效果
     *
     * 检查物品是否为花朵方块物品，如果是则返回其迷之炖菜效果。
     *
     * @param itemStack 物品堆
     * @return 如果物品是花朵且有效果，返回 pair<EffectType, durationSeconds>；否则返回 nullopt
     */
    [[nodiscard]] static std::optional<std::pair<entity::effect::EffectType, i32>> _getStewEffectFromItem(
        const ItemStack& itemStack);
};

} // namespace mc
