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

#include "EntityType.hpp"
#include "common/core/Result.hpp"
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc {

namespace entity {

// 引入 mc 命名空间的类型

using mc::Error;
using mc::ErrorCode;
using mc::Result;

// 前向声明：EntityTypeIdNumber::reset() 由 clear() 调用以同步重置全局 ID 缓存，
// 保证"注册表空 ⇔ ID 缓存全 0"不变量。定义在 EntityTypeIdNumber.cpp，
// 此处仅声明避免循环包含（EntityTypeIdNumber.hpp 经由其它路径可能依赖本头）。
namespace EntityTypeIdNumber {
void reset();
} // namespace EntityTypeIdNumber

/**
 * @brief 实体类型注册表
 *
 * 管理所有实体类型的注册和查询。
 * 支持通过ID、名称或命名空间ID查询实体类型。
 *
 * 使用方式：
 * @code
 * // 注册实体类型
 * auto& registry = EntityRegistry::instance();
 * registry.registerType("minecraft:pig", pigBuilder.build());
 *
 * // 查询实体类型
 * const EntityType* pigType = registry.getType("minecraft:pig");
 * auto pig = pigType->create(world);
 * @endcode
 */
class EntityRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static EntityRegistry& instance()
    {
        static EntityRegistry registry;
        return registry;
    }

    /**
     * @brief 注册实体类型
     * @param resourceLocation 资源位置（如 minecraft:pig）
     * @param type 实体类型
     * @return 注册结果
     *
     * 如果资源位置已存在，返回错误。
     */
    Result<EntityTypeId> registerType(const std::string& resourceLocation, EntityType type)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 检查是否已存在
        if (m_nameToId.find(resourceLocation) != m_nameToId.end()) {
            return Error(ErrorCode::AlreadyExists, "Entity type already registered: " + resourceLocation);
        }

        // 分配ID
        EntityTypeId id = m_nextId++;

        // 设置ID和名称
        const_cast<EntityTypeId&>(type.m_id) = id;
        const_cast<std::string&>(type.m_name) = resourceLocation;

        // 存储
        m_types.push_back(std::move(type));
        m_nameToId[resourceLocation] = id;

        return id;
    }

    /**
     * @brief 通过ID获取实体类型
     * @param id 实体类型ID
     * @return 实体类型指针，不存在返回nullptr
     */
    [[nodiscard]] const EntityType* getType(EntityTypeId id) const
    {
        // ID 从 1 开始，vector 索引从 0 开始
        if (id == 0 || id > static_cast<EntityTypeId>(m_types.size())) {
            return nullptr;
        }
        return &m_types[static_cast<size_t>(id - 1)];
    }

    /**
     * @brief 通过名称获取实体类型
     * @param name 资源位置（如 minecraft:pig）
     * @return 实体类型指针，不存在返回nullptr
     */
    [[nodiscard]] const EntityType* getType(const std::string& name) const
    {
        auto it = m_nameToId.find(name);
        if (it == m_nameToId.end()) {
            return nullptr;
        }
        return getType(it->second);
    }

    /**
     * @brief 通过ID获取实体类型
     * @param id 实体类型ID
     * @return 实体类型引用
     * @throws std::out_of_range 如果ID无效
     */
    [[nodiscard]] const EntityType& getTypeOrThrow(EntityTypeId id) const
    {
        if (id == 0 || id > static_cast<EntityTypeId>(m_types.size())) {
            throw std::out_of_range("Invalid entity type ID: " + std::to_string(id));
        }
        return m_types[static_cast<size_t>(id - 1)];
    }

    /**
     * @brief 检查实体类型是否存在
     * @param name 资源位置
     * @return 是否存在
     */
    [[nodiscard]] bool hasType(const std::string& name) const { return m_nameToId.find(name) != m_nameToId.end(); }

    /**
     * @brief 获取所有已注册的实体类型
     */
    [[nodiscard]] const std::vector<EntityType>& getAllTypes() const { return m_types; }

    /**
     * @brief 获取已注册的实体类型数量
     */
    [[nodiscard]] size_t size() const { return m_types.size(); }

    /**
     * @brief 清空所有注册（仅用于测试）
     *
     * 同时调用 EntityTypeIdNumber::reset() 重置全局缓存的实体类型 ID，
     * 保证"注册表空 ⇔ ID 缓存全 0"不变量，避免 clear() 后 typeId()==0
     * 与 EntityTypeIdNumber::ITEM=旧值 比较失败的测试顺序污染。
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_types.clear();
        m_nameToId.clear();
        m_nextId = 1; // 从1开始，0保留
        EntityTypeIdNumber::reset();
    }

    // 禁止拷贝和移动
    EntityRegistry(const EntityRegistry&) = delete;
    EntityRegistry& operator=(const EntityRegistry&) = delete;
    EntityRegistry(EntityRegistry&&) = delete;
    EntityRegistry& operator=(EntityRegistry&&) = delete;

private:
    EntityRegistry()
        : m_nextId(1)
    {} // 0保留给无效ID

    std::vector<EntityType> m_types;
    std::unordered_map<std::string, EntityTypeId> m_nameToId;
    EntityTypeId m_nextId;
    mutable std::mutex m_mutex;
};

/**
 * @brief 辅助宏：注册实体类型
 *
 * 使用方式：
 * @code
 * REGISTER_ENTITY_TYPE("minecraft:pig", EntityType::Builder(PigEntity::create, EntityClassification::Creature)
 *     .size(0.9f, 0.9f)
 *     .trackingRange(10)
 *     .build());
 * @endcode
 */
#define REGISTER_ENTITY_TYPE(name, type) ::mc::entity::EntityRegistry::instance().registerType(name, type)

/**
 * @brief 内置实体类型常量
 *
 * 定义常用实体类型的资源位置
 */
namespace EntityTypes {
// 普通动物
constexpr const char* PIG = "minecraft:pig";
constexpr const char* COW = "minecraft:cow";
constexpr const char* SHEEP = "minecraft:sheep";
constexpr const char* CHICKEN = "minecraft:chicken";
constexpr const char* RABBIT = "minecraft:rabbit";
constexpr const char* MOOSHROOM = "minecraft:mooshroom";

// 可驯服动物
constexpr const char* WOLF = "minecraft:wolf";
constexpr const char* CAT = "minecraft:cat";
constexpr const char* OCELOT = "minecraft:ocelot";
constexpr const char* PARROT = "minecraft:parrot";

// 特殊动物
constexpr const char* FOX = "minecraft:fox";
constexpr const char* PANDA = "minecraft:panda";
constexpr const char* POLAR_BEAR = "minecraft:polar_bear";
constexpr const char* TURTLE = "minecraft:turtle";
constexpr const char* BEE = "minecraft:bee";
constexpr const char* STRIDER = "minecraft:strider";

// 马类
constexpr const char* HORSE = "minecraft:horse";
constexpr const char* DONKEY = "minecraft:donkey";
constexpr const char* MULE = "minecraft:mule";
constexpr const char* LLAMA = "minecraft:llama";
constexpr const char* TRADER_LLAMA = "minecraft:trader_llama";
constexpr const char* SKELETON_HORSE = "minecraft:skeleton_horse";
constexpr const char* ZOMBIE_HORSE = "minecraft:zombie_horse";

// 水生生物
constexpr const char* COD = "minecraft:cod";
constexpr const char* SALMON = "minecraft:salmon";
constexpr const char* PUFFERFISH = "minecraft:pufferfish";
constexpr const char* TROPICAL_FISH = "minecraft:tropical_fish";
constexpr const char* SQUID = "minecraft:squid";
constexpr const char* GLOW_SQUID = "minecraft:glow_squid";
constexpr const char* DOLPHIN = "minecraft:dolphin";
constexpr const char* AXOLOTL = "minecraft:axolotl";
constexpr const char* NAUTILUS = "minecraft:nautilus";
constexpr const char* ZOMBIE_NAUTILUS = "minecraft:zombie_nautilus";

// 环境生物
constexpr const char* BAT = "minecraft:bat";

// 傀儡
constexpr const char* IRON_GOLEM = "minecraft:iron_golem";
constexpr const char* SNOW_GOLEM = "minecraft:snow_golem";
constexpr const char* COPPER_GOLEM = "minecraft:copper_golem";

// 怪物
constexpr const char* ZOMBIE = "minecraft:zombie";
constexpr const char* SKELETON = "minecraft:skeleton";
constexpr const char* CREEPER = "minecraft:creeper";
constexpr const char* SPIDER = "minecraft:spider";
constexpr const char* ENDERMAN = "minecraft:enderman";
constexpr const char* BLAZE = "minecraft:blaze";
constexpr const char* WITCH = "minecraft:witch";
constexpr const char* SLIME = "minecraft:slime";
constexpr const char* GIANT = "minecraft:giant";
// 海洋怪物
constexpr const char* GUARDIAN = "minecraft:guardian";
constexpr const char* ELDER_GUARDIAN = "minecraft:elder_guardian";
// 亡灵变种
constexpr const char* HUSK = "minecraft:husk";
constexpr const char* DROWNED = "minecraft:drowned";
constexpr const char* STRAY = "minecraft:stray";
constexpr const char* BOGGED = "minecraft:bogged";
constexpr const char* WITHER_SKELETON = "minecraft:wither_skeleton";
constexpr const char* PHANTOM = "minecraft:phantom";
constexpr const char* ZOMBIE_VILLAGER = "minecraft:zombie_villager";
constexpr const char* ZOMBIFIED_PIGLIN = "minecraft:zombified_piglin";
// 节肢动物变种
constexpr const char* CAVE_SPIDER = "minecraft:cave_spider";
constexpr const char* SILVERFISH = "minecraft:silverfish";
constexpr const char* ENDERMITE = "minecraft:endermite";
// 末地生物
constexpr const char* SHULKER = "minecraft:shulker";
// 地狱生物
constexpr const char* GHAST = "minecraft:ghast";
constexpr const char* MAGMA_CUBE = "minecraft:magma_cube";
constexpr const char* PIGLIN = "minecraft:piglin";
constexpr const char* PIGLIN_BRUTE = "minecraft:piglin_brute";
constexpr const char* HOGLIN = "minecraft:hoglin";
constexpr const char* ZOGLIN = "minecraft:zoglin";
// 灾厄村民
constexpr const char* VINDICATOR = "minecraft:vindicator";
constexpr const char* EVOKER = "minecraft:evoker";
constexpr const char* ILLUSIONER = "minecraft:illusioner";
constexpr const char* PILLAGER = "minecraft:pillager";
constexpr const char* RAVAGER = "minecraft:ravager";
constexpr const char* VEX = "minecraft:vex";
// 试炼密室
constexpr const char* BREEZE = "minecraft:breeze";
// Boss
constexpr const char* ENDER_DRAGON = "minecraft:ender_dragon";
constexpr const char* WITHER = "minecraft:wither";
constexpr const char* WARDEN = "minecraft:warden";
// 村民
constexpr const char* VILLAGER = "minecraft:villager";
constexpr const char* WANDERING_TRADER = "minecraft:wandering_trader";

// 其他
constexpr const char* PLAYER = "minecraft:player";
constexpr const char* ITEM = "minecraft:item";
constexpr const char* EXPERIENCE_ORB = "minecraft:experience_orb";
// 投掷物
constexpr const char* ARROW = "minecraft:arrow";
constexpr const char* SPECTRAL_ARROW = "minecraft:spectral_arrow";
constexpr const char* TRIDENT = "minecraft:trident";
constexpr const char* SPEAR = "minecraft:spear";
constexpr const char* SNOWBALL = "minecraft:snowball";
constexpr const char* EGG = "minecraft:egg";
constexpr const char* ENDER_PEARL = "minecraft:ender_pearl";
constexpr const char* POTION = "minecraft:potion";
constexpr const char* EXPERIENCE_BOTTLE = "minecraft:experience_bottle";
constexpr const char* FIREBALL = "minecraft:fireball";
constexpr const char* SMALL_FIREBALL = "minecraft:small_fireball";
constexpr const char* DRAGON_FIREBALL = "minecraft:dragon_fireball";
constexpr const char* WITHER_SKULL = "minecraft:wither_skull";
constexpr const char* LLAMA_SPIT = "minecraft:llama_spit";
constexpr const char* SHULKER_BULLET = "minecraft:shulker_bullet";
constexpr const char* EVOKER_FANGS = "minecraft:evoker_fangs";
constexpr const char* FISHING_BOBBER = "minecraft:fishing_bobber";
constexpr const char* EYE_OF_ENDER = "minecraft:eye_of_ender";
constexpr const char* FIREWORK_ROCKET = "minecraft:firework_rocket";
constexpr const char* WIND_CHARGE = "minecraft:wind_charge";
// 交通工具
constexpr const char* BOAT = "minecraft:boat";
constexpr const char* CHEST_BOAT = "minecraft:chest_boat";
constexpr const char* MINECART = "minecraft:minecart";
constexpr const char* CHEST_MINECART = "minecraft:chest_minecart";
constexpr const char* FURNACE_MINECART = "minecraft:furnace_minecart";
constexpr const char* HOPPER_MINECART = "minecraft:hopper_minecart";
constexpr const char* TNT_MINECART = "minecraft:tnt_minecart";
constexpr const char* SPAWNER_MINECART = "minecraft:spawner_minecart";
// 其他实体
constexpr const char* FALLING_BLOCK = "minecraft:falling_block";
constexpr const char* TNT = "minecraft:tnt";
constexpr const char* END_CRYSTAL = "minecraft:end_crystal";
constexpr const char* LIGHTNING_BOLT = "minecraft:lightning_bolt";
constexpr const char* AREA_EFFECT_CLOUD = "minecraft:area_effect_cloud";
constexpr const char* ARMOR_STAND = "minecraft:armor_stand";
constexpr const char* OMINOUS_ITEM_SPAWNER = "minecraft:ominous_item_spawner";
// 悬挂实体
constexpr const char* PAINTING = "minecraft:painting";
constexpr const char* ITEM_FRAME = "minecraft:item_frame";
constexpr const char* LEASH_KNOT = "minecraft:leash_knot";
} // namespace EntityTypes

} // namespace entity
} // namespace mc
