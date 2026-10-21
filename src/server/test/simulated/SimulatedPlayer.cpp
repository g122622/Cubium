#include "server/test/simulated/SimulatedPlayer.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "server/test/facade/GameTestHelper.hpp"

#include "common/core/BlockRaycastResult.hpp" // BlockRaycastResult::hit（方块侧交互入参）
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/inventory/PlayerInventory.hpp" // Player::inventory().add/setItem
#include "common/item/context/ItemUseContext.hpp"      // useItemOnBlock 构造 ItemUseContext 调 onItemUse
#include "common/item/core/ActionResult.hpp"           // ActionResultType / ItemActionResult
#include "common/item/core/BlockActionResult.hpp"      // BlockActionResult（useItemOn/useWithoutItem 返回值）
#include "common/item/core/Item.hpp"                   // Item::onItemUse/onItemRightClick
#include "common/item/core/ItemRegistry.hpp"           // ItemRegistry::getItem(itemId) 取非 const Item*
#include "common/item/core/ItemStack.hpp"              // giveItem/setItem 参数 + useItem 系列
#include "common/util/math/MathUtils.hpp"              // toDegrees / toRadians / clamp
#include "common/world/IWorld.hpp"                     // onItemRightClick / ItemUseContext 取 IWorld&
#include "common/world/block/Block.hpp"                // Block::getBlock（方块侧交互解析）
#include "common/world/block/BlockRegistry.hpp"        // BlockRegistry::getBlock（placeBlock 解析方块 id）
#include "common/world/block/BlockState.hpp"           // BlockState::blockId/isAir
#include "common/world/dimension/DimensionManager.hpp" // DimensionManager::OVERWORLD（spawn 维度注册）
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp" // PlayerManager::nextPlayerId（SimulatedPlayer 分配全局唯一 PlayerId）
#include "server/dimension/ServerDimensionManager.hpp"    // ServerDimensionManager::playerJoinDimension
#include "server/interaction/BlockInteractionManager.hpp" // IServer::dimensionManager（ServerWorld::server 返回 IServer*）
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp" // ServerPlayerEntityManager::registerExistingPlayerEntity（打通命令系统实体解析）

#include <array>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::test {

SimulatedPlayer::SimulatedPlayer(mc::EntityInstanceId id, const std::string& name, ecs::EntityRegistry& registry)
    : mc::ServerPlayer(id, name, registry)
{}

void SimulatedPlayer::onRemoval(RemovalReason reason)
{
    const auto* damage = lastDamageSource();
    const auto level = reason == RemovalReason::Killed ? spdlog::level::warn : spdlog::level::info;
    spdlog::log(level,
        "[GameTest] SimulatedPlayer removed: entity={} reason={} health={} position=({}, {}, {}) damage_type={} "
        "attacker={}",
        id(),
        static_cast<i32>(reason),
        health(),
        x(),
        y(),
        z(),
        damage != nullptr ? static_cast<i32>(damage->type()) : -1,
        lastDamageSourceTrueId());
    mc::ServerPlayer::onRemoval(reason);
}

// === 静态工厂 ===

