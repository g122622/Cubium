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

#include "FossilFeature.hpp"

#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include "common/world/gen/jigsaw/ProcessorListRegistry.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"

#include <algorithm>

namespace mc::world::gen::feature::cave {

namespace {

using Template = ::mc::world::gen::feature::template_::Template;
using TemplateManager = ::mc::world::gen::feature::template_::TemplateManager;
using PlacementSettings = ::mc::world::gen::feature::template_::PlacementSettings;
using StructureProcessorList = ::mc::world::gen::feature::template_::StructureProcessorList;

/// 旋转后的模板尺寸（MC StructureTemplate.getSize(rotation)）：90/270 度交换 x/z。
BlockPos rotatedSize(const BlockPos& size, Rotation rotation)
{
    if (rotation == Rotation::Clockwise90 || rotation == Rotation::CounterClockwise90) {
        return BlockPos(size.z, size.y, size.x);
    }
    return size;
}

/// 8 角迭代回调。
template <typename Fn>
void forEachCorner(const ::mc::world::gen::structure::StructureBoundingBox& box, Fn&& fn)
{
    const i32 xs[2] = {box.minX(), box.maxX()};
    const i32 ys[2] = {box.minY(), box.maxY()};
    const i32 zs[2] = {box.minZ(), box.maxZ()};
    for (i32 x : xs) {
        for (i32 y : ys) {
            for (i32 z : zs) {
                fn(BlockPos(x, y, z));
            }
        }
    }
}

} // namespace

// ============================================================================
// ConfiguredFossilFeature
// ============================================================================

ConfiguredFossilFeature::ConfiguredFossilFeature(std::unique_ptr<FossilConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredFossilFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    if (m_config == nullptr) {
        return false;
    }
    return m_feature.place(region, generator, random, pos, *m_config);
}

// ============================================================================
// FossilFeature
// ============================================================================

bool FossilFeature::place(
    IWorld& world, IChunkGenerator& generator, math::Random& random, const BlockPos& origin, const FossilConfig& config)
{
    if (config.fossilStructures.empty() || config.overlayStructures.empty()) {
        return false;
    }

    const Rotation rotation = static_cast<Rotation>(random.nextInt(4));
    const i32 index = random.nextInt(static_cast<i32>(config.fossilStructures.size()));

    auto& templateManager = jigsaw::JigsawAssembler::getTemplateManager();
    const Template* fossilTemplate = templateManager.getTemplate(config.fossilStructures[static_cast<size_t>(index)]);
    const Template* overlayTemplate = templateManager.getTemplate(config.overlayStructures[static_cast<size_t>(index)]);
    if (fossilTemplate == nullptr || overlayTemplate == nullptr) {
        return false;
    }

    const BlockPos vec3i = rotatedSize(fossilTemplate->getSize(), rotation);
    const BlockPos blockpos1(origin.x - vec3i.x / 2, origin.y, origin.z - vec3i.z / 2);

    // 取模板覆盖范围内 OCEAN_FLOOR_WG 的最低点作为 j。
    i32 j = origin.y;
    for (i32 k = 0; k < vec3i.x; ++k) {
        for (i32 l = 0; l < vec3i.z; ++l) {
            j = std::min(j, generator.getHeight(blockpos1.x + k, blockpos1.z + l, HeightmapType::OceanFloorWG));
        }
    }

    const i32 i1 = std::max(j - 15 - random.nextInt(10), world.getMinBuildHeight() + 10);
    // MC: getZeroPositionWithTransform(blockpos1.atY(i1), Mirror.NONE, rotation)
    const BlockPos blockpos2 =
        Template::transformBlockPos(BlockPos(blockpos1.x, i1, blockpos1.z), Mirror::None, rotation, BlockPos(0, 0, 0));

    PlacementSettings settings;
    settings.setRotation(rotation).setMirror(Mirror::None).setRandom(&random);

    const ::mc::world::gen::structure::StructureBoundingBox box = fossilTemplate->getBoundingBox(settings, blockpos2);
    if (countEmptyCorners(world, box) > config.maxEmptyCornersAllowed) {
        return false;
    }

    const StructureProcessorList* fossilProcessors =
        jigsaw::ProcessorListRegistry::instance().getList(config.fossilProcessors);
    settings.setProcessors(fossilProcessors);
    fossilTemplate->placeInWorld(world, blockpos2, settings, random, 260);

    const StructureProcessorList* overlayProcessors =
        jigsaw::ProcessorListRegistry::instance().getList(config.overlayProcessors);
    settings.setProcessors(overlayProcessors);
    overlayTemplate->placeInWorld(world, blockpos2, settings, random, 260);

    return true;
}

i32 FossilFeature::countEmptyCorners(IWorld& world, const ::mc::world::gen::structure::StructureBoundingBox& box)
{
    i32 count = 0;
    forEachCorner(box, [&](const BlockPos& corner) {
        const BlockState* state = world.getBlockState(corner);
        const bool empty = (state == nullptr) || state->isAir();
        if (empty || (state != nullptr && (state->is(VanillaBlocks::LAVA) || state->is(VanillaBlocks::WATER)))) {
            ++count;
        }
    });
    return count;
}

} // namespace mc::world::gen::feature::cave
