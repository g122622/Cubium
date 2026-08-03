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

#include "AssemblyTypes.hpp" // PlacedPiece 完整定义（访问 placed.position）
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp" // WorldGenRegion 完整定义（dynamic_cast 需要）
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeatureRegistry.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <string>
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
    // 默认携带一个 facing=UP 的连接点，target = "minecraft:bottom"，
    // 使地物拼图块可被任意向下连接的源连接点匹配。
    // sourceName 为空（地物块无源连接点名称），targetPool 为空（地物块不向外扩展）。
    JigsawJoint joint;
    joint.sourcePos = BlockPos(0, 0, 0);
    joint.sourceName = "minecraft:bottom";
    joint.targetPool = "minecraft:empty";
    joint.targetName = "minecraft:bottom";
    joint.projection = behaviour;
    joint.jointType = JigsawJointType::Rollable;
    joint.orientation = JigsawOrientation::UpNorth; // facing=UP，即 "bottom" 连接点
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
    // 在放置位置调用配置化地物，不应用任何结构处理器
    // （BlockIgnore/JigsawReplacement/Gravity 均不适用于地物块）。
    //
    // jigsaw 场景的地物（树木/仙人掌/干草堆等）由外层结构放置流程控制高度，
    // 此处仅按 placed.position 调用 configured_feature，不再走 PlacedFeature 的 placement 修饰链
    // （jigsaw 池元素直接调用 configured_feature，不含 placement modifier）。
    // 需要 WorldGenRegion 与 IChunkGenerator 才能调用 ConfiguredFeatureBase::place()。world 参数类型为
    // IWorldWriter，而 WorldGenRegion 继承自 IWorld（IWorld 继承 IWorldWriter），故 dynamic_cast 获取。

    // 地物放置需要 chunk 和 generator（ConfiguredFeatureBase::place 签名要求引用）
    if (chunk == nullptr || generator == nullptr) {
        // 非结构生成路径（如测试桩）下 chunk/generator 可能为 nullptr，跳过放置以避免空指针。
        spdlog::warn("[FeatureJigsawPiece] place() skipped: chunk or generator is null (feature='{}')", m_featureId);
        return;
    }

    // 数据驱动：m_featureId 是 configured_feature 的 ResourceLocation 字符串（如 "minecraft:pale_oak"），
    // 从 ConfiguredFeatureRegistry 按 id 解析为 const ConfiguredFeatureBase*。
    const ConfiguredFeatureBase* feature =
        ConfiguredFeatureRegistry::instance().get(ResourceLocation::parse(m_featureId));
    if (feature == nullptr) {
        spdlog::warn(
            "[FeatureJigsawPiece] feature '{}' not found in ConfiguredFeatureRegistry, skip placement", m_featureId);
        return;
    }

    // IWorldWriter → WorldGenRegion：结构放置时 world 实际是 WorldGenRegion（继承 IWorld→IWorldWriter）
    WorldGenRegion* region = dynamic_cast<WorldGenRegion*>(&world);
    if (region == nullptr) {
        // 非 WorldGenRegion 的 IWorldWriter（如测试桩/结构写入器）无法放置地物。
        // MC 1.21 FeaturePoolElement 仅在 WorldGenLevel（WorldGenRegion）上下文中调用。
        spdlog::warn("[FeatureJigsawPiece] place() skipped: world is not a WorldGenRegion (feature='{}')", m_featureId);
        return;
    }

    feature->place(*region, *chunk, *generator, rng, placed.position);
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