SimulatedPlayer* SimulatedPlayer::spawn(
    GameTestHelper& helper, const std::string& name, BlockPos relativePos, mc::GameMode gameMode)
{
    // GameTestHelper 恒绑 ServerWorld（构造契约），world() 返 IWorld& 安全向下转 ServerWorld&
    auto& world = static_cast<mc::server::ServerWorld&>(helper.world());

    // ECS 迁移：SimulatedPlayer（经 ServerPlayer→Player→Entity）构造需要 registry 句柄，
    // 从绑定的 ServerWorld 取。
    auto* ecsRegistry = world.entityRegistry();
    MC_ASSERT_RELEASE(ecsRegistry != nullptr);
    auto player = std::make_unique<SimulatedPlayer>(0, name, *ecsRegistry);
    // 分配非 0 的全局唯一 PlayerId：从 PlayerManager 的计数器取（仅借号，不进 PlayerManager.m_players）。
    // 此前 PlayerId=0 占位导致命令系统（/tp、/gamemode、/effect、/give 等）与 PlayerResolver 经
    // PlayerId 解析时全部失效（TeleportManager.teleportPlayers 对 playerId==0 直接 continue）。
    // 改用非 0 PlayerId 并在 ServerPlayerEntityManager 注册映射后，命令层可经实体管理器解析到
    // SimulatedPlayer 实体直接操作。SimulatedPlayer 仍不进 PlayerManager（无网络会话，避免
    // keepalive/广播副作用），故 PlayerManager.getPlayer 仍查不到——命令层须改用 playerEntityManager。
    const mc::PlayerId simPlayerId = world.server()->playerManager().nextPlayerId();
    player->setPlayerId(simPlayerId);
    const BlockPos worldPos = helper.worldBlockPosition(relativePos);
    player->setPosition(
        static_cast<f32>(worldPos.x) + 0.5f, static_cast<f32>(worldPos.y), static_cast<f32>(worldPos.z) + 0.5f);
    player->setServer(world.server());
    player->setWorld(&world);
    // 接通物理引擎：SimulatedPlayer 经此工厂直接构造注入世界，不经过 MinecraftServer 的自然生成
    // 回调（MinecraftServer.cpp:1122-1124 仅在该回调里给实体 setPhysicsEngine），故须在此显式设置。
    // 否则 m_physicsEngine=nullptr，Player::updatePhysics（Player.cpp:1291-1307）会退化到无碰撞的
    // Entity::move（不调 updateFallDistance、不做碰撞检测），致 SimulatedPlayer 下落不累积
    // fallDistance（canSmashAttack 恒 false）、穿透方块、moveToLocation 无碰撞等系统性失效。
    // 与自然生成实体（MinecraftServer.cpp:1123）持相同 physicsEngine 引用。
    if (world.physicsEngine() != nullptr) {
        player->setPhysicsEngine(world.physicsEngine());
    }
    player->setConnection(nullptr); // 无头模拟，所有发包路径安全 no-op
    player->setGameMode(gameMode);
    player->setHelper(helper);

    SimulatedPlayer* raw = player.get();
    const auto id = world.spawnEntity(std::move(player));
    if (id == 0) {
        // spawnEntity 失败：unique_ptr 已被接管并销毁，raw 悬垂
        return nullptr;
    }
    // 注：真实 EntityInstanceId 由 EntityManager 在 spawnEntity 内重分配（构造传 0 仅占位），
    //     raw 指针仍有效（EntityManager 持有该对象）。

    // 在 ServerPlayerEntityManager 注册 PlayerId↔EntityInstanceId 映射，打通命令系统对
    // SimulatedPlayer 的实体解析（registerExistingPlayerEntity 仅建映射+EntityTracker，
    // 不向 PlayerManager 注册，无网络簿记副作用）。
    world.server()->playerEntityManager().registerExistingPlayerEntity(simPlayerId, id, world);

    // 缺陷D修复：注册到 dimensionManager 的 m_playerDimensions。
    // SimulatedPlayer spawn 于 helper 绑定的主世界（overworld），须显式 playerJoinDimension，
    // 否则 changeDimension→transferPlayerToDimension 内 playerLeaveDimension 查不到
    // （getPlayerDimension 返回 -1），_unloadPlayerChunks/_loadPlayerChunks 链路异常。
    // _loadPlayerChunks/_unloadPlayerChunks 对 PlayerManager 中不存在的 PlayerId 安全 no-op（双重保护）。
    world.server()->dimensionManager().playerJoinDimension(simPlayerId, mc::DimensionManager::OVERWORLD);
    return raw;
}

// === GameTest 便捷方法 ===

void SimulatedPlayer::moveToLocation(BlockPos relativePos, f32 speed)
{
    MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "SimulatedPlayer::moveToLocation: helper not bound");

    const BlockPos worldPos = m_helper->worldBlockPosition(relativePos);
    // 当前位置 → 目标水平向量
    const f64 dx = static_cast<f64>(worldPos.x) + 0.5 - static_cast<f64>(x());
    const f64 dz = static_cast<f64>(worldPos.z) + 0.5 - static_cast<f64>(z());
    const f64 horizDist = std::sqrt(dx * dx + dz * dz);

    // 到达阈值内即停（避免抖动）
    if (horizDist < 0.5) {
        return;
    }

    // 转向目标：MC yaw=0→+Z, yaw=90→-X，故 yaw = atan2(-dx, dz)
    const f32 yawDeg = mc::math::toDegrees(static_cast<f32>(std::atan2(-dx, dz)));
    setRotation(yawDeg, pitch());

    // 向前走一步（直线，不绕障；TODO: 接 PathNavigator 实现真实寻路）。
    // speed 作为输入缩放倍率：1.0=正常速度，<1 放慢逼近，<0 后退（对齐基岩 speed 参数语义）。
    handleMovementInput(1.0f, 0.0f, false, false, speed);
}

