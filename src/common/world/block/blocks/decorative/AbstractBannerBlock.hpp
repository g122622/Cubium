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

#include "common/util/assert/AssertMacros.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/blockentity/interactive/BannerEntity.hpp"

#include <cstddef>
#include <memory>
#include <unordered_map>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockPos;
class BlockEntity;

namespace blocks {

/**
 * @brief 旗帜抽象基类
 *
 * 提供旗帜方块的通用功能，包括颜色管理、方块实体创建。
 * 子类：StandingBannerBlock（站立旗帜）和 WallBannerBlock（墙壁旗帜）
 *
 * vanilla 1.21.11 旗帜不持有 waterlogged 属性（不实现 SimpleWaterloggedBlock），
 * 因此本项目旗帜同样不可含水，状态容器仅含 rotation/facing。
 *
 */
class AbstractBannerBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param color 旗帜底色
     */
    AbstractBannerBlock(const BlockProperties& properties, DyeColor color);

    ~AbstractBannerBlock() override = default;

    // ========== 颜色 ==========

    /**
     * @brief 获取旗帜底色
     * @return 染料颜色
     */
    [[nodiscard]] DyeColor getColor() const noexcept
    {
        return m_color;
    }

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    /**
     * @brief 创建旗帜方块实体
     *
     * 创建 BannerEntity 并设置底色为方块的颜色。
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 放置回调 ==========

    /**
     * @brief 方块放置后回调
     *
     * 检查方块实体是否需要从物品数据加载。
     *
     * @param stack 放置该方块的物品堆（可能携带自定义名称等组件）
     */
    void onBlockPlacedBy(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const Entity* placer,
        const ItemStack& stack) override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

protected:
    /// 旗帜底色
    DyeColor m_color;
};

} // namespace blocks
} // namespace mc
