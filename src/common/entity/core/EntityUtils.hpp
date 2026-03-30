#pragma once

#include "Entity.hpp"
#include "living/LivingEntity.hpp"
#include "../world/IWorld.hpp"
#include "../util/math/Vector3.hpp"
#include <functional>
#include <type_traits>

namespace mc {

/**
 * @brief 实体工具函数集合
 *
 * 提供常用的实体搜索、距离计算等功能。
 */
namespace EntityUtils {

/**
 * @brief 查找最近的实体（指定类型）
 *
 * @tparam T 实体类型（必须是 Entity 的子类）
 * @param world 世界指针
 * @param pos 搜索中心位置
 * @param range 搜索范围
 * @param except 排除的实体（可选）
 * @param predicate 过滤条件（可选）
 * @return 最近的实体指针，如果没有找到返回 nullptr
 */
template<typename T>
T* findClosestEntity(
    IWorld* world,
    const Vector3& pos,
    f32 range,
    const Entity* except = nullptr,
    std::function<bool(T*)> predicate = nullptr)
{
    static_assert(std::is_base_of<Entity, T>::value, "T must be derived from Entity");

    if (!world) return nullptr;

    auto entities = world->getEntitiesInRange(pos, range, except);
    T* closest = nullptr;
    f32 closestDistSq = range * range;

    for (Entity* entity : entities) {
        // 尝试转换为目标类型
        T* typed = dynamic_cast<T*>(entity);
        if (!typed) continue;

        // 检查是否存活
        if (!typed->isAlive()) continue;

        // 检查过滤条件
        if (predicate && !predicate(typed)) continue;

        // 计算距离
        f32 distSq = pos.distanceSquared(typed->position());
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closest = typed;
        }
    }

    return closest;
}

/**
 * @brief 查找最近的生物实体
 *
 * 便捷方法，等价于 findClosestEntity<LivingEntity>
 */
inline LivingEntity* findClosestLiving(
    IWorld* world,
    const Vector3& pos,
    f32 range,
    const Entity* except = nullptr,
    std::function<bool(LivingEntity*)> predicate = nullptr)
{
    return findClosestEntity<LivingEntity>(world, pos, range, except, predicate);
}

/**
 * @brief 查找范围内所有指定类型的实体
 *
 * @tparam T 实体类型
 * @param world 世界指针
 * @param pos 搜索中心位置
 * @param range 搜索范围
 * @param except 排除的实体（可选）
 * @param predicate 过滤条件（可选）
 * @return 匹配的实体列表
 */
template<typename T>
std::vector<T*> findEntities(
    IWorld* world,
    const Vector3& pos,
    f32 range,
    const Entity* except = nullptr,
    std::function<bool(T*)> predicate = nullptr)
{
    static_assert(std::is_base_of<Entity, T>::value, "T must be derived from Entity");

    std::vector<T*> result;
    if (!world) return result;

    auto entities = world->getEntitiesInRange(pos, range, except);

    for (Entity* entity : entities) {
        T* typed = dynamic_cast<T*>(entity);
        if (!typed) continue;
        if (!typed->isAlive()) continue;
        if (predicate && !predicate(typed)) continue;

        result.push_back(typed);
    }

    return result;
}

/**
 * @brief 检查实体是否在范围内
 *
 * @param entity 目标实体
 * @param pos 中心位置
 * @param rangeSq 范围的平方
 * @return 是否在范围内
 */
inline bool isInRange(const Entity& entity, const Vector3& pos, f32 rangeSq) {
    return pos.distanceSquared(entity.position()) <= rangeSq;
}

/**
 * @brief 检查两个实体之间的距离
 *
 * @param a 第一个实体
 * @param b 第二个实体
 * @param range 范围
 * @return 是否在范围内
 */
inline bool isInRange(const Entity& a, const Entity& b, f32 range) {
    return a.distanceSqTo(b) <= range * range;
}

/**
 * @brief 计算从源实体到目标实体的水平角度（偏航角）
 *
 * @param source 源实体
 * @param target 目标实体
 * @return 偏航角（度），范围 [-180, 180]
 */
inline f32 calculateYawTo(const Entity& source, const Entity& target) {
    f64 dx = target.x() - source.x();
    f64 dz = target.z() - source.z();
    return static_cast<f32>(std::atan2(dz, dx) * math::RAD_TO_DEG - 90.0);
}

/**
 * @brief 计算从源实体到目标位置的偏航角
 *
 * @param source 源实体
 * @param targetX 目标X坐标
 * @param targetZ 目标Z坐标
 * @return 偏航角（度）
 */
inline f32 calculateYawTo(const Entity& source, f64 targetX, f64 targetZ) {
    f64 dx = targetX - source.x();
    f64 dz = targetZ - source.z();
    return static_cast<f32>(std::atan2(dz, dx) * math::RAD_TO_DEG - 90.0);
}

/**
 * @brief 计算从源实体到目标实体的俯仰角
 *
 * @param source 源实体
 * @param target 目标实体
 * @return 俯仰角（度），正值向下看，负值向上看
 */
inline f32 calculatePitchTo(const Entity& source, const Entity& target) {
    f64 dx = target.x() - source.x();
    f64 dy = (target.y() + target.eyeHeight()) - (source.y() + source.eyeHeight());
    f64 dz = target.z() - source.z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);
    return static_cast<f32>(-(std::atan2(dy, horizontalDist) * math::RAD_TO_DEG));
}

} // namespace EntityUtils

} // namespace mc
