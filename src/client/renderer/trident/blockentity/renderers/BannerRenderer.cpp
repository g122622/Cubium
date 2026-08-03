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
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BannerRenderer.hpp"
#include "client/renderer/trident/blockentity/IBlockEntityRenderer.hpp"
#include "client/renderer/trident/blockentity/model/BannerModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/decorative/BannerBlock.hpp"
#include "common/world/blockentity/interactive/BannerEntity.hpp"
#include <cmath>
#include <vector>

namespace mc::client::renderer::trident::blockentity {

BannerRenderer::BannerRenderer()
    : BlockEntityRenderer<mc::blockentity::BannerEntity>()
    , m_model()
{}

void BannerRenderer::render(const mc::blockentity::BannerEntity& entity, f32 partialTick, u32 light, i64 gameTime)
{
    MC_UNUSED(light);

    // 确定旗帜类型
    auto bannerType = _determineBannerType(entity);
    m_model.setBannerType(bannerType);

    // 获取旋转角度
    f32 rotation = _getRotation(entity);

    // 计算风吹飘动
    // 基于方块位置的随机偏移，模拟风吹效果
    // phase = (floorMod(x*7 + y*9 + z*13 + gameTime, 100) + partialTick) / 100.0
    const auto& pos = entity.getPos();
    i64 seed = static_cast<i64>(pos.x * 7 + pos.y * 9 + pos.z * 13);
    f32 swingTime =
        static_cast<f32>(math::floorMod(seed + gameTime, 100L) + static_cast<i64>(partialTick * 100.0f)) / 100.0f;

    // 余弦波飘动动画
    f32 waveAngle = (-0.0125f + 0.01f * std::cos(2.0f * math::PI * swingTime)) * math::PI;
    m_model.setWaveAngle(waveAngle);

    // 生成网格
    std::vector<entity::model::ModelVertex> vertices;
    std::vector<u32> indices;
    m_model.generateMesh(vertices, indices);

    // TODO: 提交到渲染管线
    // 1. 设置变换矩阵（位置 + Y轴旋转）
    // 2. 渲染底色纹理（base color texture）
    // 3. 逐层渲染图案层（pattern overlay textures + color modulation）
    //    - 每层图案使用灰度纹理 + 对应染料颜色调制
    //    - 图案列表通过 BannerEntity::composePatterns() 获取
    //    - 图案文件名通过 BannerPatterns::getFileName(type) 获取

    (void)rotation;
    (void)vertices;
    (void)indices;
}

model::BannerModel::BannerType BannerRenderer::_determineBannerType(const mc::blockentity::BannerEntity& entity) const
{
    // 通过检查方块状态判断旗帜类型
    const auto* blockState = entity.getBlockState();
    if (blockState == nullptr) {
        return model::BannerModel::BannerType::Standing;
    }

    const Block& block = blockState->getBlock();

    // 检查是否是墙壁旗帜
    const auto* wallBanner = dynamic_cast<const blocks::WallBannerBlock*>(&block);
    if (wallBanner != nullptr) {
        return model::BannerModel::BannerType::Wall;
    }

    return model::BannerModel::BannerType::Standing;
}

f32 BannerRenderer::_getRotation(const mc::blockentity::BannerEntity& entity) const
{
    const auto* blockState = entity.getBlockState();
    if (blockState == nullptr) {
        return 0.0f;
    }

    const Block& block = blockState->getBlock();

    // 站立旗帜使用 ROTATION_0_15 属性（16个方向，每22.5度）
    const auto* standingBanner = dynamic_cast<const blocks::StandingBannerBlock*>(&block);
    if (standingBanner != nullptr) {
        if (blockState->hasProperty(BlockStateProperties::ROTATION_0_15())) {
            i32 rotation = blockState->get(BlockStateProperties::ROTATION_0_15());
            return static_cast<f32>(rotation) * (360.0f / 16.0f) * (math::PI / 180.0f);
        }
    }

    // 墙壁旗帜使用 HORIZONTAL_FACING 属性
    const auto* wallBanner = dynamic_cast<const blocks::WallBannerBlock*>(&block);
    if (wallBanner != nullptr) {
        if (blockState->hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
            Direction facing = blockState->get(BlockStateProperties::HORIZONTAL_FACING());
            switch (facing) {
                case Direction::South:
                    return 0.0f;
                case Direction::West:
                    return math::PI / 2.0f;
                case Direction::North:
                    return math::PI;
                case Direction::East:
                    return 3.0f * math::PI / 2.0f;
                default:
                    return 0.0f;
            }
        }
    }

    return 0.0f;
}

} // namespace mc::client::renderer::trident::blockentity
