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

#include "GlowEffect.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/util/math/Vector4.hpp"
#include "common/util/text/TextStyle.hpp"
#include <vector>

namespace mc::client::renderer::entity::effect::glow {

bool GlowEffect::s_initialized = false;

void GlowEffect::initialize()
{
    if (s_initialized) {
        return;
    }

    // 需要：
    // 1. 创建发光帧缓冲区（用于渲染发光实体）
    // 2. 创建模糊帧缓冲区（用于模糊和膨胀）
    // 3. 加载发光着色器
    //
    // 当前等待渲染管线支持：
    // - 多渲染目标(MRT)
    // - 后处理管线
    // - 模糊着色器

    s_initialized = true;
}

void GlowEffect::cleanup()
{
    if (!s_initialized) {
        return;
    }

    // 清理发光效果系统资源
    // 当前无需清理，等待渲染管线支持后实现

    s_initialized = false;
}

bool GlowEffect::hasGlowEffect(Entity& entity)
{
    // 1. 检查 Entity 的发光标志（适用于所有实体）
    if (entity.isGlowing()) {
        return true;
    }

    // 2. 如果是 LivingEntity，检查发光药水效果
    if (auto* living = dynamic_cast<LivingEntity*>(&entity)) {
        if (living->hasEffect(::mc::entity::effect::EffectType::Glowing)) {
            return true;
        }
    }

    // 3. 团队发光规则检查
    // 如果实体在团队中，团队可以设置成员的发光效果
    // 当前实现：getTeam() 在 Entity 基类返回 nullptr，
    // ServerPlayer 子类重写该方法通过记分板获取团队
    // 注意：团队发光规则的具体实现需要额外的团队配置支持
    // Team* team = entity.getTeam();
    // if (team != nullptr) {
    //     // 检查团队是否配置了发光效果
    // }

    return false;
}

math::Vector4f GlowEffect::getGlowColor(Entity& entity)
{
    // 默认颜色为白色 (1, 1, 1, 1)
    // 特殊情况：
    // - 团队成员：团队颜色

    // 检查实体是否在团队中，使用团队颜色
    scoreboard::Team* team = entity.getTeam();
    if (team != nullptr) {
        // 获取团队颜色
        text::TextFormatting teamColor = team->getColor();

        // 将 TextFormatting 转换为 ARGB 颜色值
        u32 argb = text::getFormattingColor(teamColor);

        // 如果颜色有效（非白色默认值），返回团队颜色
        if (argb != 0xFFFFFFFF && text::isColor(teamColor)) {
            // 将 ARGB 转换为归一化的 Vector4f (RGBA, 0.0-1.0)
            return math::Vector4f(static_cast<f32>((argb >> 16) & 0xFF) / 255.0f, // R
                static_cast<f32>((argb >> 8) & 0xFF) / 255.0f,                    // G
                static_cast<f32>(argb & 0xFF) / 255.0f,                           // B
                1.0f                                                              // A
            );
        }
    }

    // 默认白色
    return math::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void GlowEffect::renderGlow(Entity& entity, f64 partialTicks, const math::Vector4f& color)
{
    // 步骤：
    // 1. 渲染实体到发光缓冲区（使用轮廓着色器）
    // 2. 应用高斯模糊（水平和垂直）
    // 3. 膨胀轮廓（使其比模型稍大）
    // 4. 将轮廓合成到主画面

    // 当前等待渲染管线支持：
    // - 发光缓冲区绑定
    // - 模糊着色器
    // - 膨胀着色器
    // - 合成着色器

    (void)entity;
    (void)partialTicks;
    (void)color;
}

void GlowEffect::renderAllGlowing(f64 partialTicks)
{
    // 1. 从世界获取所有发光实体
    // 2. 渲染到发光缓冲区
    // 3. 应用模糊和膨胀
    // 4. 合成到主画面

    // 当前需要：
    // - ClientWorld::getGlowingEntities()
    // - 后处理管线支持

    (void)partialTicks;
}

void GlowEffect::_generateGlowMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices, f64 scale)
{
    // 发光轮廓网格生成
    // 轮廓网格比原模型稍大（通过顶点法线外推实现膨胀效果）

    (void)vertices;
    (void)indices;
    (void)scale;
}

} // namespace mc::client::renderer::entity::effect::glow
