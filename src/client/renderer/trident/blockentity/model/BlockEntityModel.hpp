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
#include <memory>
#include <string>
#include <vector>

namespace mc::client::renderer::blockentity::model {

/**
 * @brief 方块实体模型基类
 *
 * 提供方块实体模型的通用功能，如部件管理、动画和网格生成。
 * 与 EntityModel 不同，方块实体模型更注重：
 * - 部件旋转动画（如箱子盖子开合）
 * - 方块坐标系的变换
 * - 方块光照系统
 */
class BlockEntityModel {
public:
    BlockEntityModel();
    virtual ~BlockEntityModel() = default;

    // 禁止拷贝
    BlockEntityModel(const BlockEntityModel&) = delete;
    BlockEntityModel& operator=(const BlockEntityModel&) = delete;

    // 允许移动
    BlockEntityModel(BlockEntityModel&&) noexcept = default;
    BlockEntityModel& operator=(BlockEntityModel&&) noexcept = default;

    // ========== 部件管理 ==========

    /**
     * @brief 创建并添加部件
     * @param name 部件名称
     * @param textureWidth 纹理宽度
     * @param textureHeight 纹理高度
     * @return 创建的部件
     */
    std::shared_ptr<entity::model::ModelRenderer> createPart(
        const std::string& name, i32 textureWidth, i32 textureHeight);

    /**
     * @brief 添加已有部件
     * @param part 部件
     */
    void addPart(std::shared_ptr<entity::model::ModelRenderer> part);

    /**
     * @brief 获取所有部件
     */
    [[nodiscard]] const std::vector<std::shared_ptr<entity::model::ModelRenderer>>& getParts() const { return m_parts; }

    // ========== 渲染 ==========

    /**
     * @brief 生成模型网格
     * @param vertices 顶点输出缓冲区
     * @param indices 索引输出缓冲区
     * @param scale 缩放因子（默认 1/16）
     */
    virtual void generateMesh(
        std::vector<entity::model::ModelVertex>& vertices, std::vector<u32>& indices, f64 scale = 1.0 / 16.0) const;

    // ========== 纹理 ==========

    [[nodiscard]] i32 textureWidth() const { return m_textureWidth; }
    [[nodiscard]] i32 textureHeight() const { return m_textureHeight; }

    void setTextureSize(i32 width, i32 height)
    {
        m_textureWidth = width;
        m_textureHeight = height;
    }

    // ========== 可见性 ==========

    /**
     * @brief 设置所有部件可见性
     */
    virtual void setAllVisible(bool visible);

protected:
    i32 m_textureWidth = 64;
    i32 m_textureHeight = 64;
    std::vector<std::shared_ptr<entity::model::ModelRenderer>> m_parts;
};

} // namespace mc::client::renderer::blockentity::model
