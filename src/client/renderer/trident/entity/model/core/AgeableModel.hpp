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

#include "EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model {

/**
 * @brief 可成长模型基类
 *
 * 支持幼体和成年两种状态的模型。
 * 幼体通常有更大的头部比例和更小的身体。
 */
class AgeableModel : public EntityModel {
public:
    /**
     * @brief 默认构造函数
     */
    AgeableModel();

    /**
     * @brief 带参数的构造函数
     * @param isChildHeadScaled 是否缩放头部
     * @param childHeadOffsetY 头部 Y 偏移
     * @param childHeadOffsetZ 头部 Z 偏移
     */
    AgeableModel(bool isChildHeadScaled, f32 childHeadOffsetY, f32 childHeadOffsetZ);

    /**
     * @brief 完整参数的构造函数
     * @param isChildHeadScaled 是否缩放头部
     * @param childHeadOffsetY 头部 Y 偏移
     * @param childHeadOffsetZ 头部 Z 偏移
     * @param childHeadScale 头部缩放
     * @param childBodyScale 身体缩放
     * @param childBodyOffsetY 身体 Y 偏移
     */
    AgeableModel(bool isChildHeadScaled,
        f32 childHeadOffsetY,
        f32 childHeadOffsetZ,
        f32 childHeadScale,
        f32 childBodyScale,
        f32 childBodyOffsetY);

    ~AgeableModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    /**
     * @brief 按原版幼体头身分离矩阵生成模型网格
     * @param vertices 顶点输出缓冲区
     * @param indices 索引输出缓冲区
     * @param scale 模型空间到渲染空间的缩放因子
     */
    void generateMesh(std::vector<ModelVertex>& vertices, std::vector<u32>& indices, f64 scale) const override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     *
     * 用于在每帧设置模型状态（位置、状态变量）
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

    // ========== 幼体状态 ==========

    /**
     * @brief 设置幼体状态
     * @param isChild 是否为幼体
     */
    void setChild(bool isChild) { m_isChild = isChild; }

    /**
     * @brief 是否为幼体
     */
    [[nodiscard]] bool isChild() const { return m_isChild; }

protected:
    /**
     * @brief 获取头部部件
     * @return 头部模型部件列表
     */
    virtual std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const { return {}; }

    /**
     * @brief 获取身体部件
     * @return 身体模型部件列表
     */
    virtual std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const { return m_parts; }

    // ========== 幼体参数 ==========
    bool m_isChildHeadScaled = false;
    f32 m_childHeadOffsetY = 5.0f;
    f32 m_childHeadOffsetZ = 2.0f;
    f32 m_childHeadScale = 2.0f;    // 头部缩放
    f32 m_childBodyScale = 2.0f;    // 身体缩放
    f32 m_childBodyOffsetY = 24.0f; // 身体 Y 偏移

    bool m_isChild = false;
};

} // namespace mc::client::renderer::entity::model