void SimulatedPlayer::lookAtLocation(BlockPos relativePos)
{
    MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "SimulatedPlayer::lookAtLocation: helper not bound");

    const BlockPos worldPos = m_helper->worldBlockPosition(relativePos);
    const f64 dx = static_cast<f64>(worldPos.x) + 0.5 - static_cast<f64>(x());
    const f64 dy = static_cast<f64>(worldPos.y) + 0.5 - getEyeY();
    const f64 dz = static_cast<f64>(worldPos.z) + 0.5 - static_cast<f64>(z());

    const f32 yawDeg = mc::math::toDegrees(static_cast<f32>(std::atan2(-dx, dz)));
    const f64 horizDist = std::sqrt(dx * dx + dz * dz);
    // pitch: 正值向下看，负值向上看；dy>0（目标在上方）应得负 pitch
    const f32 pitchDeg = -mc::math::toDegrees(static_cast<f32>(std::atan2(dy, horizDist)));
    setRotation(yawDeg, pitchDeg);
    // 同步头部旋转（LivingEntity 重写，写 m_rotationYawHead）
    setYHeadRot(yawDeg);
}

void SimulatedPlayer::lookAtEntity(const mc::Entity& target)
{
    MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "SimulatedPlayer::lookAtEntity: helper not bound");
    (void)m_helper; // 当前直接用绝对坐标算朝向，不经 helper 相对化

    const f64 dx = static_cast<f64>(target.x()) - static_cast<f64>(x());
    const f64 dy = target.getEyeY() - getEyeY();
    const f64 dz = static_cast<f64>(target.z()) - static_cast<f64>(z());

    const f32 yawDeg = mc::math::toDegrees(static_cast<f32>(std::atan2(-dx, dz)));
    const f64 horizDist = std::sqrt(dx * dx + dz * dz);
    const f32 pitchDeg = -mc::math::toDegrees(static_cast<f32>(std::atan2(dy, horizDist)));
    setRotation(yawDeg, pitchDeg);
    setYHeadRot(yawDeg);
    // TODO: 对齐基岩 lookAt 的 deltaYaw/deltaPitch 插值（当前为瞬时定向）
}

void SimulatedPlayer::lookAtBlock(BlockPos relativePos)
{
    // 语义等同 lookAtLocation（都接结构相对 BlockPos 朝向其中心）。duration 当前忽略（瞬时）。
    // TODO: 支持 LookDuration（Continuous/Instant/UntilMove）插值语义。
    lookAtLocation(relativePos);
}

void SimulatedPlayer::moveToBlock(BlockPos relativePos, f32 speed)
{
    // 语义等同 moveToLocation（直线单步驱动）。options 当前忽略。
    // TODO: 支持 MoveToOptions（maxStraightLineReach 等）。
    moveToLocation(relativePos, speed);
}

bool SimulatedPlayer::jump()
{
    // Player::jump 返回是否真的起跳（地面且跳跃冷却为 0），直接作为基岩 jump() 的布尔结果。
    return Player::jump();
}

void SimulatedPlayer::setFoodLevel(i32 level)
{
    // 转发 FoodStats::setFoodLevel（内部 clamp [0,20]）。测试扩展：让食物类测试直接设定饥饿值，
    // 绕过生存模式无 OP 权限无法 /effect 降饥饿的限制。见头文件注释。
    foodStats().setFoodLevel(level);
}

void SimulatedPlayer::disconnect()
{
    // SimulatedPlayer 无连接，"断开"= 从世界移除。转发 Entity::discard（标记 m_removed，不掉落）。
    // 对齐基岩 disconnect 触发的玩家离开流程（项目无连接管理，discard 足够）。
    discard();
}

