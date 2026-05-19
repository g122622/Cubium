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
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/model/animal/OcelotModel.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {
class OcelotEntity;
}

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 豹猫渲染器
 *
 * 参考 MC 1.16.5 OcelotRenderer
 */
class OcelotRenderer : public core::EntityRenderer {
public:
    OcelotRenderer();
    ~OcelotRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取豹猫纹理
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::OcelotEntity& entity);
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::OcelotEntity& entity) const;

private:
    model::animal::OcelotModel m_model;
    model::animal::OcelotModel m_modelBaby;
};

/**
 * @brief 注册豹猫渲染器
 */
void registerOcelotRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
