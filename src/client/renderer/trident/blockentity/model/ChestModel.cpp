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

#include "ChestModel.hpp"
#include "client/renderer/trident/blockentity/model/BlockEntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <memory>

namespace mc::client::renderer::blockentity::model {

ChestModel::ChestModel()
    : BlockEntityModel()
{
    setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);

    // 初始化所有部件
    _initSingleChest();
    _initLeftChest();
    _initRightChest();

    // 默认使用单箱
    m_bottom = m_singleBottom;
    m_lid = m_singleLid;
    m_latch = m_singleLatch;

    // 添加到部件列表
    addPart(m_singleBottom);
    addPart(m_singleLid);
    addPart(m_singleLatch);
}

void ChestModel::_initSingleChest()
{
    // 单箱箱体: (1,0,1) 到 (15,10,15)，纹理偏移 (0,19)
    // 尺寸：宽14，高10，深14
    m_singleBottom = std::make_shared<entity::model::ModelRenderer>("singleBottom");
    m_singleBottom->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_singleBottom->setTextureOffset(0, 19);
    m_singleBottom->addBox(1.0, 0.0, 1.0, 14.0, 10.0, 14.0, 0.0);

    // 单箱盖子: (1,0,0) 到 (15,5,14)，纹理偏移 (0,0)
    // 旋转点在 Y=9, Z=1
    // 尺寸：宽14，高5，深14
    m_singleLid = std::make_shared<entity::model::ModelRenderer>("singleLid");
    m_singleLid->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_singleLid->setTextureOffset(0, 0);
    m_singleLid->addBox(1.0, 0.0, 0.0, 14.0, 5.0, 14.0, 0.0);
    m_singleLid->setRotationPoint(0.0, 9.0, 1.0);

    // 单箱锁扣: (7,-1,15) 到 (9,3,16)，纹理偏移 (0,0)
    // 旋转点在 Y=8
    // 尺寸：宽2，高4，深1
    m_singleLatch = std::make_shared<entity::model::ModelRenderer>("singleLatch");
    m_singleLatch->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_singleLatch->setTextureOffset(0, 0);
    m_singleLatch->addBox(7.0, -1.0, 15.0, 2.0, 4.0, 1.0, 0.0);
    m_singleLatch->setRotationPoint(0.0, 8.0, 0.0);
}

void ChestModel::_initLeftChest()
{
    // 双箱左半箱体: (0,0,1) 到 (15,10,15)
    // 宽15（靠左）
    m_leftBottom = std::make_shared<entity::model::ModelRenderer>("leftBottom");
    m_leftBottom->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_leftBottom->setTextureOffset(0, 19);
    m_leftBottom->addBox(0.0, 0.0, 1.0, 15.0, 10.0, 14.0, 0.0);

    // 双箱左半盖子: (0,0,0) 到 (15,5,14)
    m_leftLid = std::make_shared<entity::model::ModelRenderer>("leftLid");
    m_leftLid->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_leftLid->setTextureOffset(0, 0);
    m_leftLid->addBox(0.0, 0.0, 0.0, 15.0, 5.0, 14.0, 0.0);
    m_leftLid->setRotationPoint(0.0, 9.0, 1.0);

    // 双箱左半锁扣: (0,-1,15) 到 (1,3,16)
    // 锁扣在最左边
    m_leftLatch = std::make_shared<entity::model::ModelRenderer>("leftLatch");
    m_leftLatch->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_leftLatch->setTextureOffset(0, 0);
    m_leftLatch->addBox(0.0, -1.0, 15.0, 1.0, 4.0, 1.0, 0.0);
    m_leftLatch->setRotationPoint(0.0, 8.0, 0.0);
}

void ChestModel::_initRightChest()
{
    // 双箱右半箱体: (1,0,1) 到 (16,10,15)
    // 宽15（靠右）
    m_rightBottom = std::make_shared<entity::model::ModelRenderer>("rightBottom");
    m_rightBottom->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_rightBottom->setTextureOffset(0, 19);
    m_rightBottom->addBox(1.0, 0.0, 1.0, 15.0, 10.0, 14.0, 0.0);

    // 双箱右半盖子: (1,0,0) 到 (16,5,14)
    m_rightLid = std::make_shared<entity::model::ModelRenderer>("rightLid");
    m_rightLid->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_rightLid->setTextureOffset(0, 0);
    m_rightLid->addBox(1.0, 0.0, 0.0, 15.0, 5.0, 14.0, 0.0);
    m_rightLid->setRotationPoint(0.0, 9.0, 1.0);

    // 双箱右半锁扣: (15,-1,15) 到 (16,3,16)
    // 锁扣在最右边
    m_rightLatch = std::make_shared<entity::model::ModelRenderer>("rightLatch");
    m_rightLatch->setTextureSize(TEXTURE_WIDTH, TEXTURE_HEIGHT);
    m_rightLatch->setTextureOffset(0, 0);
    m_rightLatch->addBox(15.0, -1.0, 15.0, 1.0, 4.0, 1.0, 0.0);
    m_rightLatch->setRotationPoint(0.0, 8.0, 0.0);
}

void ChestModel::setLidAngle(f32 angle)
{
    // 应用缓动函数
    const f32 easedAngle = applyEasing(angle);

    // 转换为弧度，范围 0 到 PI/2
    const f32 radians = easedAngle * (math::PI / 2.0f);

    // 设置盖子旋转（绕 X 轴负方向）
    m_lid->setRotateAngleX(-radians);

    // 锁扣跟随盖子旋转
    m_latch->setRotateAngleX(-radians);
}

f32 ChestModel::applyEasing(f32 angle)
{
    // 三次缓动
    f32 eased = 1.0f - angle;
    eased = 1.0f - eased * eased * eased;
    return eased;
}

void ChestModel::setChestType(ChestType type)
{
    m_chestType = type;

    // 隐藏所有部件
    m_singleBottom->setVisible(false);
    m_singleLid->setVisible(false);
    m_singleLatch->setVisible(false);
    m_leftBottom->setVisible(false);
    m_leftLid->setVisible(false);
    m_leftLatch->setVisible(false);
    m_rightBottom->setVisible(false);
    m_rightLid->setVisible(false);
    m_rightLatch->setVisible(false);

    // 根据类型显示对应部件
    switch (type) {
        case ChestType::Single:
            m_bottom = m_singleBottom;
            m_lid = m_singleLid;
            m_latch = m_singleLatch;
            m_singleBottom->setVisible(true);
            m_singleLid->setVisible(true);
            m_singleLatch->setVisible(true);
            break;
        case ChestType::Left:
            m_bottom = m_leftBottom;
            m_lid = m_leftLid;
            m_latch = m_leftLatch;
            m_leftBottom->setVisible(true);
            m_leftLid->setVisible(true);
            m_leftLatch->setVisible(true);
            break;
        case ChestType::Right:
            m_bottom = m_rightBottom;
            m_lid = m_rightLid;
            m_latch = m_rightLatch;
            m_rightBottom->setVisible(true);
            m_rightLid->setVisible(true);
            m_rightLatch->setVisible(true);
            break;
    }

    // 更新部件列表
    m_parts.clear();
    addPart(m_bottom);
    addPart(m_lid);
    addPart(m_latch);
}

} // namespace mc::client::renderer::blockentity::model
