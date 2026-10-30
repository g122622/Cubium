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

#include "CopyComponentsFunction.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "common/world/blockentity/core/LockableBlockEntity.hpp"
#include "common/world/blockentity/core/LootableContainerBlockEntity.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace mc {
namespace loot {

namespace {

bool _contains(const std::vector<item::component::DataComponentType>& list, item::component::DataComponentType type)
{
    return std::find(list.begin(), list.end(), type) != list.end();
}

/// 组件是否应被复制：与 vanilla bakedPredicate 一致（include 与 exclude 同时生效时取交集）
bool _shouldCopy(const CopyComponentsFunction& func, item::component::DataComponentType type)
{
    if (!func.getExclude().empty() && _contains(func.getExclude(), type)) {
        return false;
    }
    if (!func.getInclude().empty()) {
        return _contains(func.getInclude(), type);
    }
    return true;
}

/// 方块实体可导出的组件（vanilla `BlockEntity#collectComponents` 的隐式组件部分）
bool _blockEntityComponent(
    BlockEntity& blockEntity, item::component::DataComponentType type, item::component::DataComponentPayload& out)
{
    using item::component::DataComponentPayload;
    using item::component::DataComponentType;
    using namespace item::component;

    switch (type) {
        case DataComponentType::CustomName: {
            const std::string name = blockEntity.getCustomName();
            if (name.empty()) {
                return false;
            }
            out = DataComponentPayload{std::in_place_index<ComponentPayloadIndex::TextComponent>,
                std::make_unique<text::StringTextComponent>(name)};
            return true;
        }
        case DataComponentType::Lock: {
            // vanilla `BaseContainerBlockEntity.collectImplicitComponents`：只有锁非空时才写，
            // 空锁不产生组件（否则每个被撬开的箱子都会带上 lock 组件）。
            const auto* lockable = dynamic_cast<const blockentity::LockableBlockEntity*>(&blockEntity);
            if (lockable == nullptr || !lockable->isLocked()) {
                return false;
            }
            out = DataComponentPayload{
                std::in_place_index<ComponentPayloadIndex::Json>, nlohmann::json{{"key", lockable->getLockKey()}}};
            return true;
        }
        case DataComponentType::ContainerLoot: {
            // 只有「还没被打开过」的容器才带 container_loot：打开一次后战利品就摊进槽位，
            // 再复制这个组件只会让掉落物重新挂上一张已经用掉的表。
            const auto* container = dynamic_cast<const blockentity::LootableContainerBlockEntity*>(&blockEntity);
            if (container == nullptr || !container->hasLootTable()) {
                return false;
            }
            out = DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>,
                nlohmann::json{
                    {"loot_table", container->getLootTable().toString()}, {"seed", container->getLootTableSeed()}}};
            return true;
        }
        case DataComponentType::Container:
            // 载荷是物品堆列表，放不进 DataComponentPayload，由 _copyContainerComponent 处理。
            return false;
        default:
            return false;
    }
}

/// 把方块实体的槽位内容写进物品的 `container` 组件（vanilla `ItemContainerContents.fromItems`）。
void _copyContainerComponent(BlockEntity* blockEntity, ItemStack& stack)
{
    auto* container = dynamic_cast<ContainerBlockEntity*>(blockEntity);
    if (container == nullptr) {
        return;
    }
    IInventory* inventory = container->getInventory();
    if (inventory == nullptr) {
        return;
    }

    item::component::ItemContainerContentsPayload contents;
    const i32 size = inventory->getContainerSize();
    for (i32 slot = 0; slot < size; ++slot) {
        ItemStack item = inventory->getItem(slot);
        if (item.isEmpty()) {
            continue;
        }
        contents.slots.push_back(
            item::component::ItemContainerContentsPayload::Slot{slot, std::make_unique<ItemStack>(std::move(item))});
    }
    // 原版 `ItemContainerContents.fromItems` 在容器全空时返回 EMPTY，即不写组件。
    if (contents.slots.empty()) {
        return;
    }
    stack.setContainerContents(std::move(contents));
}

/// 实体可导出的组件（vanilla Entity#collectComponents）
bool _entityComponent(
    Entity& entity, item::component::DataComponentType type, item::component::DataComponentPayload& out)
{
    using item::component::DataComponentPayload;
    using item::component::DataComponentType;
    using namespace item::component;

    switch (type) {
        case DataComponentType::CustomName: {
            const auto* component = entity.getCustomNameComponent();
            if (component == nullptr) {
                return false;
            }
            out =
                DataComponentPayload{std::in_place_index<ComponentPayloadIndex::TextComponent>, component->deepCopy()};
            return true;
        }
        default:
            // TODO: Entity#collectComponents 还包含 custom_data（实体持久化数据），
            //  待实体数据组件化后补齐。
            return false;
    }
}

} // namespace

CopyComponentsFunction::CopyComponentsFunction(Source source,
    std::vector<item::component::DataComponentType> include,
    std::vector<item::component::DataComponentType> exclude)
    : m_source(source)
    , m_include(std::move(include))
    , m_exclude(std::move(exclude))
{}

ItemStack CopyComponentsFunction::run(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 按组件注册表全量遍历，逐个询问来源是否能提供该组件
    for (i32 typeId = 0; typeId <= static_cast<i32>(item::component::DataComponentType::ShulkerColor); ++typeId) {
        auto type = item::component::componentTypeById(typeId);
        if (!type.has_value() || !_shouldCopy(*this, *type)) {
            continue;
        }
        // container 组件的载荷要持有整份物品堆列表，放不进 DataComponentPayload
        // （该 variant 先于 ItemStack 定义，元素不能是 ItemStack），只能走 ItemStack
        // 自己的独立字段，因此在这里单独处理。
        if (m_source == Source::BlockEntity && *type == item::component::DataComponentType::Container) {
            auto* blockEntity = context.get(LootParams::BLOCK_ENTITY);
            _copyContainerComponent(blockEntity, stack);
            continue;
        }

        item::component::DataComponentPayload payload{};
        bool hasValue = false;
        if (m_source == Source::BlockEntity) {
            auto* blockEntity = context.get(LootParams::BLOCK_ENTITY);
            hasValue = blockEntity != nullptr && _blockEntityComponent(*blockEntity, *type, payload);
        } else {
            auto* entity = context.get(LootParams::THIS_ENTITY);
            hasValue = entity != nullptr && _entityComponent(*entity, *type, payload);
        }
        if (hasValue) {
            stack.setComponent(*type, std::move(payload));
        }
    }
    return stack;
}

std::unique_ptr<LootFunction> CopyComponentsFunction::clone() const noexcept
{
    auto func = std::make_unique<CopyComponentsFunction>(m_source, m_include, m_exclude);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
