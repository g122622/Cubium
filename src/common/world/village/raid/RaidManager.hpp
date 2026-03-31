#pragma once

#include "Raid.hpp"
#include "RaiderType.hpp"
#include "../../block/BlockPos.hpp"
#include "../../../core/Types.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>

namespace mc {

// 前向声明
class IWorld;
class Player;

namespace world {
namespace village {
class Village;
class VillageManager;
}
}

namespace world {
namespace village {
namespace raid {

/**
 * @brief 袭击管理器
 *
 * 管理所有袭击事件的创建、更新和销毁。
 * 处理不祥之兆效果触发袭击。
 *
 * 参考 MC 1.16.5 RaidManager
 */
class RaidManager {
public:
    /**
     * @brief 玩家袭击检测回调类型
     * 参数：BlockPos 村庄中心
     * 返回：如果不祥之兆等级 > 0，返回等级；否则返回 0
     */
    using BadOmenCheckCallback = std::function<i32(BlockPos)>;

    /**
     * @brief 构造函数
     * @param world 关联的世界
     * @param villageManager 村庄管理器引用
     */
    explicit RaidManager(IWorld& world, village::VillageManager& villageManager);

    ~RaidManager() = default;

    // 禁止拷贝
    RaidManager(const RaidManager&) = delete;
    RaidManager& operator=(const RaidManager&) = delete;

    // 允许移动
    RaidManager(RaidManager&&) = default;
    RaidManager& operator=(RaidManager&&) = default;

    // ========== 袭击查询 ==========

    /**
     * @brief 获取指定位置的袭击
     * @param pos 位置
     * @return 袭击指针，如果没有袭击返回 nullptr
     */
    [[nodiscard]] Raid* getRaidAt(BlockPos pos);

    /**
     * @brief 获取指定位置的袭击（const版本）
     */
    [[nodiscard]] const Raid* getRaidAt(BlockPos pos) const;

    /**
     * @brief 检查指定位置是否有袭击
     */
    [[nodiscard]] bool hasRaidAt(BlockPos pos) const;

    /**
     * @brief 获取村庄的袭击
     * @param village 村庄指针
     * @return 袭击指针，如果没有袭击返回 nullptr
     */
    [[nodiscard]] Raid* getRaidForVillage(village::Village* village);

    /**
     * @brief 获取所有袭击
     */
    [[nodiscard]] const std::vector<std::unique_ptr<Raid>>& getAllRaids() const { return m_raids; }

    /**
     * @brief 获取活跃袭击数量
     */
    [[nodiscard]] size_t getActiveRaidCount() const;

    // ========== 袭击创建 ==========

    /**
     * @brief 尝试开始袭击
     * @param pos 触发位置
     * @param badOmenLevel 不祥之兆等级
     * @return 创建的袭击，如果无法创建返回 nullptr
     */
    [[nodiscard]] Raid* tryStartRaid(BlockPos pos, i32 badOmenLevel = 1);

    /**
     * @brief 玩家进入村庄时检查袭击（Player实体版本）
     * @param player 玩家实体
     * @param village 村庄
     */
    void onPlayerEnterVillage(Player* player, village::Village* village);

    /**
     * @brief 使用回调检查玩家进入村庄
     * @param checkBadOmen 检查不祥之兆的回调，返回等级（0表示没有）
     * @param village 村庄
     */
    void onPlayerEnterVillageWithCallback(const BadOmenCheckCallback& checkBadOmen, village::Village* village);

    /**
     * @brief 设置不祥之兆检查回调
     * @param callback 回调函数
     */
    void setBadOmenCheckCallback(BadOmenCheckCallback callback) {
        m_badOmenCheckCallback = std::move(callback);
    }

    // ========== 更新 ==========

    /**
     * @brief 每tick更新
     */
    void tick();

    // ========== 袭击结束 ==========

    /**
     * @brief 袭击结束时调用
     * @param raid 结束的袭击
     */
    void onRaidEnd(Raid* raid);

    /**
     * @brief 移除已结束的袭击
     */
    void removeCompletedRaids();

private:
    /**
     * @brief 生成唯一袭击ID
     */
    [[nodiscard]] RaidId generateRaidId();

    /**
     * @brief 检查位置是否在袭击范围内
     * @param pos 要检查的位置
     * @param center 袭击中心
     * @return 是否在范围内
     */
    [[nodiscard]] bool isWithinRaidRange(BlockPos pos, BlockPos center) const;

    /**
     * @brief 查找附近的村庄
     * @param pos 位置
     * @return 村庄指针，如果没有找到返回 nullptr
     */
    [[nodiscard]] village::Village* findNearbyVillage(BlockPos pos) const;

    /**
     * @brief 检查位置是否适合开始袭击
     */
    [[nodiscard]] bool canStartRaidAt(BlockPos pos) const;

private:
    IWorld& m_world;                            ///< 关联的世界
    village::VillageManager& m_villageManager;  ///< 村庄管理器引用
    std::vector<std::unique_ptr<Raid>> m_raids; ///< 所有袭击
    RaidId m_nextRaidId = 1;                    ///< 下一个袭击ID
    BadOmenCheckCallback m_badOmenCheckCallback; ///< 不祥之兆检查回调
};

} // namespace raid
} // namespace village
} // namespace world
} // namespace mc
