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

#include "CriterionContext.hpp"

#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/StatePropertiesPredicate.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

#include <cmath>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

namespace mc::advancement {

const Entity* CriterionContext::resolveEntity(const std::string& name) const
{
    // vanilla 的 EntityTarget：this / killer / direct_killer / player_killer
    if (name == "killer" || name == "direct_killer" || name == "player_killer") {
        return killer != nullptr ? killer : entity;
    }
    // "this" 及未知名称回退到被检查实体
    return entity;
}

const ItemStack* CriterionContext::resolveItem(const std::string& /*name*/) const
{
    // 进度条件中物品上下文只有 "tool"（match_tool）与物品型 this，均指向同一物品堆
    return item;
}

bool ContextAwarePredicate::test(const CriterionContext& context) const
{
    for (const auto& condition : m_conditions) {
        if (condition == nullptr) {
            continue;
        }
        switch (condition->kind) {
            case Kind::EntityProperties: {
                const Entity* target = context.resolveEntity(condition->target);
                if (target == nullptr ||
                    (condition->entityPredicate.has_value() && !condition->entityPredicate->test(*target))) {
                    return false;
                }
                break;
            }
            case Kind::LocationCheck: {
                if (context.world == nullptr || !condition->locationPredicate.has_value()) {
                    return false;
                }
                // 方块使用类触发器显式给出方块位置时，location_check 判定该方块
                // （对齐 vanilla LevelLocation = 使用目标的方块中心）；否则按实体位置判定
                if (context.blockPos != nullptr) {
                    const f64 ox = static_cast<f64>(condition->offset.x);
                    const f64 oy = static_cast<f64>(condition->offset.y);
                    const f64 oz = static_cast<f64>(condition->offset.z);
                    if (!condition->locationPredicate->test(*context.world,
                            static_cast<f64>(context.blockPos->x) + ox,
                            static_cast<f64>(context.blockPos->y) + oy,
                            static_cast<f64>(context.blockPos->z) + oz)) {
                        return false;
                    }
                    break;
                }
                const Entity* target = context.resolveEntity(condition->target);
                if (target == nullptr ||
                    !condition->locationPredicate->test(*context.world,
                        static_cast<f64>(target->x()) + static_cast<f64>(condition->offset.x),
                        static_cast<f64>(target->y()) + static_cast<f64>(condition->offset.y),
                        static_cast<f64>(target->z()) + static_cast<f64>(condition->offset.z))) {
                    return false;
                }
                break;
            }
            case Kind::BlockStateProperty: {
                // 判定显式方块位置或实体所在位置的方块（如 heart_transplanter 的
                // creaking_heart_state=awake）
                const Entity* target = context.resolveEntity(condition->target);
                if (context.world == nullptr || (target == nullptr && context.blockPos == nullptr)) {
                    return false;
                }
                const BlockPos targetPos = context.blockPos != nullptr
                    ? *context.blockPos
                    : BlockPos(static_cast<i32>(std::floor(target->x())),
                          static_cast<i32>(std::floor(target->y())),
                          static_cast<i32>(std::floor(target->z())));
                const BlockState* state = context.world->getBlockState(targetPos);
                if (state == nullptr || state->getBlock().blockLocation() != condition->blockId) {
                    return false;
                }
                if (condition->blockState.has_value() && !condition->blockState->matches(*state)) {
                    return false;
                }
                break;
            }
            case Kind::MatchTool: {
                if (context.item == nullptr || context.item->isEmpty()) {
                    return false;
                }
                if (condition->itemPredicate.has_value() && !condition->itemPredicate->test(*context.item)) {
                    return false;
                }
                break;
            }
            case Kind::AllOf: {
                for (const auto& child : condition->children) {
                    if (child == nullptr) {
                        continue;
                    }
                    ContextAwarePredicate single;
                    single.m_conditions = {child};
                    single.m_empty = false;
                    if (!single.test(context)) {
                        return false;
                    }
                }
                break;
            }
            case Kind::AnyOf: {
                bool any = false;
                for (const auto& child : condition->children) {
                    if (child == nullptr) {
                        continue;
                    }
                    ContextAwarePredicate single;
                    single.m_conditions = {child};
                    single.m_empty = false;
                    if (single.test(context)) {
                        any = true;
                        break;
                    }
                }
                if (!any) {
                    return false;
                }
                break;
            }
            case Kind::Inverted: {
                bool matched = true;
                for (const auto& child : condition->children) {
                    if (child == nullptr) {
                        continue;
                    }
                    ContextAwarePredicate single;
                    single.m_conditions = {child};
                    single.m_empty = false;
                    if (!single.test(context)) {
                        matched = false;
                        break;
                    }
                }
                if (matched) {
                    return false;
                }
                break;
            }
        }
    }
    return true;
}

Result<std::shared_ptr<ContextAwarePredicate::Condition>> ContextAwarePredicate::_parseCondition(
    const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error::invalidArgument("ContextAwarePredicate element must be an object");
    }

    // 裸谓词对象（无 condition 键）按实体谓词处理（简化写法）
    if (!json.contains("condition")) {
        auto condition = std::make_shared<Condition>();
        condition->kind = Kind::EntityProperties;
        condition->target = "this";
        auto result = EntityPredicate::fromJson(json);
        if (result.failed()) {
            return result.error();
        }
        condition->entityPredicate = std::move(result.value());
        return condition;
    }

    const std::string type = json["condition"].get<std::string>();
    auto condition = std::make_shared<Condition>();

