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

#include "PlacementUtils.hpp"
#include "Placements.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"

namespace mc {

namespace predicate = world::gen::feature::predicate;

namespace PlacementUtils {

namespace {

/** 在放置链末尾追加一个配置化放置器 */
void appendToEnd(ConfiguredPlacement& root, std::unique_ptr<ConfiguredPlacement> next)
{
    if (!next) {
        return;
    }
    ConfiguredPlacement* current = &root;
    while (current->next() != nullptr) {
        current = current->next();
    }
    current->setNext(std::move(next));
}

} // namespace

std::unique_ptr<ConfiguredPlacement> appendBiomePlacement(
    std::unique_ptr<ConfiguredPlacement> root, std::vector<u32> allowedBiomes)
{
    if (!root || allowedBiomes.empty()) {
        return root;
    }

    auto biomeConfigured = std::make_unique<ConfiguredPlacement>(
        std::make_unique<BiomePlacement>(), std::make_unique<BiomePlacementConfig>(std::move(allowedBiomes)));

    appendToEnd(*root, std::move(biomeConfigured));
    return root;
}

std::unique_ptr<ConfiguredPlacement> appendEnvironmentScanUp(std::unique_ptr<ConfiguredPlacement> root, i32 maxSteps)
{
    if (!root) {
        return root;
    }

    auto config = std::make_unique<EnvironmentScanConfig>(Direction::Up,
        std::make_unique<predicate::HasSturdyFacePredicate>(Direction::Down),
        std::make_unique<predicate::OnlyInAirPredicate>(),
        maxSteps);

    auto scanConfigured =
        std::make_unique<ConfiguredPlacement>(std::make_unique<EnvironmentScanPlacement>(), std::move(config));

    appendToEnd(*root, std::move(scanConfigured));
    return root;
}

std::unique_ptr<ConfiguredPlacement> appendEnvironmentScanDown(std::unique_ptr<ConfiguredPlacement> root, i32 maxSteps)
{
    if (!root) {
        return root;
    }

    auto config = std::make_unique<EnvironmentScanConfig>(Direction::Down,
        std::make_unique<predicate::HasSturdyFacePredicate>(Direction::Up),
        std::make_unique<predicate::OnlyInAirPredicate>(),
        maxSteps);

    auto scanConfigured =
        std::make_unique<ConfiguredPlacement>(std::make_unique<EnvironmentScanPlacement>(), std::move(config));

    appendToEnd(*root, std::move(scanConfigured));
    return root;
}

std::unique_ptr<ConfiguredPlacement> appendVerticalOffset(std::unique_ptr<ConfiguredPlacement> root, i32 offset)
{
    if (!root) {
        return root;
    }

    auto offsetConfigured = std::make_unique<ConfiguredPlacement>(
        std::make_unique<RandomOffsetPlacement>(), RandomOffsetConfig::vertical(offset));

    appendToEnd(*root, std::move(offsetConfigured));
    return root;
}

std::unique_ptr<ConfiguredPlacement> createCountedSurfacePlacement(i32 count, i32 maxWaterDepth)
{
    auto surfacePlacement = std::make_unique<SurfacePlacement>();
    auto surfaceConfig = std::make_unique<SurfacePlacementConfig>(maxWaterDepth, false);

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(count);

    auto surfaceConfigured =
        std::make_unique<ConfiguredPlacement>(std::move(surfacePlacement), std::move(surfaceConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(surfaceConfigured));
    countConfigured->setNext(std::move(squareConfigured));
    return countConfigured;
}

std::unique_ptr<ConfiguredPlacement> createChanceSurfacePlacement(f32 chance, i32 maxWaterDepth)
{
    auto surfacePlacement = std::make_unique<SurfacePlacement>();
    auto surfaceConfig = std::make_unique<SurfacePlacementConfig>(maxWaterDepth, false);

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto chancePlacement = std::make_unique<ChancePlacement>();
    auto chanceConfig = std::make_unique<ChancePlacementConfig>(chance);

    auto surfaceConfigured =
        std::make_unique<ConfiguredPlacement>(std::move(surfacePlacement), std::move(surfaceConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto chanceConfigured = std::make_unique<ConfiguredPlacement>(std::move(chancePlacement), std::move(chanceConfig));

    squareConfigured->setNext(std::move(surfaceConfigured));
    chanceConfigured->setNext(std::move(squareConfigured));
    return chanceConfigured;
}

std::unique_ptr<ConfiguredPlacement> createCountedHeightPlacement(i32 count, i32 minY, i32 maxY)
{
    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(minY, 0, maxY);

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(count);

    auto heightConfigured = std::make_unique<ConfiguredPlacement>(std::move(heightPlacement), std::move(heightConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(heightConfigured));
    countConfigured->setNext(std::move(squareConfigured));
    return countConfigured;
}

} // namespace PlacementUtils
} // namespace mc
