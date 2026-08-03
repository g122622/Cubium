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

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <string>
#include <utility>

namespace mc {
namespace entity {
namespace attribute {
class AttributeMap;
class AttributeInstance;
} // namespace attribute
} // namespace entity

class Entity;
class LivingEntity;

namespace command {

/**
 * @brief /attribute 命令
 *
 * 查询和修改活体实体属性。支持所有 LivingEntity（玩家、僵尸、马等）。
 * 权限等级: 2 (游戏管理员)
 *
 * 用法:
 * - /attribute <target> <attribute> get [scale]
 * - /attribute <target> <attribute> base set <value>
 * - /attribute <target> <attribute> base get [scale]
 * - /attribute <target> <attribute> base reset
 * - /attribute <target> <attribute> modifier add <id> <value> (add_value|add_multiplied_base|add_multiplied_total)
 * - /attribute <target> <attribute> modifier remove <id>
 * - /attribute <target> <attribute> modifier value get <id> [scale]
 */
class AttributeCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    // 子命令处理
    static i32 _getAttribute(CommandContext<ServerCommandSource>& context);
    static i32 _getAttributeWithScale(CommandContext<ServerCommandSource>& context);
    static i32 _getBaseValue(CommandContext<ServerCommandSource>& context);
    static i32 _getBaseValueWithScale(CommandContext<ServerCommandSource>& context);
    static i32 _setBaseValue(CommandContext<ServerCommandSource>& context);
    static i32 _resetBaseValue(CommandContext<ServerCommandSource>& context);
    static i32 _addModifierAddValue(CommandContext<ServerCommandSource>& context);
    static i32 _addModifierMultiplyBase(CommandContext<ServerCommandSource>& context);
    static i32 _addModifierMultiplyTotal(CommandContext<ServerCommandSource>& context);
    static i32 _removeModifier(CommandContext<ServerCommandSource>& context);
    static i32 _getModifierValue(CommandContext<ServerCommandSource>& context);
    static i32 _getModifierValueWithScale(CommandContext<ServerCommandSource>& context);

    // 内部辅助
    static i32 _addModifierImpl(CommandContext<ServerCommandSource>& context, entity::attribute::Operation operation);

    // 实体获取辅助方法
    static bool _tryGetLivingEntityWithAttribute(CommandContext<ServerCommandSource>& context,
        ServerCommandSource& source,
        const std::string& attrName,
        LivingEntity*& outLivingEntity,
        entity::attribute::AttributeMap*& outAttrMap);

    static bool _tryGetAttributeInstance(CommandContext<ServerCommandSource>& context,
        ServerCommandSource& source,
        const std::string& attrName,
        LivingEntity*& outLivingEntity,
        entity::attribute::AttributeInstance*& outInstance);

    // 辅助方法
    static std::string _getEntityDisplayName(Entity* entity);
    static std::string _getEntityDisplayName(LivingEntity* livingEntity);
    static std::string _normalizeAttributeName(const std::string& name);
    static bool _isKnownAttribute(const std::string& name) noexcept;
    static std::pair<f64, f64> _getAttributeRange(const std::string& name) noexcept;
};

} // namespace command
} // namespace mc