    if (type == "minecraft:entity_properties" || type == "entity_properties") {
        condition->kind = Kind::EntityProperties;
        if (json.contains("entity") && json["entity"].is_string()) {
            condition->target = json["entity"].get<std::string>();
        }
        if (json.contains("predicate")) {
            auto result = EntityPredicate::fromJson(json["predicate"]);
            if (result.failed()) {
                return result.error();
            }
            condition->entityPredicate = std::move(result.value());
        }
        return condition;
    }

    if (type == "minecraft:location_check" || type == "location_check") {
        condition->kind = Kind::LocationCheck;
        // location_check 的 offsetX/offsetY/offsetZ（相对判定位置的坐标偏移）
        f64 offsetX = 0.0;
        f64 offsetY = 0.0;
        f64 offsetZ = 0.0;
        if (json.contains("offsetX") && json["offsetX"].is_number()) {
            offsetX = json["offsetX"].get<f64>();
        }
        if (json.contains("offsetY") && json["offsetY"].is_number()) {
            offsetY = json["offsetY"].get<f64>();
        }
        if (json.contains("offsetZ") && json["offsetZ"].is_number()) {
            offsetZ = json["offsetZ"].get<f64>();
        }
        condition->offset = Vector3(static_cast<f32>(offsetX), static_cast<f32>(offsetY), static_cast<f32>(offsetZ));
        if (json.contains("predicate")) {
            auto result = LocationPredicate::fromJson(json["predicate"]);
            if (result.failed()) {
                return result.error();
            }
            condition->locationPredicate = std::move(result.value());
        }
        return condition;
    }

    if (type == "minecraft:block_state_property" || type == "block_state_property") {
        condition->kind = Kind::BlockStateProperty;
        if (json.contains("block") && json["block"].is_string()) {
            condition->blockId = ResourceLocation(json["block"].get<std::string>());
        }
        if (json.contains("properties") && json["properties"].is_object()) {
            auto result = StatePropertiesPredicate::fromJson(json["properties"]);
            if (result.failed()) {
                return result.error();
            }
            condition->blockState = std::move(result.value());
        }
        return condition;
    }

    if (type == "minecraft:match_tool" || type == "match_tool") {
        condition->kind = Kind::MatchTool;
        if (json.contains("predicate")) {
            auto result = ItemPredicate::fromJson(json["predicate"]);
            if (result.failed()) {
                return result.error();
            }
            condition->itemPredicate = std::move(result.value());
        }
        return condition;
    }

    const auto parseChildren = [](const nlohmann::json& terms,
                                   std::vector<std::shared_ptr<Condition>>& out) -> Result<void> {
        if (!terms.is_array()) {
            return {};
        }
        for (const auto& term : terms) {
            auto result = ContextAwarePredicate::_parseCondition(term);
            if (result.failed()) {
                return result.error();
            }
            out.push_back(std::move(result.value()));
        }
        return {};
    };

    if (type == "minecraft:all_of" || type == "all_of") {
        condition->kind = Kind::AllOf;
        if (json.contains("terms")) {
            if (auto result = parseChildren(json["terms"], condition->children); result.failed()) {
                return result.error();
            }
        }
        return condition;
    }
    if (type == "minecraft:any_of" || type == "any_of") {
        condition->kind = Kind::AnyOf;
        if (json.contains("terms")) {
            if (auto result = parseChildren(json["terms"], condition->children); result.failed()) {
                return result.error();
            }
        }
        return condition;
    }
    if (type == "minecraft:inverted" || type == "inverted") {
        condition->kind = Kind::Inverted;
        if (json.contains("term")) {
            auto result = ContextAwarePredicate::_parseCondition(json["term"]);
            if (result.failed()) {
                return result.error();
            }
            condition->children.push_back(std::move(result.value()));
        }
        return condition;
    }

    // TODO(context-aware-predicate): 其余条件类型（survives_explosion / random_chance / reference /
    //   environment_attribute 等）在进度条件中尚未支持。无法识别的条件按"永真"处理会误放大
    //   进度达成范围，故直接拒绝该进度文件（当前 vanilla 数据包使用的 7 种条件类型均已支持）。
    return Error::invalidArgument("Unsupported ContextAwarePredicate condition: " + type);
}

Result<ContextAwarePredicate> ContextAwarePredicate::fromJson(const nlohmann::json& json)
{
    ContextAwarePredicate predicate;
    predicate.m_empty = false;

    if (json.is_null()) {
        predicate.m_empty = true;
        return predicate;
    }

    if (json.is_array()) {
        for (const auto& entry : json) {
            auto result = _parseCondition(entry);
            if (result.failed()) {
                return result.error();
            }
            predicate.m_conditions.push_back(std::move(result.value()));
        }
        predicate.m_empty = predicate.m_conditions.empty();
        return predicate;
    }

    if (json.is_object()) {
        // 单个谓词对象：既可能是 ContextAwarePredicate 也能是裸 EntityPredicate
        auto result = _parseCondition(json);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_conditions.push_back(std::move(result.value()));
        return predicate;
    }

    return Error::invalidArgument("ContextAwarePredicate must be an object or an array");
}

nlohmann::json ContextAwarePredicate::toJson() const
{
    // 序列化仅用于调试输出（条件树无法无损还原原始 JSON 写法）
    if (m_empty) {
        return nullptr;
    }
    return nlohmann::json::array();
}

} // namespace mc::advancement
