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

#include "resource/ResourceLocation.hpp"
#include "util/math/random/Random.hpp"
#include "world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "world/blockentity/storage/DoubleSidedInventory.hpp"
#include <memory>

namespace mc {

class IWorld;
class ChestBlock;
class Player;

namespace loot {
class LootTableManager;
}

namespace blockentity {

/**
 * @brief 箱子方块实体
 *
 * 存储27格物品，支持：
 * - 单箱/双箱模式
 * - 打开计数和盖子动画
 * - 红石比较器信号
 * - 锁定功能
 * - 战利品表填充（继承自 LootableContainerBlockEntity）
 *
 * 参考: net.minecraft.tileentity.ChestTileEntity
 *
 * 盖子动画:
 * - m_lidAngle 从0.0到1.0表示打开程度
 * - 每tick更新，使用插值实现平滑动画
 *
 * 双箱合并:
 * - 当两个箱子相邻放置时自动合并
 * - 使用ChestType属性标识LEFT/RIGHT/SINGLE
 * - 打开时创建DoubleSidedInventory包装两个箱子
 *
 * 战利品表:
 * - 继承自 LootableContainerBlockEntity
 * - 结构生成时设置 lootTable 和 lootTableSeed
 * - 玩家首次打开时自动填充物品
 *
 * 开关计数:
 * - 参考 MC ContainerOpenersCounter，每 RECHECK_INTERVAL ticks 重新检查
 *   附近玩家是否仍在使用此容器，自动修正计数
 * - 音效在计数从0变为1（打开）或从1变为0（关闭）时触发
 * - 双箱音效在 RIGHT 箱子侧播放（位于双箱中心位置）
 * - CONTAINER_OPEN/CONTAINER_CLOSE 游戏事件在计数变化时触发
 */
class ChestEntity : public LootableContainerBlockEntity {
public:
    /// 箱子容量（27格）
    static constexpr i32 CHEST_SIZE = 27;

    /// 重新检查打开者的间隔（ticks），参考 MC ContainerOpenersCounter.CHECK_TICK_DELAY
    static constexpr i32 RECHECK_INTERVAL = 5;

    /// 客户端完整同步间隔（ticks），参考 MC 每 200 ticks 同步一次
    static constexpr i32 SYNC_INTERVAL = 200;

    /// 玩家访问最大距离（8格），参考 MC isUsableByPlayer 默认距离
    static constexpr f32 MAX_ACCESS_DISTANCE = 8.0f;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit ChestEntity(const BlockPos& pos);

    /**
     * @brief 构造函数（指定类型）
     * @param type 方块实体类型
     * @param pos 方块位置
     */
    ChestEntity(BlockEntityType type, const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~ChestEntity() override;

    // ========== 移动操作 ==========

    ChestEntity(ChestEntity&& other) noexcept;
    ChestEntity& operator=(ChestEntity&& other) noexcept;

    // 禁止拷贝
    ChestEntity(const ChestEntity&) = delete;
    ChestEntity& operator=(const ChestEntity&) = delete;

    // ========== IInventory 接口实现 ==========

    // 注：getInventory() 返回 SimpleInventory*，SimpleInventory 已通过
    // setLootUnpackCallback() 注入战利品表延迟填充回调，因此所有通过
    // getInventory()->getItem/setItem/removeItem/clear 等路径访问容器内容
    // 都会自动触发 _unpackLootTable(nullptr)。这与 MC Java 中
    // RandomizableContainerBlockEntity 的行为一致。
    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return CHEST_SIZE; }

    // ========== 箱子特有接口 ==========

    /**
     * @brief 检查是否是双箱的一部分
     * @param world 世界引用
     * @return 如果连接到另一个箱子返回true
     */
    [[nodiscard]] bool isDoubleChest(IWorld& world) const;

    /**
     * @brief 获取相邻箱子（如果是双箱）
     * @param world 世界引用
     * @return 相邻箱子实体指针，如果不是双箱返回nullptr
     */
    [[nodiscard]] ChestEntity* getConnectedChest(IWorld& world) const;

    /**
     * @brief 获取合并后的双箱背包
     * @param world 世界引用
     * @return 双箱背包，如果是单箱返回nullptr
     */
    [[nodiscard]] std::unique_ptr<DoubleSidedInventory> getDoubleInventory(IWorld& world);

    /**
     * @brief 获取打开计数
     * @return 当前打开的玩家数量
     */
    using ContainerBlockEntity::getOpenCount;

    /**
     * @brief 玩家打开箱子
     * @param player 打开箱子的玩家（可为nullptr）
     */
    void openContainer(Player* player) override;

    /**
     * @brief 玩家关闭箱子
     * @param player 关闭箱子的玩家（可为nullptr）
     */
    void closeContainer(Player* player) override;

