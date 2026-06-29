/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, restriction the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "FeatureJigsawPiece.hpp"

#include "AssemblyTypes.hpp"                          // PlacedPiece 完整定义（访问 placed.position）
#include "common/world/gen/chunk/IChunkGenerator.hpp" // WorldGenRegion 完整定义（dynamic_cast 需要）
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

std::string FeatureJigsawPiece::s_typeName = "feature_pool_element";

FeatureJigsawPiece::FeatureJigsawPiece(const std::string& featureId, JigsawPlacementBehaviour behaviour)
    : JigsawPiece(behaviour)
    , m_featureId(featureId)
{
    // 对应 MC 1.21 FeaturePoolElement：默认携带一个 facing=UP 的连接点，
    // target = "minecraft:bottom"，使地物拼图块可被任意向下连接的源连接点匹配。
    // sourceName 为空（地物块无源连接点名称），targetPool 为空（地物块不向外扩展）。
    JigsawJoint joint;
    joint.sourcePos = BlockPos(0, 0, 0);
    joint.sourceName = "minecraft:bottom";
    joint.targetPool = "minecraft:empty";
    joint.targetName = "minecraft:bottom";
    joint.projection = behaviour;
    joint.jointType = JigsawJointType::Rollable;
    joint.orientation = JigsawOrientation::UpNorth; // facing=UP，对应 MC 的 "bottom" 连接点
    m_joints.push_back(joint);
}

void FeatureJigsawPiece::place(IWorldWriter& world,
    const PlacedPiece& placed,
    class feature::template_::TemplateManager& /*templateManager*/,
    math::Random& rng,
    const structure::StructureBoundingBox* /*bounds*/,
    world::chunk::ChunkPrimer* chunk,
    IChunkGenerator* generator)
{
    // 对应 MC 1.21 FeaturePoolElement.place()：在放置位置调用配置化地物，不应用任何结构处理器
    // （BlockIgnore/JigsawReplacement/Gravity 均不适用于地物块）。
    //
    // Cubium 与 MC 的差异：
    //   - MC 使用 PlacedFeature（ConfiguredFeature + PlacementModifiers），Cubium 无 PlacedFeature 类型，
    //     直接使用 ConfiguredFeatureBase，因此 PlacementModifiers（如 height_range/biome_filter/block_survival
    //     等）不在此处理。当前 jigsaw 场景的地物（树木/仙人掌/干草堆等）由外层结构放置流程控制高度，
    //     此处仅按 placed.position 放置。见下方 TODO。
    //   - 需要 WorldGenRegion 与 IChunkGenerator 才能调用 ConfiguredFeatureBase::place()。world 参数类型为
    //     IWorldWriter，而 WorldGenRegion 继承自 IWorld（IWorld 继承 IWorldWriter），故 dynamic_cast 获取。

    // 地物放置需要 chunk 和 generator（ConfiguredFeatureBase::place 签名要求引用）
    if (chunk == nullptr || generator == nullptr) {
        // TODO(jigsaw-refactor): chunk/generator 在非结构生成路径（如测试桩）下可能为 nullptr。
        //   MC 1.21 中 FeaturePoolElement 总在 WorldGenRegion 上下文里调用，此处跳过放置以避免空指针。
        //   参考: net.minecraft.world.level.levelgen.structure.pools.FeaturePoolElement.place()
        spdlog::warn("[FeatureJigsawPiece] place() skipped: chunk or generator is null (feature='{}')", m_featureId);
        return;
    }

    ConfiguredFeatureBase* feature = FeatureRegistry::instance().getFeatureByName(m_featureId);
    if (feature == nullptr) {
        spdlog::warn("[FeatureJigsawPiece] feature '{}' not found in FeatureRegistry, skip placement", m_featureId);
        return;
    }

    // IWorldWriter → WorldGenRegion：结构放置时 world 实际是 WorldGenRegion（继承 IWorld→IWorldWriter）
    WorldGenRegion* region = dynamic_cast<WorldGenRegion*>(&world);
    if (region == nullptr) {
        // TODO(jigsaw-refactor): 非 WorldGenRegion 的 IWorldWriter（如测试桩/结构写入器）无法放置地物。
        //   MC 1.21 FeaturePoolElement 仅在 WorldGenLevel（WorldGenRegion）上下文中调用。
        //   参考: net.minecraft.world.level.levelgen.structure.pools.FeaturePoolElement.place()
        spdlog::warn("[FeatureJigsawPiece] place() skipped: world is not a WorldGenRegion (feature='{}')", m_featureId);
        return;
    }

    // TODO(jigsaw-refactor): PlacedFeature PlacementModifiers 未实现。Cubium 无 PlacedFeature 类型，
    //   直接调用 ConfiguredFeatureBase::place()，跳过了 MC 中 PlacementModifiers 的
    //   height_range/biome_filter/block_survival/environment_scan 等修饰逻辑。
    //   对 jigsaw 场景影响有限（地物位置由父结构决定），但若后续引入 PlacedFeature 需在此补全。
    //   参考: net.minecraft.world.level.levelgen.placement.PlacedFeature.place()
    feature->place(*region, *chunk, *generator, rng, placed.position);
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
