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
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <algorithm>
#include <memory>
#include <string>

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

/// 方块实体可导出的组件（项目以 JSON 快照承载方块实体数据，只支持已建模的少数组件）
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
        case DataComponentType::Container: {
            // 方块实体容器内容：BlockEntity 尚无统一的槽位快照接口，
            //  由后续容器组件化工作补齐（见 docs 中 copy_components 说明）。
            return false;
        }
        default:
            return false;
    }
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

ItemStack CopyComponentsFunction::apply(ItemStack stack, LootContext& context) const
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
