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

#include "common/core/Result.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include <functional>
#include <memory>
#include <set>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

namespace world {
namespace gen {
namespace feature {
namespace tree {
namespace decorator {

class TreeDecorator;

/**
 * @brief 树木装饰器执行上下文（MC TreeDecorator.Context）
 *
 * 持有世界读取器、方块写入回调、随机源，以及本轮装饰涉及的 logs/leaves/roots
 * 坐标集合。 FallenTreeFeature 仅产生 logs（无 leaves/roots），decorator 据此
 * 在 log 周围放置藤蔓/蘑菇等附属方块。
 *
 * 与 MC 一致：logs/leaves/roots 在构造时按 Y 坐标排序。
 */
class TreeDecoratorContext {
public:
    /// 把装饰方块写入世界，触发完整更新。
    using DecorationSetter = std::function<void(const BlockPos&, const BlockState*)>;

    TreeDecoratorContext(WorldGenRegion& region,
        DecorationSetter setter,
        math::IRandom& random,
        std::vector<BlockPos> logs,
        std::vector<BlockPos> leaves,
        std::vector<BlockPos> roots);

    /// MC TreeDecorator.Context.setBlock。同时把该位置记入本轮装饰已放置集合。
    void setBlock(const BlockPos& pos, const BlockState* state) const
    {
        m_setter(pos, state);
        // MC: this.decorationPositions.add(pos.immutable())
        m_decorationPositions.insert(BlockPos::asLong(pos.x, pos.y, pos.z));
    }

    /// MC TreeDecorator.Context.placeVine：放默认藤蔓并把 face 属性置 true。
    void placeVine(const BlockPos& pos, const BooleanProperty& face) const;

    /// MC TreeDecorator.Context.isAir。
    [[nodiscard]] bool isAir(const BlockPos& pos) const;

    /**
     * @brief MC TreeDecorator.Context.checkBlock
     *
     * 判定该位置是否满足谓词。除世界当前方块外，还须把**本轮装饰已放置**的方块
     * 视为满足——原版用 decorationPositions 集合实现这一点。缺少该集合时，
     * 后执行的装饰器看不见先执行的装饰器刚放下的方块（如 place_on_ground 放置的
     * 树叶散落不会被后续装饰器感知），行为与原版不一致。
     */
    [[nodiscard]] bool checkBlock(const BlockPos& pos, const std::function<bool(const BlockState&)>& predicate) const;

    /// 本轮装饰已放置的方块位置（BlockPos::asLong 打包），供 checkBlock 查询。
    [[nodiscard]] const std::set<i64>& decorationPositions() const noexcept { return m_decorationPositions; }

    [[nodiscard]] WorldGenRegion& region() const noexcept { return m_region; }
    [[nodiscard]] math::IRandom& random() const noexcept { return m_random; }
    [[nodiscard]] const std::vector<BlockPos>& logs() const noexcept { return m_logs; }
    [[nodiscard]] const std::vector<BlockPos>& leaves() const noexcept { return m_leaves; }
    [[nodiscard]] const std::vector<BlockPos>& roots() const noexcept { return m_roots; }

private:
    WorldGenRegion& m_region;
    DecorationSetter m_setter;
    math::IRandom& m_random;
    std::vector<BlockPos> m_logs;
    std::vector<BlockPos> m_leaves;
    std::vector<BlockPos> m_roots;

    /// MC TreeDecorator.Context.decorationPositions。mutable 是因为原版 setBlock
    /// 会写该集合，而本项目的 place(const Context&) 以 const 引用传递上下文。
    mutable std::set<i64> m_decorationPositions;
};

/**
 * @brief 树木装饰器基类（MC TreeDecorator）
 *
 * 子类实现 place()，依据 Context.logs() 在原木周围放置藤蔓、蘑菇等。
 * 已实现：attached_to_logs、trunk_vine（fallen_tree 使用）。
 */
class TreeDecorator {
public:
    virtual ~TreeDecorator() = default;

    /// MC TreeDecorator.place。
    virtual void place(const TreeDecoratorContext& context) const = 0;
};

/**
 * @brief 树木装饰器 JSON 解析器
 *
 * 按 type 字段派发到对应子类：
 *   {"type":"minecraft:trunk_vine"}                      → TrunkVineDecorator（无配置）
 *   {"type":"minecraft:attached_to_logs",
 *    "probability":F,"block_provider":{...},"directions":["up",...]}
 *                                                        → AttachToLogsDecorator
 * 未识别的 type 返回 Error（严格报错，便于定位缺口）。
 */
[[nodiscard]] Result<std::unique_ptr<TreeDecorator>> parseDecorator(const nlohmann::json& decoratorJson);

} // namespace decorator
} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
