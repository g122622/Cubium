#include "server/test/simulated/SimulatedPlayer.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "server/test/facade/GameTestHelper.hpp"

#include "common/core/BlockRaycastResult.hpp" // BlockRaycastResult::hit（onBlockActivated 入参）
#include "common/entity/core/Entity.hpp"
#include "common/entity/inventory/PlayerInventory.hpp" // Player::inventory().add/setItem
#include "common/item/context/ItemUseContext.hpp"      // useItemOnBlock 构造 ItemUseContext 调 onItemUse
#include "common/item/core/ActionResult.hpp"           // ActionResultType / ItemActionResult
#include "common/item/core/BlockActionResult.hpp"      // BlockActionResult（onBlockActivated 返回值）
#include "common/item/core/Item.hpp"                   // Item::onItemUse/onItemRightClick
#include "common/item/core/ItemRegistry.hpp"           // ItemRegistry::getItem(itemId) 取非 const Item*
#include "common/item/core/ItemStack.hpp"              // giveItem/setItem 参数 + useItem 系列
#include "common/util/math/MathUtils.hpp"              // toDegrees / toRadians / clamp
#include "common/world/IWorld.hpp"                     // onItemRightClick / ItemUseContext 取 IWorld&
#include "common/world/block/Block.hpp"                // Block::getBlock（onBlockActivated fallback）
#include "common/world/block/BlockState.hpp"           // BlockState::blockId/isAir
#include "common/world/dimension/DimensionManager.hpp" // DimensionManager::OVERWORLD（spawn 维度注册）
#include "server/application/IServer.hpp"              // IServer::dimensionManager（ServerWorld::server 返回 IServer*）
#include "server/core/PlayerManager.hpp" // PlayerManager::nextPlayerId（SimulatedPlayer 分配全局唯一 PlayerId）
#include "server/dimension/ServerDimensionManager.hpp" // ServerDimensionManager::playerJoinDimension
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp" // ServerPlayerEntityManager::registerExistingPlayerEntity（打通命令系统实体解析）

#include <cmath>
#include <utility>

namespace mc::test {

SimulatedPlayer::SimulatedPlayer(mc::EntityInstanceId id, const std::string& name, ecs::EntityRegistry& registry)
    : mc::ServerPlayer(id, name, registry)
{}

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
    (void)speed; // TODO: speed 倍率需在 handleMovementInput/物理步长中体现（当前固定单步）

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

    // 向前走一步（直线，不绕障；TODO: 接 PathNavigator 实现真实寻路）
    handleMovementInput(1.0f, 0.0f, false, false);
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
    // Player::jump 返 void（地面 + 冷却为 0 才跳）。基岩返 bool 表示是否真跳，
    // 此处返 true 占位（调用方语义=已请求跳跃）。TODO: Player::jump 改返 bool 后回填真实判定。
    Player::jump();
    return true;
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
    // selectSlot 当前忽略（TODO: add 后定位该物所在槽并 setSelectedSlot）。
    (void)selectSlot;
    const i32 remaining = inventory().add(stack);
    return remaining == 0;
}

bool SimulatedPlayer::setItem(mc::ItemStack stack, i32 slot, bool selectSlot)
{
    // PlayerInventory::setItem 直接设槽（越界由 PlayerInventory 内部钳制）。
    // selectSlot 为 true 时同步选中该槽，使主手立即持有该物品（用于诱惑/喂食等手持判定）。
    // TODO: 槽位越界校验返 false（当前 setItem 内部处理，恒返 true）。
    inventory().setItem(slot, stack);
    if (selectSlot) {
        inventory().setSelectedSlot(slot);
    }
    return true;
}

