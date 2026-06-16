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
 * The above copyright notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <vector>

namespace mc {

class Entity;

namespace command::support {

/**
 * @brief 解析实体选择器，返回匹配的实体列表。
 *
 * 与 PlayerResolver 不同，EntityResolver 可以解析 @e 选择器并返回
 * 非玩家实体（僵尸、动物、掉落物等），同时完全支持 @p/@a/@r/@s 选择器。
 *
 * 支持的选择器参数：
 * - type=/type=!  实体类型过滤
 * - tag=/tag=!    实体标签过滤
 * - team=/team=!  队伍过滤
 * - name=/name=!  名称过滤
 * - distance=     距离过滤
 * - dx/dy/dz      体积过滤
 * - x_rotation=   俯仰角过滤
 * - y_rotation=   偏航角过滤
 * - level=        等级过滤（仅玩家）
 * - gamemode=     游戏模式过滤（仅玩家）
 * - scores=       记分板分数过滤（仅玩家）
 * - advancements= 进度过滤（仅玩家）
 * - sort=         排序方式
 * - limit=        数量限制
 *
 * 当前不支持的选择器参数：
 * - nbt=          NBT 条件过滤（待 Entity 实现 serializeNBT()）
 * - predicate=    谓词条件过滤（待 LootConditionManager 集成）
 */
class EntityResolver {
public:
    /**
     * @brief 解析实体选择器，返回所有匹配的实体列表。
     *
     * @param source 命令源
     * @param selector 实体选择器
     * @return 匹配的实体指针列表
     */
    [[nodiscard]] static std::vector<Entity*> resolve(
        const ServerCommandSource& source, const EntitySelector& selector);

    /**
     * @brief 解析实体选择器，返回单个匹配实体。
     *
     * 如果选择器匹配多个实体，返回第一个（根据排序方式决定）。
     * 如果没有匹配的实体，返回 nullptr。
     *
     * @param source 命令源
     * @param selector 实体选择器
     * @return 匹配的实体指针，无匹配时返回 nullptr
     */
    [[nodiscard]] static Entity* resolveSingle(const ServerCommandSource& source, const EntitySelector& selector);
};

} // namespace command::support
} // namespace mc
