#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/math/Vector3.hpp"
#include <functional>

namespace mc {

// 前向声明
class CreatureEntity;
class LivingEntity;
class BlockPos;
class IWorld;

namespace entity::ai::util {

/**
 * @brief 随机位置生成器
 *
 * 为AI目标生成智能随机位置。参考MC 1.16.5的RandomPositionGenerator。
 *
 * 主要功能：
 * - findRandomTarget: 在指定范围内生成随机可行走位置
 * - findRandomTargetBlockAwayFrom: 生成远离指定位置的目标
 * - findRandomTargetTowards: 生成朝向指定位置的随机目标
 * - getLandPos: 获取陆地位置
 */
class RandomPositionGenerator {
public:
    /**
     * @brief 方向枚举，用于生成位置时的方向偏好
     */
    enum class Direction : u8 { None = 0, North = 1, South = 2, East = 4, West = 8, Up = 16, Down = 32 };

    /**
     * @brief 位置候选结构
     */
    struct PositionCandidate {
        Vector3 position;
        f32 score;   // 评分，用于选择最佳位置
        bool isSafe; // 是否安全（非危险方块）
    };

    // ==================== 主要公开方法 ====================

    /**
     * @brief 在指定范围内生成随机可行走位置
     *
     * MC 1.16.5: RandomPositionGenerator.findRandomTarget(creature, xz, y)
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param[out] outPos 输出位置
     * @return 是否找到有效位置
     */
    static bool findRandomTarget(CreatureEntity* creature, i32 xzRange, i32 yRange, Vector3& outPos);

    /**
     * @brief 生成远离指定位置的目标
     *
     * MC 1.16.5: RandomPositionGenerator.findRandomTargetBlockAwayFrom
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param avoidPos 要远离的位置
     * @param[out] outPos 输出位置
     * @return 是否找到有效位置
     */
    static bool findRandomTargetBlockAwayFrom(
        CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& avoidPos, Vector3& outPos);

    /**
     * @brief 生成朝向指定位置的随机目标
     *
     * MC 1.16.5: RandomPositionGenerator.findRandomTargetTowards
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param targetPos 目标位置
     * @param[out] outPos 输出位置
     * @return 是否找到有效位置
     */
    static bool findRandomTargetTowards(
        CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& targetPos, Vector3& outPos);

    /**
     * @brief 获取陆地位置
     *
     * 寻找一个可行走的地面位置
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param[out] outPos 输出位置
     * @return 是否找到有效的陆地位置
     */
    static bool getLandPos(CreatureEntity* creature, i32 xzRange, i32 yRange, Vector3& outPos);

    /**
     * @brief 生成避开水域的随机位置
     *
     * MC 1.16.5: WaterAvoidingRandomWalkingGoal 使用
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param[out] outPos 输出位置
     * @return 是否找到有效位置
     */
    static bool findRandomTargetAvoidWater(CreatureEntity* creature, i32 xzRange, i32 yRange, Vector3& outPos);

    // ==================== 辅助方法 ====================

    /**
     * @brief 检查位置是否可行走
     *
     * @param creature 生物实体
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 是否可行走
     */
    static bool isPositionWalkable(CreatureEntity* creature, i32 x, i32 y, i32 z);

    /**
     * @brief 获取地面高度
     *
     * 从指定位置向下寻找第一个可行走方块
     *
     * @param world 世界接口
     * @param x X坐标
     * @param startZ 开始搜索的Y坐标
     * @param z Z坐标
     * @return 地面Y坐标，如果找不到返回-1
     */
    static i32 getGroundHeight(IWorld* world, i32 x, i32 startY, i32 z);

    /**
     * @brief 计算位置评分
     *
     * 基于安全性、可达性等因子计算位置评分
     *
     * @param creature 生物实体
     * @param pos 候选位置
     * @return 位置评分（越高越好）
     */
    static f32 calculatePositionScore(CreatureEntity* creature, const Vector3& pos);

private:
    // 常量
    static constexpr i32 MAX_ATTEMPTS = 10;       // 最大尝试次数
    static constexpr i32 MAX_GROUND_SEARCH = 10;  // 地面搜索最大高度差
    static constexpr f32 MIN_DISTANCE_SQ = 2.25f; // 最小距离平方（1.5格）

    /**
     * @brief 生成随机偏移
     */
    static Vector3 generateRandomOffset(
        CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& directionBias);

    /**
     * @brief 验证并优化位置
     *
     * 检查位置是否有效，如果需要则寻找地面
     */
    static bool validateAndAdjustPosition(CreatureEntity* creature, Vector3& pos);

    /**
     * @brief 生成多个候选位置并选择最佳
     */
    static bool findBestPosition(
        CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& directionBias, Vector3& outPos);
};

// Direction位运算支持
inline RandomPositionGenerator::Direction operator|(
    RandomPositionGenerator::Direction a, RandomPositionGenerator::Direction b)
{
    return static_cast<RandomPositionGenerator::Direction>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline RandomPositionGenerator::Direction operator&(
    RandomPositionGenerator::Direction a, RandomPositionGenerator::Direction b)
{
    return static_cast<RandomPositionGenerator::Direction>(static_cast<u8>(a) & static_cast<u8>(b));
}

inline bool hasDirection(RandomPositionGenerator::Direction flags, RandomPositionGenerator::Direction flag)
{
    return (static_cast<u8>(flags) & static_cast<u8>(flag)) != 0;
}

} // namespace entity::ai::util
} // namespace mc
