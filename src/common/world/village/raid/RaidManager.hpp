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

#include "command/ICommandSource.hpp"
#include "common/world/village/raid/RaiderType.hpp"
#include "core/Types.hpp"
#include "world/block/BlockPos.hpp"
#include "world/village/raid/Raid.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
class IWorld;
class Player;

namespace world::village {
class Village;
class VillageManager;
} // namespace world::village

namespace world::village::raid {

/**
 * @brief 袭击事件回调类型。
 *
 * 用于通知外部系统袭击状态变化。
 */
struct RaidCallbacks {
    /**
     * @brief 袭击开始回调。
     *
     * 当新袭击开始时调用，用于播放声音、发送消息等。
     * @param raid 开始的袭击
     * @param villageCenter 村庄中心位置
     */
    std::function<void(const Raid& raid, BlockPos villageCenter)> onRaidStarted;

    /**
     * @brief 袭击胜利回调。
     *
     * 当玩家成功防御袭击时调用。
     * @param raid 胜利的袭击
     * @param heroes 英雄玩家 UUID 列表
     * @param badOmenLevel 不祥之兆等级（用于计算英雄效果等级）
     */
    std::function<void(const Raid& raid, const std::vector<Uuid>& heroes, i32 badOmenLevel)> onRaidVictory;

    /**
     * @brief 袭击失败回调。
     *
     * 当掠夺者获胜时调用。
     * @param raid 失败的袭击
     */
    std::function<void(const Raid& raid)> onRaidLoss;

    /**
     * @brief 袭击波次开始回调。
     *
     * 当新波次开始时调用，用于播放号角声等。
     * @param raid 袭击
     * @param wave 波次编号
     * @param spawnPos 生成位置
     */
    std::function<void(const Raid& raid, i32 wave, BlockPos spawnPos)> onWaveStarted;
};

/**
 * @brief 袭击管理器。
 *
 * 负责世界范围内所有袭击的创建、更新、结束与查询。
 */
class RaidManager {
public:
    /**
     * @brief 不祥之兆检查回调类型。
     *
     * @param villageCenter 村庄中心位置。
     * @return 不祥之兆等级，0 表示没有可触发袭击的效果。
     */
    using BadOmenCheckCallback = std::function<i32(BlockPos villageCenter)>;

    /**
     * @brief 构造袭击管理器。
     *
     * @param world 关联世界。
     * @param villageManager 村庄管理器。
     */
    explicit RaidManager(IWorld& world, village::VillageManager& villageManager);

    ~RaidManager() = default;
    RaidManager(const RaidManager&) = delete;
    RaidManager& operator=(const RaidManager&) = delete;
    RaidManager(RaidManager&&) noexcept = default;
    RaidManager& operator=(RaidManager&&) = delete;

    /**
     * @brief 查询指定位置上的袭击。
     *
     * @param pos 查询位置。
     * @return 命中的袭击指针；若不存在则返回 `nullptr`。
     */
    [[nodiscard]] Raid* getRaidAt(BlockPos pos);

    /**
     * @brief 查询指定位置上的袭击。
     *
     * @param pos 查询位置。
     * @return 命中的只读袭击指针；若不存在则返回 `nullptr`。
     */
    [[nodiscard]] const Raid* getRaidAt(BlockPos pos) const;

    /**
     * @brief 判断指定位置是否存在进行中的袭击。
     *
     * @param pos 查询位置。
     * @return 是否存在进行中的袭击。
     */
    [[nodiscard]] bool hasRaidAt(BlockPos pos) const;

    /**
     * @brief 获取指定村庄的袭击（含所有状态）。
     *
     * @param village 村庄指针。
     * @return 关联袭击；若不存在则返回 `nullptr`。
     */
    [[nodiscard]] Raid* getRaidForVillage(village::Village* village);

    /**
     * @brief 获取指定村庄的进行中袭击。
     *
     * 仅返回状态为 Ongoing 的袭击，用于村庄袭击状态验证。
     *
     * @param village 村庄指针。
     * @return 关联的进行中袭击；若不存在则返回 `nullptr`。
     */
    [[nodiscard]] Raid* getOngoingRaidForVillage(village::Village* village);

