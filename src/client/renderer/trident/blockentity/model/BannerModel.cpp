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

#include "BannerModel.hpp"
#include "client/renderer/trident/blockentity/model/BlockEntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::blockentity::model {

BannerModel::BannerModel()
    : BlockEntityModel()
{
    setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    _initStanding();
}

void BannerModel::_initStanding()
{
    // 旗杆：2像素宽、42像素高、2像素深
    m_pole = std::make_shared<entity::model::ModelRenderer>("pole");
    m_pole->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_pole->setTextureOffset(0, 42);
    m_pole->addBox(-1.0, -30.0, -1.0, 2.0, 42.0, 2.0, 0.0);
    m_pole->setRotationPoint(0.0, 30.0, 0.0);

    // 旗帜面：20像素宽、40像素高、1像素深
    // 偏移到旗杆右侧
    m_flag = std::make_shared<entity::model::ModelRenderer>("flag");
    m_flag->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_flag->setTextureOffset(0, 0);
    m_flag->addBox(-10.0, -30.0, -2.0, 20.0, 40.0, 1.0, 0.0);
    m_flag->setRotationPoint(0.0, 30.0, 0.0);

    addPart(m_pole);
    addPart(m_flag);
}

void BannerModel::_initWall()
{
    // 墙壁旗帜的旗杆和旗帜面位置不同
    // 旗杆：水平方向，贴墙
    m_pole = std::make_shared<entity::model::ModelRenderer>("pole");
    m_pole->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_pole->setTextureOffset(44, 0);
    m_pole->addBox(-10.0, -24.0, -1.0, 20.0, 2.0, 2.0, 0.0);
    m_pole->setRotationPoint(0.0, 24.0, 0.0);

    // 旗帜面：贴墙垂直展开
    m_flag = std::make_shared<entity::model::ModelRenderer>("flag");
    m_flag->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_flag->setTextureOffset(0, 0);
    m_flag->addBox(-10.0, -24.0, -2.0, 20.0, 40.0, 1.0, 0.0);
    m_flag->setRotationPoint(0.0, 24.0, 0.0);

    addPart(m_pole);
    addPart(m_flag);
}

void BannerModel::setBannerType(BannerType type)
{
    if (m_bannerType == type) {
        return;
    }

    m_bannerType = type;

    // 清除旧部件
    m_parts.clear();

    // 重新初始化对应类型的部件
    if (type == BannerType::Standing) {
        _initStanding();
    } else {
        _initWall();
    }
}

void BannerModel::setWaveAngle(f32 angle)
{
    if (m_flag != nullptr) {
        m_flag->setRotateAngleX(angle);
    }
}

} // namespace mc::client::renderer::blockentity::model
