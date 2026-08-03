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

#include "BlockEntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <memory>

namespace mc::client::renderer::blockentity::model {

/**
 * @brief 箱子模型
 *
 * 渲染箱子的三个部件：箱体、盖子、锁扣。
 * 支持单箱和双箱模式。
 *
 * 纹理尺寸：64x64
 */
class ChestModel : public BlockEntityModel {
public:
    /**
     * @brief 箱子类型
     */
    enum class ChestType : u8 {
        Single, ///< 单箱
        Left,   ///< 双箱左半
        Right   ///< 双箱右半
    };

    ChestModel();
    ~ChestModel() override = default;

    // 禁止拷贝
    ChestModel(const ChestModel&) = delete;
    ChestModel& operator=(const ChestModel&) = delete;

    // 允许移动
    ChestModel(ChestModel&&) noexcept = default;
    ChestModel& operator=(ChestModel&&) noexcept = default;

    // ========== 部件访问 ==========

    /**
     * @brief 获取箱体部件
     */
    [[nodiscard]] std::shared_ptr<entity::model::ModelRenderer> getBottom() const { return m_bottom; }

    /**
     * @brief 获取盖子部件
     */
    [[nodiscard]] std::shared_ptr<entity::model::ModelRenderer> getLid() const { return m_lid; }

    /**
     * @brief 获取锁扣部件
     */
    [[nodiscard]] entity::model::ModelRenderer* getLatch() const { return m_latch.get(); }

    // ========== 动画 ==========

    /**
     * @brief 设置盖子角度
     * @param angle 角度 (0.0 = 关闭, 1.0 = 完全打开)
     *
     * 应用缓动函数后转换为弧度，范围 0 到 PI/2
     */
    void setLidAngle(f32 angle);

    /**
     * @brief 应用缓动函数
     * @param angle 原始角度 (0.0-1.0)
     * @return 缓动后的角度
     */
    [[nodiscard]] static f32 applyEasing(f32 angle);

    // ========== 类型切换 ==========

    /**
     * @brief 设置箱子类型
     * @param type 箱子类型
     *
     * 切换到对应的双箱模型部件。
     */
    void setChestType(ChestType type);

    /**
     * @brief 获取当前箱子类型
     */
    [[nodiscard]] ChestType getChestType() const { return m_chestType; }

private:
    /**
     * @brief 初始化单箱模型部件
     */
    void _initSingleChest();

    /**
     * @brief 初始化双箱左半部件
     */
    void _initLeftChest();

    /**
     * @brief 初始化双箱右半部件
     */
    void _initRightChest();

    // 单箱部件
    std::shared_ptr<entity::model::ModelRenderer> m_singleBottom;
    std::shared_ptr<entity::model::ModelRenderer> m_singleLid;
    std::shared_ptr<entity::model::ModelRenderer> m_singleLatch;

    // 双箱左半部件
    std::shared_ptr<entity::model::ModelRenderer> m_leftBottom;
    std::shared_ptr<entity::model::ModelRenderer> m_leftLid;
    std::shared_ptr<entity::model::ModelRenderer> m_leftLatch;

    // 双箱右半部件
    std::shared_ptr<entity::model::ModelRenderer> m_rightBottom;
    std::shared_ptr<entity::model::ModelRenderer> m_rightLid;
    std::shared_ptr<entity::model::ModelRenderer> m_rightLatch;

    // 当前使用的部件（根据类型）
    std::shared_ptr<entity::model::ModelRenderer> m_bottom;
    std::shared_ptr<entity::model::ModelRenderer> m_lid;
    std::shared_ptr<entity::model::ModelRenderer> m_latch;

    ChestType m_chestType = ChestType::Single;

    // 纹理尺寸常量
    static constexpr i32 TEXTURE_WIDTH = 64;
    static constexpr i32 TEXTURE_HEIGHT = 64;
};

} // namespace mc::client::renderer::blockentity::model
