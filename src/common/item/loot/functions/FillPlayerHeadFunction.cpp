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

#include "FillPlayerHeadFunction.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/functions/CopyNameFunction.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/skin/core/GameProfile.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

FillPlayerHeadFunction::FillPlayerHeadFunction(CopyNameFunction::Source source)
    : m_source(source)
{}

ItemStack FillPlayerHeadFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 检查物品是否是玩家头颅
    // 参考 MC Java: stack.is(Items.PLAYER_HEAD)，即引用相等性比较
    if (stack.getItem() != Items::PLAYER_HEAD) {
        return stack;
    }

    // 根据 Source 获取玩家档案
    const skin::GameProfile* profile = nullptr;
    skin::GameProfile tempProfile; // 用于临时存储从玩家实体构建的档案

    switch (m_source) {
        case CopyNameFunction::Source::This: {
            // 从当前实体（被击杀的实体）获取玩家信息
            auto* entity = context.get<Entity>(LootParams::THIS_ENTITY);
            if (entity != nullptr) {
                // 检查是否是玩家
                if (auto* player = dynamic_cast<Player*>(entity)) {
                    auto uuidArray = skin::GameProfile::parseUUID(player->uuid());
                    tempProfile.setUUID(uuidArray);
                    tempProfile.setName(player->username());
                    profile = &tempProfile;
                }
            }
            break;
        }

        case CopyNameFunction::Source::Killer: {
            // 从击杀者实体获取玩家信息
            auto* killer = context.get<Entity>(LootParams::KILLER_ENTITY);
            if (killer != nullptr) {
                // 检查是否是玩家
                if (auto* player = dynamic_cast<Player*>(killer)) {
                    auto uuidArray = skin::GameProfile::parseUUID(player->uuid());
                    tempProfile.setUUID(uuidArray);
                    tempProfile.setName(player->username());
                    profile = &tempProfile;
                }
            }
            break;
        }

        case CopyNameFunction::Source::KillerPlayer: {
            // 从击杀玩家获取玩家信息（仅当击杀者是玩家时有效）
            auto* player = context.get<Player>(LootParams::KILLER_PLAYER);
            if (player != nullptr) {
                auto uuidArray = skin::GameProfile::parseUUID(player->uuid());
                tempProfile.setUUID(uuidArray);
                tempProfile.setName(player->username());
                profile = &tempProfile;
            }
            break;
        }

        case CopyNameFunction::Source::BlockEntity:
            // 方块实体不支持玩家头颅填充
            break;
    }

    // 如果获取到了有效的玩家档案，写入 SkullOwner NBT 标签
    if (profile != nullptr && profile->hasValidUUID()) {
        nlohmann::json& tag = stack.getOrCreateTag();
        tag["SkullOwner"] = profile->toJson();
    }

    return stack;
}

std::unique_ptr<LootFunction> FillPlayerHeadFunction::clone() const noexcept
{
    auto func = std::make_unique<FillPlayerHeadFunction>(m_source);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
