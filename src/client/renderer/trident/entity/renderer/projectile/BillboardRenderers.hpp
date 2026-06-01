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

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <vector>

namespace mc {
class Entity;
}

namespace mc::client {
class ClientEntity;
}

namespace mc::client::renderer::entity::renderer::projectile {

/**
 * @brief 物品 billboard 渲染器基类
 *
 * 所有使用物品纹理渲染为 billboard 四边形的投掷物共享此基类。
 * 子类只需设置 m_fullbright 和 m_scale 即可。
 */
class ItemBillboardRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    /**
     * @param fullbright 是否使用全亮光照（黑暗中也清晰可见）
     * @param scale 渲染缩放比例
     */
    ItemBillboardRenderer(bool fullbright, f64 scale);
    ~ItemBillboardRenderer() override = default;

    ItemBillboardRenderer(const ItemBillboardRenderer&) = delete;
    ItemBillboardRenderer& operator=(const ItemBillboardRenderer&) = delete;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] core::PipelineMeshProvider* getPipelineMeshProvider() override { return this; }

    // ========== PipelineMeshProvider 接口 ==========

    [[nodiscard]] bool generateMesh(::mc::client::ClientEntity& entity,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices) override;

    [[nodiscard]] bool needsMeshUpdate(::mc::client::ClientEntity& entity) const override;

protected:
    bool m_fullbright;
    f64 m_scale;
};

/**
 * @brief 雪球渲染器
 */
class SnowballRenderer : public ItemBillboardRenderer {
public:
    SnowballRenderer()
        : ItemBillboardRenderer(false, 1.0)
    {}
};

/**
 * @brief 鸡蛋渲染器
 */
class EggRenderer : public ItemBillboardRenderer {
public:
    EggRenderer()
        : ItemBillboardRenderer(false, 1.0)
    {}
};

/**
 * @brief 末影珍珠渲染器
 */
class EnderPearlRenderer : public ItemBillboardRenderer {
public:
    EnderPearlRenderer()
        : ItemBillboardRenderer(false, 1.0)
    {}
};

/**
 * @brief 药水渲染器
 */
class PotionRenderer : public ItemBillboardRenderer {
public:
    PotionRenderer()
        : ItemBillboardRenderer(false, 1.0)
    {}
};

/**
 * @brief 附魔之瓶渲染器
 */
class ExperienceBottleRenderer : public ItemBillboardRenderer {
public:
    ExperienceBottleRenderer()
        : ItemBillboardRenderer(false, 1.0)
    {}
};

/**
 * @brief 末影之眼渲染器
 *
 * 末影之眼使用全亮光照（fullbright），使其在黑暗中也清晰可见。
 */
class EyeOfEnderRenderer : public ItemBillboardRenderer {
public:
    EyeOfEnderRenderer()
        : ItemBillboardRenderer(true, 1.0)
    {}
};

} // namespace mc::client::renderer::entity::renderer::projectile
