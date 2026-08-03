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

#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector4.hpp"
#include <vector>

namespace mc {
class Entity;
}

namespace mc::client::renderer::entity::effect::glow {

/**
 * @brief 发光效果管理器
 *
 * 用于渲染实体的发光轮廓（如发光药水效果、团队成员发光）。
 *
 * 发光效果来源：
 * 1. 发光药水效果 (EffectType::Glowing)
 * 2. Entity::isGlowing() 标志位
 * 3. 团队规则（通过 Entity::getTeam() 获取团队颜色）
 */
class GlowEffect {
public:
    /**
     * @brief 初始化发光效果系统
     */
    static void initialize();

    /**
     * @brief 清理发光效果系统
     */
    static void cleanup();

    /**
     * @brief 检查实体是否有发光效果
     * @param entity 实体
     * @return 是否有发光效果
     */
    [[nodiscard]] static bool hasGlowEffect(Entity& entity);

    /**
     * @brief 获取发光颜色
     * @param entity 实体
     * @return 发光颜色 (RGBA)
     *
     * 默认颜色为白色 (1, 1, 1, 1)。
     * 如果实体在团队中且有团队颜色，则返回团队颜色。
     */
    [[nodiscard]] static math::Vector4f getGlowColor(Entity& entity);

    /**
     * @brief 渲染发光轮廓
     * @param entity 实体
     * @param partialTicks 部分tick
     * @param color 发光颜色
     */
    static void renderGlow(Entity& entity, f64 partialTicks, const math::Vector4f& color);

    /**
     * @brief 渲染所有发光实体
     * @param partialTicks 部分tick
     *
     * 遍历所有发光实体并渲染轮廓。
     */
    static void renderAllGlowing(f64 partialTicks);

private:
    GlowEffect() = delete;
    ~GlowEffect() = delete;

    /**
     * @brief 生成发光轮廓网格
     */
    static void _generateGlowMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices, f64 scale);

    static bool s_initialized;
};

} // namespace mc::client::renderer::entity::effect::glow
