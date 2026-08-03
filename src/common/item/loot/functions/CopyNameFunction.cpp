/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "CopyNameFunction.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace loot {

CopyNameFunction::CopyNameFunction(Source source)
    : m_source(source)
{}

ItemStack CopyNameFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 根据来源类型从 LootContext 获取对应对象，检查是否有自定义名称，复制到物品

    switch (m_source) {
        case Source::This: {
            // 从当前实体获取名称
            auto* entity = context.get<Entity>(LootParams::THIS_ENTITY);
            if (entity != nullptr && entity->hasCustomName()) {
                auto displayName = entity->getDisplayName();
                if (displayName) {
                    stack.setCustomNameComponent(std::move(displayName));
                }
            }
            break;
        }

        case Source::Killer: {
            // 从击杀实体获取名称
            auto* killer = context.get<Entity>(LootParams::KILLER_ENTITY);
            if (killer != nullptr && killer->hasCustomName()) {
                auto displayName = killer->getDisplayName();
                if (displayName) {
                    stack.setCustomNameComponent(std::move(displayName));
                }
            }
            break;
        }

        case Source::KillerPlayer: {
            // 从击杀玩家获取名称
            auto* player = context.get<Player>(LootParams::KILLER_PLAYER);
            if (player != nullptr) {
                // 玩家总是有名称（用户名），即使没有自定义名称
                auto displayName = player->getDisplayName();
                if (displayName) {
                    stack.setCustomNameComponent(std::move(displayName));
                }
            }
            break;
        }

        case Source::BlockEntity: {
            // 从方块实体获取名称
            auto* blockEntity = context.get<BlockEntity>(LootParams::BLOCK_ENTITY);
            if (blockEntity != nullptr) {
                std::string customName = blockEntity->getCustomName();
                if (!customName.empty()) {
                    stack.setCustomName(customName);
                }
            }
            break;
        }
    }

    return stack;
}

std::unique_ptr<LootFunction> CopyNameFunction::clone() const noexcept
{
    auto func = std::make_unique<CopyNameFunction>(m_source);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
