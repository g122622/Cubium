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

/**
 * @file probe_world.hpp
 * @brief 差分探针共用的「空世界」与实体桩。
 *
 * `LootContext`、实体谓词等能力需要一个 `IWorld&`，绝大多数判定只会通过它间接读取
 * 方块、光照、时间与位置。这里提供一个「什么都没有」的世界实现：方块查询一律返回
 * nullptr、光照为 0、下雨为否、随机数种子固定。
 *
 * 意义在于：两侧（Cubium probe 与原版 oracle）都从同一个「空世界」出发，差异就只
 * 可能来自被测能力本身，而不是世界内容。
 */

#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc::diff::probe {

/**
 * @brief 无任何内容的测试世界。
 *
 * 只实现 IWorld 的纯虚函数与几处「不实现就只能崩溃」的访问器；其余走 IWorld 的
 * 默认实现（返回空、返回 0、返回默认值）。注意 ECS registry 必须真的存在，
 * 否则实体工厂 `EntityType::create` 拿不到 registry 会直接返回空指针。
 */
class NullWorld final : public IWorld {
public:
    explicit NullWorld(const loot::LootTableManager* tables = nullptr)
        : m_tables(tables)
    {}

    /// 战利品表管理器：战利品表通过 TableLootEntry 引用其它表时由它解析。
    [[nodiscard]] const loot::LootTableManager* lootTableManager() const override
    {
        return m_tables;
    }

    // ========== 方块访问 ==========

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override
    {
        return nullptr;
    }

    bool setBlockState(i32, i32, i32, const BlockState*) override
    {
        return false;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return nullptr;
    }

    // ========== 世界尺寸与光照 ==========

    [[nodiscard]] i32 getHeight(i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override
    {
        return {};
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] PhysicsEngine* physicsEngine() override
    {
        return nullptr;
    }

    [[nodiscard]] const PhysicsEngine* physicsEngine() const override
    {
        return nullptr;
    }

    // ========== 实体查询 ==========

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId) override
    {
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId) const override
    {
        return nullptr;
    }

    // ========== 维度与时间 ==========

    [[nodiscard]] DimensionId dimension() const override
    {
        return 0; // 主世界
    }

    [[nodiscard]] u64 seed() const override
    {
        return 0;
    }

    [[nodiscard]] u64 currentTick() const override
    {
        return 0;
    }

    [[nodiscard]] i64 dayTime() const override
    {
        return 0;
    }

    // ========== 方块数据 ==========

    [[nodiscard]] bool isClientSide() const override
    {
        return false;
    }

    [[nodiscard]] bool isHardcore() const override
    {
        return false;
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return Difficulty::Normal;
    }

    // ========== 天气 ==========

    [[nodiscard]] bool isRaining() const override
    {
        return false;
    }

    [[nodiscard]] bool isThundering() const override
    {
        return false;
    }

    // ========== 随机 ==========

    [[nodiscard]] math::IRandom& getRandom() override
    {
        return m_random;
    }

    [[nodiscard]] const math::IRandom& getRandom() const override
    {
        return m_random;
    }

    // ========== 世界边界与 tick ==========

    [[nodiscard]] world::border::WorldBorder& worldBorder() override
    {
        return m_worldBorder;
    }

    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override
    {
        return m_worldBorder;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        // 探针不跑 tick：任何战利品条件/函数都不该在掷骰期间读取 tick 管理器。
        throw std::logic_error("NullWorld::tickManager 不可用：战利品掷骰不应依赖世界 tick");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("NullWorld::tickManager 不可用：战利品掷骰不应依赖世界 tick");
    }

    // ========== ECS ==========

    [[nodiscard]] ecs::EntityRegistry* entityRegistry() override
    {
        return m_entityRegistry.get();
    }

    /// 非空的 registry 引用（探针在构造后立即使用，不检查空指针）。
    [[nodiscard]] ecs::EntityRegistry& registry() noexcept
    {
        return *m_entityRegistry;
    }

    [[nodiscard]] const ecs::EntityRegistry* entityRegistry() const override
    {
        return m_entityRegistry.get();
    }

    /**
     * @brief 按真实服务端的方式掷一张表，把产出收进 `stacks`
     *
     * 与服务端走同一个入口（`LootTable::generate`），合并口径也一致。
     */
    void rollLoot(const loot::LootTable& table, loot::LootContext& context, std::vector<ItemStack>& stacks)
    {
        stacks = table.generate(context);
    }

private:
    const loot::LootTableManager* m_tables;
    world::border::WorldBorder m_worldBorder;
    math::Random m_random{0};
    // IWorld::entityRegistry 返回可空指针，这里的 registry 只在构造实体时需要，
    // 探针不持有长生命周期的实体，故用独占指针表达所有权。
    std::unique_ptr<ecs::EntityRegistry> m_entityRegistry = std::make_unique<ecs::EntityRegistry>("diff_loot");
};

/**
 * @brief 在空世界坐标系里放置一个实体，供 `this_entity` 上下文使用。
 *
 * 实体的类型由调用方给出（如 `minecraft:pig`），位置固定在原点：两侧（Cubium 与
 * 原版 oracle）取同一个坐标，`origin` 相关条件（如位置检查）就有一致的输入。
 */
inline std::unique_ptr<Entity> createEntity(NullWorld& world, const std::string& typeName)
{
    const entity::EntityType* type = entity::EntityRegistry::instance().getType(typeName);
    if (type == nullptr || world.entityRegistry() == nullptr) {
        return nullptr;
    }

    auto entity = type->create(&world, *world.entityRegistry());
    if (entity == nullptr) {
        return nullptr;
    }

    entity->setPosition(0.0f, 64.0f, 0.0f);
    return entity;
}

} // namespace mc::diff::probe