i32 SimulatedPlayer::chat(const std::string& command)
{
    MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "SimulatedPlayer::chat: helper not bound");
    auto& world = static_cast<mc::server::ServerWorld&>(m_helper->world());
    // 在玩家位置、玩家权限等级执行命令（创造模式默认权限 2，对齐 OP 等级）
    const i32 permLevel = isCreative() ? 2 : 0;
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
    // TODO: 飞行物理（abilities.flying=true + travel 飞行分支），依赖 LivingEntity fall-flying 状态机细化
    (void)relativePos;
    (void)speed;
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

    // 派发优先级对齐项目网络层 ServerPlayHandler::handleBlockPlacementPacket 与 vanilla Java 1.21
    // MultiPlayerGameMode::useItemOn：先 Block.use（onBlockActivated），返回 Pass 才 fallback
    // Item.useOn（onItemUse）。此前实现只调 onItemUse，导致 onBlockActivated 类方块（堆肥桶堆肥/
    // 收获等）在 SimulatedPlayer 路径下无法触发，测试与生产行为分叉。补全 Block.use 前置分支
    // 使 SimulatedPlayer 与真实玩家网络路径行为一致。
    // hit 构造对齐 BlockInteractionManager::handleBlockUse:510。
    const mc::BlockRaycastResult hitResult = mc::BlockRaycastResult::hit(hitPos, worldPos, face, 0.0f);

    // 把传入 stack 设到主手选中槽作为权威手持物源。理由：onBlockActivated 通过
    // player.getHeldItem(hand) 读玩家手持物（非 useItemOnBlock 的 stack 形参），若不设入则读到空手
    // （堆肥桶堆肥 chance 检查失败返 Pass）。对齐 handleBlockUse:515-525 在调 onBlockActivated 前把
    // InventoryManager 手持物同步到 Player::m_inventory 的做法。设入后两路径消耗统一作用于该选中槽：
    //   - Block.use 路径：onBlockActivated 内部对 getHeldItem 引用直接 shrink（如堆肥桶堆肥成功 shrink 1）。
    //   - Item.useOn 路径：onItemUse 对 ItemUseContext 拷贝 shrink 不影响权威槽，由下方单独 shrink 补足。
    const i32 selectedSlot = inventory().getSelectedSlot();
    if (!stack.isEmpty()) {
        inventory().setItem(selectedSlot, stack);
    }

    // ① Block.use（onBlockActivated）：取目标方块 state 调其 onBlockActivated。空手右键也走此分支
    // （vanilla 空手不调 Item.useOn，但调 Block.use，如堆肥桶 level=8 空手收获骨粉）。
    const mc::BlockState* state = world.getBlockState(worldPos);
    if (state != nullptr && !state->isAir()) {
        mc::Block* block = mc::Block::getBlock(state->blockId());
        if (block != nullptr) {
            const mc::BlockActionResult blockResult =
                block->onBlockActivated(*state, world, worldPos, *this, mc::Hand::MainHand, hitResult);
            // Block.use 成功/消耗即短路返回，不再 fallback Item.useOn（避免双重消耗——如 ComposterBlock
            // 堆肥成功时 onBlockActivated 内部已 shrink 手持物作用于选中槽）。
            if (blockResult.isSuccess() || blockResult.isConsume()) {
                return true;
            }
            // Pass/Fail → fallback Item.useOn
        }
    }

    // ② Item.useOn（onItemUse）：仅手持非空物品时派发（vanilla 空手不调 Item.useOn）。
    // 骨粉催熟（BoneMealItem::onItemUse）等物品侧行为在此分支：CropBlock/CocoaBlock/MossBlock 未
    // override onBlockActivated（基类返 Pass），Block.use 前置分支放行后到此 fallback。
    if (!stack.isEmpty()) {
        const mc::Item* itemC = stack.getItem();
        if (itemC != nullptr) {
            mc::Item* item = mc::ItemRegistry::instance().getItem(itemC->itemId());
            if (item != nullptr) {
                // 构造上下文：player=this（供桶/打火石等需玩家上下文的物品），stack 为传入拷贝（onItemUse 内
                // 消耗如骨粉 shrink(1) 作用于该拷贝，不影响权威选中槽）。face/hand/yaw/pitch 对齐玩家点击语义。
                mc::ItemUseContext context(
                    world, this, stack, hitPos, worldPos, face, mc::Hand::MainHand, yaw(), pitch());
                const mc::ActionResultType result = item->onItemUse(context);
                if (result == mc::ActionResultType::Success || result == mc::ActionResultType::Consume) {
                    // 权威槽位消耗：对齐 BlockInteractionManager::handleItemUseOn:447-457「成功即对选中栈
                    // shrink(1) 回写」范式。onItemUse 对拷贝 shrink 不影响权威槽，故此处独立消耗一次。
                    // 创造模式不消耗（对齐 handleItemUseOn:458-460）。
                    if (!isCreative()) {
                        mc::ItemStack selected = inventory().getItem(selectedSlot);
                        if (!selected.isEmpty() && selected.getCount() > 0) {
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
