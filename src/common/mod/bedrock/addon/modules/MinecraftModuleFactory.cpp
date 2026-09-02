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

#include "common/mod/bedrock/addon/modules/MinecraftModuleFactory.hpp"

#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"        // mc::Entity（Entity JS 类 opaque 持此指针）
#include "common/entity/core/EquipmentSlot.hpp" // EquipmentSlot 枚举（EquippableComponent 槽位映射）
#include "common/entity/core/LivingEntity.hpp"  // LivingEntity（health/maxHealth/attributes/getEquipment）
#include "common/entity/core/MobEntity.hpp"     // MobEntity（setEquipmentDropChance，EquippableComponent 装备掉落概率）
#include "common/entity/effect/EffectInstance.hpp" // EffectInstance（getEffect/getEffects 返回效果实例）
#include "common/entity/effect/EffectType.hpp"     // EffectType + getEffectByResourceLocation/getEffectResourceLocation
#include "common/entity/entities/monster/basic/CreeperEntity.hpp" // CreeperEntity（is_charged 组件判定 isPowered）
#include "common/entity/entities/passive/basic/MooshroomEntity.hpp" // MooshroomEntity（mark_variant 组件判定哞菇红/棕变种）
#include "common/entity/entities/passive/basic/SheepEntity.hpp" // SheepEntity（color 组件判定羊毛颜色 DyeColor 0-15）
#include "common/entity/entities/passive/fish/PufferfishEntity.hpp" // PufferfishEntity（pufferfish_puff_state 组件判定膨胀等级）
#include "common/entity/entities/passive/horse/AbstractChestedHorseEntity.hpp" // AbstractChestedHorseEntity（is_chested 组件判定驴/骡/行商羊驼箱子状态，hasChest）
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp" // AbstractHorseEntity（is_saddled 组件判定马类鞍状态，马类不实现 IRideable 但有 hasSaddle）
#include "common/entity/entities/passive/tamable/TameableEntity.hpp" // TameableEntity（is_tamed 组件判定驯服状态，狼/猫/鹦鹉等驯服类基类）
#include "common/entity/entities/passive/water/GlowSquidEntity.hpp" // GlowSquidEntity（glow_squid_dark_ticks 组件判定受惊暗化剩余 tick）
#include "common/entity/entities/player/Player.hpp" // Player::username/Player::inventory（Player.name / Container）
#include "common/entity/interfaces/IRideable.hpp" // mc::entity::IRideable（is_saddled 组件判定鞍状态，猪/炽足兽/鹦鹉螺等可骑乘实体实现此接口）
#include "common/entity/inventory/IInventory.hpp"        // IInventory（Container JS 类 opaque 持此指针）
#include "common/item/core/Item.hpp"                     // Item::toString/Item::getItem（ItemStack.typeId / ItemType）
#include "common/item/core/ItemRegistry.hpp"             // ItemRegistry::getItem（ItemType/ItemStack 按 id 取 Item）
#include "common/item/core/ItemStack.hpp"                // ItemStack（Equippable.getEquipment 返回值）
#include "common/item/enchantment/EnchantmentHelper.hpp" // EnchantmentHelper::getEnchantments（ItemStack.getEnchantments）
#include "common/item/enchantment/EnchantmentRegistry.hpp" // EnchantmentRegistry::get（ItemStack.addEnchantment 附魔 id 存在性校验）
#include "common/mod/bedrock/addon/binding/ScriptBlockRef.hpp" // ScriptBlockRef/wrapBlock/unwrapBlock（Block JS 类 opaque 快照）
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp" // 跨模块 classId/proto 注册表
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/ModuleDependency.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptScheduler.hpp"
#include "common/mod/bedrock/addon/modules/ScriptCustomComponentBinding.hpp"
#include "common/mod/bedrock/addon/modules/ScriptEventBinding.hpp"
#include "common/mod/bedrock/addon/modules/types/ScriptWorldAccessor.hpp"
#include "common/resource/ResourceLocation.hpp"       // ResourceLocation::toString/parse（Block/ItemType typeId）
#include "common/scoreboard/core/Score.hpp"           // Score::getScorePoints（Objective.getScore 返回值）
#include "common/scoreboard/core/ScoreObjective.hpp"  // ScoreObjective（Objective JS 类 opaque 持此指针）
#include "common/scoreboard/core/ScorePlayerTeam.hpp" // ScorePlayerTeam（Team JS 类 opaque 持此指针）
#include "common/scoreboard/core/Scoreboard.hpp"      // Scoreboard::getObjective/getScore（Scoreboard JS 类）
#include "common/scoreboard/core/TeamEnums.hpp" // teamVisibilityToString/teamCollisionRuleToString（Team 属性字符串化）
#include "common/util/AxisAlignedBB.hpp"        // Dimension.getEntities 构造查询包围盒
#include "common/util/Direction.hpp"            // mc::Direction / Directions::fromName/toString（Direction 枚举导出）
#include "common/util/math/Vector3.hpp"
#include "common/util/text/TextStyle.hpp"       // text::toName(TextFormatting)（Team.color 字符串化）
#include "common/world/GlobalPos.hpp"           // GlobalPos（Player.getSpawnPoint 返 optional<GlobalPos>）
#include "common/world/IWorld.hpp"              // Dimension JS 类 opaque 持 IWorld*
#include "common/world/block/Block.hpp"         // Block::blockLocation/defaultState/getBlock（Block/BlockPermutation）
#include "common/world/block/BlockPos.hpp"      // BlockPos（Block.location 坐标）
#include "common/world/block/BlockRegistry.hpp" // BlockRegistry::get/getBlock（按 id 取 BlockState/Block）
#include "common/world/block/BlockState.hpp"    // BlockState（Block/BlockPermutation opaque 持此指针）
#include "common/world/blockentity/BlockEntity.hpp"             // BlockEntity（Container 经 getBlockEntity 取得）
#include "common/world/blockentity/ContainerBlockEntity.hpp"    // ContainerBlockEntity::getInventory（Container 底层）
#include "common/world/blockentity/processing/BeaconEntity.hpp" // BeaconEntity::getLevel（Block.beaconLevel 读信标等级）
#include "common/world/border/WorldBorder.hpp"                  // WorldBorder JS 类读 IWorld::worldBorder() 各 getter
#include "common/world/gamerule/GameRules.hpp"                  // Dimension.getGameRule 经 IWorld::getGameRules 取值

#include <optional>
#include <unordered_map>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

namespace {

// ============================================================================
// @minecraft/server 类补齐辅助（批次2）
//
// 下列 helper 仅供本 TU 的 Block/BlockPermutation/ItemType/Container/ItemStack 等
// 绑定回调使用。opaque 持指针语义统一经 ScriptObjectRegistry::wrap/unwrap；JS 侧
// Direction 是字符串枚举（官方首字母大写），项目 mc::Direction 配套 fromName/toString
// 用小写，故出入参须做大小写转换。
// ============================================================================

// 注：ScriptBlockRef 结构体已提升至公共头 ScriptBlockRef.hpp，供 server 侧
// （ScriptTestHelper 的 getBlock/assertBlockState 绑定）跨模块构造 Block JS 对象。
// Direction 字符串↔mc::Direction 转换、按 id 取 BlockState/Item 的 helper 在批次4/6（setBlock/
// spawnItem/rotateDirection 等）引入时按需补充，避免本批携带未使用函数触发 -Wunused-function。

/// ItemStack JS 类 classId 缓存（首次 unwrap 时惰性查 ScriptClassRegistry，避免每调用查表）。
/// 注：本 TU 跨多个回调共享，故用静态局部；引擎重建后 classId 变化，每次仍回查注册表校正。
u64 resolveItemStackClassId()
{
    return ScriptClassRegistry::instance().classIdByName("ItemStack");
}

} // namespace

void MinecraftModuleFactory::setScheduler(ScriptScheduler* scheduler)
{
    m_scheduler = scheduler;
}

void MinecraftModuleFactory::setEventSignals(const std::vector<EventSignalInfo>& signals)
{
    m_eventSignals = signals;
}

void MinecraftModuleFactory::setEventBus(ScriptEventBus* eventBus)
{
    m_eventBus = eventBus;
}

std::vector<ModuleVersion> MinecraftModuleFactory::supportedVersions() const
{
    return {ModuleVersion{2, 0, 0}, ModuleVersion{1, 17, 0}};
}

std::vector<ModuleDependency> MinecraftModuleFactory::dependencies(const ModuleVersion& version) const
{
    return {};
}