bool SimulatedPlayer::giveItem(mc::ItemStack stack, bool selectSlot)
{
    // PlayerInventory::add 尝试合并再放空槽，返回剩余未添加数。剩余 0=完全添加。
    // 先在加入前记录各槽内容，加入后找出首个"内容变化"的槽：优先新增物品所在槽，
    // 其次是被合并增量的同类槽（对齐基岩 selectSlot 让玩家立即手持刚获得的物品）。
    std::array<mc::ItemStack, mc::PlayerInventory::TOTAL_SIZE> before{};
    if (selectSlot) {
        for (i32 i = 0; i < mc::PlayerInventory::TOTAL_SIZE; ++i) {
            before[static_cast<std::size_t>(i)] = inventory().getItem(i);
        }
    }
    // add 会就地 shrink 入参，物品指针须在调用前取出。
    const mc::Item* item = stack.getItem();

    const i32 remaining = inventory().add(stack);
    const bool fullyAdded = remaining == 0;

    if (selectSlot && item != nullptr) {
        i32 targetSlot = -1;
        for (i32 i = 0; i < mc::PlayerInventory::TOTAL_SIZE; ++i) {
            // getItem() 按值返回临时对象，必须绑定到值变量：绑定 const& 会悬垂，且后续
            // getItem() 的返回值落在同一临时存储上，比较结果被静默破坏。
            const mc::ItemStack now = inventory().getItem(i);
            const mc::ItemStack was = before[static_cast<std::size_t>(i)];
            if (now.isEmpty() || now.getItem() != item) {
                continue;
            }
            const bool wasEmpty = was.isEmpty();
            const bool countGrew = !wasEmpty && now.getCount() > was.getCount();
            if (wasEmpty || countGrew) {
                targetSlot = i;
                if (wasEmpty) {
                    break; // 新增槽优先于合并槽
                }
            }
        }
        if (targetSlot >= 0) {
            inventory().setSelectedSlot(std::min(targetSlot, static_cast<i32>(mc::PlayerInventory::HOTBAR_SIZE - 1)));
        }
    }
    return fullyAdded;
}

bool SimulatedPlayer::setItem(mc::ItemStack stack, i32 slot, bool selectSlot)
{
    // 槽位越界直接拒绝（基岩 setItem 返回是否成功，不静默钳制）。
    if (slot < 0 || slot >= mc::PlayerInventory::TOTAL_SIZE) {
        return false;
    }
    // selectSlot 为 true 时同步选中该槽，使主手立即持有该物品（用于诱惑/喂食等手持判定）。
    // 仅快捷栏槽位（0-8）可作为选中槽，其余槽位置上物品但无法手持。
    inventory().setItem(slot, stack);
    if (selectSlot && slot < mc::PlayerInventory::HOTBAR_SIZE) {
        inventory().setSelectedSlot(slot);
    }
    return true;
}

i32 SimulatedPlayer::chat(const std::string& command)
{
    MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "SimulatedPlayer::chat: helper not bound");
    auto& world = static_cast<mc::server::ServerWorld&>(m_helper->world());
    // SimulatedPlayer 是测试辅助实体，始终拥有最高 OP 权限等级 4（对齐 vanilla 单人开启作弊时玩家的
    // OP 等级，可执行包括 stop/save 在内的所有命令）。OP 权限等级与游戏模式在 vanilla 中相互独立
    // （survival 模式的 OP 玩家有权执行 /gamemode 切到 creative），此处解耦 permLevel 与游戏模式：
    // 此前 permLevel = isCreative() ? 2 : 0，把"创造模式"误等同于"OP 权限"，导致 survival 模式的
    // SimulatedPlayer permLevel=0 无法执行需权限 2 的 /gamemode（陷入"要切创造需权限、要权限需创造"
    // 的死循环），/difficulty、/attribute、/gamerule 等管理命令在 survival 模式下也全部被拒。
    // 改为固定 permLevel=4 后，任意游戏模式的 SimulatedPlayer 均可执行所有管理命令，符合测试辅助语义。
    const i32 permLevel = 4;
    // rotation 传模拟玩家自身朝向 (pitch, yaw)，对齐 vanilla 玩家执行命令时
    // CommandSourceStack.rotation 取实体朝向，使 `^` 局部坐标按玩家朝向解析。
    // 传 this（SimulatedPlayer 是 ServerPlayer 子类）作为命令源 player，使 ServerCommandSource
    // 的 isPlayer()=true，解锁需玩家源的命令（/tp <coords> 传自己、/effect give @s、/give @s 等）。
    // 此前传 nullptr player 致 source.isPlayer()=false，/tp <coords> 报 "must be a player" 失败。
    return world.executeCommand(
        command, mc::math::Vector3d(x(), y(), z()), permLevel, mc::math::Vector2f(pitch(), yaw()), this);
}

