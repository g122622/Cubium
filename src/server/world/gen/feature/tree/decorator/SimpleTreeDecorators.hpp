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

#include "TreeDecorator.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "server/world/gen/feature/state/BlockStateProvider.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {
namespace decorator {

/**
 * @brief 轻量树木装饰器族（Beehive / LeaveVine / AttachedToLeaves / AlterGround / Cocoa）
 *
 * 这五个装饰器彼此独立、体量小（各自只有一个 place()），且都只依赖 TreeDecoratorContext
 * 的公开接口，故聚合在同一翻译单元中，避免为每个不足百行的类型各开一对文件。
 * 逻辑与随机数消耗顺序均逐行对齐原版对应类，详见各 place() 内注释。
 *
 * 【为什么必须实现它们而不是"未识别就跳过"】数据包中树型的 decorators 列表是整体
 * 解析的：任一成员解析失败都会让整棵 configured_feature 载入失败，进而该树种彻底
 * 不生成——表现为 oak/birch 的原木与树叶全部消失，比"少一个装饰器"严重得多。
 */

/**
 * @brief 蜂巢装饰器（MC BeehiveDecorator）
 *
 * 满足概率后在原木上挂一个蜂巢（默认朝南）。注意原版在放置后会向蜂巢的
 * BlockEntity 写入随机数量的蜜蜂，**该写入会消耗随机数**，因此即便本项目的
 * worldgen 阶段尚未创建蜂巢 BlockEntity，也必须照数消耗以保持后续装饰器的
 * 随机流一致。
 */
class BeehiveDecorator final : public TreeDecorator {
public:
    explicit BeehiveDecorator(f32 probability);

    void place(const TreeDecoratorContext& context) const override;

private:
    f32 m_probability;
};

/**
 * @brief 树叶垂藤装饰器（MC LeaveVineDecorator）
 *
 * 对每片树叶，四个水平方向各以 probability 判定一次；命中且该方向为空时，
 * 放置带朝向的藤蔓并向下延伸最多 4 格。
 */
class LeaveVineDecorator final : public TreeDecorator {
public:
    explicit LeaveVineDecorator(f32 probability);

    void place(const TreeDecoratorContext& context) const override;

private:
    f32 m_probability;
};

/**
 * @brief 附着于树叶装饰器（MC AttachedToLeavesDecorator）
 *
 * 按随机顺序遍历树叶，随机取一个方向，若该方向相邻的 required_empty_blocks 格
 * 全为空且落在互斥半径之外，则放置方块并标记其 exclusion 半径为已占用。
 * 用于苍白花园的悬挂苔藓等。
 */
class AttachedToLeavesDecorator final : public TreeDecorator {
public:
    AttachedToLeavesDecorator(f32 probability,
        i32 exclusionRadiusXZ,
        i32 exclusionRadiusY,
        std::unique_ptr<state::BlockStateProvider> blockProvider,
        i32 requiredEmptyBlocks,
        std::vector<Direction> directions);

    void place(const TreeDecoratorContext& context) const override;

private:
    [[nodiscard]] bool _hasRequiredEmptyBlocks(
        const TreeDecoratorContext& context, const BlockPos& pos, Direction direction) const;

    f32 m_probability;
    i32 m_exclusionRadiusXZ;
    i32 m_exclusionRadiusY;
    std::unique_ptr<state::BlockStateProvider> m_blockProvider;
    i32 m_requiredEmptyBlocks;
    std::vector<Direction> m_directions;
};

/**
 * @brief 改造地面装饰器（MC AlterGroundDecorator）
 *
 * 以最低树干/树根为圆心，在四周铺开圆形的地面替换（用于灰化土等"树下地面"），
 * 并在外圈随机撒 5 个点。只替换草/土类方块。
 */
class AlterGroundDecorator final : public TreeDecorator {
public:
    explicit AlterGroundDecorator(std::unique_ptr<state::BlockStateProvider> provider);

    void place(const TreeDecoratorContext& context) const override;

private:
    void _placeCircle(const TreeDecoratorContext& context, const BlockPos& center) const;
    void _placeBlockAt(const TreeDecoratorContext& context, const BlockPos& pos) const;

    std::unique_ptr<state::BlockStateProvider> m_provider;
};

/**
 * @brief 可可豆装饰器（MC CocoaDecorator）
 *
 * 满足概率后，在最低原木以上 2 格内的原木侧面按 0.25 概率挂可可豆。
 */
class CocoaDecorator final : public TreeDecorator {
public:
    explicit CocoaDecorator(f32 probability);

    void place(const TreeDecoratorContext& context) const override;

private:
    f32 m_probability;
};

} // namespace decorator
} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
