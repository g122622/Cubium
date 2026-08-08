#pragma once

#include "common/core/Types.hpp" // EntityInstanceId / GameMode / i32 / f32
#include "common/world/block/BlockPos.hpp"
#include "server/player/ServerPlayer.hpp"

#include <memory>
#include <string>

// 前向声明，避免循环 include：GameTestHelper 持 SimulatedPlayer*（spawn 时返回），
// SimulatedPlayer 持 IGameTestHelper*（回指用于坐标相对化与完成路径），两者互引故前向声明。
// 持接口 IGameTestHelper*（而非具体 GameTestHelper*）以便单元测试用 NullGameTestHelper 注入。
namespace mc::server {
class ServerWorld;
}
namespace mc::test {
class GameTestHelper;
class IGameTestHelper;
} // namespace mc::test

namespace mc::test {

/**
 * @brief 模拟玩家：GameTest 框架用的 ServerPlayer 子类（对齐基岩 SimulatedPlayer）。
 *
 * 与真实 ServerPlayer 的区别：
 * - 无网络连接（setConnection(nullptr)，所有发包路径安全 no-op）。
 * - 持非拥有 GameTestHelper* 回指，便于把绝对坐标转回结构相对坐标、触发完成路径。
 * - 提供 moveToLocation/lookAtEntity/chat 等 GameTest 专用便捷方法（基岩 SimulatedPlayer API 子集）。
 *
 * 关键限制：Player 不是 MobEntity（继承链 ServerPlayer→Player→LivingEntity→Entity，MobEntity 是
 * LivingEntity 的兄弟分支），故 MobEntity::lookAt / navigator() / AI goal 体系不可用。moveToLocation 用
 * handleMovementInput + 朝向目标的 yaw 计算手动驱动；lookAtEntity / lookAtLocation 用 setRotation
 * 直接定向。完整寻路（绕障、跳跃、门）待 PathNavigator 适配非 Mob 拥有者后实现（TODO）。
 *
 * 第一阶段实现最小可用集：spawn / moveToLocation(直线) / lookAtLocation / lookAtEntity / chat /
 * respawn / discard；fly / glide / swim / attack / interact / useItem / breakBlock / placeBlock 等 =
 * TODO stub（依赖红石 / 物品 / 交互体系）。
 *
 * 生命周期：经 spawn(helper, name, relativePos, gameMode) 静态工厂构造并注入世界，
 * 实例所有权由 ServerWorld 的 EntityManager 持有（spawnEntity 接管 unique_ptr）；
 * 调用方经返回的裸指针操作。回收经 discard()（静默移除，不掉落）或测试结束 killAllEntities。
 *
 * 门面纪律：SimulatedPlayer 作为 GameTestHelper::spawnSimulatedPlayer 的返回值对外可见，
 * 但其头仅供 facade / simulated / tests 内部使用，不对外暴露实现细节。
 */
class SimulatedPlayer : public mc::ServerPlayer {
public:
    /**
     * @brief 构造模拟玩家。
     *
     * @param id 实体实例 id（传 0，真实 id 由 EntityManager 在 spawnEntity 时分配）。
     * @param name 玩家名（用于 offline UUID 与日志）。
     * @param registry ECS 实体注册表句柄（透传给 ServerPlayer/Player/Entity 构造）。
     *
     * 注：构造后须依次 setPlayerId / setPosition / setServer / setWorld 再 spawnEntity，
     *     由 spawn() 工厂封装；外部不应直接构造。
     */
    SimulatedPlayer(mc::EntityInstanceId id, const std::string& name, ecs::EntityRegistry& registry);

    ~SimulatedPlayer() override = default;

    // 禁拷贝（Player 链已 delete 拷贝）；移动默认
    SimulatedPlayer(const SimulatedPlayer&) = delete;
    SimulatedPlayer& operator=(const SimulatedPlayer&) = delete;
    SimulatedPlayer(SimulatedPlayer&&) noexcept = default;
    SimulatedPlayer& operator=(SimulatedPlayer&&) noexcept = default;

    // === 回指 ===

    /**
     * @brief 绑定 GameTestHelper 回指（spawn 时由工厂设置）。
     *
     * 非拥有指针，helper 生命周期不短于 SimulatedPlayer（实例由 batch runner 拥有的 instance 内 helper 持有）。
     * 持接口 IGameTestHelper*，测试可注入 NullGameTestHelper。
     */
    void setHelper(IGameTestHelper& helper) noexcept { m_helper = &helper; }