void SimulatedPlayer::respawn()
{
    // Player::respawn 非虚，仅重置生命/饥饿/经验；不传送。
    // TODO: 完整重生流程需先 determineRespawnPosition 并传送（依赖 ServerPlayer 重生位置确定）
    Player::respawn();
}

// === TODO stub ===

void SimulatedPlayer::flyToLocation(BlockPos relativePos, f32 speed)
{
    MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "SimulatedPlayer::flyToLocation: helper not bound");

    const BlockPos worldPos = m_helper->worldBlockPosition(relativePos);
    const f64 dx = static_cast<f64>(worldPos.x) + 0.5 - static_cast<f64>(x());
    const f64 dy = static_cast<f64>(worldPos.y) + 0.5 - static_cast<f64>(y());
    const f64 dz = static_cast<f64>(worldPos.z) + 0.5 - static_cast<f64>(z());
    const f64 horizDist = std::sqrt(dx * dx + dz * dz);

    // 到达阈值内即停（避免抖动）
    if (horizDist < 0.5 && std::abs(dy) < 0.5) {
        return;
    }

    // 飞行需要创造/旁观模式的飞行能力（对齐 vanilla Player.abilities.flying）。
    if (!canFly()) {
        return;
    }
    setFlying(true);

    // 水平飞行：转向目标后以 speed 为输入倍率前进（飞行分支在 _applyCachedMovementInput 中生效）。
    if (horizDist >= 0.5) {
        const f32 yawDeg = mc::math::toDegrees(static_cast<f32>(std::atan2(-dx, dz)));
        setRotation(yawDeg, pitch());
        handleMovementInput(1.0f, 0.0f, false, false, speed);
    }

    // 垂直飞行：朝目标高度升降（正向 speed 靠近，负向远离），经由飞行输入的上/下分量。
    if (std::abs(dy) >= 0.5) {
        const bool ascend = (dy > 0.0) == (speed >= 0.0f);
        handleMovementInput(horizDist >= 0.5 ? 1.0f : 0.0f, 0.0f, ascend, !ascend, std::abs(speed));
    }

    // 飞行不触发坠落伤害与跳跃消耗：落地判定由物理引擎在飞行分支外维持。
}

void SimulatedPlayer::attack(mc::Entity& target)
{
    // 委托 Player::attack：内部构造 DamageSources::playerAttack(this) → target.hurt →
    // LivingEntity::actuallyHurt → setLastHurtBy(this) → 下一 tick HurtByTargetGoal::shouldExecute
    // 读 getLastHurtBy() 触发群体仇恨（alertOthers 警醒附近同类）。
    // Player::attack 已含完整伤害链：暴击/附魔/冷却衰减/击退/横扫/武器损耗/饱食度消耗。
    // SimulatedPlayer 经 ServerPlayer→Player 继承，无网络连接时发包路径 no-op（setConnection(nullptr)）。
    // 直接调 Player::attack（非 ServerPlayer::attack）以跳过 ServerPlayer 的旁观者 setCamera 网络路径。
    Player::attack(target);
}

// === 方块破坏 / 放置 / 疾跑（对齐基岩 SimulatedPlayer::breakBlock/placeBlock/setSprint）===

bool SimulatedPlayer::breakBlock(BlockPos relativePos)
{
    MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "SimulatedPlayer::breakBlock: helper not bound");
    auto& world = static_cast<mc::server::ServerWorld&>(m_helper->world());
    const BlockPos worldPos = m_helper->worldBlockPosition(relativePos);

    // 走服务端权威破坏链路：与真实玩家挖掘结束后的处理一致（出生点保护、可破坏判定、
    // 掉落物、工具耐久、粒子/声音、方块实体移除），而非直接置 air。
    auto* server = world.server();
    if (server == nullptr) {
        return false;
    }
    const auto result =
        const_cast<mc::server::IServer*>(server)->blockInteractionManager().handleBlockBreak(playerId(), worldPos);
    return result.success() && result.value().blockBroken;
}

