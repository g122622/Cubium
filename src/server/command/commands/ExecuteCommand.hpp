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
#include "server/command/ServerCommandSource.hpp"
#include <memory>
#include <string>

namespace mc {
namespace command {

/**
 * @brief /execute 命令
 *
 * 用法：
 * - /execute as <entity> run <command> - 以指定实体身份执行命令
 * - /execute at <entity> run <command> - 在指定实体位置执行命令
 * - /execute in <dimension> run <command> - 在指定维度执行命令
 * - /execute positioned <pos> run <command> - 在指定位置执行命令
 * - /execute run <command> - 直接执行命令
 * - /execute if block <pos> <block> run <command> - 如果指定位置是指定方块则执行
 * - /execute unless block <pos> <block> run <command> - 如果指定位置不是指定方块则执行
 * - /execute align <axes> run <command> - 对齐坐标后执行命令（待实现）
 * - /execute facing <pos/entity> run <command> - 朝向指定方向执行命令（待实现）
 * - /execute rotated <rot/as> run <command> - 旋转后执行命令（待实现）
 * - /execute anchored <anchor> run <command> - 锚定后执行命令（待实现）
 *
 * 权限等级：2
 */
class ExecuteCommand {
public:
    /// 注册命令及执行上下文修改器。
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

    /// 执行一条嵌套命令（/execute run 与 /return run 共用）。
    static i32 executeNestedCommand(ServerCommandSource& source, const std::string& command);

private:
    static i32 _executeForwarded(CommandContext<ServerCommandSource>& context);

    // ========== 条件求值（/execute if|unless） ==========

    /**
     * @brief 求值 `if block <pos> <block>` 条件
     *
     * 判定 pos 处的方块是否匹配给定的方块谓词。谓词中显式写出的属性必须一致，
     * 未写出的属性不做限制（对齐 vanilla 的 BlockPredicate 语义）。
     *
     * 若上下文里没有 pos / block 参数（说明是其余尚未实现求值的条件名），返回 false。
     * 注意：`biome` 条件同样带 `pos` 参数，扩展求值时仅凭参数存在性无法区分两者。
     *
     * @param target 当前命令源（用于解析相对坐标）
     * @param context 命令上下文
     * @return 条件是否成立
     */
    static bool _evaluateBlockCondition(
        const ServerCommandSource& target, CommandContext<ServerCommandSource>& context);

    /**
     * @brief 求值 `if score` 条件
     *
     * 两种形态：
     * - `score <target> <targetObjective> <op> <source> <sourceObjective>`：比较两侧计分板值
     * - `score <target> <targetObjective> matches <range>`：判断左侧值是否落在区间内
     *
     * 比较运算符是字面量节点，执行期从上下文键 `executeScoreConditionOp` 读出
     * （见 LiteralRecordingCommandNode）。任一侧计分板或目标值不存在时返回 false。
     *
     * @param target 当前命令源（用于取计分板）
     * @param context 命令上下文
     * @return 条件是否成立
     */
    static bool _evaluateScoreCondition(
        const ServerCommandSource& target, CommandContext<ServerCommandSource>& context);

    /**
     * @brief 求值 `if data` 条件
     *
     * 判定 NBT 路径是否至少命中一个标签。三种数据源由解析期写入的 `executeDataConditionType`
     * 区分（`block` / `entity` / `storage`）——`entity` 与 `storage` 共用参数名 `source`，
     * 只看参数无法区分。
     *
     * 当前实现了 `storage`；`block` / `entity` 需要方块实体与实体的 NBT 视图，尚未接通，
     * 一律返回 false。
     *
     * @param target 当前命令源
     * @param context 命令上下文
     * @return 条件是否成立
     */
    static bool _evaluateDataCondition(const ServerCommandSource& target, CommandContext<ServerCommandSource>& context);

    /**
     * @brief 求值 `if biome <pos> <biome>` 条件
     *
     * 取 pos 处的生物群系，判断它是否等于给定群系；若给的是标签（`#namespace:tag`），
     * 则判断该群系是否属于这个标签。
     *
     * 无世界时返回 false。
     *
     * @param target 当前命令源（用于解析相对坐标与取世界）
     * @param context 命令上下文
     * @return 条件是否成立
     */
    /**
     * @brief 求值 `if items <block <pos>|entity <entities>> <slots> <item_predicate>` 条件
     *
     * 按 SlotRanges 把槽位区间名展开成槽位索引，取出对应 ItemStack，逐个用
     * ItemPredicateInput::test 判定；任意一个非空且匹配即条件成立。
     *
     * 当前支持的槽位来源：方块容器（ContainerBlockEntity 的 IInventory）、
     * 玩家背包（PlayerInventory，0..40）、任意 LivingEntity 的装备槽（98..106）。
     * 末影箱/村民/马匹/合成等槽位族需要各自的容器视图，尚无通路，按不匹配处理。
     *
     * @param target 当前命令源
     * @param context 命令上下文
     * @return 条件是否成立
     */
    /**
     * @brief 求值 if predicate <id> 条件
     *
     * 从 LootTableManager 取战利品谓词，按与 /loot 相同的方式构建 LootContext
     * （世界 + 随机源 + 表/谓词解析器 + THIS_ENTITY + ORIGIN），再调用
     * LootCondition::test。
     *
     * 谓词标签形式（#tag）当前不支持——项目没有谓词标签注册表，按不成立处理并 warn。
     * 无世界或无 server 时返回 false。
     *
     * @param target 当前命令源
     * @param context 命令上下文
     * @return 条件是否成立
     */
    /**
     * @brief 求值 if function <name> 条件
     *
     * 执行指定函数，取它通过 /return 给出的返回值；返回值 > 0 才算成立。
     * 函数不存在时 warn 并返回 false；函数没有 return 时也返回 false（与原版一致）。
     *
     * @param target 当前命令源
     * @param context 命令上下文
     * @return 条件是否成立
     */
    static bool _evaluateFunctionCondition(
        const ServerCommandSource& target, CommandContext<ServerCommandSource>& context);

    static bool _evaluatePredicateCondition(
        const ServerCommandSource& target, CommandContext<ServerCommandSource>& context);

    static bool _evaluateItemsCondition(
        const ServerCommandSource& target, CommandContext<ServerCommandSource>& context);

    static bool _evaluateBiomeCondition(
        const ServerCommandSource& target, CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
