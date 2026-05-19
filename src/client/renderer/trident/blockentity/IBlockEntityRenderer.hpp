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

#include "common/core/Types.hpp"
#include <memory>

namespace mc {

class MatrixStack;
class BlockEntity;

namespace client::renderer::trident::blockentity {

/**
 * @brief 方块实体渲染器基类（类型擦除）
 *
 * 用于存储不同类型的方块实体渲染器。
 * 参考 MC 1.16.5 TileEntityRenderer
 */
class BlockEntityRendererBase {
public:
    virtual ~BlockEntityRendererBase() = default;

    /**
     * @brief 渲染方块实体（类型擦除接口）
     *
     * @param entity 方块实体引用（基类类型）
     * @param partialTick 帧间插值系数（0.0-1.0）
     * @param light 组合光照值（天空光 << 4 | 方块光 << 20）
     * @return 是否成功渲染
     */
    virtual bool render(const BlockEntity& entity, f32 partialTick, u32 light) = 0;

    /**
     * @brief 是否为全局渲染器
     *
     * 全局渲染器可以在任意距离看到（如信标光束）。
     * 普通渲染器受渲染距离限制。
     */
    [[nodiscard]] virtual bool isGlobalRenderer() const { return false; }

    /**
     * @brief 获取最大渲染距离平方
     * @return 渲染距离平方（方块数），默认64（8格）
     */
    [[nodiscard]] virtual f64 getMaxRenderDistanceSquared() const { return 64.0; }
};

/**
 * @brief 方块实体渲染器模板
 *
 * 提供类型安全的渲染接口。
 * 参考 MC 1.16.5 TileEntityRenderer
 *
 * @tparam TEntity 方块实体类型
 */
template <typename TEntity>
class BlockEntityRenderer : public BlockEntityRendererBase {
public:
    /**
     * @brief 渲染方块实体（类型安全接口）
     *
     * @param entity 方块实体实例
     * @param partialTick 帧间插值系数（0.0-1.0）
     * @param light 组合光照值
     */
    virtual void render(const TEntity& entity, f32 partialTick, u32 light) = 0;

    /**
     * @brief 渲染方块实体（类型擦除接口实现）
     *
     * 执行类型安全转换后调用类型安全的render方法。
     */
    bool render(const BlockEntity& entity, f32 partialTick, u32 light) override
    {
        const TEntity* typedEntity = dynamic_cast<const TEntity*>(&entity);
        if (typedEntity == nullptr) {
            return false;
        }
        render(*typedEntity, partialTick, light);
        return true;
    }
};

} // namespace client::renderer::trident::blockentity
} // namespace mc
