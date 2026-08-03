/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software", to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY FOR ANY CLAIM, DAMAGES OR OTHER
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
 * @brief 旗帜模型
 *
 * 渲染旗帜的旗杆和旗帜面。
 * 支持站立旗帜和墙壁旗帜两种形态。
 *
 * 纹理尺寸：64x64
 */
class BannerModel : public BlockEntityModel {
public:
    /**
     * @brief 旗帜类型
     */
    enum class BannerType : u8 {
        Standing, ///< 站立旗帜（地面放置，16方向旋转）
        Wall      ///< 墙壁旗帜（贴墙放置，4方向朝向）
    };

    BannerModel();
    ~BannerModel() override = default;

    // 禁止拷贝
    BannerModel(const BannerModel&) = delete;
    BannerModel& operator=(const BannerModel&) = delete;

    // 允许移动
    BannerModel(BannerModel&&) noexcept = default;
    BannerModel& operator=(BannerModel&&) noexcept = default;

    // ========== 类型切换 ==========

    /**
     * @brief 设置旗帜类型
     * @param type 站立或墙壁
     */
    void setBannerType(BannerType type);

    /**
     * @brief 获取当前旗帜类型
     */
    [[nodiscard]] BannerType getBannerType() const { return m_bannerType; }

    // ========== 部件访问 ==========

    /**
     * @brief 获取旗杆部件
     */
    [[nodiscard]] std::shared_ptr<entity::model::ModelRenderer> getPole() const { return m_pole; }

    /**
     * @brief 获取旗帜面部件（用于图案层叠渲染）
     */
    [[nodiscard]] std::shared_ptr<entity::model::ModelRenderer> getFlag() const { return m_flag; }

    // ========== 动画 ==========

    /**
     * @brief 设置风吹飘动角度
     * @param angle 飘动角度（弧度）
     */
    void setWaveAngle(f32 angle);

private:
    /**
     * @brief 初始化站立旗帜模型部件
     */
    void _initStanding();

    /**
     * @brief 初始化墙壁旗帜模型部件
     */
    void _initWall();

    // 旗杆（站立和墙壁共用）
    std::shared_ptr<entity::model::ModelRenderer> m_pole;

    // 旗帜面（用于图案层叠渲染）
    std::shared_ptr<entity::model::ModelRenderer> m_flag;

    BannerType m_bannerType = BannerType::Standing;

    // 纹理尺寸常量
    static constexpr i32 TEXTURE_WIDTH = 64;
    static constexpr i32 TEXTURE_HEIGHT = 64;
};

} // namespace mc::client::renderer::blockentity::model