    [[nodiscard]] IGameTestHelper* helper() const noexcept { return m_helper; }

    // === GameTest 便捷方法（最小可用集）===

    /**
     * @brief 朝目标方块位置行走一格（直线，单 tick 驱动一次 handleMovementInput）。
     *
     * 对齐基岩 SimulatedPlayer::moveToLocation：每 tick 调用一次，逐步逼近目标。
     * 当前实现：把 yaw 转向目标，调 handleMovementInput(1,0,false,false) 走一步；
     * 到达阈值内即停。不绕障——遇墙卡住（TODO: 接 PathNavigator 实现真实寻路）。
     *
     * @param relativePos 结构相对坐标（经 helper 变换为世界绝对坐标）。
     * @param speed 行走速度倍率（1.0=默认；当前阶段忽略，固定单步）。
     */
    void moveToLocation(BlockPos relativePos, f32 speed);

    /**
     * @brief 转头朝向某方块位置（设置 yaw / pitch + 头部旋转）。
     *
     * 对齐基岩 SimulatedPlayer::lookAtLocation。
     *
     * @param relativePos 结构相对坐标。
     */
    void lookAtLocation(BlockPos relativePos);

    /**
     * @brief 转头朝向某实体。
     *
     * 对齐基岩 SimulatedPlayer::lookAtEntity。Player 不是 MobEntity 故无 MobEntity::lookAt，
     * 此处用 setRotation 直接定向（瞬时，无插值；TODO: 加插值对齐基岩 deltaYaw / deltaPitch）。
     *
     * @param target 目标实体。
     */
    void lookAtEntity(const mc::Entity& target);

    /**
     * @brief 以该玩家身份执行命令（对齐基岩 SimulatedPlayer::chat 的命令变体）。
     *
     * 经 ServerWorld::executeCommand 在玩家位置、玩家权限等级执行。
     *
     * @param command 命令串（含或不含前导 / 均可）。
     * @return 命令返回值（>0=成功，0=失败/无权限）。
     */
    [[nodiscard]] i32 chat(const std::string& command);

    /**
     * @brief 重生玩家（重置生命 / 饥饿 / 经验，对齐基岩 SimulatedPlayer::respawn）。
     *
     * 调 Player::respawn()（非虚，仅重置状态）。不传送——完整流程需先 determineRespawnPosition，
     * 第一阶段 TODO: 接 ServerPlayer 重生位置确定。
     */
    void respawn();

    // === TODO stub（依赖未就绪体系）===

    // TODO: flyToLocation / glide / swim（飞行/滑翔/游泳物理，依赖 LivingEntity fall-flying 状态机细化）
    void flyToLocation(BlockPos relativePos, f32 speed);
    // TODO: attack(entity)（依赖攻击/伤害事件派发链）
    void attack(mc::Entity& target) override;
    // TODO: interact(entity/block)（依赖交互管理器 handleItemUseOn 完整化）
    // TODO: useItem(itemStack)（依赖物品使用派发）
    // TODO: breakBlock(pos)（依赖 destroyBlock + 战利品表，见 GameTestHelper::destroyBlock 同类 TODO）
    // TODO: placeBlock(pos, block)（依赖方块放置 + 玩家朝向）
    // TODO: jump / sprint 切换（jump 已有 Player::jump；sprint 切换待封装）

    // === 静态工厂 ===

    /**
     * @brief 在 GameTest 结构内生成一个 SimulatedPlayer。
     *
     * 封装构造→注入→spawn 全流程：构造实例 → setPlayerId(0) → 经 helper 变换相对坐标为世界坐标并
     * setPosition → setServer(world.server()) → setWorld(world) → setConnection(nullptr) → setGameMode →
     * spawnEntity → 绑定 helper 回指 → 返回裸指针（所有权归 ServerWorld::EntityManager）。
     *
     * @param helper 测试体门面（提供 world / origin / transform）。
     * @param name 玩家名。
     * @param relativePos 结构相对坐标（脚部位置）。
     * @param gameMode 初始游戏模式。
     * @return 生成的 SimulatedPlayer 裸指针（失败返回 nullptr）。
     */
    [[nodiscard]] static SimulatedPlayer* spawn(
        GameTestHelper& helper, const std::string& name, BlockPos relativePos, mc::GameMode gameMode);

private:
    IGameTestHelper* m_helper = nullptr; // 非拥有回指，spawn 时绑定
};

} // namespace mc::test