    /**
     * @brief 计算红石比较器信号
     * @param world 世界引用
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getComparatorSignal(IWorld& world) const;

    // ========== 战利品表接口 ==========

    // 注：hasLootTable(), getLootTable(), getLootTableSeed(), setLootTable(), needsLootFill()
    // fillWithLoot() 继承自 LootableContainerBlockEntity，无需重写

    // ========== 动画支持 ==========

    /**
     * @brief 获取盖子打开角度
     * @return 角度 (0.0 = 关闭, 1.0 = 完全打开)
     */
    [[nodiscard]] f32 getLidAngle() const { return m_lidAngle; }

    /**
     * @brief 获取上一帧的盖子角度（用于插值）
     * @return 角度
     */
    [[nodiscard]] f32 getPrevLidAngle() const { return m_prevLidAngle; }

    /**
     * @brief 计算插值后的盖子角度
     * @param partialTick 部分tick时间
     * @return 插值后的角度
     */
    [[nodiscard]] f32 getInterpolatedLidAngle(f32 partialTick) const
    {
        return m_prevLidAngle + (m_lidAngle - m_prevLidAngle) * partialTick;
    }

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return true; }

    // ========== BlockEvent 同步 ==========

    /**
     * @brief 处理方块事件（客户端同步用）
     *
     * 服务端通过 IWorld::blockEvent() 发送事件，客户端收到后调用此方法。
     * 事件 id=1 用于箱子盖子开合动画：
     * - type > 0 表示打开，设置 lidAngle 为打开状态
     * - type == 0 表示关闭，设置 lidAngle 为关闭状态
     *
     * 参考: net.minecraft.tileentity.ChestTileEntity.triggerEvent
     *
     * @param id 事件ID（1=盖子动画）
     * @param type 事件参数（>0=打开, 0=关闭）
     * @return true 如果事件被处理
     */
    [[nodiscard]] bool triggerEvent(i32 id, i32 type) override;

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

    /**
     * @brief 从 NBT 加载（结构模板 / 客户端同步）
     *
     * 调用基类处理战利品表引用后，若无未解包的战利品表则加载容器物品列表。
     * LootTable 与 Items 互斥，与 MC Java RandomizableContainer 一致。
     */
    bool loadFromNBT(const nbt::CompoundTag& tag) override;

    /**
     * @brief 保存到 NBT（结构模板 / 客户端同步）
     *
     * 调用基类处理战利品表引用后，若无未解包的战利品表则保存容器物品列表。
     */
    void saveToNBT(nbt::CompoundTag& tag) const override;

    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    [[nodiscard]] std::string getDefaultName() const override { return "container.chest"; }

    /**
     * @brief 广播打开/关闭事件
     *
     * 通知客户端方块实体数据更新，并更新红石邻居和比较器。
     * 参考 MC ChestBlockEntity.signalOpenCount
     *
     * @param world 世界引用
     * @param open true=打开, false=关闭
     */
    void broadcastChestState(IWorld& world, bool open);

private:
    /**
     * @brief 播放开/关音效
     *
     * 参考 MC ChestBlockEntity.playSound：
     * - LEFT 箱子不播放音效（由 RIGHT 箱子统一播放）
     * - SINGLE 箱子在方块中心播放
     * - RIGHT 箱子音效位置向 LEFT 方向偏移 0.5 格（双箱中心）
     * - 音量 0.5，音调随机 0.9~1.0
     *
     * @param world 世界引用
     * @param open true=打开音效, false=关闭音效
     */
    void _playSound(IWorld& world, bool open);

    /**
     * @brief 重新检查打开者数量
     *
     * 参考 MC ContainerOpenersCounter.recheckOpeners：
     * 遍历附近玩家，检查哪些玩家仍在使用此容器，
     * 修正 m_openCount 使其与实际打开者数量一致。
     *
     * @param world 世界引用
     */
    void _recheckOpeners(IWorld& world);

    /**
     * @brief 检查玩家当前打开的容器菜单是否持有此箱子
     *
     * 参考 MC ChestBlockEntity.isOwnContainer：
     * - 单箱情况：ChestContainer 的底层容器直接是此箱子的 m_inventory
     * - 双箱情况：ChestContainer 的底层容器是 DoubleSidedInventory，
     *   通过 isPartOfLargeChest 检查此箱子是否是双箱的一部分
     *
     * @param player 要检查的玩家
     * @return 如果玩家打开的是此箱子（含双箱场景）返回 true
     */
    [[nodiscard]] bool _isOwnContainer(const Player& player) const;

    SimpleInventory m_inventory; ///< 27格物品存储
    f32 m_lidAngle = 0.0f;       ///< 当前盖子角度 (0-1)
    f32 m_prevLidAngle = 0.0f;   ///< 上一帧盖子角度
    i32 m_ticksSinceSync = 0;    ///< 同步计数器
};

} // namespace blockentity
} // namespace mc
