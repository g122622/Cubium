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

#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class StatePropertiesPredicate;
class BlockPos;
class Entity;
class ItemStack;
class IWorld;
class LootCondition;

namespace advancement {

/**
 * @brief 条件上下文（对齐 vanilla LootContext 中进度判定所需的字段）
 *
 * 数据包的进度条件使用 ContextAwarePredicate：`[{"condition": "...", "predicate": {...}}]`。
 * 除实体谓词外，还常引用：
 *  - "this"：被检查的实体本身（多数触发器中与触发者相同）；
 *  - "killer"：击杀者（击杀类触发器）；
 *  - "tool" / "this"（物品上下文）：参与判定的物品堆（使用物品、钓竿、事件物品等）。
 *
 * Cubium 原无该上下文，触发器条件一律只按"谓词对象"解析，导致
 * `[{"condition":"minecraft:entity_properties", ...}]` 这类写法被静默忽略、
 * 条件退化为恒真（如 remove_wolf_armor 任何物品都能满足）。
 */
struct CriterionContext {
    /// 被检查的实体（"this"）
    const Entity* entity = nullptr;
    /// 击杀者（"killer"）
    const Entity* killer = nullptr;
    /// 参与判定的物品（"tool" / 物品上下文）
    const ItemStack* item = nullptr;
    /// 世界（位置相关条件需要）
    const IWorld* world = nullptr;
    /// 显式方块位置（方块使用类触发器：location_check / block_state_property 判定该方块，
    /// 而非实体所在方块；为空时按实体位置取方块，对齐 vanilla 的 LevelLocation 上下文）
    const BlockPos* blockPos = nullptr;

    [[nodiscard]] const Entity* resolveEntity(const std::string& name) const;
    [[nodiscard]] const ItemStack* resolveItem(const std::string& name) const;
};

/**
 * @brief 上下文感知谓词（对齐 vanilla ContextAwarePredicate）
 *
 * 两种 JSON 形态：
 *  - 单对象：`{"type": "minecraft:player", ...}`（EntityPredicate 直接写法）；
 *  - 条件列表：`[{"condition": "minecraft:entity_properties", "entity": "this",
 *               "predicate": {"type": "minecraft:wolf"}}, ...]`。
 *
 * 列表中元素为 AND 语义；已知条件类型：entity_properties / location_check /
 * match_tool / block_state_property / all_of / any_of / inverted。
 */
class ContextAwarePredicate {
public:
    ContextAwarePredicate() = default;

    [[nodiscard]] bool isAny() const noexcept { return m_empty; }
    [[nodiscard]] bool test(const CriterionContext& context) const;

    static Result<ContextAwarePredicate> fromJson(const nlohmann::json& json);

    [[nodiscard]] nlohmann::json toJson() const;

private:
    /// 已知条件的内部表示
    enum class Kind : u8 {
        EntityProperties,
        LocationCheck,
        MatchTool,
        BlockStateProperty,
        AllOf,
        AnyOf,
        Inverted,
    };

    struct Condition {
        Kind kind = Kind::EntityProperties;
        /// block_state_property 的方块 ID
        ResourceLocation blockId;
        /// location_check 的坐标偏移（vanilla offsetX/offsetY/offsetZ）
        Vector3 offset;
        /// block_state_property 期望的方块状态（复用战利品侧 StatePropertiesPredicate）
        std::optional<class StatePropertiesPredicate> blockState;
        /// entity_properties / location_check 的检查对象名（"this" / "killer" 等）
        std::string target;
        /// entity_properties 的实体谓词
        std::optional<EntityPredicate> entityPredicate;
        /// location_check 的位置谓词
        std::optional<LocationPredicate> locationPredicate;
        /// match_tool 的物品谓词
        std::optional<ItemPredicate> itemPredicate;
        /// all_of / any_of / inverted 的子条件（用 shared_ptr 打破不完整类型递归）
        std::vector<std::shared_ptr<Condition>> children;
    };

    static Result<std::shared_ptr<Condition>> _parseCondition(const nlohmann::json& json);

    std::vector<std::shared_ptr<Condition>> m_conditions;
    bool m_empty = true;
};

} // namespace advancement
} // namespace mc