bool SimulatedPlayer::placeBlock(BlockPos relativePos, const std::string& block)
{
    MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "SimulatedPlayer::placeBlock: helper not bound");
    auto& world = static_cast<mc::server::ServerWorld&>(m_helper->world());
    const BlockPos worldPos = m_helper->worldBlockPosition(relativePos);

    const mc::Block* blockType = mc::BlockRegistry::instance().getBlock(mc::ResourceLocation(block));
    if (blockType == nullptr) {
        return false;
    }

    // 手持权威槽换成目标方块物品：放置链路从选中槽取物品并消耗。
    // 方块物品的注册 id 与方块 id 相同（见 BlockItemRegistry），经物品注册表反查。
    const mc::Item* blockItem = mc::ItemRegistry::instance().getItem(blockType->blockLocation());
    if (blockItem == nullptr) {
        return false;
    }
    const i32 selectedSlot = inventory().getSelectedSlot();
    inventory().setItem(selectedSlot, mc::ItemStack(blockItem, 1));

    // 放置以"下方支撑方块 + 命中其顶面"表达：击中面为 Up，放置位置即目标格。
    const mc::BlockState* supportState = world.getBlockState(worldPos.down());
    if (supportState == nullptr || supportState->isAir()) {
        return false;
    }
    const mc::BlockPos placementPos = worldPos;
    const mc::BlockState* before = world.getBlockState(placementPos);
    if (before != nullptr && !before->isAir() && !before->getBlock().isReplaceable()) {
        return false;
    }

    auto* server = world.server();
    if (server == nullptr) {
        return false;
    }
    const mc::Vector3 hitPos(
        static_cast<f32>(worldPos.x) + 0.5f, static_cast<f32>(worldPos.y), static_cast<f32>(worldPos.z) + 0.5f);
    const auto useResult = const_cast<mc::server::IServer*>(server)->blockInteractionManager().handleBlockUse(
        playerId(), worldPos.down(), mc::Hand::MainHand, hitPos, mc::Direction::Up);
    if (useResult.failed() || !useResult.value().success) {
        return false;
    }

    const mc::BlockState* after = world.getBlockState(placementPos);
    return after != nullptr && &after->getBlock() == blockType;
}

void SimulatedPlayer::setSprint(bool sprinting)
{
    setSprinting(sprinting);
}

// === 物品使用 ===
//
// 派发路径对齐 vanilla Item.useOn / Item.use（项目名 onItemUse / onItemRightClick）。
// useItem/useItemInSlot 走 onItemRightClick（右键空气，vanilla 不消耗物品）。
// useItemOnBlock/useItemInSlotOnBlock 走 onItemUse（右键方块，消耗由 onItemUse 内部决定，如骨粉 shrink(1)）。
// 坐标语义：方法接收结构相对 BlockPos，经 m_helper->worldBlockPosition 转世界绝对坐标。
// player 参数传 this（SimulatedPlayer 是 Player）：对齐 vanilla ItemUseContext 持 player，使桶/打火石等
// 需玩家上下文的物品可正确派发（BlockInteractionManager::handleItemUseOn 传 nullptr 是因其消耗由
// InventoryManager 统一管理；SimulatedPlayer 无该管理层，传 this 让 onItemUse 内部消耗作用于传入栈拷贝）。

bool SimulatedPlayer::useItem(mc::ItemStack stack)
{
    // 空物品不可使用。
    if (stack.isEmpty()) {
        return false;
    }
    const mc::Item* itemC = stack.getItem();
    if (itemC == nullptr) {
        return false;
    }
    // 冷却门控（对齐 MC Java 1.21.11 ServerPlayerGameMode.useItem:298-299）：
    // 物品在冷却中时直接返回不使用，不进入 onItemRightClick → setActiveHand。
    // 否则破盾（斧头 100 tick 冷却）、风弹、紫颂果等设的冷却形同虚设——玩家冷却期内
    // 仍可立即重新使用。真实玩家路径在 ServerPlayHandler::handleUseItemPacket 同补。
    if (hasItemCooldown(itemC)) {
        return false;
    }
    // Item 是无状态策略单例，onItemRightClick 非 const；经 ItemRegistry 取非 const 句柄调用。
    mc::Item* item = mc::ItemRegistry::instance().getItem(itemC->itemId());
    if (item == nullptr) {
        return false;
    }
    MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "SimulatedPlayer::useItem: helper not bound");
    mc::IWorld& world = m_helper->world();
    // vanilla Item.use 不消耗物品（onItemRightClick 返回的 ItemActionResult 的 stack 仅作结果传递）。
    // success 判定对齐 handleItemUseOn：Success || Consume 视为已使用。
    mc::ItemActionResult result = item->onItemRightClick(world, *this, mc::Hand::MainHand);
    return result.isSuccessOrConsume();
}