    /**
     * @brief 获取全部袭击列表。
     *
     * @return 内部持有的袭击容器引用。
     *
     * @warning 调用方不得缓存其中元素地址跨越管理器的增删操作。
     */
    [[nodiscard]] const std::vector<std::unique_ptr<Raid>>& getAllRaids() const { return m_raids; }

    /**
     * @brief 获取仍在进行中的袭击数量。
     */
    [[nodiscard]] size_t getActiveRaidCount() const;

    /**
     * @brief 尝试在指定位置启动袭击。
     *
     * @param pos 触发位置。
     * @param badOmenLevel 不祥之兆等级。
     * @return 新建袭击指针；若无法启动则返回 `nullptr`。
     */
    [[nodiscard]] Raid* tryStartRaid(BlockPos pos, i32 badOmenLevel = 1);

    /**
     * @brief 处理玩家进入村庄事件。
     *
     * @param player 玩家实体。
     * @param village 玩家进入的村庄。
     *
     * @note 该接口依赖玩家效果系统；若相关系统未接入，则不会触发袭击。
     */
    void onPlayerEnterVillage(Player* player, village::Village* village);

    /**
     * @brief 使用外部回调检查并触发袭击。
     *
     * @param checkBadOmen 不祥之兆检查回调。
     * @param village 玩家进入的村庄。
     */
    void onPlayerEnterVillageWithCallback(const BadOmenCheckCallback& checkBadOmen, village::Village* village);

    /**
     * @brief 设置默认不祥之兆检查回调。
     *
     * @param callback 回调函数对象。
     */
    void setBadOmenCheckCallback(BadOmenCheckCallback callback) { m_badOmenCheckCallback = std::move(callback); }

    /**
     * @brief 设置袭击事件回调。
     *
     * @param callbacks 回调结构体。
     */
    void setCallbacks(RaidCallbacks callbacks) { m_callbacks = std::move(callbacks); }

    /**
     * @brief 获取袭击事件回调。
     *
     * @return 回调结构体引用。
     */
    [[nodiscard]] RaidCallbacks& callbacks() { return m_callbacks; }
    [[nodiscard]] const RaidCallbacks& callbacks() const { return m_callbacks; }

    /**
     * @brief 执行一次管理器 tick。
     *
     * @note 该方法应由世界主线程稳定调用。
     */
    void tick();

    /**
     * @brief 处理袭击结束事件。
     *
     * @param raid 已结束的袭击。
     */
    void onRaidEnd(Raid* raid);

    /**
     * @brief 清理已结束袭击。
     */
    void removeCompletedRaids();

private:
    /**
     * @brief 生成新的袭击 ID。
     */
    [[nodiscard]] RaidId _generateRaidId();

    /**
     * @brief 判断指定位置是否处于某次袭击范围内。
     *
     * @param pos 待检查位置。
     * @param center 袭击中心。
     * @return 是否命中范围。
     */
    [[nodiscard]] bool _isWithinRaidRange(BlockPos pos, BlockPos center) const;

    /**
     * @brief 查找附近村庄。
     *
     * @param pos 参考位置。
     * @return 命中的村庄；若无则返回 `nullptr`。
     */
    [[nodiscard]] village::Village* _findNearbyVillage(BlockPos pos) const;

    /**
     * @brief 判断指定位置是否允许开启袭击。
     *
     * @param pos 触发位置。
     * @return 是否允许开启袭击。
     */
    [[nodiscard]] bool _canStartRaidAt(BlockPos pos) const;

private:
    IWorld& m_world;
    village::VillageManager& m_villageManager;
    std::vector<std::unique_ptr<Raid>> m_raids;
    RaidId m_nextRaidId = 1;
    BadOmenCheckCallback m_badOmenCheckCallback;
    RaidCallbacks m_callbacks; ///< 袭击事件回调
};

} // namespace world::village::raid
} // namespace mc
