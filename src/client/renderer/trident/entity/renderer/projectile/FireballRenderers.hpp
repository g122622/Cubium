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

#include "BillboardRenderers.hpp"

namespace mc::client::renderer::entity::renderer::projectile {

/**
 * @brief 火球渲染器
 *
 * 渲染恶魂发射的火球实体。使用实体纹理渲染为 billboard 四边形，
 * 全亮光照（fullbright），缩放比例 3.0。
 */
class FireballRenderer : public ItemBillboardRenderer {
public:
    FireballRenderer()
        : ItemBillboardRenderer(true, 3.0)
    {}
};

/**
 * @brief 小火球渲染器
 *
 * 渲染烈焰人发射的小火球实体。使用实体纹理渲染为 billboard 四边形，
 * 全亮光照（fullbright），缩放比例 0.75。
 */
class SmallFireballRenderer : public ItemBillboardRenderer {
public:
    SmallFireballRenderer()
        : ItemBillboardRenderer(true, 0.75)
    {}
};

} // namespace mc::client::renderer::entity::renderer::projectile
