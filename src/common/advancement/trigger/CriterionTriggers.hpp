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

#include "CriterionTrigger.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::advancement {

/**
 * @brief 触发器注册表
 *
 * 管理所有触发器类型的实例，提供ID到触发器的映射。
 *
 * 使用示例：
 * @code
 * // 初始化
 * CriterionTriggers::instance().registerBuiltinTriggers();
 *
 * // 获取触发器
 * auto* trigger = CriterionTriggers::instance().getTrigger<InventoryChangedTrigger>();
 *
 * // 触发
 * trigger->trigger(player, inventory, stack);
 * @endcode
 */
class CriterionTriggers {
public:
    /**
     * @brief 获取单例实例
     */
    static CriterionTriggers& instance() noexcept;

    // 禁止拷贝
    CriterionTriggers(const CriterionTriggers&) = delete;
    CriterionTriggers& operator=(const CriterionTriggers&) = delete;

    // ========== 触发器管理 ==========

    /**
     * @brief 注册触发器
     * @param trigger 触发器实例
     */
    void registerTrigger(std::unique_ptr<ICriterionTriggerBase> trigger);

    /**
     * @brief 获取触发器
     * @param id 触发器ID
     * @return 触发器，如果不存在返回nullptr
     */
    [[nodiscard]] ICriterionTriggerBase* getTrigger(const ResourceLocation& id);

    /**
     * @brief 获取触发器（模板版本）
     */
    template <typename T>
    [[nodiscard]] T* getTrigger()
    {
        return static_cast<T*>(getTrigger(ResourceLocation(T::TRIGGER_ID)));
    }

    /**
     * @brief 检查触发器是否存在
     */
    [[nodiscard]] bool hasTrigger(const ResourceLocation& id) const noexcept;

    /**
     * @brief 获取所有触发器ID
     */
    [[nodiscard]] std::vector<ResourceLocation> getAllTriggerIds() const noexcept;

    /**
     * @brief 清空所有触发器
     */
    void clear() noexcept;

    // ========== 内置触发器注册 ==========

    /**
     * @brief 注册所有内置触发器
     */
    void registerBuiltinTriggers();

private:
    CriterionTriggers() = default;

    std::unordered_map<ResourceLocation, std::unique_ptr<ICriterionTriggerBase>> m_triggers;
};

// ========== 触发器类型ID常量 ==========

namespace triggers {

// 物品栏相关
constexpr const char* INVENTORY_CHANGED = "minecraft:inventory_changed";
constexpr const char* CONSUME_ITEM = "minecraft:consume_item";
constexpr const char* ITEM_DURABILITY_CHANGED = "minecraft:item_durability_changed";
constexpr const char* ENCHANTED_ITEM = "minecraft:enchanted_item";
constexpr const char* FILLED_BUCKET = "minecraft:filled_bucket";

// 实体相关
constexpr const char* PLAYER_KILLED_ENTITY = "minecraft:player_killed_entity";
constexpr const char* ENTITY_KILLED_PLAYER = "minecraft:entity_killed_player";
constexpr const char* PLAYER_HURT_ENTITY = "minecraft:player_hurt_entity";
constexpr const char* ENTITY_HURT_PLAYER = "minecraft:entity_hurt_player";
constexpr const char* SUMMONED_ENTITY = "minecraft:summoned_entity";
constexpr const char* TAME_ANIMAL = "minecraft:tame_animal";
constexpr const char* BRED_ANIMALS = "minecraft:bred_animals";
constexpr const char* CURED_ZOMBIE_VILLAGER = "minecraft:cured_zombie_villager";
constexpr const char* VILLAGER_TRADE = "minecraft:villager_trade";
constexpr const char* FISHING_ROD_HOOKED = "minecraft:fishing_rod_hooked";
constexpr const char* CHANNELED_LIGHTNING = "minecraft:channeled_lightning";
constexpr const char* PLAYER_INTERACTED_WITH_ENTITY = "minecraft:player_interacted_with_entity";

// 位置相关
constexpr const char* LOCATION = "minecraft:location";
constexpr const char* SLEPT_IN_BED = "minecraft:slept_in_bed";
constexpr const char* HERO_OF_THE_VILLAGE = "minecraft:hero_of_the_village";
constexpr const char* VOLUNTARY_EXILE = "minecraft:voluntary_exile";
constexpr const char* NETHER_TRAVEL = "minecraft:nether_travel";
constexpr const char* CHANGED_DIMENSION = "minecraft:changed_dimension";
constexpr const char* LEVITATION = "minecraft:levitation";

// 方块相关
constexpr const char* ENTER_BLOCK = "minecraft:enter_block";
constexpr const char* PLACED_BLOCK = "minecraft:placed_block";
constexpr const char* SLIDE_DOWN_BLOCK = "minecraft:slide_down_block";
constexpr const char* BEE_NEST_DESTROYED = "minecraft:bee_nest_destroyed";
constexpr const char* TARGET_HIT = "minecraft:target_hit";
constexpr const char* ITEM_USED_ON_BLOCK = "minecraft:item_used_on_block";

// 效果相关
constexpr const char* EFFECTS_CHANGED = "minecraft:effects_changed";
constexpr const char* BREWED_POTION = "minecraft:brewed_potion";

// 振动相关
constexpr const char* AVOID_VIBRATION = "minecraft:avoid_vibration";

// 其他
constexpr const char* IMPOSSIBLE = "minecraft:impossible";
constexpr const char* TICK = "minecraft:tick";
constexpr const char* RECIPE_UNLOCKED = "minecraft:recipe_unlocked";
constexpr const char* CONSTRUCT_BEACON = "minecraft:construct_beacon";
constexpr const char* USED_ENDER_EYE = "minecraft:used_ender_eye";
constexpr const char* USED_TOTEM = "minecraft:used_totem";
constexpr const char* SHOT_CROSSBOW = "minecraft:shot_crossbow";
constexpr const char* KILLED_BY_CROSSBOW = "minecraft:killed_by_crossbow";
constexpr const char* PLAYER_GENERATES_CONTAINER_LOOT = "minecraft:player_generates_container_loot";
constexpr const char* THROWN_ITEM_PICKED_UP_BY_ENTITY = "minecraft:thrown_item_picked_up_by_entity";

} // namespace triggers

} // namespace mc::advancement
