#include "server/test/minecraft/structure/MinecraftStructurePlacer.hpp"

#include "common/core/Types.hpp" // i32
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"  // MC_ASSERT_RELEASE
#include "common/world/IWorld.hpp"              // IWorld（setBlockState/getRandom）
#include "common/world/block/BlockRegistry.hpp" // mc::BlockRegistry::airState
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/template/Template.hpp"        // Template + PlacementSettings
#include "common/world/gen/feature/template/TemplateManager.hpp" // TemplateManager
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"           // JigsawAssembler::getTemplateManager
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/ServerWorld.hpp"

#include <spdlog/spdlog.h>

namespace mc::test {

namespace {

// 全限定命名空间路径，规避 mc::test 内非限定名两段查找不回退 mc::world 的遮蔽坑（见 BossBarState 内存）
using Template = mc::world::gen::feature::template_::Template;
using TemplateManager = mc::world::gen::feature::template_::TemplateManager;
using PlacementSettings = mc::world::gen::feature::template_::PlacementSettings;
using StructureBoundingBox = mc::world::gen::structure::StructureBoundingBox;
using JigsawAssembler = mc::world::gen::jigsaw::JigsawAssembler;

/**
 * @brief 取结构模板（经全局 TemplateManager 单例）。
 *
 * 项目无 server 级 TemplateManager 访问器；结构模板管理器是 `JigsawAssembler` 持有的全局单例，
 * 由 `RegistryBootstrap` 在服务端启动期注入资源包/数据包。GameTestServer/IntegratedServer 启动后可用。
 */
const Template* _getTemplate(const std::string& structureName)
{
    auto& tm = JigsawAssembler::getTemplateManager();
    const mc::resource::ResourceLocation loc(structureName);
    return tm.getTemplate(loc);
}

/**
 * @brief 用 air 清空指定 StructureBoundingBox 范围（含方块更新）。
 */
void _clearBox(mc::server::ServerWorld& world, const StructureBoundingBox& box)
{
    const mc::BlockState* air = mc::BlockRegistry::instance().airState();
    for (i32 x = box.minX(); x <= box.maxX(); ++x) {
        for (i32 y = box.minY(); y <= box.maxY(); ++y) {
            for (i32 z = box.minZ(); z <= box.maxZ(); ++z) {
                world.setBlockState(x, y, z, air);
            }
        }
    }
}

} // namespace

std::unique_ptr<StructureBounds> MinecraftStructurePlacer::place(
    mc::server::ServerWorld& world, const BlockPos& origin, const TestData& data)
{
    const auto* tpl = _getTemplate(data.structure());
    if (tpl == nullptr) {
        spdlog::warn("GameTest: structure '{}' not found in TemplateManager", data.structure());
        return nullptr;
    }

    // 计算旋转后包围盒（用 Template::getBoundingBox 取权威包围盒，含旋转）
    PlacementSettings settings;
    settings.setRotation(data.rotation());
    settings.setIgnoreEntities(false);
    const StructureBoundingBox placedBox = tpl->getBoundingBox(settings, origin);

    // 放置前清理周边（padding 格），避免残留方块/实体干扰测试
    if (data.padding() > 0) {
        const StructureBoundingBox padBox(placedBox.minX() - data.padding(),
            placedBox.minY() - data.padding(),
            placedBox.minZ() - data.padding(),
            placedBox.maxX() + data.padding(),
            placedBox.maxY() + data.padding(),
            placedBox.maxZ() + data.padding());
        _clearBox(world, padBox);
    }

    // 放置结构（含方块/方块实体/实体），flags=18 对齐 vanilla 默认（UPDATE_CLIENTS|NOTIFY）
    mc::math::Random& rng = world.getRandom();
    const bool ok = tpl->placeInWorld(world, origin, settings, rng, 18);
    if (!ok) {
        spdlog::warn("GameTest: failed to place structure '{}'", data.structure());
        return nullptr;
    }

    // 屏障包裹：当 !skyAccess 时顶部封顶（隔离光照）；周边已在 padding 阶段清理
    // TODO: 完整屏障包裹（玻璃/屏障方块边界）待 1C 后续完善；当前仅清理周边 air 隔离
    if (!data.skyAccess() && data.padding() > 0) {
        const StructureBoundingBox topBox(placedBox.minX() - data.padding(),
            placedBox.maxY() + 1,
            placedBox.minZ() - data.padding(),
            placedBox.maxX() + data.padding(),
            placedBox.maxY() + 1,
            placedBox.maxZ() + data.padding());
        _clearBox(world, topBox);
    }

    // 结构尺寸由 placedBox 跨度推导（旋转后权威值）
    const BlockPos size{
        placedBox.maxX() - placedBox.minX() + 1,
        placedBox.maxY() - placedBox.minY() + 1,
        placedBox.maxZ() - placedBox.minZ() + 1,
    };
    return std::make_unique<StructureBounds>(origin, size, data.rotation());
}

void MinecraftStructurePlacer::clearArea(mc::server::ServerWorld& world, const StructureBounds& bounds, i32 padding)
{
    _clearBox(world, bounds.paddingBounds(padding));
}

} // namespace mc::test
