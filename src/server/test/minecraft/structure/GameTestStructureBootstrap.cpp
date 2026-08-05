#include "server/test/minecraft/structure/GameTestStructureBootstrap.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp" // JigsawAssembler::getTemplateManager

#include <spdlog/spdlog.h>

namespace mc::test {

namespace {

// 全限定命名空间路径，规避 mc::test 内非限定名两段查找不回退 mc::world 的遮蔽坑（见 BossBarState 内存）
using TemplateManager = mc::world::gen::feature::template_::TemplateManager;
using JigsawAssembler = mc::world::gen::jigsaw::JigsawAssembler;

// 注入 namespace 前缀：所有内置 GameTest 结构走 gametest: 命名空间
constexpr const char* kGameTestNamespace = "gametest";

} // namespace

bool injectProceduralStructure(std::string_view name, i32 width, i32 height, i32 depth)
{
    auto& tm = JigsawAssembler::getTemplateManager();
    auto tpl = tm.createProceduralTemplate(std::string(name), width, height, depth);
    if (tpl == nullptr) {
        spdlog::warn("[GameTest] failed to create procedural structure template '{}'", std::string(name));
        return false;
    }
    const mc::resource::ResourceLocation loc{std::string(kGameTestNamespace), std::string(name)};
    tm.addTemplate(loc, std::move(tpl));
    return true;
}

void ensureBuiltinStructureTemplates()
{
    // empty_3x3：框架样例测试（BuiltinNativeTests）与 JS register 默认结构引用。
    // 3×3×3 全 air，placeInWorld 立即成功不写方块，结构放置阶段不 fail。
    // TODO: 后续提供正式 .nbt 资源到资源包后，可移除此程序化兜底（当前资源缺失）。
    injectProceduralStructure("empty_3x3", 3, 3, 3);
}

} // namespace mc::test
