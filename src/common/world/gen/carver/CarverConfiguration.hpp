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

#include "common/core/Types.hpp"
#include "common/world/gen/surface/SurfaceRules.hpp"
#include "common/world/gen/surface/VerticalAnchor.hpp"
#include "common/world/gen/valueprovider/FloatProvider.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"
#include <memory>
#include <utility>

namespace mc {

namespace valueprovider = world::gen::valueprovider;
namespace surface = world::gen::surface;

class BlockTag;

// ============================================================================
// CarverConfiguration — 雕刻器基础配置
// ============================================================================

/**
 * @brief 雕刻器基础配置
 *
 * 包含所有雕刻器类型共享的配置参数。
 */
struct CarverConfiguration {
    f32 probability = 0.0f;
    std::unique_ptr<valueprovider::HeightProvider> y;
    std::unique_ptr<valueprovider::FloatProvider> yScale;
    surface::VerticalAnchor lavaLevel;
    const BlockTag* replaceable = nullptr;

    CarverConfiguration() = default;

    CarverConfiguration(f32 prob,
        std::unique_ptr<valueprovider::HeightProvider> yPos,
        std::unique_ptr<valueprovider::FloatProvider> yScaleVal,
        surface::VerticalAnchor lava,
        const BlockTag* replaceableTag)
        : probability(prob)
        , y(std::move(yPos))
        , yScale(std::move(yScaleVal))
        , lavaLevel(lava)
        , replaceable(replaceableTag)
    {}
};

// ============================================================================
// CaveCarverConfiguration — 洞穴雕刻器配置
// ============================================================================

/**
 * @brief 洞穴雕刻器配置
 *
 * 在 CarverConfiguration 基础上增加洞穴特有的水平/垂直半径乘数和地板高度。
 */
struct CaveCarverConfiguration : public CarverConfiguration {
    std::unique_ptr<valueprovider::FloatProvider> horizontalRadiusMultiplier;
    std::unique_ptr<valueprovider::FloatProvider> verticalRadiusMultiplier;
    std::unique_ptr<valueprovider::FloatProvider> floorLevel;

    CaveCarverConfiguration() = default;

    CaveCarverConfiguration(f32 prob,
        std::unique_ptr<valueprovider::HeightProvider> yPos,
        std::unique_ptr<valueprovider::FloatProvider> yScaleVal,
        surface::VerticalAnchor lava,
        const BlockTag* replaceableTag,
        std::unique_ptr<valueprovider::FloatProvider> hRadiusMult,
        std::unique_ptr<valueprovider::FloatProvider> vRadiusMult,
        std::unique_ptr<valueprovider::FloatProvider> floor)
        : CarverConfiguration(prob, std::move(yPos), std::move(yScaleVal), lava, replaceableTag)
        , horizontalRadiusMultiplier(std::move(hRadiusMult))
        , verticalRadiusMultiplier(std::move(vRadiusMult))
        , floorLevel(std::move(floor))
    {}
};

// ============================================================================
// CanyonShapeConfiguration — 峡谷形状配置
// ============================================================================

/**
 * @brief 峡谷形状配置
 */
struct CanyonShapeConfiguration {
    std::unique_ptr<valueprovider::FloatProvider> distanceFactor;
    std::unique_ptr<valueprovider::FloatProvider> thickness;
    i32 widthSmoothness = 3;
    std::unique_ptr<valueprovider::FloatProvider> horizontalRadiusFactor;
    f32 verticalRadiusDefaultFactor = 1.0f;
    f32 verticalRadiusCenterFactor = 0.0f;

    CanyonShapeConfiguration() = default;

    CanyonShapeConfiguration(std::unique_ptr<valueprovider::FloatProvider> distFactor,
        std::unique_ptr<valueprovider::FloatProvider> thick,
        i32 smoothness,
        std::unique_ptr<valueprovider::FloatProvider> hRadiusFactor,
        f32 vDefaultFactor,
        f32 vCenterFactor)
        : distanceFactor(std::move(distFactor))
        , thickness(std::move(thick))
        , widthSmoothness(smoothness)
        , horizontalRadiusFactor(std::move(hRadiusFactor))
        , verticalRadiusDefaultFactor(vDefaultFactor)
        , verticalRadiusCenterFactor(vCenterFactor)
    {}
};

// ============================================================================
// CanyonCarverConfiguration — 峡谷雕刻器配置
// ============================================================================

/**
 * @brief 峡谷雕刻器配置
 *
 * 在 CarverConfiguration 基础上增加峡谷特有的垂直旋转和形状参数。
 */
struct CanyonCarverConfiguration : public CarverConfiguration {
    std::unique_ptr<valueprovider::FloatProvider> verticalRotation;
    CanyonShapeConfiguration shape;

    CanyonCarverConfiguration() = default;

    CanyonCarverConfiguration(f32 prob,
        std::unique_ptr<valueprovider::HeightProvider> yPos,
        std::unique_ptr<valueprovider::FloatProvider> yScaleVal,
        surface::VerticalAnchor lava,
        const BlockTag* replaceableTag,
        std::unique_ptr<valueprovider::FloatProvider> vRotation,
        CanyonShapeConfiguration shapeConfig)
        : CarverConfiguration(prob, std::move(yPos), std::move(yScaleVal), lava, replaceableTag)
        , verticalRotation(std::move(vRotation))
        , shape(std::move(shapeConfig))
    {}
};

} // namespace mc
