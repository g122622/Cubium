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

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <memory>
#include <optional>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class LivingEntity;
class BlockPos;

namespace blockentity {

/**
 * @brief 可刷方块实体（可疑沙 / 可疑沙砾）
 *
 * 持有考古战利品表引用，玩家使用刷子刷扫 10 次后掉落物品并将方块
 * 转换为对应的普通方块（沙 / 沙砾）。
 *
 * 核心机制（对齐 MC 1.21.11 `BrushableBlockEntity`）：
 * - `brushCount`：累计刷扫次数，达到 `REQUIRED_BRUSHES_TO_BREAK (10)` 时完成。
 * - `coolDownEndsAtTick`：刷扫冷却结束 tick，每次成功刷扫后设置为 `gameTime + 10`，
 *   冷却期间 `brush()` 直接返回 false。
 * - `brushCountResetsAtTick`：刷扫计数重置 tick，每次 `brush()` 调用都设置为
 *   `gameTime + 40`，`checkReset()` 在该 tick 到达后将 `brushCount` 递减 2。
 * - `hitDirection`：首次刷扫命中的方向，用于物品掉落位置偏移；`brushCount` 归零时清空。
 * - `item`：缓存从战利品表生成的物品（`unpackLootTable()` 一次性生成）。
 * - `lootTable` / `lootTableSeed`：考古战利品表引用，`unpackLootTable()` 后置空。
 *
 * DUSTED 方块状态属性（0-3）由 `getCompletionState()` 决定：
 * - 0：brushCount == 0
 * - 1：brushCount < 3
 * - 2：brushCount < 6
 * - 3：brushCount >= 6
 *
 * 序列化：
 * - JSON（区块存档）：LootTable / LootTableSeed / item / hit_direction
 * - NBT（结构模板 / 客户端同步）：同上，键名一致
 *
 * 参考: net.minecraft.world.level.block.entity.BrushableBlockEntity
 */
class BrushableBlockEntity : public BlockEntity {
public:
    // ========== 常量 ==========

    /// 刷扫冷却 tick 数（两次成功刷扫之间的最小间隔）
    static constexpr i32 BRUSH_COOLDOWN_TICKS = 10;

    /// 刷扫计数重置 tick 数（无刷扫后多久开始递减 brushCount）
    static constexpr i32 BRUSH_RESET_TICKS = 40;

    /// 完成刷扫所需的累计刷扫次数
    static constexpr i32 REQUIRED_BRUSHES_TO_BREAK = 10;

    /// checkReset 中每次递减后的重置间隔（对齐 MC 1.21.11 的 4L）
    static constexpr i32 BRUSH_RESET_RETRY_TICKS = 4;

    /// 方块 tick 调度延迟（对齐 MC BrushableBlock.TICK_DELAY = 2）
    static constexpr i32 TICK_DELAY = 2;

    // ========== NBT 键名 ==========

    static constexpr const char* LOOT_TABLE_TAG = "LootTable";
    static constexpr const char* LOOT_TABLE_SEED_TAG = "LootTableSeed";
    static constexpr const char* HIT_DIRECTION_TAG = "hit_direction";
    static constexpr const char* ITEM_TAG = "item";

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit BrushableBlockEntity(const BlockPos& pos);

    ~BrushableBlockEntity() noexcept override = default;

    // ========== 核心刷扫逻辑 ==========

    /**
     * @brief 玩家刷扫方块
     *
     * 对齐 MC 1.21.11 `BrushableBlockEntity.brush`：
     * 1. 若 `hitDirection` 为空，记录首次命中的方向
     * 2. 更新 `brushCountResetsAtTick = gameTime + 40`
     * 3. 若在冷却期内（`gameTime < coolDownEndsAtTick`），返回 false
     * 4. 设置 `coolDownEndsAtTick = gameTime + 10`
     * 5. 调用 `unpackLootTable()` 一次性生成物品
     * 6. `++brushCount`，若 `>= 10` 则调用 `brushingCompleted()` 并返回 true
     * 7. 否则调度 2 tick 后的方块 tick，并按需更新 DUSTED 方块状态
     *
     * @param gameTime 当前游戏 tick
     * @param world 世界引用
     * @param entity 刷扫的实体（通常是玩家）
     * @param direction 命中方向
     * @param stack 刷子物品堆（用于战利品表 TOOL 参数）
     * @return true 表示刷扫完成（刷出物品并转换方块），false 表示仍在进行中
     */
    bool brush(i64 gameTime, IWorld& world, LivingEntity& entity, Direction direction, ItemStack& stack);

    /**
     * @brief 检查并重置刷扫计数
     *
     * 对齐 MC 1.21.11 `BrushableBlockEntity.checkReset`：
     * - 当 `brushCount != 0` 且 `gameTime >= brushCountResetsAtTick` 时，
     *   `brushCount = max(0, brushCount - 2)`，并更新 DUSTED 状态，
     *   然后设置 `brushCountResetsAtTick = gameTime + 4`
     * - 若 `brushCount == 0`，清空 `hitDirection` / `brushCountResetsAtTick` / `coolDownEndsAtTick`
     * - 否则调度 2 tick 后的方块 tick 以便再次检查
     *
     * 由 `BrushableBlock::tick()` 在方块计划刻时调用。
     *
     * @param world 世界引用
     */
    void checkReset(IWorld& world);

