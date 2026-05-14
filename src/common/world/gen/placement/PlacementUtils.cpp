#include "PlacementUtils.hpp"

namespace mc {
namespace PlacementUtils {

std::unique_ptr<ConfiguredPlacement> appendBiomePlacement(
    std::unique_ptr<ConfiguredPlacement> root, std::vector<u32> allowedBiomes)
{
    if (!root || allowedBiomes.empty()) {
        return root;
    }

    auto biomeConfigured = std::make_unique<ConfiguredPlacement>(
        std::make_unique<BiomePlacement>(), std::make_unique<BiomePlacementConfig>(std::move(allowedBiomes)));

    ConfiguredPlacement* current = root.get();
    while (current->next() != nullptr) {
        current = current->next();
    }
    current->setNext(std::move(biomeConfigured));
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
