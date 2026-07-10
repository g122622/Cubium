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

#pragma once

#include "DragonRespawnAnimation.hpp"
#include "IDragonBossBar.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/text/ITextComponentFwd.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/state/pattern/BlockPattern.hpp"
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc {

// 前向声明
class IWorld;
class DamageSource;
class Entity;

namespace entity {
class EnderCrystalEntity;
class EnderDragonEntity;
} // namespace entity

namespace test {
class EndDragonFightTestAccessor; // 测试访问器，声明为 friend 以访问 private 成员
} // namespace test

/**
 * @brief 末影龙战斗管理器
 *
 * 协调末影龙击杀后的奖励分发，包括龙蛋放置、末地折跃门生成和经验掉落区分。
 * 管理末影龙存活状态检测、旧存档状态扫描、末影龙 UUID 追踪和 Boss 栏同步。
 *
 * 生命周期：
 * - 由末地维度的 ServerWorld 持有
 * - 构造时从世界种子初始化折跃门列表，并创建 Boss 栏（默认 NullDragonBossBar）
 * - 服务端初始化时通过 setDragonBossBar() 注入 ServerDragonBossBar
 * - 可从存档数据恢复 previouslyKilled、dragonKilled 和 dragonUUID 状态
 * - 龙死亡时通过 setDragonKilled() 触发奖励逻辑并隐藏 Boss 栏
 * - 龙存活时每 tick 通过 updateDragon() 同步 Boss 栏血量/名称
 * - 每个游戏 tick 通过 tick() 更新状态（含旧存档状态扫描、Boss 栏可见性和玩家追踪）
 *
 * Boss 栏同步（对应 MC 1.21.11 EndDragonFight.dragonEvent）：
 * - tick() 中每 tick 设置 Boss 栏可见性 = !dragonKilled
 * - tick() 中每 20 tick 扫描附近玩家，更新 Boss 栏可见玩家列表
 * - updateDragon() 中同步龙血量百分比和名称到 Boss 栏
 * - setDragonKilled() 中设置百分比为 0 并隐藏 Boss 栏
 *
 * 对应 MC Java: EndDragonFight
 */
class EndDragonFight {
public:
    /**
     * @brief 战斗数据，用于存档保存/加载
     */
    struct Data {
        bool needsStateScanning = true;             ///< 是否需要扫描旧世界状态
        bool dragonKilled = false;                  ///< 龙当前是否已死
        bool previouslyKilled = false;              ///< 是否曾经击杀过龙
        bool isRespawning = false;                  ///< 是否正在重生（存档恢复时用于重启重生序列）
        std::optional<std::string> dragonUUID;      ///< 末影龙的 UUID（nullopt 表示无龙或未追踪）
        std::optional<BlockPos> exitPortalLocation; ///< 出口传送门位置（nullopt 表示未记录）
        std::optional<std::vector<i32>> gateways;   ///< 剩余折跃门索引列表（空=全部消耗，nullopt=首次初始化）

        /**
         * @brief 从 JSON 反序列化
         */
        static Data fromJson(const nlohmann::json& json);

        /**
         * @brief 序列化为 JSON
         */
        [[nodiscard]] nlohmann::json toJson() const;
    };

    // ========== 常量 ==========

    /// 折跃门总数
    static constexpr i32 GATEWAY_COUNT = 20;

    /// 折跃门距离原点的水平距离
    static constexpr i32 GATEWAY_DISTANCE = 96;

    /// 折跃门的 Y 坐标
    static constexpr i32 GATEWAY_Y = 75;

    /// 竞技场区块扫描半径（原点周围 -8 到 +8 区块）
    static constexpr i32 ARENA_CHUNK_RADIUS = 8;

    /// 玩家扫描间隔（tick），对应 MC Java TIME_BETWEEN_PLAYER_SCANS = 20
    static constexpr i32 TIME_BETWEEN_PLAYER_SCANS = 20;

    /// 末影水晶扫描间隔（tick），对应 MC Java TIME_BETWEEN_CRYSTAL_SCANS = 100
    /// 每 100 tick 扫描柱顶末影水晶数量，更新 crystalsAlive 计数
    static constexpr i32 TIME_BETWEEN_CRYSTAL_SCANS = 100;

    /// 龙失联后的重生检查阈值（tick），对应 MC Java MAX_TICKS_BEFORE_DRAGON_RESPAWN = 1200
    static constexpr i32 MAX_TICKS_BEFORE_DRAGON_RESPAWN = 1200;

    /// Boss 栏玩家追踪半径，对应 MC Java EndDragonFight.validPlayer 的 192.0
    static constexpr f32 PLAYER_TRACKING_RADIUS = 192.0f;

    /// 末影龙生成 Y 坐标，对应 MC Java EndDragonFight.DRAGON_SPAWN_Y
    /// 龙在末地原点 (0, DRAGON_SPAWN_Y, 0) 生成，位于出口讲台正上方 128 格高空
    static constexpr i32 DRAGON_SPAWN_Y = 128;

    // ========== 构造/析构 ==========

    /**
     * @brief 构造末影龙战斗管理器
     *
     * @param worldSeed 世界种子，用于初始化折跃门列表的随机顺序
     * @param data 存档数据，nullopt 表示新世界首次创建
     */
    explicit EndDragonFight(u64 worldSeed, const std::optional<Data>& data);

    ~EndDragonFight() = default;

    // 禁止拷贝
    EndDragonFight(const EndDragonFight&) = delete;
    EndDragonFight& operator=(const EndDragonFight&) = delete;

    // 允许移动
    EndDragonFight(EndDragonFight&&) noexcept = default;
    EndDragonFight& operator=(EndDragonFight&&) noexcept = default;

    // ========== 核心逻辑 ==========

    /**
     * @brief 每游戏 tick 调用
     *
     * 处理：
     * 1. 旧存档状态扫描（needsStateScanning=true 且竞技场区块已加载时）
     * 2. Boss 栏可见性更新（setVisible(!dragonKilled)）
     * 3. 每 TIME_BETWEEN_PLAYER_SCANS tick 扫描附近玩家，更新 Boss 栏可见玩家列表
     * 4. 龙失联检查（ticksSinceDragonSeen >= MAX_TICKS_BEFORE_DRAGON_RESPAWN 时尝试重新查找龙）
     *
     * 对应 MC Java: EndDragonFight.tick()
     *
     * @param world 末地世界引用
     */
    void tick(IWorld& world);

    /**
     * @brief 末影龙被击杀时调用
     *
     * 执行以下逻辑：
     * 1. 创建激活态出口传送门
     * 2. 生成一个末地折跃门（如果还有剩余）
     * 3. 首次击杀时在祭坛顶部放置龙蛋
     * 4. 设置 previouslyKilled = true, dragonKilled = true
     * 5. 清空末影龙 UUID
     * 6. Boss 栏：设置百分比为 0，隐藏（对应 MC dragonEvent.setProgress(0.0F).setVisible(false)）
     *
     * 对应 MC Java: EndDragonFight.setDragonKilled(EnderDragon)
     *
     * @param world 末地世界引用
     */
    void setDragonKilled(IWorld& world);

    /**
     * @brief 由末影龙每 tick 调用以同步 Boss 栏状态
     *
     * 当龙的 UUID 与 m_dragonUUID 匹配时：
     * 1. 设置 Boss 栏百分比为 health / maxHealth
     * 2. 重置 ticksSinceDragonSeen = 0
     * 3. 如果龙有自定义名称，设置 Boss 栏名称为龙的自定义名称
     *
     * 对应 MC Java: EndDragonFight.updateDragon(EnderDragon)
     *
     * @param dragon 末影龙实体
     */
    void updateDragon(entity::EnderDragonEntity& dragon);

    /**
     * @brief 尝试触发龙重生
     *
     * 当龙已死且未在重生中时，检查出口传送门四个水平方向 2 格外是否有末影水晶。
     * 如果四个方向都有水晶，启动重生序列。
     *
     * 对应 MC Java: EndDragonFight.tryRespawn()
     *
     * @param world 末地世界引用
     */
    void tryRespawn(IWorld& world);

    /**
     * @brief 设置重生阶段（用于阶段切换和测试跳转）
     *
     * 仅在重生序列进行中时可调用。如果新阶段为 END：
     * - 清除重生状态（respawnStage = nullptr）
     * - 设置 dragonKilled = false
     * - 创建新龙
     *
     * 对应 MC Java: EndDragonFight.setRespawnStage(DragonRespawnAnimation)
     *
     * @param world 末地世界引用（END 阶段创建新龙时使用）
     * @param stage 新的重生阶段
     */
    void setRespawnStage(IWorld& world, DragonRespawnAnimation stage);

    /**
     * @brief 末影水晶被破坏时调用
     *
     * 如果重生序列进行中且被破坏的水晶是重生水晶：
     * - 中止重生序列
     * - 重置柱顶水晶（清除无敌和光束）
     * - 重新生成激活态出口传送门
     * 否则：
     * - 更新 crystalsAlive 计数
     * - 如果存在末影龙，调用龙的 onCrystalDestroyed
     *
     * 对应 MC Java: EndDragonFight.onCrystalDestroyed(EndCrystal, DamageSource)
     *
     * @param world 末地世界引用
     * @param crystal 被破坏的水晶
     * @param source 伤害来源
     */
    void onCrystalDestroyed(IWorld& world, entity::EnderCrystalEntity* crystal, DamageSource& source);

    /**
     * @brief 重置柱顶水晶（清除无敌和光束）
     *
     * 遍历所有柱顶末影水晶，设置：
     * - setInvulnerable(false)
     * - setBeamTarget(null)
     *
     * 对应 MC Java: EndDragonFight.resetSpikeCrystals()
     *
     * @param world 末地世界引用
     */
    void resetSpikeCrystals(IWorld& world);

    /**
     * @brief 获取当前存活的末影水晶数量（柱顶）
     *
     * 由 tick() 每 TIME_BETWEEN_CRYSTAL_SCANS tick 自动更新。
     *
     * @return 柱顶末影水晶数量
     */
    [[nodiscard]] i32 crystalsAlive() const { return m_crystalsAlive; }

    /**
     * @brief 是否正在重生
     */
    [[nodiscard]] bool isRespawning() const { return m_respawnStage.has_value(); }

    /**
     * @brief 获取当前重生阶段
     *
     * @return 当前阶段，如果未在重生中返回 nullopt
     */
    [[nodiscard]] std::optional<DragonRespawnAnimation> respawnStage() const { return m_respawnStage; }

    /**
     * @brief 获取出口传送门位置
     */
    [[nodiscard]] const std::optional<BlockPos>& portalLocation() const { return m_portalLocation; }

    // ========== 状态查询 ==========

    /**
     * @brief 是否曾经击杀过龙
     *
     * 首次击杀时为 false，击杀后永久为 true。
     * 控制龙蛋放置和经验掉落数量。
     */
    [[nodiscard]] bool hasPreviouslyKilled() const { return m_previouslyKilled; }

    /**
     * @brief 龙当前是否已死
     */
    [[nodiscard]] bool isDragonKilled() const { return m_dragonKilled; }

    /**
     * @brief 获取剩余折跃门数量
     */
    [[nodiscard]] i32 remainingGatewayCount() const { return static_cast<i32>(m_gateways.size()); }

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] u64 worldSeed() const { return m_worldSeed; }

    /**
     * @brief 获取 Boss 栏引用（用于服务端注入和测试）
     *
     * 默认返回 NullDragonBossBar，服务端通过 setDragonBossBar() 注入真实实现。
     */
    [[nodiscard]] IDragonBossBar& dragonBossBar() { return *m_dragonBossBar; }
    [[nodiscard]] const IDragonBossBar& dragonBossBar() const { return *m_dragonBossBar; }

    // ========== Boss 栏注入 ==========

    /**
     * @brief 注入服务端 Boss 栏实现
     *
     * 由服务端在 EndDragonFight 构造后调用，注入 ServerDragonBossBar。
     * 传入 nullptr 时恢复为 NullDragonBossBar。
     *
     * @param bossBar Boss 栏实现（可为 nullptr）
     */
    void setDragonBossBar(std::unique_ptr<IDragonBossBar> bossBar);

    /**
     * @brief 创建默认的 Boss 栏名称
     *
     * 返回 "entity.minecraft.ender_dragon" 翻译键文本组件。
     * 服务端在创建 ServerDragonBossBar 时调用此方法获取初始名称。
     *
     * 对应 MC Java: Component.translatable("entity.minecraft.ender_dragon")
     */
    [[nodiscard]] static std::unique_ptr<text::ITextComponent> createDefaultBossName();

    // ========== 数据保存 ==========

    /**
     * @brief 保存战斗数据
     *
     * 用于维度存档时序列化战斗状态。
     */
    [[nodiscard]] Data saveData() const;

private:
    /**
     * @brief 从存档数据恢复状态
     */
    void _loadData(const Data& data);

    /**
     * @brief 扫描旧存档状态
     *
     * 检查出口传送门是否存在来推断 previouslyKilled：
     * - 如果存在活跃的出口传送门（END_PORTAL 方块），则 previouslyKilled = true
     * - 如果不存在活跃出口传送门，则 previouslyKilled = false，
     *   并检查是否存在讲台结构；如果不存在则创建非激活讲台
     *
     * 同时检查末影龙实体是否仍存活：
     * - 如果没有末影龙实体：dragonKilled = true
     * - 如果有末影龙实体：记录 dragonUUID，dragonKilled = false
     *   - 若同时无活跃传送门，则丢弃该龙（discard），因为无传送门的龙是无效状态
     * - 最终安全检查：如果 !previouslyKilled && dragonKilled，则 dragonKilled = false
     *
     * 对应 MC Java: EndDragonFight.scanState()
     *
     * @param world 末地世界引用
     */
    void _scanState(IWorld& world);

    /**
     * @brief 查找或创建末影龙
     *
     * 当龙失联（dragonUUID 为空或 ticksSinceDragonSeen 超阈值）时调用：
     * 1. 查找世界中已存在的末影龙实体（IWorld::getEntitiesByType(ENDER_DRAGON)）
     * 2. 若存在，记录其 UUID 到 m_dragonUUID（复用已有龙，避免重复生成）
     * 3. 若不存在，调用 _createNewDragon() 创建新龙
     *
     * 对应 MC Java: EndDragonFight.findOrCreateDragon()
     *
     * @param world 末地世界引用
     */
    void _findOrCreateDragon(IWorld& world);

    /**
     * @brief 创建新末影龙并加入世界
     *
     * 流程：
     * 1. 通过 EntityRegistry 获取末影龙 EntityType，调用工厂方法创建实例
     * 2. 设置生成位置 (0, DRAGON_SPAWN_Y, 0)，随机 yaw（0~360°），pitch=0
     * 3. 设置初始阶段为 HoldingPattern
     * 4. 通过 IWorld::spawnEntity 加入世界
     * 5. 记录新龙的 UUID 到 m_dragonUUID
     *
     * 对应 MC Java: EndDragonFight.createNewDragon()
     * 注意：MC 原版在生成前调用 level.getChunkAt() 强制加载生成位置区块，
     * Cubium 的调用方（tick）已通过 _isArenaLoaded() 保证区块加载，此处不再重复。
     *
     * @param world 末地世界引用
     * @return 新创建的末影龙实体指针（nullptr 表示创建失败）；返回的指针所有权属于世界，
     *         调用方不应释放。对齐 MC Java createNewDragon() 返回 @Nullable EnderDragon
     */
    Entity* _createNewDragon(IWorld& world);

    /**
     * @brief 检查是否存在活跃的出口传送门
     *
     * 扫描原点周围区块，查找 END_PORTAL 方块。
     * 活跃出口传送门表明龙曾被击杀过。
     *
     * @param world 末地世界引用
     * @return true 如果找到活跃出口传送门
     */
    [[nodiscard]] static bool _hasActiveExitPortal(IWorld& world);

    /**
     * @brief 查找出口传送门讲台结构
     *
     * 对应 MC Java: EndDragonFight.findExitPortal()
     *
     * 使用 exitPortalPattern（7×7×5 BlockPattern，匹配讲台基岩轮廓）在世界中
     * 精确定位出口讲台。采用两级搜索策略：
     *
     * 1. 扫描原点周围 ARENA_CHUNK_RADIUS 范围内所有 END_PORTAL 方块位置，
     *    对每个位置尝试模式匹配。MC 原版扫描 TheEndPortalBlockEntity 实例，
     *    Cubium 由于无此方块实体，改为扫描 END_PORTAL 方块状态。
     *
     * 2. 若策略 1 未找到，退化为原点高度图扫描：从 EndPodiumFeature 的原点
     *    位置 (0, 0, 0) 取 MOTION_BLOCKING 高度，从该高度向下逐格尝试模式匹配。
     *
     * 找到匹配后，通过 `match.getBlock(3, 3, 3).pos()` 获取讲台中心位置（即出口
     * 传送门正上方的基岩柱顶端位置）。若 m_portalLocation 为空，记录该位置。
     *
     * @param world 末地世界引用
     * @return 匹配结果（nullopt 表示未找到出口讲台）
     */
    [[nodiscard]] std::optional<blockpattern::BlockPatternMatch> _findExitPortal(IWorld& world);

    /**
     * @brief 构造出口传送门模式（exitPortalPattern）
     *
     * 对应 MC Java: EndDragonFight 构造函数中通过 BlockPatternBuilder 构建的
     * 7×7×5 讲台基岩轮廓模式。模式仅匹配讲台结构（基岩十字外环 + 中央基岩柱 +
     * 底座），不匹配末地石外圈和空气区域，确保精确识别完整讲台。
     *
     * 模式布局（5 层 aisle，每层 7×7）：
     * - 第 0..2 层：中央基岩柱（讲台中心 Y 方向 4 格高基岩柱的中下 3 格）
     * - 第 3 层：十字形基岩外环（PODIUM_RADIUS=4 的 7×7 十字轮廓）
     * - 第 4 层：5×5 中心讲台底座
     *
     * '#' 绑定到 `BlockInWorld::hasState(s.is(BEDROCK))` 谓词。
     *
     * @return 构造完成的 BlockPattern
     */
    [[nodiscard]] static std::unique_ptr<blockpattern::BlockPattern> _buildExitPortalPattern();

    /**
     * @brief 检查竞技场区块是否已全部加载
     *
     * 检查原点周围 ARENA_CHUNK_RADIUS 范围内的区块是否已加载，
     * 只有区块加载后才能进行状态扫描。
     *
     * @param world 末地世界引用
     * @return true 如果竞技场区块已加载
     */
    [[nodiscard]] static bool _isArenaLoaded(IWorld& world);

    /**
     * @brief 生成一个新的末地折跃门
     *
     * 从 gateways 列表中取出一个索引，按极坐标公式计算位置，
     * 在该位置放置折跃门结构。
     *
     * @param world 末地世界引用
     */
    void _spawnNewGateway(IWorld& world);

    /**
     * @brief 在指定位置生成折跃门结构
     *
     * @param world 末地世界引用
     * @param pos 折跃门底部中心位置
     */
    static void _spawnNewGatewayAt(IWorld& world, const BlockPos& pos);

    /**
     * @brief 在祭坛顶部放置龙蛋
     *
     * 在讲台中心位置的 MOTION_BLOCKING 高度图最高方块上方放置龙蛋方块。
     *
     * @param world 末地世界引用
     */
    static void _placeDragonEgg(IWorld& world);

    /**
     * @brief 更新 Boss 栏可见玩家列表
     *
     * 每 TIME_BETWEEN_PLAYER_SCANS tick 调用一次。
     * 扫描 PLAYER_TRACKING_RADIUS 半径内的玩家，添加到 Boss 栏可见列表，
     * 移除离开范围的玩家。
     *
     * 对应 MC Java: EndDragonFight.updatePlayers()
     *
     * @param world 末地世界引用
     */
    void _updatePlayers(IWorld& world);

    /**
     * @brief 更新柱顶末影水晶计数
     *
     * 遍历所有柱子，统计柱顶碰撞箱内的末影水晶数量。
     * 对应 MC Java: EndDragonFight.updateCrystalCount()
     *
     * @param world 末地世界引用
     */
    void _updateCrystalCount(IWorld& world);

    /**
     * @brief 启动龙重生序列
     *
     * 前置条件已满足（4 个水晶就位），执行：
     * 1. 将出口传送门区域的基岩/末地传送门方块替换为末地石（清除现有传送门）
     * 2. 设置 respawnStage = START
     * 3. 重置 respawnTime = 0
     * 4. 创建非激活态出口传送门
     * 5. 记录重生水晶列表
     *
     * 对应 MC Java: EndDragonFight.respawnDragon(List<EndCrystal>)
     *
     * @param world 末地世界引用
     * @param crystals 重生水晶列表
     */
    void _respawnDragon(IWorld& world, std::vector<entity::EnderCrystalEntity*> crystals);

    // ========== 成员变量 ==========

    u64 m_worldSeed;                  ///< 世界种子
    bool m_previouslyKilled = false;  ///< 是否曾经击杀过龙
    bool m_dragonKilled = false;      ///< 龙当前是否已死
    bool m_needsStateScanning = true; ///< 是否需要扫描旧世界状态
    std::string m_dragonUUID;         ///< 末影龙的 UUID（空字符串表示无龙或未追踪）
    std::vector<i32> m_gateways;      ///< 剩余折跃门索引列表（随机打乱）

    /// 出口传送门模式（7×7×5 讲台基岩轮廓），构造时构建，用于 _findExitPortal 精确定位讲台
    std::unique_ptr<blockpattern::BlockPattern> m_exitPortalPattern;

    // ========== 重生序列相关 ==========

    /// 当前重生阶段（nullopt 表示未在重生中）
    std::optional<DragonRespawnAnimation> m_respawnStage;

    /// 当前重生阶段已持续的 tick 数
    i32 m_respawnTime = 0;

    /// 重生水晶列表（玩家放置在出口传送门四周的 4 个水晶，所有权属于世界）
    std::vector<entity::EnderCrystalEntity*> m_respawnCrystals;

    /// 出口传送门位置（nullopt 表示未记录，首次扫描或重生时记录）
    std::optional<BlockPos> m_portalLocation;

    /// 柱顶存活末影水晶数量（由 _updateCrystalCount 每 100 tick 更新）
    i32 m_crystalsAlive = 0;

    // ========== Boss 栏相关 ==========

    /// Boss 栏（默认 NullDragonBossBar，服务端注入 ServerDragonBossBar）
    std::unique_ptr<IDragonBossBar> m_dragonBossBar = std::make_unique<NullDragonBossBar>();

    /// 距上次玩家扫描的 tick 数（对应 MC Java ticksSinceLastPlayerScan）
    i32 m_ticksSinceLastPlayerScan =
        TIME_BETWEEN_PLAYER_SCANS; ///< 初始为 TIME_BETWEEN_PLAYER_SCANS 以确保首次 tick 立即扫描

    /// 距上次看到龙的 tick 数（对应 MC Java ticksSinceDragonSeen）
    i32 m_ticksSinceDragonSeen = 0;

    /// 距上次末影水晶扫描的 tick 数（对应 MC Java ticksSinceCrystalsScanned）
    i32 m_ticksSinceCrystalsScanned = 0;

    friend class test::EndDragonFightTestAccessor;
    friend void dragon_respawn::tickStart(
        IWorld&, EndDragonFight&, std::vector<entity::EnderCrystalEntity*>&, i32, const BlockPos&);
    friend void dragon_respawn::tickPreparingToSummonPillars(
        IWorld&, EndDragonFight&, std::vector<entity::EnderCrystalEntity*>&, i32, const BlockPos&);
    friend void dragon_respawn::tickSummoningPillars(
        IWorld&, EndDragonFight&, std::vector<entity::EnderCrystalEntity*>&, i32, const BlockPos&);
    friend void dragon_respawn::tickSummoningDragon(
        IWorld&, EndDragonFight&, std::vector<entity::EnderCrystalEntity*>&, i32, const BlockPos&);
    friend void dragon_respawn::tickEnd(
        IWorld&, EndDragonFight&, std::vector<entity::EnderCrystalEntity*>&, i32, const BlockPos&);
};

} // namespace mc