    // ========== 战利品表接口 ==========

    /**
     * @brief 设置考古战利品表
     *
     * 由结构生成器（如 DesertPyramidStructure）在放置可疑沙时调用。
     * 物品在首次 `brush()` 时通过 `unpackLootTable()` 延迟生成。
     *
     * @param lootTable 战利品表资源位置
     * @param seed 随机种子
     */
    void setLootTable(const ResourceLocation& lootTable, i64 seed);

    // ========== 访问器 ==========

    /**
     * @brief 获取缓存的物品（若未 unpack 则为空）
     *
     * 注意：返回的是拷贝，修改不会影响方块实体内部状态。
     */
    [[nodiscard]] ItemStack getItem() const { return m_item; }

    /**
     * @brief 获取命中方向
     * @return 命中方向，若未被刷过返回 std::nullopt
     */
    [[nodiscard]] std::optional<Direction> getHitDirection() const { return m_hitDirection; }

    /**
     * @brief 获取当前完成度对应的 DUSTED 状态值
     *
     * - 0：brushCount == 0
     * - 1：brushCount < 3
     * - 2：brushCount < 6
     * - 3：brushCount >= 6
     *
     * @return DUSTED 属性值（0-3）
     */
    [[nodiscard]] i32 getCompletionState() const;

    /**
     * @brief 获取累计刷扫次数
     */
    [[nodiscard]] i32 getBrushCount() const noexcept { return m_brushCount; }

    // ========== BlockEntity 重写 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return false; }
    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    bool loadFromNBT(const nbt::tags::compound_tag& tag) override;
    void saveToNBT(nbt::tags::compound_tag& tag) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    // ========== 私有辅助 ==========

    /**
     * @brief 从战利品表生成物品
     *
     * 对齐 MC 1.21.11 `BrushableBlockEntity.unpackLootTable`：
     * - 使用 ARCHAEOLOGY 参数集（本项目复用 chest 参数集 + BLOCK_POS）
     * - 设置 TOOL（刷子）、THIS_ENTITY（玩家）、BLOCK_ENTITY 参数
     * - 使用 `lootTableSeed` 作为随机种子
     * - 取生成列表的第一个物品（空列表则 m_item 为空）
     * - 生成后清空 `lootTable` 标记，防止重复生成
     *
     * @param world 世界引用
     * @param entity 刷扫实体
     * @param tool 刷子物品堆
     */
    void unpackLootTable(IWorld& world, LivingEntity& entity, ItemStack& tool);

    /**
     * @brief 刷扫完成处理
     *
     * 对齐 MC 1.21.11 `BrushableBlockEntity.brushingCompleted`：
     * 1. 调用 `dropContent()` 掉落物品
     * 2. 触发 `BRUSH_BLOCK_COMPLETE (3008)` 世界事件
     * 3. 将方块替换为 `BrushableBlock::getTurnsInto()` 的默认状态
     *
     * @param world 世界引用
     * @param entity 刷扫实体
     * @param tool 刷子物品堆
     */
    void brushingCompleted(IWorld& world, LivingEntity& entity, ItemStack& tool);

    /**
     * @brief 掉落缓存的物品
     *
     * 对齐 MC 1.21.11 `BrushableBlockEntity.dropContent`：
     * - 先调用 `unpackLootTable()` 确保物品已生成
     * - 在命中方向相邻位置生成 ItemEntity
     * - 物品数量为 `split(random.nextInt(21) + 10)`（10-30 个）
     * - 生成后清空 `m_item`
     *
     * @param world 世界引用
     * @param entity 刷扫实体
     * @param tool 刷子物品堆
     */
    void dropContent(IWorld& world, LivingEntity& entity, ItemStack& tool);

    /**
     * @brief 调度方块 tick
     *
     * 对齐 MC `level.scheduleTick(pos, block, 2)`。
     * 仅在服务端调度，客户端不调度。
     *
     * @param world 世界引用
     */
    void scheduleBlockTick(IWorld& world);

    // ========== 成员变量 ==========

    i32 m_brushCount = 0;                    ///< 累计刷扫次数
    i64 m_brushCountResetsAtTick = 0;        ///< 刷扫计数重置 tick
    i64 m_coolDownEndsAtTick = 0;            ///< 刷扫冷却结束 tick
    ItemStack m_item;                        ///< 缓存的考古物品（unpack 后填充）
    std::optional<Direction> m_hitDirection; ///< 首次刷扫命中方向
    ResourceLocation m_lootTable;            ///< 战利品表资源位置
    i64 m_lootTableSeed = 0;                 ///< 战利品表随机种子
    bool m_hasLootTable = false;             ///< 是否有待生成的战利品表
};

} // namespace blockentity
} // namespace mc
