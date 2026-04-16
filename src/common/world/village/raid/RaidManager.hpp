#pragma once

#include "Raid.hpp"
#include "../../block/BlockPos.hpp"
#include "../../../core/Types.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace mc {
class IWorld;
class Player;

namespace world::village {
class Village;
class VillageManager;
}

namespace world::village::raid {

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
    RaidManager(RaidManager&&) = default;
    RaidManager& operator=(RaidManager&&) = default;

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
     * @brief 获取指定村庄的袭击。
     *
     * @param village 村庄指针。
     * @return 关联袭击；若不存在则返回 `nullptr`。
     */
    [[nodiscard]] Raid* getRaidForVillage(village::Village* village);

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
    void setBadOmenCheckCallback(BadOmenCheckCallback callback) {
        m_badOmenCheckCallback = std::move(callback);
    }

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
    [[nodiscard]] RaidId generateRaidId();

    /**
     * @brief 判断指定位置是否处于某次袭击范围内。
     *
     * @param pos 待检查位置。
     * @param center 袭击中心。
     * @return 是否命中范围。
     */
    [[nodiscard]] bool isWithinRaidRange(BlockPos pos, BlockPos center) const;

    /**
     * @brief 查找附近村庄。
     *
     * @param pos 参考位置。
     * @return 命中的村庄；若无则返回 `nullptr`。
     */
    [[nodiscard]] village::Village* findNearbyVillage(BlockPos pos) const;

    /**
     * @brief 判断指定位置是否允许开启袭击。
     *
     * @param pos 触发位置。
     * @return 是否允许开启袭击。
     */
    [[nodiscard]] bool canStartRaidAt(BlockPos pos) const;

private:
    IWorld& m_world;
    village::VillageManager& m_villageManager;
    std::vector<std::unique_ptr<Raid>> m_raids;
    RaidId m_nextRaidId = 1;
    BadOmenCheckCallback m_badOmenCheckCallback;
};

} // namespace world::village::raid
} // namespace mc
