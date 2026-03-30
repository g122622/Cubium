#pragma once

#include "EntityType.hpp"
#include "../../core/Result.hpp"
#include <unordered_map>
#include <mutex>
#include <vector>

namespace mc {

namespace entity {

// 引入 mc 命名空间的类型
using mc::String;
using mc::Error;
using mc::ErrorCode;
using mc::Result;

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
 *
 * 参考 MC 1.16.5 Registry.ENTITY_TYPE
 */
class EntityRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static EntityRegistry& instance() {
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
    Result<EntityTypeId> registerType(const String& resourceLocation, EntityType type) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 检查是否已存在
        if (m_nameToId.find(resourceLocation) != m_nameToId.end()) {
            return Error(ErrorCode::AlreadyExists,
                        "Entity type already registered: " + resourceLocation);
        }

        // 分配ID
        EntityTypeId id = m_nextId++;

        // 设置ID和名称
        const_cast<EntityTypeId&>(type.m_id) = id;
        const_cast<String&>(type.m_name) = resourceLocation;

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
    [[nodiscard]] const EntityType* getType(EntityTypeId id) const {
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
    [[nodiscard]] const EntityType* getType(const String& name) const {
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
    [[nodiscard]] const EntityType& getTypeOrThrow(EntityTypeId id) const {
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
    [[nodiscard]] bool hasType(const String& name) const {
        return m_nameToId.find(name) != m_nameToId.end();
    }

    /**
     * @brief 获取所有已注册的实体类型
     */
    [[nodiscard]] const std::vector<EntityType>& getAllTypes() const {
        return m_types;
    }

    /**
     * @brief 获取已注册的实体类型数量
     */
    [[nodiscard]] size_t size() const {
        return m_types.size();
    }

    /**
     * @brief 清空所有注册（仅用于测试）
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_types.clear();
        m_nameToId.clear();
        m_nextId = 1; // 从1开始，0保留
    }

    // 禁止拷贝和移动
    EntityRegistry(const EntityRegistry&) = delete;
    EntityRegistry& operator=(const EntityRegistry&) = delete;
    EntityRegistry(EntityRegistry&&) = delete;
    EntityRegistry& operator=(EntityRegistry&&) = delete;

private:
    EntityRegistry() : m_nextId(1) {} // 0保留给无效ID

    std::vector<EntityType> m_types;
    std::unordered_map<String, EntityTypeId> m_nameToId;
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
#define REGISTER_ENTITY_TYPE(name, type) \
    ::mc::entity::EntityRegistry::instance().registerType(name, type)

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

    // 水生生物
    constexpr const char* COD = "minecraft:cod";
    constexpr const char* SALMON = "minecraft:salmon";
    constexpr const char* PUFFERFISH = "minecraft:pufferfish";
    constexpr const char* TROPICAL_FISH = "minecraft:tropical_fish";
    constexpr const char* SQUID = "minecraft:squid";
    constexpr const char* DOLPHIN = "minecraft:dolphin";

    // 环境生物
    constexpr const char* BAT = "minecraft:bat";

    // 傀儡
    constexpr const char* IRON_GOLEM = "minecraft:iron_golem";
    constexpr const char* SNOW_GOLEM = "minecraft:snow_golem";

    // 怪物
    constexpr const char* ZOMBIE = "minecraft:zombie";
    constexpr const char* SKELETON = "minecraft:skeleton";
    constexpr const char* CREEPER = "minecraft:creeper";
    constexpr const char* SPIDER = "minecraft:spider";
    constexpr const char* ENDERMAN = "minecraft:enderman";
    constexpr const char* BLAZE = "minecraft:blaze";
    constexpr const char* WITCH = "minecraft:witch";
    constexpr const char* SLIME = "minecraft:slime";
    // 亡灵变种
    constexpr const char* HUSK = "minecraft:husk";
    constexpr const char* DROWNED = "minecraft:drowned";
    constexpr const char* STRAY = "minecraft:stray";
    constexpr const char* WITHER_SKELETON = "minecraft:wither_skeleton";
    // 节肢动物变种
    constexpr const char* CAVE_SPIDER = "minecraft:cave_spider";
    // Boss
    constexpr const char* ENDER_DRAGON = "minecraft:ender_dragon";
    constexpr const char* WITHER = "minecraft:wither";

    // 其他
    constexpr const char* PLAYER = "minecraft:player";
    constexpr const char* ITEM = "minecraft:item";
    constexpr const char* EXPERIENCE_ORB = "minecraft:experience_orb";
    constexpr const char* ARROW = "minecraft:arrow";
    constexpr const char* BOAT = "minecraft:boat";
    constexpr const char* MINECART = "minecraft:minecart";
}

} // namespace entity
} // namespace mc
