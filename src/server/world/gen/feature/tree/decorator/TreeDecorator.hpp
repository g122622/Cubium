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
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// ChunkPrimer 实际定义在 mc::world::chunk；须用限定前向声明 + using 引入，
// 否则会在 mc 下声明出一个同名但不同的类型，使各 override 退化为隐藏重载。
namespace world::chunk {
class ChunkPrimer;
}

using world::chunk::ChunkPrimer;

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

    /// chunk/generator 可空：FallenTreeFeature 路径不持有它们（其装饰器不需要委派子特征）。
    TreeDecoratorContext(WorldGenRegion& region,
        ChunkPrimer* chunk,
        IChunkGenerator* generator,
        DecorationSetter setter,
        math::IRandom& random,
        std::vector<BlockPos> logs,
        std::vector<BlockPos> leaves,
        std::vector<BlockPos> roots);

    /// 将装饰方块写入世界，后续条件读取写入后的实际状态。
    void setBlock(const BlockPos& pos, const BlockState* state) const
    {
        m_setter(pos, state);
    }

    /// MC TreeDecorator.Context.placeVine：放默认藤蔓并把 face 属性置 true。
    void placeVine(const BlockPos& pos, const BooleanProperty& face) const;

    /// MC TreeDecorator.Context.isAir。
    [[nodiscard]] bool isAir(const BlockPos& pos) const;

    /**
     * @brief 检查世界中当前方块是否满足谓词
     *
     * 写入历史不改变条件：新放的落叶仍不是实心地面，藤蔓仍不是原木。
     */
    [[nodiscard]] bool checkBlock(const BlockPos& pos, const std::function<bool(const BlockState&)>& predicate) const;

    [[nodiscard]] WorldGenRegion& region() const noexcept
    {
        return m_region;
    }

    /// 供需要委派给其它 configured_feature 的装饰器使用（如 pale_moss 调用 pale_moss_patch）。
    [[nodiscard]] ChunkPrimer* chunk() const noexcept
    {
        return m_chunk;
    }

    [[nodiscard]] IChunkGenerator* generator() const noexcept
    {
        return m_generator;
    }

    [[nodiscard]] math::IRandom& random() const noexcept
    {
        return m_random;
    }

    [[nodiscard]] const std::vector<BlockPos>& logs() const noexcept
    {
        return m_logs;
    }

    [[nodiscard]] const std::vector<BlockPos>& leaves() const noexcept
    {
        return m_leaves;
    }

    [[nodiscard]] const std::vector<BlockPos>& roots() const noexcept
    {
        return m_roots;
    }

private:
    WorldGenRegion& m_region;
    ChunkPrimer* m_chunk;
    IChunkGenerator* m_generator;
    DecorationSetter m_setter;
    math::IRandom& m_random;
    std::vector<BlockPos> m_logs;
    std::vector<BlockPos> m_leaves;
    std::vector<BlockPos> m_roots;
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