bool SimulatedPlayer::useItemInSlot(i32 slot)
{
    // 取该槽位物品拷贝（getItem 按值返回），转 useItem。useItem 不消耗，无需回写槽位。
    // 槽位越界由 PlayerInventory::getItem 内部钳制（返空堆），空堆在 useItem 内返 false。
    mc::ItemStack stack = inventory().getItem(slot);
    if (stack.isEmpty()) {
        return false;
    }
    return useItem(std::move(stack));
}

bool SimulatedPlayer::useItemOnBlock(
    mc::ItemStack stack, BlockPos blockLocation, mc::Direction face, mc::Vector3 faceLocation)
{
    MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "SimulatedPlayer::useItemOnBlock: helper not bound");
    mc::IWorld& world = m_helper->world();
    // 结构相对坐标 → 世界绝对坐标。
    const BlockPos worldPos = m_helper->worldBlockPosition(blockLocation);
    // hitPos = 方块原点 + faceLocation（faceLocation 是方块内 0-1 相对坐标，对齐官方
    // "Location relative to the bottom north-west corner of the block"）。
    const mc::Vector3 hitPos(static_cast<f32>(worldPos.x) + faceLocation.x,
        static_cast<f32>(worldPos.y) + faceLocation.y,
        static_cast<f32>(worldPos.z) + faceLocation.z);

    // 派发优先级对齐 vanilla Java 1.21.11 MultiPlayerGameMode::useItemOn / 项目网络层
    // BlockInteractionManager::handleBlockUse：
    //   Block::useItemOn（手持物参与）→ 返回 TryEmptyHandInteraction 才回退 Block::useWithoutItem
    //   → 仍未消费则 fallback Item::useOn（onItemUse）。
    // 此前实现只调 onItemUse，导致方块侧交互类行为（堆肥桶堆肥/蜡烛熄灭等）在 SimulatedPlayer
    // 路径下无法触发，测试与生产行为分叉。
    // hit 构造对齐 BlockInteractionManager::_dispatchBlockUse。
    const mc::BlockRaycastResult hitResult = mc::BlockRaycastResult::hit(hitPos, worldPos, face, 0.0f);

    // 与生产路径一致：方块侧交互统一从玩家主手权威槽（getSelectedStackRef）取手持物，方块对其
    // 的修改直接落到该槽。故先把传入 stack 设入选中槽作为权威手持物源。
    // 传空 stack 表示「空手右键」——必须显式清空选中槽，否则残留物品会参与 useItemOn，
    // 使本该空手的交互按手持物品派发（与调用方意图相反）。
    const i32 selectedSlot = inventory().getSelectedSlot();
    inventory().setItem(selectedSlot, stack);

    // ① Block::useItemOn / useWithoutItem：取目标方块 state 派发方块侧交互。
    // 空手右键也走 useItemOn（vanilla 空手同样调用 blockstate.useItemOn）。
    const mc::BlockState* state = world.getBlockState(worldPos);
    if (state != nullptr && !state->isAir()) {
        mc::Block* block = mc::Block::getBlock(state->blockId());
        if (block != nullptr) {
            mc::ItemStack& handStack = inventory().getSelectedStackRef();
            mc::BlockActionResult blockResult =
                block->useItemOn(handStack, *state, world, worldPos, *this, mc::Hand::MainHand, hitResult);
            if (!blockResult.consumesAction() && blockResult.isTryEmptyHandInteraction()) {
                blockResult = block->useWithoutItem(*state, world, worldPos, *this, hitResult);
            }
            // 方块侧交互消费动作即短路返回，不再 fallback Item.useOn（避免双重消耗——如 ComposterBlock
            // 堆肥成功时内部已 shrink 手持物作用于选中槽）。
            if (blockResult.consumesAction()) {
                return true;
            }
            // Pass/Fail → fallback Item.useOn
        }
    }

    // ② Item.useOn（onItemUse）：仅手持非空物品时派发（vanilla 空手不调 Item.useOn）。
    // 骨粉催熟（BoneMealItem::onItemUse）等物品侧行为在此分支：CropBlock/CocoaBlock/MossBlock 未
    // override 方块交互（基类返 TryEmptyHandInteraction→Pass），方块前置分支放行后到此 fallback。
    if (!stack.isEmpty()) {
        const mc::Item* itemC = stack.getItem();
        if (itemC != nullptr) {
            mc::Item* item = mc::ItemRegistry::instance().getItem(itemC->itemId());
            if (item != nullptr) {
                // 构造上下文：player=this（供桶/打火石等需玩家上下文的物品），stack 为传入拷贝（onItemUse 内
                // 消耗如骨粉 shrink(1) 作用于该拷贝，不影响权威选中槽）。face/hand/yaw/pitch 对齐玩家点击语义。
                // 记录 onItemUse 前权威槽 itemId：部分物品（FishBucketItem 鱼桶→空桶）在 onItemUse 内通过
                // player->getHeldItem 直接替换权威手持物（自管理消耗+返回新物品），此后外层不应再 shrink
                // （否则会把返回的空桶也消耗成空堆）。骨粉等"仅 shrink 原物品不替换"的物品 itemId 不变，
                // 仍走外层 shrink 补足（其 onItemUse 对拷贝 shrink 不回写权威槽）。
                // 记录 onItemUse 前权威槽 itemId+damage：部分物品在 onItemUse 内通过 player->getHeldItem
                // 直接改权威手持物——(1)自管理替换（鱼桶→空桶，itemId 变化）；(2)耐久损耗（打火石/锄/斧/锹
                // hurtAndBreak，damage 变化，itemId 不变）。两种情况外层都不应再 shrink（否则误消耗返回物或
                // 把耐久损耗误当数量消耗）。骨粉等"仅 shrink 原物品不替换不改耐久"的物品 itemId+damage 均不变，
                // 仍走外层 shrink 补足（其 onItemUse 对拷贝 shrink 不回写权威槽）。
                const u32 itemIdBefore = [&] {
                    const mc::ItemStack sel = inventory().getItem(selectedSlot);
                    return sel.isEmpty() ? 0u : sel.getItem()->itemId();
                }();
                const i32 damageBefore = [&] {
                    const mc::ItemStack sel = inventory().getItem(selectedSlot);
                    return sel.isEmpty() ? 0 : sel.getDamage();
                }();
                mc::ItemUseContext context(
                    world, this, stack, hitPos, worldPos, face, mc::Hand::MainHand, yaw(), pitch());
                const mc::ActionResultType result = item->onItemUse(context);
                if (result == mc::ActionResultType::Success || result == mc::ActionResultType::Consume) {
                    // 权威槽位消耗：对齐 BlockInteractionManager::handleItemUseOn:447-457「成功即对选中栈
                    // shrink(1) 回写」范式。onItemUse 对拷贝 shrink 不影响权威槽，故此处独立消耗一次。
                    // 创造模式不消耗（对齐 handleItemUseOn:458-460）。
                    // 若 onItemUse 已改变权威槽 itemId（自管理替换，如鱼桶→空桶）或 damage（耐久损耗，如打火石
                    // hurtAndBreak），跳过 shrink——物品已自行完成消耗，外层再 shrink 会误消耗返回物/把耐久
                    // 损耗误当数量消耗。
                    if (!isCreative()) {
                        mc::ItemStack selected = inventory().getItem(selectedSlot);
                        const u32 itemIdAfter = selected.isEmpty() ? 0u : selected.getItem()->itemId();
                        const i32 damageAfter = selected.isEmpty() ? 0 : selected.getDamage();
                        const bool selfManaged = (itemIdAfter != itemIdBefore) || (damageAfter != damageBefore);
                        if (!selfManaged && !selected.isEmpty() && selected.getCount() > 0) {
                            selected.shrink(1);
                            inventory().setItem(selectedSlot, selected);
                        }
                    }
                    return true;
                }
                return false;
            }
        }
    }

    return false;
}

bool SimulatedPlayer::useItemInSlotOnBlock(
    i32 slot, BlockPos blockLocation, mc::Direction face, mc::Vector3 faceLocation)
{
    // 取该槽位物品拷贝判定可用性。
    mc::ItemStack stack = inventory().getItem(slot);
    if (stack.isEmpty()) {
        return false;
    }
    // 把指定槽设为选中槽：useItemOnBlock 内部以选中槽为权威手持物源（设入 stack、消耗作用于选中槽）。
    // 对齐 handleBlockUse:519 把 InventoryManager 选中槽同步到 Player 的做法。
    inventory().setSelectedSlot(slot);
    // useItemOnBlock 已统一管理两路径（Block.use / Item.useOn）的消耗，此处不再单独 shrink，
    // 否则与 useItemOnBlock 内部消耗叠加导致双倍消耗。
    return useItemOnBlock(std::move(stack), blockLocation, face, faceLocation);
}

} // namespace mc::test