bool MinecraftModuleFactory::registerBindings(IScriptContext& context)
{
    auto& ctx = context.bindingContext();

    spdlog::info("[BedrockAddon] Registering @minecraft/server module bindings");

    // 创建模块构建器
    NativeModuleBuilder builder(ctx, "@minecraft/server");

    // ====== 注册类 ======

    // --- System类 ---
    u64 systemClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* systemProto = builder.exportClass("System", systemClassId);

    ClassRegistrar<void> systemReg(ctx, systemClassId, systemProto);

    ScriptScheduler* scheduler = m_scheduler;

    systemReg.method(
        "run",
        [scheduler](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("system.run requires a function argument");
            }
            if (!scheduler) {
                return ctx.throwInternalError("Script scheduler not available");
            }

            // 保留回调引用（持久化到调度执行时）
            void* callback = args[0];
            ctx.retainValue(callback);
            auto* ctxPtr = &ctx;

            auto runId = scheduler->run([ctxPtr, callback]() {
                void* undef = ctxPtr->createUndefined();
                void* result = ctxPtr->callFunction0(callback, undef);
                if (ctxPtr->isException(result)) {
                    void* exc = ctxPtr->getException();
                    auto msg = ctxPtr->getExceptionMessage(exc);
                    spdlog::warn("[BedrockAddon] system.run callback error: {}", msg);
                    ctxPtr->releaseValue(exc);
                }
                ctxPtr->releaseValue(result);
                ctxPtr->releaseValue(undef);
                ctxPtr->releaseValue(callback);
            });

            return ctx.createInt32(static_cast<i32>(runId));
        },
        1);

    systemReg.method(
        "runInterval",
        [scheduler](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("system.runInterval requires a function argument");
            }

            u32 tickInterval = 1;
            if (argc >= 2 && ctx.isNumber(args[1])) {
                auto interval = ctx.toInt32(args[1]);
                if (interval && *interval > 0) {
                    tickInterval = static_cast<u32>(*interval);
                }
            }

            if (!scheduler) {
                return ctx.throwInternalError("Script scheduler not available");
            }

            void* callback = args[0];
            ctx.retainValue(callback);
            auto* ctxPtr = &ctx;

            auto runId = scheduler->runInterval(
                [ctxPtr, callback]() {
                    void* undef = ctxPtr->createUndefined();
                    void* result = ctxPtr->callFunction0(callback, undef);
                    if (ctxPtr->isException(result)) {
                        void* exc = ctxPtr->getException();
                        auto msg = ctxPtr->getExceptionMessage(exc);
                        spdlog::warn("[BedrockAddon] system.runInterval callback error: {}", msg);
                        ctxPtr->releaseValue(exc);
                    }
                    ctxPtr->releaseValue(result);
                    ctxPtr->releaseValue(undef);
                },
                tickInterval);

            return ctx.createInt32(static_cast<i32>(runId));
        },
        2);

    systemReg.method(
        "runTimeout",
        [scheduler](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("system.runTimeout requires a function argument");
            }

            u32 tickDelay = 1;
            if (argc >= 2 && ctx.isNumber(args[1])) {
                auto delay = ctx.toInt32(args[1]);
                if (delay && *delay > 0) {
                    tickDelay = static_cast<u32>(*delay);
                }
            }

            if (!scheduler) {
                return ctx.throwInternalError("Script scheduler not available");
            }

            void* callback = args[0];
            ctx.retainValue(callback);
            auto* ctxPtr = &ctx;

            auto runId = scheduler->runTimeout(
                [ctxPtr, callback]() {
                    void* undef = ctxPtr->createUndefined();
                    void* result = ctxPtr->callFunction0(callback, undef);
                    if (ctxPtr->isException(result)) {
                        void* exc = ctxPtr->getException();
                        auto msg = ctxPtr->getExceptionMessage(exc);
                        spdlog::warn("[BedrockAddon] system.runTimeout callback error: {}", msg);
                        ctxPtr->releaseValue(exc);
                    }
                    ctxPtr->releaseValue(result);
                    ctxPtr->releaseValue(undef);
                    ctxPtr->releaseValue(callback);
                },
                tickDelay);

            return ctx.createInt32(static_cast<i32>(runId));
        },
        2);

    systemReg.method(
        "clearRun",
        [scheduler](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("system.clearRun requires a run ID argument");
            }
            if (!scheduler) {
                return ctx.createUndefined();
            }
            auto runId = ctx.toInt32(args[0]);
            if (!runId) {
                return ctx.createUndefined();
            }
            scheduler->clearRun(static_cast<ScriptScheduler::RunId>(*runId));
            return ctx.createUndefined();
        },
        1);

    systemReg.readonlyProperty("currentTick", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        return ctx.createInt64(static_cast<i64>(ScriptWorldAccessor::instance().currentTick()));
    });

    // --- World类 ---
    u64 worldClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* worldProto = builder.exportClass("World", worldClassId);

    ClassRegistrar<void> worldReg(ctx, worldClassId, worldProto);
    worldReg.method("getDimension", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
        // 对齐基岩 world.getDimension(id: string): Dimension。按维度名经 ScriptWorldAccessor
        // 回调解析（ServerDimensionManager，含 "minecraft:nether"->"minecraft:the_nether" 归一化），
        // 拿到 IWorld* 后 wrap 成 Dimension JS 对象（owned=false，世界由服务器管理）。
        // 模板来源：ScriptTestHelper.cpp 的 test.getDimension 绑定（classIdByName 动态查，
        // 不捕获 dimensionClassId——其在下方 :316 才声明，捕获会编译失败）。
        MC_UNUSED(thisVal);
        if (argc < 1 || !ctx.isString(args[0])) {
            return ctx.throwTypeError("world.getDimension requires a string argument");
        }
        auto idOpt = ctx.toString(args[0]);
        if (!idOpt) {
            return ctx.createUndefined();
        }
        mc::IWorld* world = ScriptWorldAccessor::instance().getDimension(*idOpt);
        if (world == nullptr) {
            return ctx.createUndefined(); // 维度不存在或回调未注册
        }
        const u64 dimClassId = ScriptClassRegistry::instance().classIdByName("Dimension");
        void* dimProto = ScriptClassRegistry::instance().proto(dimClassId);
        if (dimProto == nullptr) {
            return ctx.createUndefined(); // Dimension 类未注册（模块未加载）
        }
        return ScriptObjectRegistry::wrap(ctx, dimClassId, dimProto, world, false, "Dimension");
    });
    worldReg.method("getAllPlayers", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
        auto names = ScriptWorldAccessor::instance().getAllPlayerNames();
        void* arr = ctx.createArray();
        for (u32 i = 0; i < names.size(); ++i) {
            ctx.setArrayElementString(arr, i, names[i]);
        }
        return arr;
    });
    worldReg.method("sendMessage", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
        if (argc < 1 || !ctx.isString(args[0])) {
            return ctx.throwTypeError("world.sendMessage requires a string argument");
        }
        auto msg = ctx.toString(args[0]);
        if (!msg) {
            return ctx.throwInternalError("Failed to convert message to string");
        }
        ScriptWorldAccessor::instance().sendMessage(*msg);
        return ctx.createUndefined();
    });

    // world.scoreboard：对齐基岩 world.scoreboard 只读属性，返回服务器 Scoreboard JS 对象。
    // 经 ScriptWorldAccessor::getScoreboard() 取 ServerScoreboard*（向上转 Scoreboard 基类），
    // wrap 成 Scoreboard JS 对象（owned=false，记分板由服务器管理，生命周期随服务器）。
    // 供 GameTest JS 经 world.scoreboard.getObjective(name).getScore(participant) 读分数做断言。
    worldReg.readonlyProperty("scoreboard", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        MC_UNUSED(thisVal);
        auto* scoreboard = ScriptWorldAccessor::instance().getScoreboard();
        if (scoreboard == nullptr) {
            return ctx.createUndefined(); // 回调未注册（非服务器环境）
        }
        const u64 sbClassId = ScriptClassRegistry::instance().classIdByName("Scoreboard");
        void* sbProto = ScriptClassRegistry::instance().proto(sbClassId);
        if (sbProto == nullptr) {
            return ctx.createUndefined(); // Scoreboard 类未注册
        }
        return ScriptObjectRegistry::wrap(ctx, sbClassId, sbProto, scoreboard, false, "Scoreboard");
    });

    // world.bossbar：返回 BossBarManager JS 对象（单例 facade，方法经 ScriptWorldAccessor 全局回调）。
    // BossBar 类型在 server 层，common 层以 BossBarView 值快照桥接，故 Manager/BossBar JS 对象不持
    // server 层指针：Manager opaque 持哨兵（无状态），BossBar opaque 持堆 id 字符串（owned，GC 释放）。
    // 供 /bossbar 命令测试经 world.bossbar.get(id).value/color/... 读取属性做断言。
    worldReg.readonlyProperty("bossbar", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        MC_UNUSED(thisVal);
        const u64 mgrClassId = ScriptClassRegistry::instance().classIdByName("BossBarManager");
        void* mgrProto = ScriptClassRegistry::instance().proto(mgrClassId);
        if (mgrProto == nullptr) {
            return ctx.createUndefined(); // BossBarManager 类未注册
        }
        // 哨兵指针：Manager 无状态，opaque 仅作占位（owned=false 不销毁）。每次访问 world.bossbar 返新
        // JS 对象，但均无状态，等价单例。
        static int s_sentinel = 0;
        return ScriptObjectRegistry::wrap(ctx, mgrClassId, mgrProto, &s_sentinel, false, "BossBarManager");
    });

    // world.getDefaultSpawn：返回世界出生点 {x,y,z,angle}（经 ScriptWorldAccessor::getWorldSpawn 取主世界
    // ServerWorld 快照）。ServerWorld 在 server 层，common 层以 WorldSpawnView 值桥接。供 /setworldspawn
    // 命令测试读取世界出生点做断言。每次访问重新取快照保证 set 后 JS 立即可见。回调未注册返 undefined。
    // Cubium 扩展属性（官方基岩 API world 无 getDefaultSpawn，1.21.x 新增 getDefaultSpawnLocation 但本项目
    // 命名沿用基岩旧风格）。
    worldReg.readonlyProperty("getDefaultSpawn", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        MC_UNUSED(thisVal);
        auto view = ScriptWorldAccessor::instance().getWorldSpawn();
        if (!view.exists) {
            return ctx.createUndefined();
        }
        void* obj = ctx.createObject();
        ctx.setPropertyFloat(obj, "x", view.x);
        ctx.setPropertyFloat(obj, "y", view.y);
        ctx.setPropertyFloat(obj, "z", view.z);
        ctx.setPropertyFloat(obj, "angle", static_cast<f64>(view.angle));
        return obj;
    });

    // world.getWorldBorder()：返回主世界 WorldBorder JS 对象（对齐基岩 world.getWorldBorder(): WorldBorder）。
    // WorldBorder 是 common 层类型（IWorld::worldBorder() 返回 world::border::WorldBorder&，common 层虚函数），
    // 故脚本绑定可直接访问，无需 ScriptWorldAccessor 值快照桥接（区别于 server 层的 BossBar/WorldSpawn）。
    // 取主世界 IWorld*（GameTest 单世界场景，/worldborder 作用于执行者所在维度=主世界），wrap 成 WorldBorder
    // JS 对象（owned=false，IWorld* 由服务器管理）。WorldBorder 类在下方注册，用 classIdByName 动态查
    // （不捕获 classId，其在下方才声明，捕获会编译失败，同 world.getDimension 模式）。
    worldReg.method("getWorldBorder", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
        MC_UNUSED(thisVal);
        MC_UNUSED(argc);
        MC_UNUSED(args);
        mc::IWorld* world = ScriptWorldAccessor::instance().getDimension("minecraft:overworld");
        if (world == nullptr) {
            return ctx.createUndefined(); // 维度不存在或回调未注册
        }
        const u64 borderClassId = ScriptClassRegistry::instance().classIdByName("WorldBorder");
        void* borderProto = ScriptClassRegistry::instance().proto(borderClassId);
        if (borderProto == nullptr) {
            return ctx.createUndefined(); // WorldBorder 类未注册（模块未加载）
        }
        return ScriptObjectRegistry::wrap(ctx, borderClassId, borderProto, world, false, "WorldBorder");
    });

    // --- Dimension类 ---
    // opaque 持 mc::IWorld*（非拥有，世界由服务器管理）。GameTest 单维度场景下维度即世界；
    // test.getDimension() 与 world.getDimension() 均 wrap 同一 IWorld*。getEntities 按基岩语义
    // {type, location, volume} 查询：location 是包围盒的 min 角点，volume 是从该角点延伸的尺寸，
    // AABB = [location, location + volume]。这与 utils/entity/assert.ts 的角点意图一致
    // （assert.ts 取 from 角点为 location、to-from 为 volume），基岩 BDS 实测同此语义。
    u64 dimensionClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* dimensionProto = builder.exportClass("Dimension", dimensionClassId);
    ScriptClassRegistry::instance().registerClass(dimensionClassId, dimensionProto, "Dimension");

    ClassRegistrar<void> dimensionReg(ctx, dimensionClassId, dimensionProto);
    dimensionReg.readonlyProperty("id", [dimensionClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 读 IWorld::dimension()（IWorld.hpp:975 纯虚，ServerWorld 实现）映射到基岩维度名字符串。
        // 输出侧用基岩名（"minecraft:nether" 非 "minecraft:the_nether"），与 world.getDimension
        // 读入侧的归一化（ServerScriptManager）对称。nullptr 容错返回 overworld（不应发生）。
        // DimensionId 取值：0=OVERWORLD，-1=NETHER，1=THE_END（DimensionManager.hpp:64-70）。
        auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, dimensionClassId));
        if (world == nullptr) {
            return ctx.createString("minecraft:overworld");
        }
        switch (world->dimension()) {
            case 0:
                return ctx.createString("minecraft:overworld");
            case -1:
                return ctx.createString("minecraft:nether");
            case 1:
                return ctx.createString("minecraft:the_end");
            default:
                return ctx.createString("minecraft:overworld");
        }
    });
    dimensionReg.method(
        "getEntities",
        [dimensionClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, dimensionClassId));
            if (world == nullptr) {
                return ctx.createArray(); // 空数组，property getter 不抛
            }
            // 解析查询参数 {type?, location?, volume?}（均可选，对齐基岩 EntityQueryOptions）
            std::string typeFilter;
            bool hasType = false;
            // 基岩 EntityQueryOptions.type 既接受 "minecraft:zoglin" 也接受 "zoglin"（自动补前缀）。
            // getTypeId() 恒返回带 "minecraft:" 前缀的完整 id，故将 typeFilter 规范化为完整 id 后比较。
            // 此前 normalizedType 仅在 hasLoc&&hasVol 分支内声明，无 location 的全类型遍历分支
            // （下方 getEntitiesByType）误用未规范化的 typeFilter（如 "spider"），与 getTypeId()
            // （"minecraft:spider"）不等致 getEntities({type:"spider"}) 恒返回空——蜘蛛攻击测试
            // succeedWhen 取不到实体而超时。此处在外层计算，AABB 与全类型两分支共用。
            std::string normalizedType;
            if (argc >= 1 && ctx.isObject(args[0])) {
                void* opts = args[0];
                void* typeVal = ctx.getProperty(opts, "type");
                if (typeVal != nullptr && ctx.isString(typeVal)) {
                    auto t = ctx.toString(typeVal);
                    if (t) {
                        typeFilter = *t;
                        hasType = true;
                    }
                }
                ctx.releaseValue(typeVal);
                normalizedType = typeFilter;
                if (hasType && normalizedType.find(':') == std::string::npos) {
                    normalizedType = "minecraft:" + normalizedType;
                }

                // location 中心 + volume 全尺寸 → AABB
                f32 lx = 0, ly = 0, lz = 0;
                bool hasLoc = false;
                void* locVal = ctx.getProperty(opts, "location");
                if (locVal != nullptr && ctx.isObject(locVal)) {
                    auto x = ctx.getPropertyFloat(locVal, "x");
                    auto y = ctx.getPropertyFloat(locVal, "y");
                    auto z = ctx.getPropertyFloat(locVal, "z");
                    if (x && y && z) {
                        lx = static_cast<f32>(*x);
                        ly = static_cast<f32>(*y);
                        lz = static_cast<f32>(*z);
                        hasLoc = true;
                    }
                }
                ctx.releaseValue(locVal);

                f32 vx = 0, vy = 0, vz = 0;
                bool hasVol = false;
                void* volVal = ctx.getProperty(opts, "volume");
                if (volVal != nullptr && ctx.isObject(volVal)) {
                    auto x = ctx.getPropertyFloat(volVal, "x");
                    auto y = ctx.getPropertyFloat(volVal, "y");
                    auto z = ctx.getPropertyFloat(volVal, "z");
                    if (x && y && z) {
                        vx = static_cast<f32>(*x);
                        vy = static_cast<f32>(*y);
                        vz = static_cast<f32>(*z);
                        hasVol = true;
                    }
                }
                ctx.releaseValue(volVal);

                if (hasLoc && hasVol) {
                    // 角点语义：location 是 min 角点，AABB = [location, location + volume]。
                    mc::AxisAlignedBB box(lx, ly, lz, lx + vx, ly + vy, lz + vz);
                    auto entities = world->getEntitiesInAABB(box, nullptr);
                    void* arr = ctx.createArray();
                    u32 outIdx = 0;
                    const u64 entClassId = ScriptClassRegistry::instance().classIdByName("Entity");
                    void* entProto = ScriptClassRegistry::instance().proto(entClassId);
                    for (auto* ent : entities) {
                        if (ent == nullptr) {
                            continue;
                        }
                        if (hasType && ent->getTypeId() != normalizedType) {
                            continue;
                        }
                        if (entProto != nullptr) {
                            // 传 ent->id() 登记 ScriptHandleRegistry：实体销毁时置 ptr=nullptr 防 UAF
                            // （owned=false 裸 Entity* 跨 tick 悬垂，见 ScriptHandleRegistry.hpp）。
                            void* jsEnt = ScriptObjectRegistry::wrap(
                                ctx, entClassId, entProto, ent, false, "Entity", nullptr, ent->id());
                            ctx.setArrayElement(arr, outIdx, jsEnt); // 不消耗所有权，须手动 release
                            ctx.releaseValue(jsEnt);
                            ++outIdx;
                        }
                    }
                    return arr;
                }
            }
            // 无 location/volume：退化为全类型遍历（仅按 type 过滤），对齐基岩空 query 语义。
            // TODO: 无区域约束的全局遍历在大世界性能差，GameTest 小范围场景可接受；生产侧慎用。
            if (hasType) {
                auto entities = world->getEntitiesByType(normalizedType);
                void* arr = ctx.createArray();
                u32 outIdx = 0;
                const u64 entClassId = ScriptClassRegistry::instance().classIdByName("Entity");
                void* entProto = ScriptClassRegistry::instance().proto(entClassId);
                for (auto* ent : entities) {
                    if (ent == nullptr || entProto == nullptr) {
                        continue;
                    }
                    void* jsEnt =
                        ScriptObjectRegistry::wrap(ctx, entClassId, entProto, ent, false, "Entity", nullptr, ent->id());
                    ctx.setArrayElement(arr, outIdx, jsEnt);
                    ctx.releaseValue(jsEnt);
                    ++outIdx;
                }
                return arr;
            }
            return ctx.createArray();
        },
        1);

    // Dimension.getTimeOfDay()：对齐基岩 world.getTimeOfDay()，返回当前维度一天内时间 (0-23999)。
    // IWorld::dayTimeOfDay() 返回 dayTime % 24000。/time set 修改 dayTime 后此处立即可读，
    // 解锁 TimeCommand 端到端测试（此前脚本侧仅 system.currentTick 是游戏总 tick 非 dayTime，
    // 无法断言 /time set 效果）。
    dimensionReg.method(
        "getTimeOfDay",
        [dimensionClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, dimensionClassId));
            if (world == nullptr) {
                return ctx.createInt32(0);
            }
            return ctx.createInt32(static_cast<i32>(world->dayTimeOfDay()));
        },
        0);
    // Dimension.getDayTime()：返回原始 dayTime（可超 24000，累计天数）。对齐基岩 getDayTime()。
    dimensionReg.method(
        "getDayTime",
        [dimensionClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, dimensionClassId));
            if (world == nullptr) {
                return ctx.createInt64(0);
            }
            return ctx.createInt64(world->dayTime());
        },
        0);

    // Dimension.isRaining()/isThundering()：对齐基岩 world 状态读取。IWorld::isRaining/isThundering
    // 经 ServerWorld override 委托 WeatherManager（检查 rainStrength/thunderStrength 超阈值，非裸标志）。
    // 注：/weather rain 设置后 rainStrength 有渐变延迟，刚设置时 isRaining 可能仍 false，
    // 测试须等待强度渐变（runAtTickTime 延迟或 pollUntilSucceed 轮询）。
    dimensionReg.method(
        "isRaining",
        [dimensionClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, dimensionClassId));
            if (world == nullptr) {
                return ctx.createBoolean(false);
            }
            return ctx.createBoolean(world->isRaining());
        },
        0);
    dimensionReg.method(
        "isThundering",
        [dimensionClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, dimensionClassId));
            if (world == nullptr) {
                return ctx.createBoolean(false);
            }
            return ctx.createBoolean(world->isThundering());
        },
        0);

    // Dimension.getGameRule(ruleName)：按名取 gamerule 当前值的字符串表示。对齐基岩无直接 API
    // （基岩 gamerule 仅命令侧），此处补脚本读取入口解锁 GameRuleCommand 端到端测试。
    // 布尔规则返 "true"/"false"，整数规则返十进制串，规则不存在返空串。GameRules::getValueAsString
    // 优先取当前值 map，回退注册表默认值。
    dimensionReg.method(
        "getGameRule",
        [dimensionClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, dimensionClassId));
            if (world == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createString("");
            }
            auto name = ctx.toString(args[0]);
            if (!name) {
                return ctx.createString("");
            }
            return ctx.createString(world->getGameRules().getValueAsString(*name));
        },
        1);

    // Dimension.getDifficulty()：返回当前世界难度的字符串名（"peaceful"/"easy"/"normal"/"hard"）。
    // 对齐基岩无直接脚本读取入口（基岩 difficulty 仅命令侧），此处补脚本读取解锁 DifficultyCommand
    // 端到端测试。IWorld::difficulty() 经 ServerWorld override 委托真实难度表。Difficulty 枚举
    // (Peaceful=0/Easy=1/Normal=2/Hard=3，Types.hpp:153) 本地映射到命令名串。
    dimensionReg.method(
        "getDifficulty",
        [dimensionClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, dimensionClassId));
            if (world == nullptr) {
                return ctx.createString("");
            }
            switch (world->difficulty()) {
                case mc::Difficulty::Peaceful:
                    return ctx.createString("peaceful");
                case mc::Difficulty::Easy:
                    return ctx.createString("easy");
                case mc::Difficulty::Normal:
                    return ctx.createString("normal");
                case mc::Difficulty::Hard:
                    return ctx.createString("hard");
                default:
                    return ctx.createString("");
            }
        },
        0);

    // --- WorldBorder类 ---
    // opaque 持 mc::IWorld*（非拥有，世界由服务器管理）。world.getWorldBorder() wrap 主世界 IWorld*。
    // 属性每次从 IWorld::worldBorder() 取最新值（WorldBorder 是 common 层类型，IWorld::worldBorder() 是
    // common 层虚函数返回 world::border::WorldBorder&，故脚本绑定可直接调 getter，无需 ScriptWorldAccessor
    // 值快照桥接）。对齐基岩 world.getWorldBorder(): WorldBorder 的属性集：
    //   size(直径)、center({x,z})、damagePerBlock、damageSafeZone(buffer)、warningBlocks、warningTime。
    // 供 /worldborder set/center/damage/warning 命令测试读取边界状态做断言。
    u64 worldBorderClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* worldBorderProto = builder.exportClass("WorldBorder", worldBorderClassId);
    ScriptClassRegistry::instance().registerClass(worldBorderClassId, worldBorderProto, "WorldBorder");

    ClassRegistrar<void> worldBorderReg(ctx, worldBorderClassId, worldBorderProto);
    // size：当前边界直径（getSize，渐变中为实时插值值）。/worldborder set/add 修改。
    worldBorderReg.readonlyProperty("size", [worldBorderClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, worldBorderClassId));
        if (world == nullptr) {
            return ctx.createFloat64(0.0);
        }
        return ctx.createFloat64(world->worldBorder().getSize());
    });
    // center：边界中心 {x, z}。/worldborder center 修改。
    worldBorderReg.readonlyProperty("center", [worldBorderClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, worldBorderClassId));
        void* obj = ctx.createObject();
        if (world == nullptr) {
            ctx.setPropertyFloat(obj, "x", 0.0);
            ctx.setPropertyFloat(obj, "z", 0.0);
            return obj;
        }
        const auto& border = world->worldBorder();
        ctx.setPropertyFloat(obj, "x", border.getCenterX());
        ctx.setPropertyFloat(obj, "z", border.getCenterZ());
        return obj;
    });
    // damagePerBlock：超出边界每格伤害（getDamagePerBlock）。/worldborder damage amount 修改。
    worldBorderReg.readonlyProperty(
        "damagePerBlock", [worldBorderClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, worldBorderClassId));
            if (world == nullptr) {
                return ctx.createFloat64(0.0);
            }
            return ctx.createFloat64(world->worldBorder().getDamagePerBlock());
        });
    // damageSafeZone：伤害安全距离/缓冲（getDamageBuffer）。/worldborder damage buffer 修改。
    // 基岩属性名 damageSafeZone 对应 vanilla damageBuffer。
    worldBorderReg.readonlyProperty(
        "damageSafeZone", [worldBorderClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, worldBorderClassId));
            if (world == nullptr) {
                return ctx.createFloat64(0.0);
            }
            return ctx.createFloat64(world->worldBorder().getDamageBuffer());
        });
    // warningBlocks：警告距离（getWarningDistance）。/worldborder warning distance 修改。
    worldBorderReg.readonlyProperty(
        "warningBlocks", [worldBorderClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, worldBorderClassId));
            if (world == nullptr) {
                return ctx.createInt32(0);
            }
            return ctx.createInt32(world->worldBorder().getWarningDistance());
        });
    // warningTime：警告时间秒（getWarningTime）。/worldborder warning time 修改。
    worldBorderReg.readonlyProperty(
        "warningTime", [worldBorderClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, worldBorderClassId));
            if (world == nullptr) {
                return ctx.createInt32(0);
            }
            return ctx.createInt32(world->worldBorder().getWarningTime());
        });

    // --- Scoreboard类 ---
    // opaque 持 mc::scoreboard::Scoreboard*（非拥有，ServerScoreboard 由服务器管理）。world.scoreboard
    // 属性 wrap 服务器记分板。getObjective/getObjectives 读目标，供 GameTest JS 验证 /scoreboard 命令
    // 写入的分数。Cubium 的 score holder 是字符串名（ScoreboardCommand target 是裸字符串非选择器，
    // 见 [[scoreboard-script-binding-and-command-tests]]），故 getScore/getParticipants 用字符串名索引。
    u64 scoreboardClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* scoreboardProto = builder.exportClass("Scoreboard", scoreboardClassId);
    ScriptClassRegistry::instance().registerClass(scoreboardClassId, scoreboardProto, "Scoreboard");

    ClassRegistrar<void> scoreboardReg(ctx, scoreboardClassId, scoreboardProto);
    scoreboardReg.method(
        "getObjective",
        [scoreboardClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* scoreboard =
                static_cast<mc::scoreboard::Scoreboard*>(ScriptObjectRegistry::unwrap(ctx, thisVal, scoreboardClassId));
            if (scoreboard == nullptr) {
                return ctx.createUndefined();
            }
            if (argc < 1 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("scoreboard.getObjective requires a string argument");
            }
            auto nameOpt = ctx.toString(args[0]);
            if (!nameOpt) {
                return ctx.createUndefined();
            }
            auto* objective = scoreboard->getObjective(*nameOpt);
            if (objective == nullptr) {
                return ctx.createUndefined(); // 目标不存在
            }
            const u64 objClassId = ScriptClassRegistry::instance().classIdByName("Objective");
            void* objProto = ScriptClassRegistry::instance().proto(objClassId);
            if (objProto == nullptr) {
                return ctx.createUndefined(); // Objective 类未注册
            }
            return ScriptObjectRegistry::wrap(ctx, objClassId, objProto, objective, false, "Objective");
        },
        1);
    scoreboardReg.method(
        "getObjectives",
        [scoreboardClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* scoreboard =
                static_cast<mc::scoreboard::Scoreboard*>(ScriptObjectRegistry::unwrap(ctx, thisVal, scoreboardClassId));
            if (scoreboard == nullptr) {
                return ctx.createArray();
            }
            const u64 objClassId = ScriptClassRegistry::instance().classIdByName("Objective");
            void* objProto = ScriptClassRegistry::instance().proto(objClassId);
            void* arr = ctx.createArray();
            if (objProto == nullptr) {
                return arr; // Objective 类未注册，返回空数组
            }
            u32 i = 0;
            for (auto* objective : scoreboard->getObjectives()) {
                void* objVal = ScriptObjectRegistry::wrap(ctx, objClassId, objProto, objective, false, "Objective");
                ctx.setArrayElement(arr, i, objVal); // setArrayElement 内部 DupValue，仍须 releaseValue
                ctx.releaseValue(objVal);
                ++i;
            }
            return arr;
        },
        0);
    scoreboardReg.method(
        "getTeam",
        [scoreboardClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            // 对齐基岩 Scoreboard.getTeam：按名取队伍，不存在返 undefined。供 /team 命令测试断言队伍存在性。
            auto* scoreboard =
                static_cast<mc::scoreboard::Scoreboard*>(ScriptObjectRegistry::unwrap(ctx, thisVal, scoreboardClassId));
            if (scoreboard == nullptr) {
                return ctx.createUndefined();
            }
            if (argc < 1 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("scoreboard.getTeam requires a string argument");
            }
            auto nameOpt = ctx.toString(args[0]);
            if (!nameOpt) {
                return ctx.createUndefined();
            }
            auto* team = scoreboard->getTeam(*nameOpt);
            if (team == nullptr) {
                return ctx.createUndefined(); // 队伍不存在
            }
            const u64 teamClassId = ScriptClassRegistry::instance().classIdByName("Team");
            void* teamProto = ScriptClassRegistry::instance().proto(teamClassId);
            if (teamProto == nullptr) {
                return ctx.createUndefined(); // Team 类未注册
            }
            // ScorePlayerTeam* 隐式上转 Team* 后传给 wrap（opaque 存基类指针，Team JS 类 static_cast 解包）。
            return ScriptObjectRegistry::wrap(ctx, teamClassId, teamProto, team, false, "Team");
        },
        1);
    scoreboardReg.method(
        "getTeams",
        [scoreboardClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            // 对齐基岩 Scoreboard.getTeams：返回所有队伍数组。供 /team list 测试断言队伍集合。
            auto* scoreboard =
                static_cast<mc::scoreboard::Scoreboard*>(ScriptObjectRegistry::unwrap(ctx, thisVal, scoreboardClassId));
            if (scoreboard == nullptr) {
                return ctx.createArray();
            }
            const u64 teamClassId = ScriptClassRegistry::instance().classIdByName("Team");
            void* teamProto = ScriptClassRegistry::instance().proto(teamClassId);
            void* arr = ctx.createArray();
            if (teamProto == nullptr) {
                return arr; // Team 类未注册，返回空数组
            }
            u32 i = 0;
            for (auto* team : scoreboard->getTeams()) {
                void* teamVal = ScriptObjectRegistry::wrap(ctx, teamClassId, teamProto, team, false, "Team");
                ctx.setArrayElement(arr, i, teamVal);
                ctx.releaseValue(teamVal);
                ++i;
            }
            return arr;
        },
        0);

    // --- Objective类 ---
    // opaque 持 mc::scoreboard::ScoreObjective*（非拥有，记分板管理生命周期）。getScore 读分数，
    // getParticipants 读所有有分数的 holder 名，id/displayName 读目标元数据。getScore 返回 undefined
    // 当该 holder 无分数（对齐基岩 Objective.getScore 返回 number|undefined）。
    u64 objectiveClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* objectiveProto = builder.exportClass("Objective", objectiveClassId);
    ScriptClassRegistry::instance().registerClass(objectiveClassId, objectiveProto, "Objective");

    ClassRegistrar<void> objectiveReg(ctx, objectiveClassId, objectiveProto);
    objectiveReg.readonlyProperty("id", [objectiveClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* objective =
            static_cast<mc::scoreboard::ScoreObjective*>(ScriptObjectRegistry::unwrap(ctx, thisVal, objectiveClassId));
        if (objective == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(objective->getName());
    });
    objectiveReg.readonlyProperty(
        "displayName", [objectiveClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            // 对齐基岩 Objective.displayName：返回显示名纯文本（无格式码）。Cubium 的 getDisplayName
            // 返回 ITextComponent*，取纯文本需经 Component flatten。TODO: 未接入 ITextComponent 纯文本
            // 提取，当前用 name 兜底（/scoreboard objectives add 未指定显示名时 display==name，对测试足够）。
            auto* objective = static_cast<mc::scoreboard::ScoreObjective*>(
                ScriptObjectRegistry::unwrap(ctx, thisVal, objectiveClassId));
            if (objective == nullptr) {
                return ctx.createUndefined();
            }
            return ctx.createString(objective->getName());
        });
    objectiveReg.method(
        "getScore",
        [objectiveClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* objective = static_cast<mc::scoreboard::ScoreObjective*>(
                ScriptObjectRegistry::unwrap(ctx, thisVal, objectiveClassId));
            if (objective == nullptr) {
                return ctx.createUndefined();
            }
            if (argc < 1 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("objective.getScore requires a string argument");
            }
            auto nameOpt = ctx.toString(args[0]);
            if (!nameOpt) {
                return ctx.createUndefined();
            }
            auto* score = objective->getScoreboard().getScore(*nameOpt, *objective);
            if (score == nullptr) {
                return ctx.createUndefined(); // 该 holder 无分数
            }
            return ctx.createInt32(score->getScorePoints());
        },
        1);
    objectiveReg.method(
        "getParticipants",
        [objectiveClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            // 对齐基岩 Objective.getParticipants：返回该目标所有有分数的 holder 名数组。
            // Cubium 的 Scoreboard::getSortedScores(objective) 返回该目标的 Score* 列表，
            // 经 Score::getPlayerName() 取 holder 名。
            auto* objective = static_cast<mc::scoreboard::ScoreObjective*>(
                ScriptObjectRegistry::unwrap(ctx, thisVal, objectiveClassId));
            if (objective == nullptr) {
                return ctx.createArray();
            }
            void* arr = ctx.createArray();
            u32 i = 0;
            for (auto* score : objective->getScoreboard().getSortedScores(*objective)) {
                ctx.setArrayElementString(arr, i, score->getPlayerName());
                ++i;
            }
            return arr;
        },
        0);

    // --- Team类 ---
    // opaque 持 mc::scoreboard::ScorePlayerTeam*（非拥有，记分板管理生命周期）。Scoreboard.getTeam/getTeams
    // wrap 队伍指针。id 读队名，getMembers/hasMember 读成员名集合，color/friendlyFire/seeFriendlyInvisibles/
    // nametagVisibility/deathMessageVisibility/collisionRule 读 modify 子命令设置的属性。供 /team 命令测试
    // 断言队伍状态（add/remove/join/leave/empty/modify）。成员按名字索引（TeamCommand join/leave 经
    // EntityArgumentType 选择器解析为玩家名后加入，与 ScoreboardCommand target 裸 string 不同，@s 生效）。
    u64 teamClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* teamProto = builder.exportClass("Team", teamClassId);
    ScriptClassRegistry::instance().registerClass(teamClassId, teamProto, "Team");

    ClassRegistrar<void> teamReg(ctx, teamClassId, teamProto);
    teamReg.readonlyProperty("id", [teamClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* team =
            static_cast<mc::scoreboard::ScorePlayerTeam*>(ScriptObjectRegistry::unwrap(ctx, thisVal, teamClassId));
        if (team == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(team->getName());
    });
    teamReg.readonlyProperty("displayName", [teamClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 对齐基岩无 Team.displayName，此处提供测试用显示名纯文本。TODO: getDisplayName 返 ITextComponent*，
        // 未接入纯文本提取，当前用 name 兜底（/team add 未指定显示名时 display==name，对测试足够）。
        auto* team =
            static_cast<mc::scoreboard::ScorePlayerTeam*>(ScriptObjectRegistry::unwrap(ctx, thisVal, teamClassId));
        if (team == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(team->getName());
    });
    teamReg.readonlyProperty("color", [teamClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 对齐 Java Team.color：返回颜色名（"red"/"blue"/"dark_green" 等），经 text::toName 字符串化。
        auto* team =
            static_cast<mc::scoreboard::ScorePlayerTeam*>(ScriptObjectRegistry::unwrap(ctx, thisVal, teamClassId));
        if (team == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(mc::text::toName(team->getColor()));
    });
    teamReg.readonlyProperty("friendlyFire", [teamClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* team =
            static_cast<mc::scoreboard::ScorePlayerTeam*>(ScriptObjectRegistry::unwrap(ctx, thisVal, teamClassId));
        if (team == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createBoolean(team->getAllowFriendlyFire());
    });
    teamReg.readonlyProperty(
        "seeFriendlyInvisibles", [teamClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* team =
                static_cast<mc::scoreboard::ScorePlayerTeam*>(ScriptObjectRegistry::unwrap(ctx, thisVal, teamClassId));
            if (team == nullptr) {
                return ctx.createUndefined();
            }
            return ctx.createBoolean(team->canSeeFriendlyInvisibles());
        });
    teamReg.readonlyProperty("nametagVisibility", [teamClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* team =
            static_cast<mc::scoreboard::ScorePlayerTeam*>(ScriptObjectRegistry::unwrap(ctx, thisVal, teamClassId));
        if (team == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(mc::scoreboard::teamVisibilityToString(team->getNameTagVisibility()));
    });
    teamReg.readonlyProperty(
        "deathMessageVisibility", [teamClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* team =
                static_cast<mc::scoreboard::ScorePlayerTeam*>(ScriptObjectRegistry::unwrap(ctx, thisVal, teamClassId));
            if (team == nullptr) {
                return ctx.createUndefined();
            }
            return ctx.createString(mc::scoreboard::teamVisibilityToString(team->getDeathMessageVisibility()));
        });
    teamReg.readonlyProperty("collisionRule", [teamClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* team =
            static_cast<mc::scoreboard::ScorePlayerTeam*>(ScriptObjectRegistry::unwrap(ctx, thisVal, teamClassId));
        if (team == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(mc::scoreboard::teamCollisionRuleToString(team->getCollisionRule()));
    });
    teamReg.method(
        "getMembers",
        [teamClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            // 对齐基岩无直接等价；返回队伍成员名数组，供 /team join/leave/empty 测试断言成员集合。
            // Cubium 的 ScorePlayerTeam::getMembers 返 const std::set<std::string>&（按名字排序）。
            auto* team =
                static_cast<mc::scoreboard::ScorePlayerTeam*>(ScriptObjectRegistry::unwrap(ctx, thisVal, teamClassId));
            if (team == nullptr) {
                return ctx.createArray();
            }
            void* arr = ctx.createArray();
            u32 i = 0;
            for (const auto& member : team->getMembers()) {
                ctx.setArrayElementString(arr, i, member);
                ++i;
            }
            return arr;
        },
        0);
    teamReg.method(
        "hasMember",
        [teamClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* team =
                static_cast<mc::scoreboard::ScorePlayerTeam*>(ScriptObjectRegistry::unwrap(ctx, thisVal, teamClassId));
            if (team == nullptr) {
                return ctx.createBoolean(false);
            }
            if (argc < 1 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("team.hasMember requires a string argument");
            }
            auto nameOpt = ctx.toString(args[0]);
            if (!nameOpt) {
                return ctx.createBoolean(false);
            }
            return ctx.createBoolean(team->hasMember(*nameOpt));
        },
        1);

    // --- BossBarManager类 ---
    // 无状态 facade：opaque 持哨兵指针（owned=false 不销毁）。get(id)/getAll() 经 ScriptWorldAccessor
    // 全局回调读 server 层 CustomServerBossInfoManager（BossBar 类型在 server 层，common 层以 BossBarView
    // 值桥接）。world.bossbar 属性返回本类单例对象。
    u64 bossBarMgrClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* bossBarMgrProto = builder.exportClass("BossBarManager", bossBarMgrClassId);
    ScriptClassRegistry::instance().registerClass(bossBarMgrClassId, bossBarMgrProto, "BossBarManager");

    ClassRegistrar<void> bossBarMgrReg(ctx, bossBarMgrClassId, bossBarMgrProto);
    bossBarMgrReg.method(
        "get",
        [](IScriptBindingContext& ctx, void* /*thisVal*/, i32 argc, void** args) -> void* {
            // 按 id 取 BossBar JS 对象。id 不存在时返回 undefined（对齐 /bossbar get 对不存在 id 报错语义，
            // 脚本侧 get(id)===undefined 判存在性）。BossBar JS 对象 opaque 持堆 id 字符串（owned，GC 释放）。
            if (argc < 1 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("bossBarManager.get requires a string argument");
            }
            auto idOpt = ctx.toString(args[0]);
            if (!idOpt) {
                return ctx.createUndefined();
            }
            // 先取快照确认存在；不存在返 undefined。
            auto view = ScriptWorldAccessor::instance().getBossBar(*idOpt);
            if (!view.exists) {
                return ctx.createUndefined();
            }
            const u64 barClassId = ScriptClassRegistry::instance().classIdByName("BossBar");
            void* barProto = ScriptClassRegistry::instance().proto(barClassId);
            if (barProto == nullptr) {
                return ctx.createUndefined(); // BossBar 类未注册
            }
            // 堆分配 id 字符串，BossBar JS 对象 owned 持有，GC 时 destroy 释放。
            auto* idPtr = new std::string(*idOpt);
            return ScriptObjectRegistry::wrap(ctx, barClassId, barProto, idPtr, true, "BossBar", [](void* p) {
                delete static_cast<std::string*>(p);
            });
        },
        1);
    bossBarMgrReg.method(
        "getAll",
        [](IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/) -> void* {
            // 返回所有 BossBar 的 JS 对象数组（按 id 列表逐个 wrap）。
            const u64 barClassId = ScriptClassRegistry::instance().classIdByName("BossBar");
            void* barProto = ScriptClassRegistry::instance().proto(barClassId);
            void* arr = ctx.createArray();
            if (barProto == nullptr) {
                return arr; // BossBar 类未注册，返回空数组
            }
            u32 i = 0;
            for (const auto& id : ScriptWorldAccessor::instance().getBossBarIds()) {
                auto* idPtr = new std::string(id);
                void* barVal =
                    ScriptObjectRegistry::wrap(ctx, barClassId, barProto, idPtr, true, "BossBar", [](void* p) {
                        delete static_cast<std::string*>(p);
                    });
                ctx.setArrayElement(arr, i, barVal);
                ctx.releaseValue(barVal);
                ++i;
            }
            return arr;
        },
        0);

    // --- BossBar类 ---
    // opaque 持堆 std::string*（id，owned，GC 释放）。属性每次经 ScriptWorldAccessor::getBossBar(id)
    // 取最新 BossBarView 快照读字段，保证 set value/max/color 后 JS 立即可见。BossBar 被 remove 后
    // 快照 exists=false，属性返 undefined（对齐已删除语义）。
    u64 bossBarClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* bossBarProto = builder.exportClass("BossBar", bossBarClassId);
    ScriptClassRegistry::instance().registerClass(bossBarClassId, bossBarProto, "BossBar");

    ClassRegistrar<void> bossBarReg(ctx, bossBarClassId, bossBarProto);
    bossBarReg.readonlyProperty("id", [bossBarClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 返回 BossBar 完整 id（namespace:path，经 bar->id().toString()）。idPtr 是创建时传入的查询 id
        // （可能不带命名空间，如 "bar"），bar->id() 才是 manager 存储的规范 id（"minecraft:bar"）。
        // 故从 view.id 取，保证 get("bar").id 与 getAll()[i].id 一致（均带命名空间）。
        auto* idPtr = static_cast<std::string*>(ScriptObjectRegistry::unwrap(ctx, thisVal, bossBarClassId));
        if (idPtr == nullptr) {
            return ctx.createUndefined();
        }
        auto view = ScriptWorldAccessor::instance().getBossBar(*idPtr);
        if (!view.exists) {
            return ctx.createUndefined();
        }
        return ctx.createString(view.id);
    });
    bossBarReg.readonlyProperty("name", [bossBarClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* idPtr = static_cast<std::string*>(ScriptObjectRegistry::unwrap(ctx, thisVal, bossBarClassId));
        if (idPtr == nullptr) {
            return ctx.createUndefined();
        }
        auto view = ScriptWorldAccessor::instance().getBossBar(*idPtr);
        if (!view.exists) {
            return ctx.createUndefined();
        }
        return ctx.createString(view.name);
    });
    bossBarReg.readonlyProperty("value", [bossBarClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* idPtr = static_cast<std::string*>(ScriptObjectRegistry::unwrap(ctx, thisVal, bossBarClassId));
        if (idPtr == nullptr) {
            return ctx.createUndefined();
        }
        auto view = ScriptWorldAccessor::instance().getBossBar(*idPtr);
        if (!view.exists) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(view.value);
    });
    bossBarReg.readonlyProperty("max", [bossBarClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* idPtr = static_cast<std::string*>(ScriptObjectRegistry::unwrap(ctx, thisVal, bossBarClassId));
        if (idPtr == nullptr) {
            return ctx.createUndefined();
        }
        auto view = ScriptWorldAccessor::instance().getBossBar(*idPtr);
        if (!view.exists) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(view.max);
    });
    bossBarReg.readonlyProperty("color", [bossBarClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* idPtr = static_cast<std::string*>(ScriptObjectRegistry::unwrap(ctx, thisVal, bossBarClassId));
        if (idPtr == nullptr) {
            return ctx.createUndefined();
        }
        auto view = ScriptWorldAccessor::instance().getBossBar(*idPtr);
        if (!view.exists) {
            return ctx.createUndefined();
        }
        return ctx.createString(view.color);
    });
    bossBarReg.readonlyProperty("overlay", [bossBarClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* idPtr = static_cast<std::string*>(ScriptObjectRegistry::unwrap(ctx, thisVal, bossBarClassId));
        if (idPtr == nullptr) {
            return ctx.createUndefined();
        }
        auto view = ScriptWorldAccessor::instance().getBossBar(*idPtr);
        if (!view.exists) {
            return ctx.createUndefined();
        }
        return ctx.createString(view.overlay);
    });
    bossBarReg.readonlyProperty("visible", [bossBarClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* idPtr = static_cast<std::string*>(ScriptObjectRegistry::unwrap(ctx, thisVal, bossBarClassId));
        if (idPtr == nullptr) {
            return ctx.createUndefined();
        }
        auto view = ScriptWorldAccessor::instance().getBossBar(*idPtr);
        if (!view.exists) {
            return ctx.createUndefined();
        }
        return ctx.createBoolean(view.visible);
    });
    bossBarReg.readonlyProperty("players", [bossBarClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 返回 BossBar 关联玩家 UUID 字符串数组（CustomServerBossInfo::playerUuids）。
        auto* idPtr = static_cast<std::string*>(ScriptObjectRegistry::unwrap(ctx, thisVal, bossBarClassId));
        if (idPtr == nullptr) {
            return ctx.createArray();
        }
        auto view = ScriptWorldAccessor::instance().getBossBar(*idPtr);
        void* arr = ctx.createArray();
        if (!view.exists) {
            return arr;
        }
        u32 i = 0;
        for (const auto& uuid : view.players) {
            ctx.setArrayElementString(arr, i, uuid);
            ++i;
        }
        return arr;
    });

    // --- Entity类 ---
    // opaque 持 mc::Entity*（非拥有，EntityManager 管理生命周期）。test.spawn 经 ScriptClassRegistry
    // 取本 classId/proto wrap 真实指针。getComponent 派发 rideable/health/movement/equippable/onfire 组件。
    u64 entityClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* entityProto = builder.exportClass("Entity", entityClassId);
    ScriptClassRegistry::instance().registerClass(entityClassId, entityProto, "Entity");

    ClassRegistrar<void> entityReg(ctx, entityClassId, entityProto);
    entityReg.readonlyProperty("id", [entityClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
        if (ent == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt64(static_cast<i64>(ent->id()));
    });
    entityReg.readonlyProperty("typeId", [entityClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
        if (ent == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(ent->getTypeId());
    });
    entityReg.method("getDimension",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr) {
                return ctx.createUndefined();
            }
            auto* world = ent->world();
            if (world == nullptr) {
                return ctx.createUndefined();
            }
            const u64 dimClassId = ScriptClassRegistry::instance().classIdByName("Dimension");
            void* dimProto = ScriptClassRegistry::instance().proto(dimClassId);
            if (dimProto == nullptr) {
                return ctx.createUndefined();
            }
            return ScriptObjectRegistry::wrap(ctx, dimClassId, dimProto, world, false, "Dimension");
        });
    entityReg.method("getLocation",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr) {
                return ctx.createUndefined();
            }
            auto pos = ent->position();
            void* obj = ctx.createObject();
            ctx.setPropertyFloat(obj, "x", static_cast<f64>(pos.x));
            ctx.setPropertyFloat(obj, "y", static_cast<f64>(pos.y));
            ctx.setPropertyFloat(obj, "z", static_cast<f64>(pos.z));
            return obj;
        });
    // 基岩 Entity.location 是标准 readonly 属性（@minecraft/server Entity.location: Vector3），
    // 返回实体世界坐标。与 getLocation() 方法等价（基岩同时暴露属性与方法，社区测试多用 .location）。
    // 此前仅绑定 getLocation() 方法，测试用 entity.location 取坐标得 undefined →
    // "cannot read property 'x' of undefined"。补 readonly property 委托同一逻辑。
    entityReg.readonlyProperty("location", [entityClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
        if (ent == nullptr) {
            return ctx.createUndefined();
        }
        auto pos = ent->position();
        void* obj = ctx.createObject();
        ctx.setPropertyFloat(obj, "x", static_cast<f64>(pos.x));
        ctx.setPropertyFloat(obj, "y", static_cast<f64>(pos.y));
        ctx.setPropertyFloat(obj, "z", static_cast<f64>(pos.z));
        return obj;
    });
    // 基岩 Entity.getVelocity() 是标准 API（@minecraft/server Entity.getVelocity(): Vector3），
    // 返回实体当前速度向量（含 Y 分量）。此前 Cubium 仅绑 location 未绑 getVelocity，测试无法读取
    // 实体速度（如重锤风爆 Wind Burst 弹起后玩家 Y 速度、击退后实体速度），致依赖速度断言的行为
    // 无法测试。补全对齐基岩官方 API，返回 mc::Entity::velocity()（m_builtIn.velocity->m_velocity）。
    entityReg.method("getVelocity",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr) {
                return ctx.createUndefined();
            }
            auto vel = ent->velocity();
            void* obj = ctx.createObject();
            ctx.setPropertyFloat(obj, "x", static_cast<f64>(vel.x));
            ctx.setPropertyFloat(obj, "y", static_cast<f64>(vel.y));
            ctx.setPropertyFloat(obj, "z", static_cast<f64>(vel.z));
            return obj;
        });
    // Cubium 扩展 Entity.setVelocity(vector: Vector3)：直接设置实体速度向量。
    // 基岩官方 @minecraft/server 无公开的 setVelocity（投射物设速度无标准 API，applyImpulse 对投射物
    // 通常无效——投射物物理由 velocity 字段驱动不响应冲量）。Cubium 自研绑定直接调
    // mc::Entity::setVelocity（Entity.cpp:718 设 m_builtIn.velocity->m_velocity），投射物下一 tick
    // performRayTrace（ProjectileEntity.cpp:381）用此 velocity 做射线终点，故 setVelocity 可让
    // spawn 出的静止投射物获得初速度命中目标方块/实体。
    //
    // 用途：集成测试需操控投射物飞行命中固定方块/实体以验证命中链路（如龙息火球命中方块生成龙息云、
    // 雪球命中烈焰人 3 伤害）。此前脚本层只能 spawn 静止投射物（test.spawn 不接受速度参数），静止投射物
    // performRayTrace delta≈0 必 miss（ProjectileEntity.cpp:415），永不触发 onBlockHit/onEntityHit，
    // 致投射物命中链路端到端测试不可构造。setVelocity 补全此能力。
    //
    // 签名对齐 teleport 的 Vector3 解析范式（isObject + getPropertyFloat x/y/z）。
    entityReg.method(
        "setVelocity",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr || argc < 1 || !ctx.isObject(args[0])) {
                return ctx.createUndefined();
            }
            void* velObj = args[0];
            auto xOpt = ctx.getPropertyFloat(velObj, "x");
            auto yOpt = ctx.getPropertyFloat(velObj, "y");
            auto zOpt = ctx.getPropertyFloat(velObj, "z");
            if (!xOpt || !yOpt || !zOpt) {
                return ctx.throwTypeError("setVelocity requires {x,y,z}");
            }
            ent->setVelocity(static_cast<f32>(*xOpt), static_cast<f32>(*yOpt), static_cast<f32>(*zOpt));
            return ctx.createUndefined();
        },
        1);
    // 对齐基岩官方 Entity.setOnFire(seconds: number, useEffects?: boolean = true): boolean。
    // 官方语义：点燃实体 seconds 秒，useEffects=true 时考虑水/雨/火焰保护等条件与副作用（如解冻）。
    // 返回是否成功点燃（seconds<=0 / 实体湿 / 火焰免疫时返 false）。
    //
    // Cubium 绑定实现：
    //   - useEffects=true（默认）→ mc::Entity::igniteForSeconds(seconds)（Entity.hpp:1649）：
    //     秒→tick（×20），LivingEntity 重写乘 BURNING_TIME 属性（火焰保护缩减），清冰冻（thaw），
    //     仅在新时间>当前剩余时更新（不覆盖免疫期）。等价 vanilla useEffects 的副作用链。
    //   - useEffects=false → mc::Entity::setFire(seconds*20)（Entity.hpp:1684）：直接设 tick 不检查
    //     不乘属性不清冰冻（对齐 vanilla useEffects=false 跳过条件直接设）。
    //   返回 true（Cubium igniteForSeconds/setFire 不区分成功失败，统一返 true 对齐"已设置"语义；
    //   vanilla 的 false 仅在湿/免疫时返回，Cubium igniteForSeconds 仍会写入 fire 字段——为避免误导
    //   测试，此处不模拟湿/免疫失败，TODO: 后续按 isWet/fireImmune 精确返回 false）。
    //
    // 用途：集成测试需点燃实体验证灭火链路（如水瓶 _onHitAsWater extinguishFire、雨/水伤害门控、
    // 火焰保护 BURNING_TIME 缩减）。此前脚本层仅 readonly onFireTicksRemaining（无法主动点火），
    // 致灭火/燃烧相关端到端测试不可构造。setOnFire 补全此能力。
    entityReg.method(
        "setOnFire",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr || argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.createBoolean(false);
            }
            const auto secondsOpt = ctx.toFloat64(args[0]);
            if (!secondsOpt) {
                return ctx.createBoolean(false);
            }
            const f64 seconds = *secondsOpt;
            // useEffects 默认 true；显式传 false 时跳过副作用。
            bool useEffects = true;
            if (argc >= 2) {
                const auto bOpt = ctx.toBool(args[1]);
                if (bOpt) {
                    useEffects = *bOpt;
                }
            }
            if (seconds <= 0.0) {
                return ctx.createBoolean(false);
            }
            if (useEffects) {
                ent->igniteForSeconds(static_cast<f32>(seconds));
            } else {
                ent->setFire(static_cast<i32>(seconds * 20.0));
            }
            return ctx.createBoolean(true);
        },
        2);
    entityReg.method(
        "getComponent",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            // 基岩 Entity.getComponent(componentId) 返回组件对象，组件不存在返 undefined。
            // componentId 既接受 "minecraft:health" 也接受 "health"，不含 ':' 时补 minecraft: 前缀。
            // 派发：rideable（OOP 合成）/ health/movement/equippable（须 LivingEntity）/ onfire（须 isOnFire）。
            // 各组件 JS 类 opaque 持同一 mc::Entity*（owned=false），getter 内 dynamic_cast/tryGetComponent 现取数据。
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createUndefined();
            }
            // 实体有效性守卫：JS 包装对象 owned=false 持裸 mc::Entity*，实体销毁后句柄悬垂。
            // 项目 EntityManager 有 graveyard 延迟析构——remove() 标记后对象存活到下一 tick 末尾才 free，
            // 此窗口内 isRemoved()=true。此处检查挡住"已 remove 但尚未 free"的实体（对齐 graveyard 设计
            // 意图：给裸指针持有方一帧时间通过 isAlive/isRemoved 检查避免 UAF，见 EntityManager.hpp 注释）。
            // 注：对"立即 free"路径（removeEntity 丢弃 unique_ptr，如区块卸载）此检查仍可能 UAF，
            // 彻底根治需脚本句柄持 EntityInstanceId 而非裸指针（TODO: 后续重构）。
            if (ent->isRemoved()) {
                return ctx.createUndefined();
            }
            auto compId = ctx.toString(args[0]);
            if (!compId) {
                return ctx.createUndefined();
            }
            // normalize 前缀（对齐 Dimension.getEntities typeFilter 规范化语义）。
            std::string normalized = *compId;
            if (normalized.find(':') == std::string::npos) {
                normalized = "minecraft:" + normalized;
            }

            // 按类名 wrap 组件 JS 对象（opaque 持 ent，owned=false）。类未注册时返 undefined。
            // 传 ent->id() 登记 ScriptHandleRegistry：组件对象与 Entity 对象 opaque 持同一 ent，
            // 登记同一 entityId，实体销毁时 invalidateAll 一次清空 Entity + 所有组件句柄防 UAF。
            auto wrapComponent = [&ctx, ent](const char* className) -> void* {
                const u64 classId = ScriptClassRegistry::instance().classIdByName(className);
                void* proto = ScriptClassRegistry::instance().proto(classId);
                if (proto == nullptr) {
                    return ctx.createUndefined();
                }
                return ScriptObjectRegistry::wrap(ctx, classId, proto, ent, false, className, nullptr, ent->id());
            };

            if (normalized == "minecraft:rideable") {
                return wrapComponent("RideableComponent");
            }
            if (normalized == "minecraft:health" || normalized == "minecraft:movement" ||
                normalized == "minecraft:equippable") {
                // health/movement/equippable 仅 LivingEntity attach，非 LivingEntity 返 undefined。
                if (dynamic_cast<mc::LivingEntity*>(ent) == nullptr) {
                    return ctx.createUndefined();
                }
                if (normalized == "minecraft:health") return wrapComponent("HealthComponent");
                if (normalized == "minecraft:movement") return wrapComponent("MovementComponent");
                return wrapComponent("EquippableComponent");
            }
            if (normalized == "minecraft:onfire") {
                // 对齐基岩 OnFireComponent："When present on an entity, this entity is on fire"。
                if (!ent->isOnFire()) {
                    return ctx.createUndefined();
                }
                return wrapComponent("OnFireComponent");
            }
            if (normalized == "minecraft:inventory") {
                // 对齐基岩 EntityInventoryComponent：仅 Player 持有背包组件（基岩原版仅玩家/容器实体有
                // inventory 组件）。dynamic_cast Player 失败（非玩家实体）返 undefined。组件对象的 container
                // 只读属性返回包装 Player::inventory()（PlayerInventory : IInventory）的 Container。
                if (dynamic_cast<mc::Player*>(ent) == nullptr) {
                    return ctx.createUndefined();
                }
                return wrapComponent("EntityInventoryComponent");
            }
            if (normalized == "minecraft:is_charged") {
                // 对齐基岩 EntityIsChargedComponent（componentId="minecraft:is_charged"）：
                // "When added, this component signifies that this entity is charged"。组件存在即充能。
                // 仅 CreeperEntity 有充能概念，dynamic_cast 后查 isPowered()；非苦力怕或未充能返 undefined。
                auto* creeper = dynamic_cast<mc::CreeperEntity*>(ent);
                if (creeper == nullptr || !creeper->isPowered()) {
                    return ctx.createUndefined();
                }
                return wrapComponent("IsChargedComponent");
            }
            if (normalized == "minecraft:mark_variant") {
                // 借用基岩 mark_variant（int 标记变种组件）承载哞菇红/棕变种。基岩原版哞菇无此组件，
                // 此处为集成测试可读变种而设：dynamic_cast 到 MooshroomEntity，读 getMooshroomType()
                // 映射为 int（Red=0/Brown=1，与 MooshroomType 枚举值及 NBT "Type" 字段一致）。
                // 非哞菇返 undefined（对齐基岩"组件不存在则 getComponent 返 undefined"）。
                auto* mooshroom = dynamic_cast<mc::MooshroomEntity*>(ent);
                if (mooshroom == nullptr) {
                    return ctx.createUndefined();
                }
                return wrapComponent("MarkVariantComponent");
            }
            if (normalized == "minecraft:glow_squid_dark_ticks") {
                // 借用自定义 componentId 承载发光鱿鱼受惊暗化剩余 tick（Java SynchedData
                // DarkTicksRemaining，基岩原版无此标准组件）。发光鱿鱼受击后 setDarkTicks(100)（对齐
                // wiki"被攻击后的100内停止发光"），tick() 逐帧递减。此处为集成测试可读暗化状态而设：
                // dynamic_cast 到 GlowSquidEntity，读 getDarkTicksRemaining()，>0 返组件否则 undefined。
                // 非发光鱿鱼或未暗化返 undefined（对齐基岩"组件不存在则 getComponent 返 undefined"）。
                auto* glowSquid = dynamic_cast<mc::GlowSquidEntity*>(ent);
                if (glowSquid == nullptr || glowSquid->getDarkTicksRemaining() <= 0) {
                    return ctx.createUndefined();
                }
                return wrapComponent("GlowSquidDarkTicksComponent");
            }
            if (normalized == "minecraft:pufferfish_puff_state") {
                // 借用自定义 componentId 承载河豚膨胀等级（Java SynchedData PUFF_STATE，基岩原版无此
                // 标准组件）。膨胀等级 0=未膨胀/1=半膨胀/2=完全膨胀，由 PuffGoal 检测 2 格内 scary
                // 生物触发（startPuffTimer→tick 状态机：1 tick→SemiPuffed，40 tick→FullyPuffed）。
                // 此处为集成测试可读膨胀状态而设：dynamic_cast 到 PufferfishEntity，读 getPuffState()
                // （优先从 DataParameter 取同步值），非河豚返 undefined（对齐基岩"组件不存在则
                // getComponent 返 undefined"）。注意：未膨胀(0)时仍返组件（value=0），以便测试断言
                // 膨胀前后等级变化；与 is_charged/glow_squid_dark_ticks"满足条件才返"的语义不同。
                auto* pufferfish = dynamic_cast<mc::PufferfishEntity*>(ent);
                if (pufferfish == nullptr) {
                    return ctx.createUndefined();
                }
                return wrapComponent("PufferfishPuffStateComponent");
            }
            if (normalized == "minecraft:is_tamed") {
                // 对齐基岩 EntityIsTamedComponent（componentId="minecraft:is_tamed"）：
                // 驯服状态标记组件，wolf/cat/parrot 等驯服类生物经 setTamed(true) 置位。TameableEntity
                // 是所有驯服类基类（WolfEntity/CatEntity/ParrotEntity 均继承自 TameableEntity），
                // dynamic_cast<TameableEntity*> 命中即读 isTamed()（DATA_FLAGS_PARAM 位 4）。
                // 非驯服类实体返 undefined（对齐基岩"组件不存在则 getComponent 返 undefined"）。
                // 此前 WolfTests 驯服判定用 effectiveMax>=20（驯服后血量上限 8→20）间接断言，hacky 且
                // 不适用于鹦鹉（驯服不改血量）；补 is_tamed 组件后所有驯服类可读 value 精确断言。
                //
                // 马类（AbstractHorseEntity）特例：vanilla 马可驯服（玩家骑上反复尝试直至驯服，经
                // RunAroundLikeCrazyGoal + setTamedBy 置 HorseStatusComponent.m_tame），但不继承
                // TameableEntity（继承 AnimalEntity+IJumpingMount+IEquipable），有独立 isTame()/setTame()。
                // 故补 dynamic_cast<AbstractHorseEntity*> 分支读 isTame()，使马驯服状态可读（与 is_saddled
                // 对马类的双路 dynamic_cast 范式一致）。修复前马驯服后 getComponent("is_tamed") 返 undefined，
                // 脚本无法断言马驯服（只能用"玩家是否被甩下"间接判定，不精确）。
                auto* tameable = dynamic_cast<mc::TameableEntity*>(ent);
                if (tameable != nullptr) {
                    return wrapComponent("IsTamedComponent");
                }
                auto* horse = dynamic_cast<mc::AbstractHorseEntity*>(ent);
                if (horse != nullptr) {
                    return wrapComponent("IsTamedComponent");
                }
                return ctx.createUndefined();
            }
            if (normalized == "minecraft:is_sitting") {
                // 对齐基岩 EntityIsSittingComponent（componentId="minecraft:is_sitting"）：
                // 坐下状态标记组件，wolf/cat/parrot 等驯服类生物经 setSitting(true)/toggleSitting() 置位
                // （TameableEntity::setSitting 写 m_sitting + DATA_FLAGS_PARAM 位 0）。TameableEntity 是所有
                // 驯服类基类，dynamic_cast<TameableEntity*> 命中即读 isSitting()。非驯服类实体返 undefined
                // （对齐基岩"组件不存在则 getComponent 返 undefined"）。
                // 对所有 TameableEntity 都返组件（value true/false），以便测试断言坐下前后状态变化
                // （区别于 is_charged"满足条件才返"的存在性语义）。
                // 马类不继承 TameableEntity 且无坐下概念，不覆盖（与 is_tamed 对马类的双路不同）。
                // 供集成测试断言鹦鹉已驯服后 interactMob toggleSitting 切换坐下链路。
                auto* tameable = dynamic_cast<mc::TameableEntity*>(ent);
                if (tameable != nullptr) {
                    return wrapComponent("IsSittingComponent");
                }
                return ctx.createUndefined();
            }
            if (normalized == "minecraft:is_saddled") {
                // 对齐基岩 EntityIsSaddledComponent（componentId="minecraft:is_saddled"）：
                // "When added, this component signifies that this entity is currently saddled"。
                // 组件存在即已装鞍（与 is_charged 同款存在性语义，无 property）。鞍实体经
                // SaddleItem::itemInteractionForEntity 调 IRideable::setSaddle(true) 或
                // AbstractHorseEntity::setSaddle(true) 置位。鞍实体分两类（hasSaddle 接口不统一）：
                //   1) 实现 mc::entity::IRideable 的可骑乘实体：PigEntity/StriderEntity/AbstractNautilusEntity
                //      （IRideable::hasSaddle 纯虚，统一 dynamic_cast<IRideable*> 覆盖）。
                //   2) 马类 AbstractHorseEntity：注释明确不实现 IRideable（控制逻辑经乘客系统非 ride()），
                //      但有 public hasSaddle()，需单独 dynamic_cast 覆盖。
                // 两路任一命中且 hasSaddle()==true 才返组件；未装鞍或非鞍实体返 undefined
                // （对齐基岩"组件不存在则 getComponent 返 undefined"，装鞍前不存在装鞍后存在）。
                const auto* rideable = dynamic_cast<mc::entity::IRideable*>(ent);
                if (rideable != nullptr) {
                    if (!rideable->hasSaddle()) {
                        return ctx.createUndefined();
                    }
                    return wrapComponent("IsSaddledComponent");
                }
                const auto* horse = dynamic_cast<mc::AbstractHorseEntity*>(ent);
                if (horse != nullptr) {
                    if (!horse->hasSaddle()) {
                        return ctx.createUndefined();
                    }
                    return wrapComponent("IsSaddledComponent");
                }
                return ctx.createUndefined();
            }
            if (normalized == "minecraft:is_chested") {
                // 对齐基岩 EntityIsChestedComponent（componentId="minecraft:is_chested"）：
                // "When added, this component signifies that this entity is currently carrying a chest"。
                // 组件存在即已装箱（存在性语义，无 property）。驴/骡/行商羊驼经
                // AbstractChestedHorseEntity::equipChest 调 setChest(true) 置位（ChestedHorseComponent.m_hasChest）。
                // 仅 AbstractChestedHorseEntity 有 hasChest()（普通马/骷髅马/僵尸马不继承此中间层不 attach
                // ChestedHorseComponent），故单路 dynamic_cast<AbstractChestedHorseEntity*> 覆盖。
                // 已装箱返组件，未装箱或非箱实体返 undefined（对齐基岩"组件不存在则 getComponent 返 undefined"）。
                const auto* chested = dynamic_cast<mc::AbstractChestedHorseEntity*>(ent);
                if (chested == nullptr || !chested->hasChest()) {
                    return ctx.createUndefined();
                }
                return wrapComponent("IsChestedComponent");
            }
            if (normalized == "minecraft:color") {
                // 对齐基岩 EntityColorComponent（componentId="minecraft:color"）：
                // "Defines the entity's color. Only works on certain entities that have predefined color
                // values (e.g. sheep, llama, shulker)"，value 为 PaletteColor 0-15（与 DyeColor 数值一致）。
                // 此处先覆盖 SheepEntity（羊毛颜色经 getFleeceColor 返回 DyeColor 0-15）。羊驼 LlamaColor
                // (0-3)/潜影贝 ShulkerColor 枚举与 PaletteColor 数值不统一，留 TODO 按需补全。
                // 非羊返 undefined（对齐基岩"组件不存在则 getComponent 返 undefined"）。
                auto* sheep = dynamic_cast<mc::SheepEntity*>(ent);
                if (sheep == nullptr) {
                    return ctx.createUndefined();
                }
                return wrapComponent("ColorComponent");
            }
            // TODO: 其他基岩合法 componentId（is_baby/lava_movement 等标记/属性族，羊驼/潜影贝 color）按需补全。
            return ctx.createUndefined();
        },
        1);

    // --- RideableComponent类（合成，非基岩标准 API 名，仅 GameTest addRider 用）---
    // opaque 持载具 mc::Entity*。addRider(passengerEntity) 调 passenger->startRiding(*vehicle)。
    // 注意：不能调 vehicle->addPassenger(*passenger)，因其前置条件 passenger.getVehicle()==vehicle.id()
    // 未满足会断言失败；startRiding 是公共入口，内部设 m_vehicle 后再调 addPassenger。
    u64 rideableClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* rideableProto = builder.exportClass("RideableComponent", rideableClassId);
    ScriptClassRegistry::instance().registerClass(rideableClassId, rideableProto, "RideableComponent");

    ClassRegistrar<void> rideableReg(ctx, rideableClassId, rideableProto);
    rideableReg.method(
        "addRider",
        [rideableClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* vehicle = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, rideableClassId));
            if (vehicle == nullptr) {
                return ctx.throwInternalError("RideableComponent.addRider: invalid vehicle");
            }
            if (argc < 1) {
                return ctx.throwTypeError("addRider(passengerEntity)");
            }
            const u64 entClassId = ScriptClassRegistry::instance().classIdByName("Entity");
            auto* passenger = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, args[0], entClassId));
            if (passenger == nullptr) {
                return ctx.throwTypeError("addRider: argument must be an Entity");
            }
            if (!passenger->startRiding(*vehicle)) {
                return ctx.throwInternalError("addRider: passenger failed to start riding vehicle");
            }
            return ctx.createUndefined();
        },
        1);

    // --- OnFireComponent类（minecraft:onfire）---
    // opaque 持 mc::Entity*。FireComponent 在 Entity 层 attach，无 LivingEntity 约束。
    // 对齐基岩 EntityOnFireComponent：仅 readonly onFireTicksRemaining（设火走 Entity.setOnFire，不在此暴露）。
    u64 onFireClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* onFireProto = builder.exportClass("OnFireComponent", onFireClassId);
    ScriptClassRegistry::instance().registerClass(onFireClassId, onFireProto, "OnFireComponent");

    ClassRegistrar<void> onFireReg(ctx, onFireClassId, onFireProto);
    onFireReg.readonlyProperty(
        "onFireTicksRemaining", [onFireClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, onFireClassId));
            if (ent == nullptr) {
                return ctx.createUndefined();
            }
            return ctx.createInt32(ent->getRemainingFireTicks());
        });

    // --- IsChargedComponent类（minecraft:is_charged）---
    // opaque 持 mc::Entity*。对齐基岩 EntityIsChargedComponent：组件存在即代表已充能，
    // 无属性（基岩原版仅以 componentId 存在性标识 charged）。getComponent 已按 isPowered() 过滤，
    // 此处仅作为存在性标记返回，不暴露额外属性。
    u64 isChargedClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* isChargedProto = builder.exportClass("IsChargedComponent", isChargedClassId);
    ScriptClassRegistry::instance().registerClass(isChargedClassId, isChargedProto, "IsChargedComponent");
    // 无 property/method：组件对象存在即 charged（与基岩 EntityIsChargedComponent 一致）。

    // --- MarkVariantComponent类（minecraft:mark_variant，承载哞菇红/棕变种）---
    // opaque 持 mc::Entity*。getComponent 已按 MooshroomEntity 过滤，此处 dynamic_cast 现取变种值。
    // readonly value：MooshroomType 枚举值（Red=0/Brown=1），与 NBT "Type" 字段一致。
    // 供集成测试断言闪电劈中后红↔棕翻转（MooshroomEntity::onStruckByLightning）。
    u64 markVariantClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* markVariantProto = builder.exportClass("MarkVariantComponent", markVariantClassId);
    ScriptClassRegistry::instance().registerClass(markVariantClassId, markVariantProto, "MarkVariantComponent");

    ClassRegistrar<void> markVariantReg(ctx, markVariantClassId, markVariantProto);
    markVariantReg.readonlyProperty("value", [markVariantClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, markVariantClassId));
        auto* mooshroom = dynamic_cast<mc::MooshroomEntity*>(ent);
        if (mooshroom == nullptr) {
            return ctx.createUndefined();
        }
        // MooshroomType::Red=0/Brown=1，static_cast 取枚举底层数值。
        return ctx.createInt32(static_cast<i32>(mooshroom->getMooshroomType()));
    });

    // --- GlowSquidDarkTicksComponent类（minecraft:glow_squid_dark_ticks，承载发光鱿鱼受惊暗化剩余 tick）---
    // opaque 持 mc::Entity*。getComponent 已按 GlowSquidEntity + getDarkTicksRemaining()>0 过滤，此处
    // dynamic_cast 现取剩余暗化 tick 数。readonly value：剩余暗化 tick（受击后 100，逐帧递减）。
    // 供集成测试断言发光鱿鱼受击后暗化链路（GlowSquidEntity::hurt→setDarkTicks）。
    u64 glowSquidDarkClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* glowSquidDarkProto = builder.exportClass("GlowSquidDarkTicksComponent", glowSquidDarkClassId);
    ScriptClassRegistry::instance().registerClass(
        glowSquidDarkClassId, glowSquidDarkProto, "GlowSquidDarkTicksComponent");

    ClassRegistrar<void> glowSquidDarkReg(ctx, glowSquidDarkClassId, glowSquidDarkProto);
    glowSquidDarkReg.readonlyProperty(
        "value", [glowSquidDarkClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, glowSquidDarkClassId));
            auto* glowSquid = dynamic_cast<mc::GlowSquidEntity*>(ent);
            if (glowSquid == nullptr) {
                return ctx.createUndefined();
            }
            return ctx.createInt32(glowSquid->getDarkTicksRemaining());
        });

    // --- PufferfishPuffStateComponent类（minecraft:pufferfish_puff_state，承载河豚膨胀等级）---
    // opaque 持 mc::Entity*。getComponent 已按 PufferfishEntity 过滤，此处 dynamic_cast 现取膨胀等级。
    // readonly value：膨胀等级 0=未膨胀/1=半膨胀/2=完全膨胀（PuffState 枚举底层数值）。
    // 供集成测试断言 PuffGoal 触发膨胀链路（玩家/敌对生物接近 2 格→startPuffTimer→tick 状态机升级）。
    u64 pufferfishPuffClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* pufferfishPuffProto = builder.exportClass("PufferfishPuffStateComponent", pufferfishPuffClassId);
    ScriptClassRegistry::instance().registerClass(
        pufferfishPuffClassId, pufferfishPuffProto, "PufferfishPuffStateComponent");

    ClassRegistrar<void> pufferfishPuffReg(ctx, pufferfishPuffClassId, pufferfishPuffProto);
    pufferfishPuffReg.readonlyProperty(
        "value", [pufferfishPuffClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, pufferfishPuffClassId));
            auto* pufferfish = dynamic_cast<mc::PufferfishEntity*>(ent);
            if (pufferfish == nullptr) {
                return ctx.createUndefined();
            }
            return ctx.createInt32(static_cast<i32>(pufferfish->getPuffState()));
        });

    // --- IsTamedComponent类（minecraft:is_tamed，承载驯服状态）---
    // opaque 持 mc::Entity*。getComponent 已按 TameableEntity 过滤，此处 dynamic_cast 现取驯服状态。
    // readonly value：bool，true=已驯服/false=未驯服（TameableEntity::isTamed 读 DATA_FLAGS_PARAM 位 4）。
    // 供集成测试断言驯服类生物（wolf/cat/parrot）喂食驯服物品后 setTamed(true) 链路。区别于
    // is_charged/glow_squid_dark_ticks"满足条件才返组件"的语义——is_tamed 对所有 TameableEntity 都返
    // 组件（value true/false），以便测试断言驯服前后状态变化。
    u64 isTamedClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* isTamedProto = builder.exportClass("IsTamedComponent", isTamedClassId);
    ScriptClassRegistry::instance().registerClass(isTamedClassId, isTamedProto, "IsTamedComponent");

    ClassRegistrar<void> isTamedReg(ctx, isTamedClassId, isTamedProto);
    isTamedReg.readonlyProperty("value", [isTamedClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, isTamedClassId));
        // 优先 TameableEntity（狼/猫/鹦鹉），其次 AbstractHorseEntity（马类不继承 TameableEntity
        // 但有独立 isTame()，见 getComponent("is_tamed") 注释）。两路任一命中读 isTamed()。
        auto* tameable = dynamic_cast<mc::TameableEntity*>(ent);
        if (tameable != nullptr) {
            return ctx.createBoolean(tameable->isTamed());
        }
        auto* horse = dynamic_cast<mc::AbstractHorseEntity*>(ent);
        if (horse != nullptr) {
            return ctx.createBoolean(horse->isTame());
        }
        return ctx.createUndefined();
    });

    // --- IsSittingComponent类（minecraft:is_sitting，承载坐下状态）---
    // opaque 持 mc::Entity*。getComponent 已按 TameableEntity 过滤，此处 dynamic_cast 现取坐下状态。
    // readonly value：bool，true=已坐下/false=未坐下（TameableEntity::isSitting 读 m_sitting 字段，
    // 由 setSitting/toggleSitting 写入，经 DATA_FLAGS_PARAM 位 0 同步客户端）。供集成测试断言驯服类
    // 生物（wolf/cat/parrot）坐下切换链路（如鹦鹉 interactMob toggleSitting）。对所有 TameableEntity
    // 都返组件（value true/false），以便测试断言坐下前后状态变化。
    u64 isSittingClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* isSittingProto = builder.exportClass("IsSittingComponent", isSittingClassId);
    ScriptClassRegistry::instance().registerClass(isSittingClassId, isSittingProto, "IsSittingComponent");

    ClassRegistrar<void> isSittingReg(ctx, isSittingClassId, isSittingProto);
    isSittingReg.readonlyProperty("value", [isSittingClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, isSittingClassId));
        auto* tameable = dynamic_cast<mc::TameableEntity*>(ent);
        if (tameable != nullptr) {
            return ctx.createBoolean(tameable->isSitting());
        }
        return ctx.createUndefined();
    });

    // --- IsSaddledComponent类（minecraft:is_saddled，承载鞍装备状态）---
    // opaque 持 mc::Entity*。对齐基岩 EntityIsSaddledComponent：组件存在即代表已装鞍，
    // 无属性（基岩原版仅以 componentId 存在性标识 saddled）。getComponent 已按 IRideable/AbstractHorse
    // + hasSaddle()==true 过滤，此处仅作为存在性标记返回，不暴露额外属性。
    // 供集成测试断言装鞍链路（SaddleItem::itemInteractionForEntity→setSaddle(true)）。
    u64 isSaddledClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* isSaddledProto = builder.exportClass("IsSaddledComponent", isSaddledClassId);
    ScriptClassRegistry::instance().registerClass(isSaddledClassId, isSaddledProto, "IsSaddledComponent");
    // 无 property/method：组件对象存在即 saddled（与基岩 EntityIsSaddledComponent 一致）。

    // --- IsChestedComponent类（minecraft:is_chested，承载箱子装备状态）---
    // opaque 持 mc::Entity*。对齐基岩 EntityIsChestedComponent：组件存在即代表已装箱，无属性
    // （基岩原版仅以 componentId 存在性标识 chested）。getComponent 已按 AbstractChestedHorseEntity
    // + hasChest()==true 过滤，此处仅作为存在性标记返回，不暴露额外属性。
    // 供集成测试断言驴/骡装箱链路（AbstractChestedHorseEntity::equipChest→setChest(true)）。
    u64 isChestedClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* isChestedProto = builder.exportClass("IsChestedComponent", isChestedClassId);
    ScriptClassRegistry::instance().registerClass(isChestedClassId, isChestedProto, "IsChestedComponent");
    // 无 property/method：组件对象存在即 chested（与基岩 EntityIsChestedComponent 一致）。

    // --- ColorComponent类（minecraft:color，承载实体颜色）---
    // opaque 持 mc::Entity*。对齐基岩 EntityColorComponent：value 为 PaletteColor 0-15（与 DyeColor 数值
    // 一致）。getComponent 已按 SheepEntity 过滤，此处 dynamic_cast 现取羊毛颜色。readonly value：int
    // 0-15（DyeColor 枚举底层数值，White=0..Black=15）。供集成测试断言唤魔者 Wololo 变色（蓝11→红14）、
    // 染料染色等链路。羊驼/潜影贝颜色枚举不统一留 TODO。
    u64 colorClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* colorProto = builder.exportClass("ColorComponent", colorClassId);
    ScriptClassRegistry::instance().registerClass(colorClassId, colorProto, "ColorComponent");

    ClassRegistrar<void> colorReg(ctx, colorClassId, colorProto);
    colorReg.readonlyProperty("value", [colorClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, colorClassId));
        auto* sheep = dynamic_cast<mc::SheepEntity*>(ent);
        if (sheep == nullptr) {
            return ctx.createUndefined();
        }
        // DyeColor : u8，static_cast 取枚举底层数值（0-15，与基岩 PaletteColor 一致）。
        return ctx.createInt32(static_cast<i32>(sheep->getFleeceColor()));
    });

    // --- Effect 工具：构造基岩 Entity.getEffect 返回的 Effect 普通对象 ---
    // 基岩 Entity.getEffect(effectType) 返回 Effect 对象（{ typeId, amplifier, duration }），
    // 无该效果返回 undefined。effectType 既接受 "minecraft:blindness" 也接受简写 "blindness"。
    // 此处定义为 lambda 供 Entity.getEffect / getEffects 复用：解析 effectType→EffectType，
    // 从 LivingEntity::getEffect 取 EffectInstance，构造普通对象（不注册 JS 类，属性快照现取）。
    auto buildEffectObject =
        [](IScriptBindingContext& ctx, mc::LivingEntity* living, mc::entity::effect::EffectType type) -> void* {
        const mc::entity::effect::EffectInstance* inst = living->getEffect(type);
        if (inst == nullptr) {
            return ctx.createUndefined();
        }
        void* obj = ctx.createObject();
        // typeId：基岩用 "minecraft:blindness" 形式的资源位置。
        ctx.setPropertyString(obj, "typeId", mc::entity::effect::getEffectResourceLocation(type).toString());
        ctx.setPropertyInt(obj, "amplifier", inst->amplifier());
        ctx.setPropertyInt(obj, "duration", inst->duration());
        return obj;
    };
    // 解析 JS effectType 参数（string 或 EffectType 对象）为 EffectType，失败返 nullopt。
    // 基岩允许传字符串（"blindness"）或 EffectType 对象（.id）。Cubium 暂仅支持字符串。
    auto parseEffectType = [](IScriptBindingContext& ctx, void* arg) -> std::optional<mc::entity::effect::EffectType> {
        if (arg == nullptr || !ctx.isString(arg)) {
            return std::nullopt;
        }
        auto s = ctx.toString(arg);
        if (!s) {
            return std::nullopt;
        }
        std::string id = *s;
        if (id.find(':') == std::string::npos) {
            id = "minecraft:" + id;
        }
        return mc::entity::effect::getEffectByResourceLocation(mc::ResourceLocation(id));
    };

    // Entity.getEffect(effectType)：对齐基岩 Entity.getEffect，返回 Effect 普通对象
    // （{ typeId, amplifier, duration }），无该效果/非 LivingEntity 返回 undefined。
    // 用于 GameTest 检测实体状态效果（如幻术师镜像隐身、失明法术）。
    entityReg.method(
        "getEffect",
        [entityClassId, &buildEffectObject, &parseEffectType](
            IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr || argc < 1) {
                return ctx.createUndefined();
            }
            auto typeOpt = parseEffectType(ctx, args[0]);
            if (!typeOpt.has_value()) {
                return ctx.createUndefined();
            }
            return buildEffectObject(ctx, living, *typeOpt);
        },
        1);

    // Entity.addEffect(effectType, duration, options?): void —— 对齐基岩官方 Entity.addEffect。
    // 官方签名：addEffect(effectType: EffectType | string, duration: number, options?:
    // EntityEffectOptions): Effect | undefined。options={ amplifier?: number, showParticles?: boolean }。
    // 成功返回 undefined（对齐官方"成功返回 undefined"）。effectType 支持简写("weakness")或全称
    // ("minecraft:weakness")。duration 单位 tick。非 LivingEntity（掉落物/经验球等）无效果管理器，
    // 返 undefined。
    // 用于 GameTest 给实体施加状态效果（如僵尸村民治愈需先施虚弱、测试中毒/凋零伤害等），
    // 此前仅 EffectCommand(/effect) 能施效果且只支持玩家选择器，对非玩家实体不可用。
    // TODO: 官方还支持 effectType 传 EffectType 对象（.id），Cubium 暂仅支持字符串（同 parseEffectType 限制）。
    entityReg.method(
        "addEffect",
        [entityClassId, &parseEffectType](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr || argc < 2) {
                return ctx.createUndefined();
            }
            auto typeOpt = parseEffectType(ctx, args[0]);
            if (!typeOpt.has_value()) {
                return ctx.createUndefined();
            }
            // duration: number（tick），基岩范围 [1, 20000000]。
            // args[1] 是 JS number 原始值，用 toInt32 取整（对齐基岩 duration 取整语义）。
            auto durationOpt = ctx.toInt32(args[1]);
            if (!durationOpt.has_value()) {
                return ctx.createUndefined();
            }
            i32 duration = *durationOpt;
            // options?: EntityEffectOptions（可选对象）。
            i32 amplifier = 0;
            bool showParticles = true; // 对齐 EffectInstance 默认 visible=true
            if (argc >= 3 && ctx.isObject(args[2])) {
                void* opts = args[2];
                auto ampOpt = ctx.getPropertyInt(opts, "amplifier");
                if (ampOpt.has_value()) {
                    amplifier = *ampOpt;
                }
                auto partOpt = ctx.getPropertyBool(opts, "showParticles");
                if (partOpt.has_value()) {
                    showParticles = *partOpt;
                }
            }
            // EffectInstance(type, duration, amplifier, ambient=false, visible=showParticles)。
            // 对齐 EffectCommand.cpp:187 用法（ambient 固定 false，visible 对应 showParticles）。
            mc::entity::effect::EffectInstance effect(*typeOpt, duration, amplifier, false, showParticles);
            living->addEffect(std::move(effect));
            return ctx.createUndefined();
        },
        3);

    // Entity.teleport(location: Vector3, teleportOptions?: TeleportOptions): void
    // 对齐基岩 Entity.teleport。同维度（无 options.dimension 或 dimension 与实体当前维度相同）
    // 传送；跨维度（options.dimension 指定不同维度）走虚派发 changeDimension（ServerPlayer override
    // 调真实实现）。
    //
    // checkForBlocks 语义（对齐基岩 TeleportOptions.checkForBlocks）：
    // - teleport 默认 checkForBlocks=false → 强制 setPosition，不经碰撞检测（基岩官方示例
    //   teleportMovement.ts 把实体传到可能嵌入方块的位置，证明默认强制传送）。
    // - 显式 checkForBlocks=true → 走 attemptTeleport（带 findSafeTeleportPosition + 碰撞检测）。
    // tryTeleport 默认 checkForBlocks=true（见下方），与 teleport 默认相反。
    //
    // 注意：changeDimension 不接受自定义位置（位置由 Teleporter 计算：末地固定 (100,49,0)，
    // 下界 1:8 缩放），故跨维度时 location 参数被忽略。这与基岩 teleport 跨维度支持 location 有差异。
    // TODO: 若需支持跨维度自定义位置，需扩展 changeDimension 接受 optional<Vector3d> 并传给
    //       transferPlayerToDimension（其已支持 position 参数）。
    entityReg.method(
        "teleport",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr || argc < 1 || !ctx.isObject(args[0])) {
                return ctx.createUndefined();
            }
            // 解析 location: Vector3 {x,y,z}
            void* locObj = args[0];
            auto xOpt = ctx.getPropertyFloat(locObj, "x");
            auto yOpt = ctx.getPropertyFloat(locObj, "y");
            auto zOpt = ctx.getPropertyFloat(locObj, "z");
            if (!xOpt || !yOpt || !zOpt) {
                return ctx.throwTypeError("teleport location requires {x,y,z}");
            }
            f64 x = *xOpt, y = *yOpt, z = *zOpt;

            // 解析 options.dimension（可选）：若指定且与当前维度不同则跨维度传送。
            // 解析 options.checkForBlocks（可选）：teleport 默认 false（强制传送）。
            mc::DimensionId targetDim = ent->dimension();
            bool crossDim = false;
            bool checkForBlocks = false; // teleport 默认不检查碰撞
            if (argc >= 2 && ctx.isObject(args[1])) {
                void* opts = args[1];
                void* dimVal = ctx.getProperty(opts, "dimension");
                if (dimVal != nullptr && ctx.isObject(dimVal)) {
                    // Dimension JS 对象 opaque 持 IWorld*，unwrap 后读 IWorld::dimension()
                    const u64 dimClassId = ScriptClassRegistry::instance().classIdByName("Dimension");
                    auto* dimWorld = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, dimVal, dimClassId));
                    if (dimWorld != nullptr) {
                        targetDim = dimWorld->dimension();
                        crossDim = (targetDim != ent->dimension());
                    }
                }
                ctx.releaseValue(dimVal);
                auto checkOpt = ctx.getPropertyBool(opts, "checkForBlocks");
                if (checkOpt.has_value()) {
                    checkForBlocks = *checkOpt;
                }
            }

            if (crossDim) {
                // 跨维度：经虚派发 changeDimension（ServerPlayer override 调真实实现）。
                // 非 Player 实体基类返回 false（不传送），对齐当前 Cubium 限制。
                ent->changeDimension(targetDim);
            } else if (checkForBlocks) {
                // 同维度 + 检查碰撞：attemptTeleport（findSafeTeleportPosition + 碰撞检测）。
                ent->attemptTeleport(x, y, z, false);
            } else {
                // 同维度 + 强制传送：直接 setPosition，跳过碰撞检测（对齐基岩 checkForBlocks=false）。
                ent->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
            }
            return ctx.createUndefined();
        },
        2);

    // Entity.tryTeleport(location: Vector3, teleportOptions?: TeleportOptions): boolean
    // 对齐基岩 tryTeleport：返回是否传送成功。tryTeleport 默认 checkForBlocks=true
    // （基岩文档："can fail if...intersecting with blocks"）。同维度走 attemptTeleport 返回值；
    // 跨维度走 changeDimension 返回值。显式 checkForBlocks=false 时强制传送（恒成功）。
    entityReg.method(
        "tryTeleport",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr || argc < 1 || !ctx.isObject(args[0])) {
                return ctx.createBoolean(false);
            }
            void* locObj = args[0];
            auto xOpt = ctx.getPropertyFloat(locObj, "x");
            auto yOpt = ctx.getPropertyFloat(locObj, "y");
            auto zOpt = ctx.getPropertyFloat(locObj, "z");
            if (!xOpt || !yOpt || !zOpt) {
                return ctx.createBoolean(false);
            }
            f64 x = *xOpt, y = *yOpt, z = *zOpt;

            mc::DimensionId targetDim = ent->dimension();
            bool crossDim = false;
            bool checkForBlocks = true; // tryTeleport 默认检查碰撞
            if (argc >= 2 && ctx.isObject(args[1])) {
                void* opts = args[1];
                void* dimVal = ctx.getProperty(opts, "dimension");
                if (dimVal != nullptr && ctx.isObject(dimVal)) {
                    const u64 dimClassId = ScriptClassRegistry::instance().classIdByName("Dimension");
                    auto* dimWorld = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, dimVal, dimClassId));
                    if (dimWorld != nullptr) {
                        targetDim = dimWorld->dimension();
                        crossDim = (targetDim != ent->dimension());
                    }
                }
                ctx.releaseValue(dimVal);
                auto checkOpt = ctx.getPropertyBool(opts, "checkForBlocks");
                if (checkOpt.has_value()) {
                    checkForBlocks = *checkOpt;
                }
            }

            if (crossDim) {
                bool ok = ent->changeDimension(targetDim);
                return ctx.createBoolean(ok);
            }
            if (checkForBlocks) {
                bool ok = ent->attemptTeleport(x, y, z, false);
                return ctx.createBoolean(ok);
            }
            // 强制传送（checkForBlocks=false）：直接 setPosition，恒成功。
            ent->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
            return ctx.createBoolean(true);
        },
        2);

    // Entity.getEffects()：对齐基岩 Entity.getEffects，返回 Effect[]（实体当前所有效果）。
    entityReg.method(
        "getEffects",
        [entityClassId, &buildEffectObject](
            IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr) {
                return ctx.createArray();
            }
            const auto& effects = living->effectManager().getAllEffects();
            void* arr = ctx.createArray();
            u32 outIdx = 0;
            for (const auto& inst : effects) {
                void* obj = buildEffectObject(ctx, living, inst.type());
                // buildEffectObject 无该效果返 undefined（理论上 getAllEffects 不含空项，守卫跳过）。
                if (obj != nullptr && !ctx.isUndefined(obj)) {
                    ctx.setArrayElement(arr, outIdx, obj); // 不消耗所有权
                    ctx.releaseValue(obj);
                    ++outIdx;
                }
            }
            return arr;
        },
        0);

    // Entity.getTags()/hasTag()/addTag()/removeTag()：对齐基岩 @minecraft/server Entity 标签 API。
    // 标签存储在 Entity 基类 m_tags（std::set<string>），/tag 命令、scoreboard 实体选择器、
    // 团队归属等均依赖之。此前仅 C++ 侧有 addTag/getTags，脚本侧无绑定，致 GameTest 无法断言
    // /tag 命令效果、无法按标签筛选实体。getTags 返回 string[]（对齐基岩 getTags(): string[]），
    // hasTag/addTag/removeTag 返回 boolean（addTag 标签已存在返 false，removeTag 不存在返 false，
    // 对齐基岩 Entity.addTag/removeTag 语义）。
    entityReg.method(
        "getTags",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr) {
                return ctx.createArray();
            }
            const auto& tags = ent->getTags();
            void* arr = ctx.createArray();
            u32 outIdx = 0;
            for (const auto& tag : tags) {
                void* str = ctx.createString(tag);
                ctx.setArrayElement(arr, outIdx, str);
                ctx.releaseValue(str);
                ++outIdx;
            }
            return arr;
        },
        0);
    entityReg.method(
        "hasTag",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createBoolean(false);
            }
            auto tag = ctx.toString(args[0]);
            return ctx.createBoolean(tag ? ent->hasTag(*tag) : false);
        },
        1);
    entityReg.method(
        "addTag",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createBoolean(false);
            }
            auto tag = ctx.toString(args[0]);
            return ctx.createBoolean(tag ? ent->addTag(*tag) : false);
        },
        1);
    entityReg.method(
        "removeTag",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createBoolean(false);
            }
            auto tag = ctx.toString(args[0]);
            return ctx.createBoolean(tag ? ent->removeTag(*tag) : false);
        },
        1);

    // --- HealthComponent类（minecraft:health，Attribute 族）---
    // opaque 持 mc::Entity*。HealthComponent 仅 LivingEntity attach，getter 内 dynamic_cast<LivingEntity*>，
    // 失败返 undefined（对齐基岩"组件不存在则 getComponent 返 undefined"）。currentValue/effectiveMax 走
    // LivingEntity::health()/maxHealth()（HealthComponent 真相源 + 属性系统）；setCurrentValue/resetToMaxValue
    // 走 setHealth（带 DataParameter 同步副作用）。effectiveMin/defaultValue 保守硬编码留 TODO。
    u64 healthClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* healthProto = builder.exportClass("HealthComponent", healthClassId);
    ScriptClassRegistry::instance().registerClass(healthClassId, healthProto, "HealthComponent");

    ClassRegistrar<void> healthReg(ctx, healthClassId, healthProto);
    healthReg.readonlyProperty("currentValue", [healthClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createFloat64(static_cast<f64>(living->health()));
    });
    healthReg.readonlyProperty("effectiveMax", [healthClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createFloat64(static_cast<f64>(living->maxHealth()));
    });
    healthReg.readonlyProperty("effectiveMin", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: 读 AttributeInstance::attribute().minValue()，属性边界后续完善。
        return ctx.createFloat64(0.0);
    });
    healthReg.readonlyProperty("defaultValue", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: HealthComponent 默认 m_health{20.0f}，按实体类型差异后续完善。
        return ctx.createFloat64(20.0);
    });
    healthReg.method(
        "setCurrentValue",
        [healthClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr) {
                return ctx.createBoolean(false);
            }
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("health.setCurrentValue requires a number argument");
            }
            auto value = ctx.toFloat64(args[0]);
            if (!value) {
                return ctx.createBoolean(false);
            }
            living->setHealth(static_cast<f32>(*value)); // 内部 clamp 到 [0, maxHealth]
            return ctx.createBoolean(true);
        },
        1);
    healthReg.method("resetToMaxValue",
        [healthClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                living->setHealth(living->maxHealth());
            }
            return ctx.createUndefined();
        });
    healthReg.method("resetToMinValue",
        [healthClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            // TODO: 按 AttributeInstance::attribute().minValue() 设置，暂用 1.0（属性 min=1.0）。
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                living->setHealth(1.0f);
            }
            return ctx.createUndefined();
        });
    healthReg.method("resetToDefaultValue",
        [healthClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            // TODO: 按实体类型默认 health 设置，暂用 20.0。
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                living->setHealth(20.0f);
            }
            return ctx.createUndefined();
        });

    // --- MovementComponent类（minecraft:movement，Attribute 族）---
    // opaque 持 mc::Entity*。移动速度走 AttributeComponent 持有的 AttributeMap（Attributes::MOVEMENT_SPEED）。
    // dynamic_cast<LivingEntity*> 后经 attributes() 读写。effectiveMax/Min/defaultValue 走
    // AttributeInstance::attribute().maxValue()/minValue()/defaultValue()，属性实例缺失时 fallback。
    u64 movementClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* movementProto = builder.exportClass("MovementComponent", movementClassId);
    ScriptClassRegistry::instance().registerClass(movementClassId, movementProto, "MovementComponent");

    ClassRegistrar<void> movementReg(ctx, movementClassId, movementProto);
    movementReg.readonlyProperty("currentValue", [movementClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createFloat64(living->attributes().getValue(mc::entity::attribute::Attributes::MOVEMENT_SPEED, 0.0));
    });
    movementReg.readonlyProperty("effectiveMax", [movementClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        const auto* inst = living->attributes().getInstance(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
        // 属性实例缺失时 fallback 1024.0（vanilla MOVEMENT_SPEED 上界）。
        return ctx.createFloat64(inst != nullptr ? inst->attribute().maxValue() : 1024.0);
    });
    movementReg.readonlyProperty("effectiveMin", [movementClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        const auto* inst = living->attributes().getInstance(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
        return ctx.createFloat64(inst != nullptr ? inst->attribute().minValue() : 0.0);
    });
    movementReg.readonlyProperty("defaultValue", [movementClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        const auto* inst = living->attributes().getInstance(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
        // TODO: 各实体默认移动速度不同，inst 缺失时 fallback 0.25（vanilla 通用值）。
        return ctx.createFloat64(inst != nullptr ? inst->attribute().defaultValue() : 0.25);
    });
    movementReg.method(
        "setCurrentValue",
        [movementClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr) {
                return ctx.createBoolean(false);
            }
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("movement.setCurrentValue requires a number argument");
            }
            auto value = ctx.toFloat64(args[0]);
            if (!value) {
                return ctx.createBoolean(false);
            }
            living->attributes().setBaseValue(mc::entity::attribute::Attributes::MOVEMENT_SPEED, *value);
            return ctx.createBoolean(true);
        },
        1);
    movementReg.method("resetToDefaultValue",
        [movementClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                living->attributes().resetBaseValue(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
            }
            return ctx.createUndefined();
        });
    movementReg.method("resetToMaxValue",
        [movementClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                const auto* inst = living->attributes().getInstance(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
                if (inst != nullptr) {
                    living->attributes().setBaseValue(
                        mc::entity::attribute::Attributes::MOVEMENT_SPEED, inst->attribute().maxValue());
                }
            }
            return ctx.createUndefined();
        });
    movementReg.method("resetToMinValue",
        [movementClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                const auto* inst = living->attributes().getInstance(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
                if (inst != nullptr) {
                    living->attributes().setBaseValue(
                        mc::entity::attribute::Attributes::MOVEMENT_SPEED, inst->attribute().minValue());
                }
            }
            return ctx.createUndefined();
        });

    // --- EquippableComponent类（minecraft:equippable）---
    // opaque 持 mc::Entity*。dynamic_cast<LivingEntity*> 后经 getEquipment/setEquipment（虚派发，Player 重写
    // 走 PlayerInventory）。基岩 EquipmentSlot 字符串值 "Head"/"Chest"/"Legs"/"Feet"/"Mainhand"/"Offhand"/"Body"
    // 映射项目 EquipmentSlot 枚举（注意 Mainhand/Offhand 的 h/a 小写，无 Saddle）。
    // getEquipment 返回 owned 拷贝 ItemStack（规避 setEquipment 改写数组致引用悬垂）。
    // setEquipment 支持清空（undefined/null → EMPTY）与写入 ItemStack（unwrap 入参后拷贝写入装备数组）。
    u64 equippableClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* equippableProto = builder.exportClass("EquippableComponent", equippableClassId);
    ScriptClassRegistry::instance().registerClass(equippableClassId, equippableProto, "EquippableComponent");

    ClassRegistrar<void> equippableReg(ctx, equippableClassId, equippableProto);
    equippableReg.readonlyProperty("totalArmor", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: 读 attributes().getValue(Attributes::ARMOR)，属性未必对所有 LivingEntity 注册，暂返 0。
        return ctx.createFloat64(0.0);
    });
    equippableReg.readonlyProperty("totalToughness", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: 读 attributes().getValue(Attributes::ARMOR_TOUGHNESS)，暂返 0。
        return ctx.createFloat64(0.0);
    });
    equippableReg.method(
        "getEquipment",
        [equippableClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, equippableClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createUndefined();
            }
            auto slotStr = ctx.toString(args[0]);
            if (!slotStr) {
                return ctx.createUndefined();
            }
            // 基岩 EquipmentSlot 字符串 → 项目枚举（精确匹配，未知返 undefined）。
            std::optional<mc::EquipmentSlot> slot = std::nullopt;
            const std::string& s = *slotStr;
            if (s == "Head")
                slot = mc::EquipmentSlot::Head;
            else if (s == "Chest")
                slot = mc::EquipmentSlot::Chest;
            else if (s == "Legs")
                slot = mc::EquipmentSlot::Legs;
            else if (s == "Feet")
                slot = mc::EquipmentSlot::Feet;
            else if (s == "Mainhand")
                slot = mc::EquipmentSlot::MainHand;
            else if (s == "Offhand")
                slot = mc::EquipmentSlot::OffHand;
            else if (s == "Body")
                slot = mc::EquipmentSlot::Body;
            if (!slot.has_value()) {
                return ctx.createUndefined();
            }
            const mc::ItemStack& stack = living->getEquipment(*slot);
            if (stack.isEmpty()) {
                return ctx.createUndefined();
            }
            // owned 拷贝：JS GC 时 delete，规避 setEquipment 改写装备数组致 owned=false 引用悬垂。
            const u64 itemStackClassId = ScriptClassRegistry::instance().classIdByName("ItemStack");
            void* itemStackProto = ScriptClassRegistry::instance().proto(itemStackClassId);
            if (itemStackProto == nullptr) {
                return ctx.createUndefined();
            }
            auto* owned = new mc::ItemStack(stack); // 拷贝（含 NBT 深拷贝）
            return ScriptObjectRegistry::wrap(
                ctx, itemStackClassId, itemStackProto, owned, true, "ItemStack", [](void* p) {
                    delete static_cast<mc::ItemStack*>(p);
                });
        },
        1);
    equippableReg.method(
        "setEquipment",
        [equippableClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, equippableClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr || argc < 2 || !ctx.isString(args[0])) {
                return ctx.createBoolean(false);
            }
            auto slotStr = ctx.toString(args[0]);
            if (!slotStr) {
                return ctx.createBoolean(false);
            }
            std::optional<mc::EquipmentSlot> slot = std::nullopt;
            const std::string& s = *slotStr;
            if (s == "Head")
                slot = mc::EquipmentSlot::Head;
            else if (s == "Chest")
                slot = mc::EquipmentSlot::Chest;
            else if (s == "Legs")
                slot = mc::EquipmentSlot::Legs;
            else if (s == "Feet")
                slot = mc::EquipmentSlot::Feet;
            else if (s == "Mainhand")
                slot = mc::EquipmentSlot::MainHand;
            else if (s == "Offhand")
                slot = mc::EquipmentSlot::OffHand;
            else if (s == "Body")
                slot = mc::EquipmentSlot::Body;
            if (!slot.has_value()) {
                return ctx.createBoolean(false);
            }
            // 清空槽位：undefined/null → ItemStack::EMPTY。
            void* itemArg = args[1];
            if (ctx.isUndefined(itemArg) || ctx.getType(itemArg) == ScriptType::Null) {
                living->setEquipment(*slot, mc::ItemStack::EMPTY);
                return ctx.createBoolean(true);
            }
            // ItemStack 入参：按 classId unwrap（非拥有，仅拷贝写入装备数组）。复用 Container.setItem
            // 的 unwrap 范式（resolveItemStackClassId + ScriptObjectRegistry::unwrap）。
            const u64 isClassId = resolveItemStackClassId();
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, itemArg, isClassId));
            if (stack == nullptr) {
                return ctx.throwTypeError(
                    "EquippableComponent.setEquipment: argument must be an ItemStack, undefined, or null");
            }
            living->setEquipment(*slot, *stack);
            return ctx.createBoolean(true);
        },
        2);
    // setEquipmentDropChance(slot, chance)：设置装备槽掉落概率（对齐 vanilla Mob.dropChances）。
    // 仅 MobEntity 支持（LivingEntity 基类无掉落概率概念）。chance 语义同 C++ setEquipmentDropChance：
    // 0.0=永不掉落，0.085=默认概率，>1.0=保整（isPreserved，无条件掉落，用于测试与首领固定掉落）。
    // 测试用 >1.0 构造确定性装备掉落（绕过 8.5% 概率与 recentlyHitByPlayer 门控）。
    equippableReg.method(
        "setEquipmentDropChance",
        [equippableClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, equippableClassId));
            auto* mob = dynamic_cast<mc::MobEntity*>(ent);
            if (mob == nullptr || argc < 2 || !ctx.isString(args[0]) || !ctx.isNumber(args[1])) {
                return ctx.createBoolean(false);
            }
            auto slotStr = ctx.toString(args[0]);
            if (!slotStr) {
                return ctx.createBoolean(false);
            }
            std::optional<mc::EquipmentSlot> slot = std::nullopt;
            const std::string& s = *slotStr;
            if (s == "Head")
                slot = mc::EquipmentSlot::Head;
            else if (s == "Chest")
                slot = mc::EquipmentSlot::Chest;
            else if (s == "Legs")
                slot = mc::EquipmentSlot::Legs;
            else if (s == "Feet")
                slot = mc::EquipmentSlot::Feet;
            else if (s == "Mainhand")
                slot = mc::EquipmentSlot::MainHand;
            else if (s == "Offhand")
                slot = mc::EquipmentSlot::OffHand;
            else if (s == "Body")
                slot = mc::EquipmentSlot::Body;
            if (!slot.has_value()) {
                return ctx.createBoolean(false);
            }
            auto chance = ctx.toFloat64(args[1]);
            if (!chance) {
                return ctx.createBoolean(false);
            }
            mob->setEquipmentDropChance(*slot, static_cast<mc::f32>(*chance));
            return ctx.createBoolean(true);
        },
        2);

    // --- Player类（继承Entity） ---
    u64 playerClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* playerProto = builder.exportClass("Player", playerClassId);
    // 登记进 ScriptClassRegistry，供 test.kill 等需识别 Player 形参的绑定经 classIdByName("Player") unwrap。
    // 注：Cubium JS 类按独立原型注册不自动反映 C++ 继承（见 [[simulated-player-js-class-no-entity-inheritance]]），
    // Player JS 对象的 opaque class_id 是 playerClassId 而非 Entity classId，故 ScriptObjectRegistry::unwrap
    // 传 Entity classId 严格匹配会返 nullptr。test.kill 等绑定需枚举 Entity/Player/SimulatedPlayer 多 classId
    // 依次 unwrap，此处登记使 classIdByName("Player") 可查。
    ScriptClassRegistry::instance().registerClass(playerClassId, playerProto, "Player");

    ClassRegistrar<void> playerReg(ctx, playerClassId, playerProto);
    playerReg.readonlyProperty("name", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // Player JS 类 opaque 持 mc::Entity*（实际为 Player*）。name 走 Player::username（非 nameTag/customName）。
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
        auto* player = dynamic_cast<mc::Player*>(ent);
        if (player == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(player->username());
    });
    // getSpawnPoint：返回玩家重生点 {x,y,z,dimensionId}（Player::getSpawnPoint 返 optional<GlobalPos>）。
    // 供 /spawnpoint 命令测试读取重生点做断言。无重生点（nullopt）返回 undefined。dimensionId 为整数
    // （DimensionManager::OVERWORLD=0 等）。Cubium 扩展属性（官方基岩 API 无 Player.getSpawnPoint）。
    playerReg.readonlyProperty("getSpawnPoint", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
        auto* player = dynamic_cast<mc::Player*>(ent);
        if (player == nullptr) {
            return ctx.createUndefined();
        }
        auto spawnOpt = player->getSpawnPoint();
        if (!spawnOpt.has_value()) {
            return ctx.createUndefined(); // 无重生点
        }
        const GlobalPos& gp = spawnOpt.value();
        void* obj = ctx.createObject();
        ctx.setPropertyInt(obj, "x", static_cast<i32>(gp.x()));
        ctx.setPropertyInt(obj, "y", static_cast<i32>(gp.y()));
        ctx.setPropertyInt(obj, "z", static_cast<i32>(gp.z()));
        ctx.setPropertyInt(obj, "dimensionId", static_cast<i32>(gp.getDimensionId()));
        return obj;
    });

    // --- BlockPermutation类（@minecraft/server）---
    // 不可变方块状态包装。opaque 持 const mc::BlockState*（非拥有，BlockRegistry 全局拥有）。
    // 官方 BlockPermutation：type（→typeId，对应方块资源位置）/isValid。底层 BlockState 就绪。
    // 登记进 ScriptClassRegistry 供 setBlockPermutation 等 unwrap 入参路径跨回调取 classId。
    u64 blockPermutationClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* blockPermutationProto = builder.exportClass("BlockPermutation", blockPermutationClassId);
    ScriptClassRegistry::instance().registerClass(blockPermutationClassId, blockPermutationProto, "BlockPermutation");

    ClassRegistrar<void> blockPermutationReg(ctx, blockPermutationClassId, blockPermutationProto);
    // type/isValid/resolve 的 unwrap/wrap classId 均运行时回查 classIdByName（不捕获注册期 blockPermutationClassId），
    // 对齐 resolveItemStackClassId 模式：引擎重建后 classId 重新分配，ScriptClassRegistry 被新注册覆盖，
    // 闭包捕获的旧 classId 会与 setBlockPermutation 等 unwrap 入参路径用 classIdByName 取的最新 classId 失配，
    // 致 JS_GetOpaque2 返回 nullptr。回查保证 wrap 与 unwrap 用同一（最新）classId。详见 resolve 处注释。
    blockPermutationReg.readonlyProperty("type", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 官方 BlockPermutation.type 返回 BlockType（方块类型对象，含 id）。
        // 项目无独立 BlockType 类，复用 ItemType 同构：返回 typeId 字符串。TODO: 完整 BlockType 类。
        const u64 classId = ScriptClassRegistry::instance().classIdByName("BlockPermutation");
        auto* state = static_cast<const mc::BlockState*>(ScriptObjectRegistry::unwrap(ctx, thisVal, classId));
        if (state == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(state->blockLocation().toString());
    });
    blockPermutationReg.readonlyProperty("isValid", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 官方 isValid 表示此 permutation 是否为有效状态（非 air 占位即视为有效）。
        const u64 classId = ScriptClassRegistry::instance().classIdByName("BlockPermutation");
        auto* state = static_cast<const mc::BlockState*>(ScriptObjectRegistry::unwrap(ctx, thisVal, classId));
        return ctx.createBoolean(state != nullptr);
    });
    // getState(propertyName) -> boolean | number | string | undefined。
    // 官方 BlockPermutation.getState 读取指定 block state 属性的当前值。未命名属性返 undefined。
    // 取值路径对齐 StateHolder::toString：state->values() 返回 vector<PropertyEntry{IProperty*, valueIndex}>，
    // 按 entry.property->name() 匹配，按 property->typeName() 调度返回 JS 类型：
    //   IntegerProperty → number（valueToString 返数字字符串，转 i32）；
    //   BooleanProperty → boolean（valueToString 返 "true"/"false"）；
    //   EnumProperty    → string（valueToString 返枚举名字符串）。
    // typeName 统一经 IProperty 虚接口返回，无需 dynamic_cast 到具体 Property<T>，与 toString 范式一致。
    blockPermutationReg.method(
        "getState", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1) {
                return ctx.throwTypeError("BlockPermutation.getState(propertyName: string)");
            }
            const u64 classId = ScriptClassRegistry::instance().classIdByName("BlockPermutation");
            auto* state = static_cast<const mc::BlockState*>(ScriptObjectRegistry::unwrap(ctx, thisVal, classId));
            if (state == nullptr) {
                return ctx.createUndefined();
            }
            auto propName = ctx.toString(args[0]);
            if (!propName) {
                return ctx.throwTypeError("BlockPermutation.getState: propertyName must be a string");
            }
            // 遍历 state 的全部属性，按名匹配。
            for (const auto& entry : state->values()) {
                if (entry.property == nullptr) {
                    continue;
                }
                if (entry.property->name() != *propName) {
                    continue;
                }
                const std::string valStr = entry.property->valueToString(entry.valueIndex);
                const char* tn = entry.property->typeName();
                if (tn == std::string("IntegerProperty")) {
                    // 数字属性：值字符串形如 "2"，转 i32 返 number。
                    try {
                        return ctx.createInt32(static_cast<i32>(std::stol(valStr)));
                    }
                    catch (...) {
                        return ctx.createString(valStr); // 解析失败降级返字符串
                    }
                }
                if (tn == std::string("BooleanProperty")) {
                    return ctx.createBoolean(valStr == "true");
                }
                // EnumProperty 或未知类型：返字符串（枚举名）。
                return ctx.createString(valStr);
            }
            // 属性不存在：返 undefined（对齐官方 getState 语义）。
            return ctx.createUndefined();
        });

    // BlockPermutation.resolve(blockType, states?) 静态方法：按 typeId + 可选 states 映射构造一个
    // BlockPermutation（不写入世界，仅返回 permutation 对象）。供 setBlockPermutation 等 API 消费。
    // 官方签名：resolve(blockType: BlockType | string, states?: Record<string, boolean|number|string>)。
    // 本项目无独立 BlockType 类（type 属性返回 typeId 字符串），故 blockType 仅处理 string 形式。
    //
    // 解析链路对齐 GameTestHelper::setBlockWithStates（GameTestHelper.cpp:346-380）：
    //   string → ResourceLocation（补 "minecraft:" 前缀 + 别名表）→ BlockRegistry::getBlock 取 Block*
    //   → defaultState() → 逐 states 属性 StateContainer::getProperty + IProperty::parseValue +
    //   BlockState::withValueIndex 应用 → 返回非拥有 wrap 的 BlockPermutation。
    // states 值支持 boolean/number/string 三种 JS 类型，统一经 toString 转字符串后交 parseValue
    // （IProperty::parseValue 接 string_view，如 IntegerProperty 解析 "2"、BooleanProperty 解析 "true"）。
    // 未知属性名静默忽略（容错，与 setBlockWithStates 一致）；非法值抛 TypeError。
    blockPermutationReg.staticMethod(
        "resolve",
        [blockPermutationClassId, blockPermutationProto](
            IScriptBindingContext& ctx, void* /*thisVal*/, i32 argc, void** args) -> void* {
            if (argc < 1) {
                return ctx.throwTypeError("BlockPermutation.resolve(blockType, states?)");
            }
            // 第一参：typeId 字符串。
            auto blockType = ctx.toString(args[0]);
            if (!blockType) {
                return ctx.throwTypeError("BlockPermutation.resolve: blockType must be a string");
            }

            // string → Block*（补命名空间前缀 + 别名表，复用 setBlockWithStates 的别名对齐逻辑）。
            std::string full = blockType->find(':') == std::string::npos ? "minecraft:" + *blockType : *blockType;
            const mc::ResourceLocation loc(full);

            static const std::unordered_map<std::string, std::string> kBlockAliases = {
                {"minecraft:brick_block", "minecraft:bricks"},
            };

            const mc::Block* block = mc::BlockRegistry::instance().getBlock(loc);
            if (block == nullptr) {
                auto it = kBlockAliases.find(full);
                if (it != kBlockAliases.end()) {
                    block = mc::BlockRegistry::instance().getBlock(mc::ResourceLocation(it->second));
                }
            }
            if (block == nullptr) {
                return ctx.throwTypeError(
                    ("BlockPermutation.resolve: unknown block type '" + *blockType + "'").c_str());
            }

            // 从默认 state 出发，逐属性应用 states。
            const mc::BlockState* state = &block->defaultState();
            const auto& container = block->stateContainer();

            if (argc >= 2 && ctx.isObject(args[1])) {
                // 遍历 states 对象的自身可枚举字符串键（getPropertyNames 补齐的枚举能力）。
                for (const auto& propName : ctx.getPropertyNames(args[1])) {
                    const mc::IProperty* prop = container.getProperty(propName);
                    if (prop == nullptr) {
                        continue; // 未知属性静默忽略（容错）
                    }
                    // 取属性值句柄（owned），按 JS 类型转字符串供 parseValue。
                    void* valHandle = ctx.getProperty(args[1], propName.c_str());
                    std::optional<std::string> valueStr;
                    if (ctx.isString(valHandle)) {
                        valueStr = ctx.toString(valHandle);
                    } else if (ctx.isNumber(valHandle)) {
                        auto d = ctx.toFloat64(valHandle);
                        if (d) {
                            valueStr = std::to_string(static_cast<i64>(*d));
                        }
                    } else {
                        auto b = ctx.toBool(valHandle);
                        if (b) {
                            valueStr = std::string(*b ? "true" : "false");
                        }
                    }
                    ctx.releaseValue(valHandle);

                    if (!valueStr) {
                        return ctx.throwTypeError(
                            ("BlockPermutation.resolve: invalid value for property '" + propName + "'").c_str());
                    }
                    auto parsed = prop->parseValue(*valueStr);
                    if (!parsed.has_value()) {
                        return ctx.throwTypeError(("BlockPermutation.resolve: invalid value '" + *valueStr +
                            "' for property '" + propName + "' on block '" + *blockType + "'")
                                .c_str());
                    }
                    state = &state->withValueIndex(*prop, *parsed);
                }
            }

            // 返回非拥有 wrap 的 BlockPermutation（state 由 BlockRegistry 全局拥有，与 Block.permutation
            // 属性及 setBlockPermutation unwrap 路径一致）。
            //
            // classId 经 classIdByName 运行时回查（而非闭包捕获注册期的 blockPermutationClassId）：
            // 引擎重建后 QuickJS 重新分配 classId（JS_NewClassID 按 runtime 递增），ScriptClassRegistry
            // 被新注册覆盖为新 classId，但旧 JS 模块对象（BlockPermutation ctor/resolve 闭包）若跨重建
            // 存活仍持有旧 classId。setBlockPermutation 等 unwrap 入参路径用 classIdByName 取最新 classId，
            // 若 resolve wrap 用捕获的旧 classId，对象 class_id 与 unwrap classId 失配 → JS_GetOpaque2 返回
            // nullptr → "first arg must be a BlockPermutation"。对齐 resolveItemStackClassId 的既定模式
            // （"引擎重建后 classId 变化，每次仍回查注册表校正"）。proto 仍用注册期捕获值（不影响 unwrap，
            // unwrap 只看对象 class_id 与 opaque；proto 仅决定方法/属性查找，旧 proto 跨重建仍存活）。
            const u64 resolveClassId = ScriptClassRegistry::instance().classIdByName("BlockPermutation");
            return ScriptObjectRegistry::wrap(ctx,
                resolveClassId,
                blockPermutationProto,
                const_cast<mc::BlockState*>(state),
                false,
                "BlockPermutation");
        },
        2);

    // --- ItemType类（@minecraft/server）---
    // 物品类型包装。opaque 持 const mc::Item*（非拥有，ItemRegistry 全局拥有）。
    // 官方 ItemType：id（资源位置）。Item::toString 返回 itemLocation().toString()，形如 "minecraft:diamond"。
    u64 itemTypeClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* itemTypeProto = builder.exportClass("ItemType", itemTypeClassId);
    ScriptClassRegistry::instance().registerClass(itemTypeClassId, itemTypeProto, "ItemType");

    ClassRegistrar<void> itemTypeReg(ctx, itemTypeClassId, itemTypeProto);
    itemTypeReg.readonlyProperty("id", [itemTypeClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* item = static_cast<const mc::Item*>(ScriptObjectRegistry::unwrap(ctx, thisVal, itemTypeClassId));
        if (item == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(item->toString());
    });

    // --- Block类（实化，原为空壳）---
    // 官方 Block：world 内某坐标处方块快照。opaque 持 ScriptBlockRef*（owned=true，JS GC 时 delete）。
    // 属性 typeId（方块资源位置）/permutation（BlockPermutation 包装 state）/x/y/z/location。
    // 底层 BlockState/Block/BlockRegistry 就绪。登记 ScriptClassRegistry 供跨回调 wrap/unwrap。
    u64 blockClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* blockProto = builder.exportClass("Block", blockClassId);
    ScriptClassRegistry::instance().registerClass(blockClassId, blockProto, "Block");

    ClassRegistrar<void> blockReg(ctx, blockClassId, blockProto);
    blockReg.readonlyProperty("typeId", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr || ref->state == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(ref->state->blockLocation().toString());
    });
    blockReg.readonlyProperty(
        "permutation", [blockClassId, blockPermutationProto](IScriptBindingContext& ctx, void* thisVal) -> void* {
            // 包装内嵌 state 为 BlockPermutation（非拥有，state 由 BlockRegistry 全局拥有）。
            // classId 运行时回查 classIdByName（引擎重建后 classId 失配，详见 BlockPermutation.type 处注释）。
            auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
            if (ref == nullptr || ref->state == nullptr) {
                return ctx.createUndefined();
            }
            const u64 permClassId = ScriptClassRegistry::instance().classIdByName("BlockPermutation");
            return ScriptObjectRegistry::wrap(ctx,
                permClassId,
                blockPermutationProto,
                const_cast<mc::BlockState*>(ref->state),
                false,
                "BlockPermutation");
        });
    blockReg.readonlyProperty("x", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(static_cast<i32>(ref->pos.x));
    });
    blockReg.readonlyProperty("y", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(static_cast<i32>(ref->pos.y));
    });
    blockReg.readonlyProperty("z", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(static_cast<i32>(ref->pos.z));
    });
    blockReg.readonlyProperty("location", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 官方 Block.location 返回 Vector3（浮点坐标，对齐 Entity.getLocation）。
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr) {
            return ctx.createUndefined();
        }
        void* obj = ctx.createObject();
        ctx.setPropertyFloat(obj, "x", static_cast<f64>(ref->pos.x));
        ctx.setPropertyFloat(obj, "y", static_cast<f64>(ref->pos.y));
        ctx.setPropertyFloat(obj, "z", static_cast<f64>(ref->pos.z));
        return obj;
    });
    blockReg.readonlyProperty("isWaterlogged", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 官方 Block.isWaterlogged：方块是否被水淹没。读 BlockState 流体状态非空判定。
        // TODO: 流体状态判定后续完善，暂保守返回 false。
        (void)thisVal;
        return ctx.createBoolean(false);
    });
    // --- Block 光照查询扩展（Cubium 专有，官方 @minecraft/server Block 无光照 API）---
    // 为集成测试直接读取光照数值而扩展，对齐 Java Level/World 的光照查询方法。
    // 经 ScriptBlockRef.world（IWorld*，test.getBlock 构造时回指）调 IWorld 光照虚方法。
    // IWorld::getBlockLight/getSkyLight/canSeeSky 主线程读 visible 侧 SWMRNibbleArray，atomic
    // acquire 无锁安全（GameTest 回调在服务端主线程 post-tick 执行）。
    // blockLight/skyLight：原始方块光/天空光等级（0-15），不含天气/时间衰减，可单独验证传播算法。
    // brightness：综合亮度（IWorld::getLight，含天空减暗），对齐 Java Level.getLight，用于作物生长等判定。
    // canSeeSky：是否露天（skyLight>=15，无天空光维度恒 false）。
    // 类型声明见各包 src/cubium-gametest-augment.d.ts 的 @minecraft/server module augmentation。
    blockReg.readonlyProperty("blockLight", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr || ref->world == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(static_cast<i32>(ref->world->getBlockLight(ref->pos)));
    });
    blockReg.readonlyProperty("skyLight", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr || ref->world == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(static_cast<i32>(ref->world->getSkyLight(ref->pos)));
    });
    blockReg.readonlyProperty("brightness", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 综合光照等级（含天空减暗），对齐 Java Level.getLight(BlockPos)。
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr || ref->world == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(static_cast<i32>(ref->world->getLight(ref->pos)));
    });
    blockReg.readonlyProperty("canSeeSky", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr || ref->world == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createBoolean(ref->world->canSeeSky(ref->pos));
    });

    // Block.beaconLevel（Cubium 专有扩展）：读信标方块实体的金字塔等级（0-4）。
    // 信标 _updateLevels 每 80 tick 检测金字塔并 setLevel。本属性经 ScriptBlockRef.world 回指的 IWorld
    // 调 getBlockEntity(pos) 取 BlockEntity，dynamic_cast 到 blockentity::BeaconEntity 后读 getLevel()。
    // 非信标方块（无 BlockEntity 或类型不匹配）返回 -1，便于测试区分"非信标"与"等级 0"。
    // 用途：集成测试验证信标金字塔等级检测（1/2/3/4 级）核心行为，对齐 wiki tech_信标.txt#激活。
    blockReg.readonlyProperty("beaconLevel", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr || ref->world == nullptr) {
            return ctx.createInt32(-1);
        }
        BlockEntity* entity = ref->world->getBlockEntity(ref->pos);
        if (entity == nullptr || entity->getType() != BlockEntityType::Beacon) {
            return ctx.createInt32(-1);
        }
        auto* beacon = static_cast<blockentity::BeaconEntity*>(entity);
        return ctx.createInt32(beacon->getLevel());
    });

    // --- ItemStack类 ---
    // opaque 持 mc::ItemStack*。EquippableComponent.getEquipment 以 owned=true 拷贝 wrap（new ItemStack(s)，
    // JS GC 时 delete），规避 setEquipment 改写装备数组致 owned=false 引用悬垂。typeId/amount 为只读快照。
    // 构造函数支持官方基岩 API：new ItemStack(typeId: string, amount?: number)，按 typeId 查 Item 构造
    // owned 拷贝（JS GC 时 delete），与 Container.getItem/Equippable.getEquipment 的 wrap 范式一致。
    u64 itemStackClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* itemStackProto = builder.exportClass("ItemStack",
        itemStackClassId,
        [](IScriptBindingContext& ctx, void* /*thisVal*/, i32 argc, void** args) -> void* {
            // new ItemStack(typeId: string, amount?: number = 1)
            if (argc < 1 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("new ItemStack(typeId: string, amount?: number)");
            }
            auto typeId = ctx.toString(args[0]);
            if (!typeId) {
                return ctx.throwInternalError("Failed to read typeId");
            }
            i32 amount = 1;
            if (argc >= 2 && ctx.isNumber(args[1])) {
                auto a = ctx.toInt32(args[1]);
                if (a && *a > 0) {
                    amount = *a;
                }
            }
            const mc::Item* item = mc::ItemRegistry::instance().getItem(mc::ResourceLocation::parse(*typeId));
            if (item == nullptr) {
                return ctx.throwTypeError(("Unknown item type: " + *typeId).c_str());
            }
            const u64 isClassId = resolveItemStackClassId();
            void* isProto = ScriptClassRegistry::instance().proto(isClassId);
            if (isProto == nullptr) {
                return ctx.createUndefined();
            }
            auto* stack = new mc::ItemStack(item, amount); // owned，JS GC 时 delete
            return ScriptObjectRegistry::wrap(ctx, isClassId, isProto, stack, true, "ItemStack");
        });
    ScriptClassRegistry::instance().registerClass(itemStackClassId, itemStackProto, "ItemStack");

    ClassRegistrar<void> itemStackReg(ctx, itemStackClassId, itemStackProto);
    itemStackReg.readonlyProperty("typeId", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
        if (stack == nullptr) {
            return ctx.createUndefined();
        }
        const mc::Item* item = stack->getItem();
        if (item == nullptr) {
            return ctx.createUndefined();
        }
        // Item::toString 返回 itemLocation().toString()，形如 "minecraft:diamond_sword"。
        return ctx.createString(item->toString());
    });
    itemStackReg.property(
        "amount",
        [](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
            if (stack == nullptr) {
                return ctx.createUndefined();
            }
            return ctx.createInt32(stack->getCount());
        },
        [](IScriptBindingContext& ctx, void* thisVal, void* value) {
            // amount setter：设数量（setCount 内部 <=0 置空）。仅当 owned（Equippable.getEquipment 拷贝）
            // 时安全写回；非拥有快照写入会被 C++ 侧覆盖，故非拥有时静默忽略留 TODO。
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
            if (stack == nullptr) {
                return;
            }
            auto v = ctx.toInt32(value);
            if (!v) {
                return;
            }
            stack->setCount(*v);
        });

    // --- ItemStack.nameTag（读写）---
    // 对齐基岩 @minecraft/server ItemStack.nameTag：自定义名（命名牌命名/铁砧改名后的显示名）。
    // getter 返回 getCustomName()（无自定义名返空串）。setter 调 setCustomName(string)，空串清除自定义名
    // （setCustomName 内部 name.empty() 置 m_customName=nullptr）。
    // 用途：测试需构造已命名命名牌（NameTagItem::itemInteractionForEntity 检查 hasCustomName）等场景，
    // 脚本侧此前无法给 ItemStack 设自定义名，导致命名牌命名生物测试无法构造已命名命名牌。
    // 仅当 owned（构造函数/Equippable.getEquipment 拷贝）时 setCustomName 安全写回对象；非拥有快照
    // 写入会被 C++ 侧覆盖，但 setCustomName 改的是 ItemStack 自身 m_customName（非外部引用），故无论
    // owned 与否写入均作用于 unwrap 出的 mc::ItemStack* 对象本身（与 amount setter 同语义）。
    itemStackReg.property(
        "nameTag",
        [](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
            if (stack == nullptr) {
                return ctx.createString("");
            }
            return ctx.createString(stack->getCustomName());
        },
        [](IScriptBindingContext& ctx, void* thisVal, void* value) {
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
            if (stack == nullptr) {
                return;
            }
            auto v = ctx.toString(value);
            if (!v) {
                return;
            }
            stack->setCustomName(*v);
        });

    // --- ItemStack.getEnchantments(): Enchantment[] ---
    // 对齐基岩 @minecraft/server ItemStack.getEnchantments。返回附魔对象数组，每项 { type, level }：
    //   - type：附魔 id（如 "minecraft:sharpness"，Enchantment::id()）
    //   - level：附魔等级（1-based）
    // 无附魔返回空数组。供命令测试（/enchant）与装备附魔查询判定附魔生效。读 ItemStack 附魔 NBT
    // （EnchantmentHelper::getEnchantments 解析 stack 的 Enchantments 标签）。
    itemStackReg.method(
        "getEnchantments",
        [](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
            if (stack == nullptr) {
                return ctx.createArray();
            }
            const auto enchantments = mc::item::enchant::EnchantmentHelper::getEnchantments(*stack);
            void* arr = ctx.createArray();
            u32 outIdx = 0;
            for (const auto& [ench, level] : enchantments) {
                if (ench == nullptr) {
                    continue;
                }
                void* obj = ctx.createObject();
                ctx.setPropertyString(obj, "type", ench->id());
                ctx.setPropertyInt(obj, "level", level);
                ctx.setArrayElement(arr, outIdx, obj); // 不消耗所有权
                ctx.releaseValue(obj);
                ++outIdx;
            }
            return arr;
        },
        0);

    // --- ItemStack.addEnchantment(enchantment: { type, level }): void ---
    // 给 ItemStack 添加一条附魔。基岩侧该方法挂在 ItemStack.getComponent("minecraft:enchantable")
    // 返回的组件对象上；Cubium 脚本侧未实现 minecraft:enchantable 组件派发（getComponent 仅派发
    // rideable/health/movement/equippable/onfire/inventory），故此处将 addEnchantment 直接挂在
    // ItemStack 类上（与既有 getEnchantments 同级），提供等价写入能力，供测试构造带附魔装备
    // （如火焰保护盔甲）绕过 /enchant 仅对玩家生效的限制。
    //
    // 参数 enchantment：{ type: string, level: number }。
    //   - type：附魔 id，接受 "minecraft:fire_protection" 或 "fire_protection"（无命名空间时补 minecraft:）。
    //   - level：附魔等级，整数 ≥1。基岩范围由各附魔 maxLevel 约束，超范围抛 EnchantmentLevelOutOfBoundsError；
    //     此处仅校验 ≥1（Cubium EnchantmentRegistry 未提供 maxLevel 查询，超范围不抛错，与基岩略有差异，
    //     TODO: 接入 Enchantment::getMaxLevel 后补范围校验）。
    //
    // id 存在性校验：EnchantmentRegistry::get(id) 查不到时 throwTypeError（对应基岩
    // EnchantmentTypeUnknownIdError），避免写入未注册的垃圾附魔 id。校验通过后调
    // ItemStack::addEnchantment(id, level) 写入附魔容器（EnchantmentContainer::set，同 id 覆盖等级）。
    //
    // 注意：基岩 addEnchantment 会做附魔兼容性校验（canAddEnchantment，与现有附魔冲突时抛
    // EnchantmentTypeNotCompatibleError），Cubium 未接入兼容性校验（火焰保护/水下呼吸等保护类互相兼容，
    // 测试场景不触发冲突；TODO: 需要时接入 Enchantment::checkCompatibility）。
    itemStackReg.method(
        "addEnchantment",
        [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
            if (stack == nullptr) {
                return ctx.createUndefined();
            }
            if (argc < 1 || !ctx.isObject(args[0])) {
                return ctx.throwTypeError("ItemStack.addEnchantment requires an enchantment object { type, level }");
            }
            void* enchObj = args[0];
            // type：字符串，无命名空间时补 minecraft:。
            void* typeVal = ctx.getProperty(enchObj, "type");
            if (!ctx.isString(typeVal)) {
                return ctx.throwTypeError("ItemStack.addEnchantment: enchantment.type must be a string");
            }
            auto typeOpt = ctx.toString(typeVal);
            if (!typeOpt.has_value()) {
                return ctx.throwTypeError("ItemStack.addEnchantment: enchantment.type must be a string");
            }
            std::string typeId = *typeOpt;
            if (typeId.find(':') == std::string::npos) {
                typeId = "minecraft:" + typeId;
            }
            // level：整数 ≥1。
            auto levelOpt = ctx.getPropertyInt(enchObj, "level");
            if (!levelOpt.has_value()) {
                return ctx.throwTypeError("ItemStack.addEnchantment: enchantment.level must be a number");
            }
            const i32 level = *levelOpt;
            if (level < 1) {
                return ctx.throwTypeError("ItemStack.addEnchantment: enchantment.level must be >= 1");
            }
            // id 存在性校验（对齐基岩 EnchantmentTypeUnknownIdError）。
            const mc::item::enchant::Enchantment* ench = mc::item::enchant::EnchantmentRegistry::get(typeId);
            if (ench == nullptr) {
                const std::string msg = "ItemStack.addEnchantment: unknown enchantment type '" + typeId + "'";
                return ctx.throwTypeError(msg.c_str());
            }
            stack->addEnchantment(typeId, level);
            return ctx.createUndefined();
        },
        1);

    // --- ItemStack.getComponent(componentId): object | undefined ---
    // 对齐基岩 @minecraft/server ItemStack.getComponent(componentId)：按组件 id 返回组件数据对象，
    // 组件不存在（物品不具备该组件）返 undefined。基岩原版 ItemStack.getComponent 支持 durability/
    // enchantable/food/cooldown 等多个 item 组件，Cubium 此前未实现 ItemStack.getComponent（仅 Entity 有），
    // 致所有耐久类测试只能用"数量不变"间接验证耐久损耗（盾牌/弓/打火石/锄/剪刀），无法直接断言耐久值下降。
    //
    // 本方法分发 minecraft:durability 分支，返回纯数据对象 { damage, maxDurability }：
    //   - damage：已损耗耐久（ItemStack::getDamage 即 m_damage，0 表示满耐久）。
    //   - maxDurability：最大耐久（ItemStack::getMaxDamage 转发 Item::maxDamage，如盾牌 336、三叉戟 250）。
    // 非可损坏物品（isDamageable()==false，maxDamage==0，如石头/木棍）返 undefined，对齐基岩"组件不存在
    // 返 undefined"语义（README:63 多处强调此约定）。
    //
    // 返回纯数据对象而非 wrap 组件类（区别于 Entity.getComponent 的 wrapComponent）：durability 是值快照，
    // 无需 ScriptClassRegistry 注册 DurabilityComponent、无需管理句柄生命周期，与 getEnchantments 返回对象
    // 同范式（createObject + setPropertyInt）。getDamage/getMaxDamage 是 const 只读，owned 拷贝
    // （Equippable.getEquipment）与非 owned 快照（Container.getItem）均安全。
    //
    // TODO: 其他 item componentId（enchantable/food/cooldown 等）按需补全——当前仅 durability 有测试需求。
    itemStackReg.method(
        "getComponent",
        [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
            if (stack == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createUndefined();
            }
            auto compId = ctx.toString(args[0]);
            if (!compId) {
                return ctx.createUndefined();
            }
            // normalize 前缀（对齐 Entity.getComponent:1328-1331 规范化语义）。
            std::string normalized = *compId;
            if (normalized.find(':') == std::string::npos) {
                normalized = "minecraft:" + normalized;
            }
            if (normalized == "minecraft:durability") {
                // 非可损坏物品返 undefined（对齐基岩"组件不存在返 undefined"）。
                if (!stack->isDamageable()) {
                    return ctx.createUndefined();
                }
                void* obj = ctx.createObject();
                ctx.setPropertyInt(obj, "damage", stack->getDamage());
                ctx.setPropertyInt(obj, "maxDurability", stack->getMaxDamage());
                return obj;
            }
            return ctx.createUndefined();
        },
        1);

    // --- Direction 枚举对象（@minecraft/server）---
    // 官方 Direction 是字符串枚举（Down="Down"...West="West"），非数字。导出为只读对象，
    // 各键值均为自身字符串名（对齐 system/world 全局对象导出模式）。接收 JS Direction 字符串时
    // 绑定层用 directionFromApiString 转 mc::Direction（见 ScriptTestHelper 等）。
    {
        void* directionObj = ctx.createObject();
        ctx.setPropertyString(directionObj, "Down", "Down");
        ctx.setPropertyString(directionObj, "East", "East");
        ctx.setPropertyString(directionObj, "North", "North");
        ctx.setPropertyString(directionObj, "South", "South");
        ctx.setPropertyString(directionObj, "Up", "Up");
        ctx.setPropertyString(directionObj, "West", "West");
        builder.exportValue("Direction", directionObj);
        ctx.releaseValue(directionObj);
    }

    // --- FluidType类（@minecraft/server）---
    // 流体类型包装。项目流体体系（Fluids namespace）尚未暴露完整查询入口，本类占位：
    // opaque 持 std::string*（owned=true，JS GC 时 delete，存资源位置 id 如 "minecraft:water"）。
    // 仅暴露 id 属性；完整 fluid state 体系后续 TODO。
    u64 fluidTypeClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* fluidTypeProto = builder.exportClass("FluidType", fluidTypeClassId);
    ScriptClassRegistry::instance().registerClass(fluidTypeClassId, fluidTypeProto, "FluidType");

    ClassRegistrar<void> fluidTypeReg(ctx, fluidTypeClassId, fluidTypeProto);
    fluidTypeReg.readonlyProperty("id", [fluidTypeClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* id = static_cast<std::string*>(ScriptObjectRegistry::unwrap(ctx, thisVal, fluidTypeClassId));
        if (id == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(*id);
    });

    // --- Container类（@minecraft/server）---
    // 容器包装。opaque 持 mc::IInventory*（非拥有，容器由 BlockEntity 或 Player 拥有）。
    // 官方 Container：size/emptySlotsCount + getItem(slot)/setItem(slot,itemStack)/addItem/transferItem/clearAll。
    // 底层 IInventory/ContainerBlockEntity::getInventory 就绪。getItem 返回 owned ItemStack 拷贝
    // （规避 setItem 改写致引用悬垂）；setItem 接 ItemStack unwrap（非拥有，仅拷贝写入容器）。
    u64 containerClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* containerProto = builder.exportClass("Container", containerClassId);
    ScriptClassRegistry::instance().registerClass(containerClassId, containerProto, "Container");

    ClassRegistrar<void> containerReg(ctx, containerClassId, containerProto);
    containerReg.readonlyProperty("size", [containerClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
        if (inv == nullptr) {
            return ctx.createInt32(0);
        }
        return ctx.createInt32(inv->getContainerSize());
    });
    containerReg.readonlyProperty(
        "emptySlotsCount", [containerClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            // 统计空槽位数（getItem(i).isEmpty()）。
            auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
            if (inv == nullptr) {
                return ctx.createInt32(0);
            }
            i32 empty = 0;
            for (i32 i = 0; i < inv->getContainerSize(); ++i) {
                if (inv->getItem(i).isEmpty()) {
                    ++empty;
                }
            }
            return ctx.createInt32(empty);
        });
    containerReg.method(
        "getItem",
        [containerClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
            if (inv == nullptr || argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.createUndefined();
            }
            auto slot = ctx.toInt32(args[0]);
            if (!slot || *slot < 0 || *slot >= inv->getContainerSize()) {
                return ctx.createUndefined();
            }
            // getItem 返回 owned 拷贝（new ItemStack(inv->getItem(slot))），JS GC 时 delete。
            const u64 isClassId = resolveItemStackClassId();
            void* isProto = ScriptClassRegistry::instance().proto(isClassId);
            if (isProto == nullptr) {
                return ctx.createUndefined();
            }
            auto* copy = new mc::ItemStack(inv->getItem(*slot));
            return ScriptObjectRegistry::wrap(ctx, isClassId, isProto, copy, true, "ItemStack");
        },
        1);
    containerReg.method(
        "setItem",
        [containerClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
            if (inv == nullptr || argc < 2 || !ctx.isNumber(args[0])) {
                return ctx.createUndefined();
            }
            auto slot = ctx.toInt32(args[0]);
            if (!slot || *slot < 0 || *slot >= inv->getContainerSize()) {
                return ctx.createUndefined();
            }
            // unwrap 入参 ItemStack（按 classId 校验类型）。
            const u64 isClassId = resolveItemStackClassId();
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, args[1], isClassId));
            if (stack == nullptr) {
                return ctx.throwTypeError("Container.setItem: argument must be an ItemStack");
            }
            inv->setItem(*slot, *stack);
            return ctx.createUndefined();
        },
        2);
    containerReg.method(
        "addItem",
        [containerClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
            if (inv == nullptr || argc < 1) {
                return ctx.createUndefined();
            }
            const u64 isClassId = resolveItemStackClassId();
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, args[0], isClassId));
            if (stack == nullptr) {
                return ctx.throwTypeError("Container.addItem: argument must be an ItemStack");
            }
            // addItem 返回剩余未放入的堆；官方无返回值，此处忽略剩余（对齐基岩 void 语义）。
            (void)inv->addItem(*stack);
            return ctx.createUndefined();
        },
        1);
    containerReg.method("clearAll",
        [containerClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
            if (inv != nullptr) {
                inv->clear();
            }
            return ctx.createUndefined();
        });
    containerReg.method(
        "transferItem",
        [containerClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            // TODO: transferItem(fromSlot,toContainer,toSlot) 跨容器迁移依赖第二个 Container 入参 unwrap，
            //       框架已具备（同 containerClassId），完整迁移语义（堆叠/空槽选择）后续补全。
            (void)thisVal;
            (void)argc;
            (void)args;
            return ctx.createUndefined();
        },
        3);

    // --- EntityInventoryComponent类（minecraft:inventory，Player 背包组件）---
    // opaque 持 mc::Entity*（owned=false，与 HealthComponent 等同范式）。getComponent("minecraft:inventory")
    // 已按 Player 过滤，此处 container 只读属性 dynamic_cast<Player*> 取 inventory()（PlayerInventory:
    // IInventory）包装为 Container 返回。Container opaque 持 IInventory*（非拥有，Player 拥有
    // m_inventory 成员），实体销毁时 ScriptHandleRegistry 经 entityId invalidate 组件句柄防 UAF
    // （与 onfire 等组件同机制）。
    // 官方 EntityInventoryComponent 仅一个只读属性 container: Container。
    u64 entityInventoryClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* entityInventoryProto = builder.exportClass("EntityInventoryComponent", entityInventoryClassId);
    ScriptClassRegistry::instance().registerClass(
        entityInventoryClassId, entityInventoryProto, "EntityInventoryComponent");

    ClassRegistrar<void> entityInventoryReg(ctx, entityInventoryClassId, entityInventoryProto);
    entityInventoryReg.readonlyProperty(
        "container", [entityInventoryClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityInventoryClassId));
            auto* player = dynamic_cast<mc::Player*>(ent);
            if (player == nullptr) {
                return ctx.createUndefined();
            }
            // resolve Container classId/proto（运行时按名查，与 Container.getItem resolveItemStackClassId 同范式）。
            const u64 containerClassIdResolved = ScriptClassRegistry::instance().classIdByName("Container");
            void* containerProto = ScriptClassRegistry::instance().proto(containerClassIdResolved);
            if (containerProto == nullptr) {
                return ctx.createUndefined();
            }
            // PlayerInventory : IInventory，取其地址作为 Container opaque 持有的 IInventory*（非拥有）。
            mc::IInventory* inv = &player->inventory();
            // 传 player->id() 登记 ScriptHandleRegistry：Container 句柄 owned=false 持 IInventory*（Player
            // m_inventory 成员地址），实体销毁时 invalidateAll 经同一 entityId 清空组件+Container 句柄防 UAF
            // （对齐 onfire 等组件 owned=false 句柄登记范式，见 ScriptHandleRegistry）。
            return ScriptObjectRegistry::wrap(
                ctx, containerClassIdResolved, containerProto, inv, false, "Container", nullptr, player->id());
        });

    // ====== 注册常量 ======

    builder.exportConst("GameModeSurvival", 0);
    builder.exportConst("GameModeCreative", 1);
    builder.exportConst("GameModeAdventure", 2);
    builder.exportConst("GameModeSpectator", 3);

    builder.exportConstString("MinecraftDimensionTypesOverworld", "minecraft:overworld");
    builder.exportConstString("MinecraftDimensionTypesNether", "minecraft:nether");
    builder.exportConstString("MinecraftDimensionTypesTheEnd", "minecraft:the_end");

    // ====== 导出全局对象 ======

    // 创建system全局对象
    void* systemObj = ScriptObjectRegistry::wrap(ctx, systemClassId, systemProto, nullptr, false, "System");
    builder.exportValue("system", systemObj);
    ctx.releaseValue(systemObj);

    // 创建world全局对象
    void* worldObj = ScriptObjectRegistry::wrap(ctx, worldClassId, worldProto, nullptr, false, "World");
    builder.exportValue("world", worldObj);

    // ====== 注册事件绑定 ======
    if (m_eventBus && !m_eventSignals.empty()) {
        registerEventBindings(ctx, worldObj, *m_eventBus, m_eventSignals);
    }

    // ====== 注册自定义组件绑定 ======
    registerCustomComponentBindings(builder);

    // ====== 完成模块注册 ======
    if (!builder.finalize()) {
        spdlog::error("[BedrockAddon] Failed to finalize @minecraft/server module");
        ctx.releaseValue(worldObj);
        return false;
    }

    ctx.releaseValue(worldObj);

    spdlog::info("[BedrockAddon] @minecraft/server module bindings registered successfully");
    return true;
}

} // namespace mc::mod::bedrock::addon
