#include "server/test/simulated/SimulatedPlayer.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "server/test/facade/GameTestHelper.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/inventory/PlayerInventory.hpp" // Player::inventory().add/setItem
#include "common/item/core/ItemStack.hpp"              // giveItem/setItem 参数
#include "common/util/math/MathUtils.hpp"              // toDegrees / toRadians / clamp
#include "server/world/ServerWorld.hpp"

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
    player->setPlayerId(0); // 模拟玩家无网络会话，PlayerId=0 占位（不经 PlayerManager 分配）
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
    // PlayerInventory::setItem 直接设槽（越界由 PlayerInventory 内部断言/钳制）。
    // selectSlot 当前忽略（TODO: 设 setSelectedSlot(slot)）。
    // TODO: 槽位越界校验返 false（当前 setItem 内部处理，恒返 true）。
    (void)selectSlot;
    inventory().setItem(slot, stack);
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
    return world.executeCommand(
        command, mc::math::Vector3d(x(), y(), z()), permLevel, mc::math::Vector2f(pitch(), yaw()));
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
    // TODO: 派发攻击事件 + 伤害计算（依赖攻击/伤害事件派发链）
    (void)target;
}

} // namespace mc::test
