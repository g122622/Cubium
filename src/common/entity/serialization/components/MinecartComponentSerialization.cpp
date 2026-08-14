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

#include "common/entity/serialization/components/MinecartComponentSerialization.hpp"

#include "common/core/Result.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/ecs/components/SpawnerMinecartComponent.hpp"
#include "common/util/nbt/Nbt.hpp"

namespace mc::entity::serialization::components {

// ============================================================================
// SpawnerMinecartComponent — SpawnerLogic 全部参数
// 对齐 vanilla 1.21.11 MinecartSpawner：MinecartSpawner 无自有 addAdditionalSaveData，
// 持久化完全委托内部 BaseSpawner（本项目为 SpawnerLogic）。SpawnerLogic 自身已实现
// saveToNBT/loadFromNBT（与 MobSpawnerBlockEntity 共用同一逻辑类），序列化器仅透传：
// 取组件内 SpawnerLogic 引用，直接调其 saveToNBT/loadFromNBT 把键平铺到实体 compound 根层。
// SpawnerLogic 键（Delay/MinSpawnDelay/.../SpawnData/SpawnPotentials）与 minecart 基类
// 组件序列化键（Pos/Motion/Rotation 等）无冲突，平铺安全。
// ============================================================================

static void saveSpawnerMinecart(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::SpawnerMinecartComponent>();
    if (c == nullptr) {
        return; // 非 SpawnerMinecart 早退
    }
    // 直接把 SpawnerLogic 字段平铺写入实体 compound 根层。
    // SpawnerLogic::saveToNBT 只 put/emplace 不清空 tag，与基类组件键无冲突。
    c->m_spawnerLogic.saveToNBT(tag);
}

static Result<void> loadSpawnerMinecart(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::SpawnerMinecartComponent>();
    if (c == nullptr) {
        return {}; // 非 SpawnerMinecart 早退
    }
    // SpawnerLogic::loadFromNBT 内部对各键做存在性检查后读取，缺键保留默认值，安全。
    c->m_spawnerLogic.loadFromNBT(tag);
    return {};
}

// ============================================================================
// 注册
// ============================================================================

void registerMinecartComponentSerializers(ComponentSerializerRegistry& registry)
{
    // priority=0（无跨组件依赖，SpawnerLogic 字段自包含）
    registry.registerSerializer<ecs::SpawnerMinecartComponent>(saveSpawnerMinecart, loadSpawnerMinecart);
}

} // namespace mc::entity::serialization::components
