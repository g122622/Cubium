#pragma once

#include "common/core/Types.hpp"        // EntityInstanceId / GameMode / Direction / i32 / f32
#include "common/util/math/Vector3.hpp" // Vector3（useItemOnBlock 的 faceLocation 参数）
#include "common/world/block/BlockPos.hpp"
#include "server/player/ServerPlayer.hpp"

#include <memory>
#include <string>

// 前向声明，避免循环 include：GameTestHelper 持 SimulatedPlayer*（spawn 时返回），
// SimulatedPlayer 持 IGameTestHelper*（回指用于坐标相对化与完成路径），两者互引故前向声明。
// 持接口 IGameTestHelper*（而非具体 GameTestHelper*）以便单元测试用 NullGameTestHelper 注入。
namespace mc {
class ItemStack;
} // namespace mc
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
     * @brief 转头朝向某方块位置（对齐基岩 SimulatedPlayer::lookAtBlock）。
     *
     * 语义等同 lookAtLocation（都接结构相对 BlockPos），duration 参数当前忽略（瞬时定向），
     * TODO: 加插值对齐基岩 LookDuration（Continuous/Instant/UntilMove）。
     *
     * @param relativePos 结构相对坐标。
     * @param duration 朝向持续时间语义（LookDuration），当前忽略。
     */
    void lookAtBlock(BlockPos relativePos);

    /**
     * @brief 朝目标方块位置行走（对齐基岩 SimulatedPlayer::moveToBlock）。
     *
     * 语义等同 moveToLocation（直线单步驱动）。options（maxStraightLineReach 等）当前忽略，TODO。
     *
     * @param relativePos 结构相对坐标。
     * @param speed 行走速度倍率（当前忽略，固定单步）。
     */
    void moveToBlock(BlockPos relativePos, f32 speed);

    /**
     * @brief 使模拟玩家跳跃（对齐基岩 SimulatedPlayer::jump）。
     *
     * 转发 Player::jump（地面 + 冷却为 0 才跳）。基岩返 bool 表示是否真的跳了，
     * 项目 Player::jump 返 void，此处返 true 占位（TODO: Player::jump 改返 bool 后回填真实判定）。
     *
     * @return 是否执行了跳跃（当前恒 true 占位）。
     */
    bool jump();

    /**
     * @brief 直接设置模拟玩家的饥饿值（Cubium 测试扩展，基岩 SimulatedPlayer 无此 API）。
     *
     * 转发 Player::foodStats().setFoodLevel(level)（内部 clamp 到 [0,20]）。用于食物类方块/物品
     * 集成测试控制进食前提（canEat 需 foodLevel<20）：生存模式玩家初始 foodLevel=20 满饥饿，
     * 无法通过命令/效果快速可靠降饥饿（生存模式无 OP 权限执行 /effect），故暴露此绑定让测试
     * 直接设定饥饿值，确定性验证「饥饿<20 才能吃」的进食行为。
     *
     * @param level 饥饿值（0-20，超范围由 FoodStats 钳制）。
     */
    void setFoodLevel(i32 level);

    /**
     * @brief 模拟玩家断开连接（对齐基岩 SimulatedPlayer::disconnect）。
     *
     * SimulatedPlayer 无网络连接，"断开"语义即从世界移除该实体。转发 Entity::discard（标记 m_removed，
     * 由 EntityManager tick 清理，不掉落）。对齐基岩 disconnect 触发的玩家离开流程。
     */
    void disconnect();

    /**
     * @brief 给模拟玩家物品（对齐基岩 SimulatedPlayer::giveItem）。
     *
     * 经 PlayerInventory::add 注入物品栈（尝试合并再放空槽）。selectSlot 当前忽略（TODO: 设选中槽）。
     *
     * @param stack 物品栈（按值传入，add 内部按需修改/拷贝）。
     * @param selectSlot 是否设为选中槽（当前忽略）。
     * @return 是否完全添加（add 返回剩余 0 即完全添加）。
     */
    bool giveItem(mc::ItemStack stack, bool selectSlot);

    /**
     * @brief 设模拟玩家指定槽位的物品（对齐基岩 SimulatedPlayer::setItem）。
     *
     * 经 PlayerInventory::setItem 直接设槽。selectSlot 当前忽略（TODO）。
     *
     * @param stack 物品栈。
     * @param slot 槽位索引（0..40）。
     * @param selectSlot 是否设为选中槽（当前忽略）。
     * @return 是否设置成功（当前恒 true，TODO: 槽位越界校验后回填）。
     */
    bool setItem(mc::ItemStack stack, i32 slot, bool selectSlot);

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
    // attack(entity) 已实现：转发 Player::attack(target) 走完整玩家攻击伤害链
    // （playerAttack source → target.hurt → actuallyHurt → setLastHurtBy → 群体仇恨触发）。
    void attack(mc::Entity& target) override;
    // TODO: interact(entity/block)（依赖交互管理器 handleItemUseOn 完整化）
    // TODO: breakBlock(pos)（依赖 destroyBlock + 战利品表，见 GameTestHelper::destroyBlock 同类 TODO）
    // TODO: placeBlock(pos, block)（依赖方块放置 + 玩家朝向）
    // TODO: jump / sprint 切换（jump 已有 Player::jump；sprint 切换待封装）

    // === 物品使用（对齐基岩 SimulatedPlayer::useItem/useItemOnBlock/useItemInSlot/useItemInSlotOnBlock）===
    // 经 ItemUseContext + Item::onItemUse/onItemRightClick 派发，支持骨粉/桶/锄头/斧头等所有非 block-item
    // 的"对方块使用物品"语义。消耗由 onItemUse 内部决定（如 BoneMealItem::onItemUse 调 shrink(1)）。

    /**
     * @brief 使用物品（右键空气，对齐 useItem）。
     *
     * 调 Item::onItemRightClick（vanilla Item.use）。官方语义：不消耗物品。空物品或物品在冷却中返 false。
     *
     * @param stack 要使用的物品（拷贝，onItemRightClick 不应修改权威物品栏）。
     * @return 物品是否被使用（onItemRightClick 非默认动作即视为使用）。
     */
    bool useItem(mc::ItemStack stack);

    /**
     * @brief 使用指定槽位的物品（右键空气，对齐 useItemInSlot）。
     *
     * 取玩家该槽位 ItemStack，转 useItem。槽位越界或空槽返 false。
     *
     * @param slot 物品栏槽位索引。
     * @return 物品是否被使用。
     */
    bool useItemInSlot(i32 slot);

    /**
     * @brief 对方块使用物品（对齐 useItemOnBlock）。
     *
     * 构造 ItemUseContext（player=this，stack 为传入物品）调 Item::onItemUse。onItemUse 内部按需
     * 消耗（骨粉 shrink(1)）。blockLocation 为结构相对坐标，内部经 worldBlockPosition 转世界坐标。
     * face 为击中面（默认 Up），faceLocation 为方块内相对击中点（0-1，默认方块中心 0.5）。
     *
     * @param stack 要使用的物品（拷贝传入；onItemUse 的消耗作用于该拷贝，调用方据此感知数量变化）。
     * @param blockLocation 结构相对方块坐标。
     * @param face 击中面（默认 Up）。
     * @param faceLocation 方块内相对击中点（默认中心）。
     * @return 物品是否被使用（onItemUse 返回 Success/Consume）。
     */
    bool useItemOnBlock(mc::ItemStack stack,
        BlockPos blockLocation,
        mc::Direction face = mc::Direction::Up,
        mc::Vector3 faceLocation = mc::Vector3(0.5f, 0.5f, 0.5f));

    /**
     * @brief 用指定槽位物品对方块使用（对齐 useItemInSlotOnBlock）。
     *
     * 取玩家该槽位 ItemStack，转 useItemOnBlock。成功消耗后回写该槽位（非创造模式）。槽位越界/空槽返 false。
     *
     * @param slot 物品栏槽位索引。
     * @param blockLocation 结构相对方块坐标。
     * @param face 击中面（默认 Up）。
     * @param faceLocation 方块内相对击中点（默认中心）。
     * @return 物品是否被使用。
     */
    bool useItemInSlotOnBlock(i32 slot,
        BlockPos blockLocation,
        mc::Direction face = mc::Direction::Up,
        mc::Vector3 faceLocation = mc::Vector3(0.5f, 0.5f, 0.5f));

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
