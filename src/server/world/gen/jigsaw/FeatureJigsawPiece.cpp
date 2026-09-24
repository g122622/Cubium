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
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp" // WorldGenRegion 完整定义（dynamic_cast 需要）
#include "server/world/gen/feature/ConfiguredFeature.hpp"
#include "server/world/gen/feature/ConfiguredFeatureRegistry.hpp"
#include "server/world/gen/jigsaw/JigsawPiece.hpp"
#include "server/world/gen/jigsaw/JigsawTypes.hpp"
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
    // 虚拟连接点，对应 MC FeaturePoolElement.fillDefaultJigsawNBT() +
    // getShuffledJigsawBlocks()：name=minecraft:bottom、target=minecraft:empty、
    // pool=minecraft:empty、joint=ROLLABLE，方块状态为
    // `FrontAndTop.fromFrontAndTop(Direction.DOWN, Direction.SOUTH)` 即 down_south。
    //
    // 【朝向必须是 down_south】连接匹配要求"父连接点的正面 == 子连接点正面的反向"。
    // 村庄里所有形如 bottom 的父连接点朝向都是 up_north（正面朝上，表示"我上面可以放东西"），
    // 因此地物的虚拟连接点必须正面朝下；写成朝上会让地物池（树/花/干草堆）**永远匹配失败**，
    // 整片村庄因此缺少全部植被类装饰，且不产生任何报错。
    // 名字取地物 id：构件标识（templateLocation）依赖它，否则地物构件在诊断/parity 比对里
    // 完全不可区分（"是哪个地物"直接决定构件内容）
    setName(m_featureId);

    JigsawJoint joint;
    joint.sourcePos = BlockPos(0, 0, 0);
    joint.sourceName = "minecraft:bottom";
    joint.targetPool = "minecraft:empty";
    joint.targetName = "minecraft:empty";
    joint.projection = behaviour;
    joint.jointType = JigsawJointType::Rollable;
    joint.orientation = JigsawOrientation::DownSouth;
    m_joints.push_back(joint);
}

void FeatureJigsawPiece::place(IWorldWriter& world,
    const PlacedPiece& placed,
    class feature::template_::TemplateManager& /*templateManager*/,
    math::IRandom& rng,
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
