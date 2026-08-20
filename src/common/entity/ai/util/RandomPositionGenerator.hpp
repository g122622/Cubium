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

#include "../../../core/Types.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/block/BlockPos.hpp"
#include <functional>
#include <optional>

namespace mc {

// 前向声明
class CreatureEntity;
class LivingEntity;
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
        Vector3 position{0.0f, 0.0f, 0.0f};
        f32 score = 0.0f;    // 评分，用于选择最佳位置
        bool isSafe = false; // 是否安全（非危险方块）
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
     * @brief 生成朝向指定位置的缩放随机目标
     *
     * MC 1.16.5: RandomPositionGenerator.findRandomTargetTowardsScaled
     * 用于海豚寻宝等场景，在目标方向生成一个缩放后的随机目标位置。
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param targetPos 目标位置（宝藏位置）
     * @param angleRange 方向角限制范围（弧度，PI/8 表示左右各 PI/16）
     * @param[out] outPos 输出位置
     * @return 是否找到有效位置
     */
    static bool findRandomTargetTowardsScaled(
        CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& targetPos, f64 angleRange, Vector3& outPos);

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

    /**
     * @brief 生成随机方块目标位置
     *
     * MC 1.16.5: RandomPositionGenerator.findRandomTargetBlock
     * 用于飞行实体，选择随机的方块位置作为目标。
     * 与 findRandomTarget 不同，此方法不要求位置可行走。
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param avoidPos 可选的回避位置（std::nullopt 表示不回避）
     * @param[out] outPos 输出位置
     * @return 是否找到有效位置
     */
    static bool findRandomTargetBlock(
        CreatureEntity* creature, i32 xzRange, i32 yRange, std::optional<Vector3> avoidPos, Vector3& outPos);

    /**
     * @brief 生成朝向目标位置的方块目标位置
     *
     * MC 1.16.5: RandomPositionGenerator.findRandomTargetBlockTowards
     * 用于水生生物（如海豚），在朝向目标的方向选择一个方块位置。
     * 不要求位置可行走，但会检查是否是水或可通过的方块。
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param targetPos 目标位置
     * @param[out] outPos 输出位置
     * @return 是否找到有效位置
     */
    static bool findRandomTargetBlockTowards(
        CreatureEntity* creature, i32 xzRange, i32 yRange, const Vector3& targetPos, Vector3& outPos);

    // ==================== 飞行位置生成方法 ====================

    /**
     * @brief 生成悬停位置（在固体方块上方指定高度范围内）
     *
     * 对应 MC 1.21.11 的 HoverRandomPos.getPos。
     * 生成一个随机的空中位置，确保该位置在固体方块上方 minAboveSolid~maxAboveSolid 格范围内。
     * 用于蜜蜂、鹦鹉等飞行实体的悬停漫游目标选择。
     *
     * 算法流程：
     * 1. 在指定方向的角度范围内生成随机方向偏移
     * 2. 将偏移转换为世界坐标，并检查是否在可行范围内
     * 3. 将位置向上移动到固体方块上方指定高度
     * 4. 排除在水中或有寻路惩罚的位置
     * 5. 从10次尝试中选择评分最高的位置
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param xDir 方向向量的X分量（归一化）
     * @param zDir 方向向量的Z分量（归一化）
     * @param maxAngle 最大角度偏移（弧度，PI/2 表示左右各90度）
     * @param maxAboveSolid 固体方块上方最大格数
     * @param minAboveSolid 固体方块上方最小格数
     * @param[out] outPos 输出位置
     * @return 是否找到有效位置
     */
    static bool findHoverPosition(CreatureEntity* creature,
        i32 xzRange,
        i32 yRange,
        f64 xDir,
        f64 zDir,
        f32 maxAngle,
        i32 maxAboveSolid,
        i32 minAboveSolid,
        Vector3& outPos);

    /**
     * @brief 生成空中或水中位置
     *
     * 对应 MC 1.21.11 的 AirAndWaterRandomPos.getPos。
     * 生成一个随机的空中位置，如果起始位置在固体方块内部，则向上移出固体方块。
     * 不排除水中位置（与 findAirPositionTowards 不同）。
     * 用于蜜蜂等飞行实体的悬停位置回退选择。
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param yOffset Y轴额外偏移
     * @param xDir 方向向量的X分量（归一化）
     * @param zDir 方向向量的Z分量（归一化）
     * @param maxAngle 最大角度偏移（弧度）
     * @param[out] outPos 输出位置
     * @return 是否找到有效位置
     */
    static bool findAirAndWaterPosition(CreatureEntity* creature,
        i32 xzRange,
        i32 yRange,
        i32 yOffset,
        f64 xDir,
        f64 zDir,
        f32 maxAngle,
        Vector3& outPos);

    /**
     * @brief 生成朝向目标的空中位置
     *
     * 对应 MC 1.21.11 的 AirRandomPos.getPosTowards。
     * 生成一个朝向目标方向的随机空中位置，排除水中位置。
     * 用于蜜蜂飞向蜂巢/花朵时的中间导航点选择。
     *
     * @param creature 生物实体
     * @param xzRange 水平搜索范围（格）
     * @param yRange 垂直搜索范围（格）
     * @param yOffset Y轴额外偏移
     * @param targetPos 目标位置
     * @param maxAngle 最大角度偏移（弧度，PI/10 表示左右各9度）
     * @param[out] outPos 输出位置
     * @return 是否找到有效位置
     */
    static bool findAirPositionTowards(CreatureEntity* creature,
        i32 xzRange,
        i32 yRange,
        i32 yOffset,
        const Vector3& targetPos,
        f32 maxAngle,
        Vector3& outPos);

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
    static constexpr i32 MAX_ATTEMPTS = 10;                // 最大尝试次数
    static constexpr i32 MAX_GROUND_SEARCH = 10;           // 地面搜索最大高度差
    static constexpr f32 MIN_DISTANCE_SQ = 2.25f;          // 最小距离平方（1.5格）
    static constexpr f64 SQRT_OF_TWO = 1.4142135623730951; // sqrt(2)

    /**
     * @brief 在指定角度范围内生成随机方向偏移
     *
     * 对应 MC 的 RandomPos.generateRandomDirectionWithinRadians。
     * 生成一个在 [xDir, zDir] 方向的 maxAngle 弧度范围内的随机方向偏移。
     * 距离使用 sqrt 分布以实现均匀的面积覆盖。
     *
     * @param rng 随机数生成器
     * @param minRange 最小距离（通常为0）
     * @param maxRange 最大水平距离
     * @param verticalRange 垂直偏移范围
     * @param yOffset 固定Y偏移
     * @param xDir 方向向量X分量
     * @param zDir 方向向量Z分量
     * @param maxAngle 最大角度偏移（弧度）
     * @return 随机偏移的 BlockPos，如果超出范围则返回 nullopt
     */
    static std::optional<BlockPos> generateRandomDirectionWithinRadians(math::Random& rng,
        f64 minRange,
        f64 maxRange,
        i32 verticalRange,
        i32 yOffset,
        f64 xDir,
        f64 zDir,
        f64 maxAngle);

    /**
     * @brief 将随机偏移转换为世界坐标并施加家区域约束
     *
     * 对应 MC 的 RandomPos.generateRandomPosTowardDirection。
     * 将相对偏移加上实体当前位置，若实体有家区域则将位置拉向家区域。
     *
     * @param creature 生物实体
     * @param range 搜索范围（用于家区域偏移计算）
     * @param isRestricted 是否受家区域约束
     * @param offset 相对偏移
     * @return 绝对世界坐标的 BlockPos
     */
    static BlockPos generatePosTowardDirection(
        CreatureEntity* creature, f64 range, bool isRestricted, const BlockPos& offset);

    /**
     * @brief 将位置向上移出固体方块
     *
     * 对应 MC 的 RandomPos.moveUpOutOfSolid。
     * 如果起始位置在固体方块内，向上移动直到找到非固体方块或到达 maxY。
     *
     * @param pos 起始位置
     * @param maxY 世界最大Y坐标
     * @param isSolid 判断方块是否固体的谓词
     * @return 移动后的位置
     */
    static BlockPos moveUpOutOfSolid(
        const BlockPos& pos, i32 maxY, const std::function<bool(const BlockPos&)>& isSolid);

    /**
     * @brief 将位置向上移到固体方块上方指定高度
     *
     * 对应 MC 的 RandomPos.moveUpToAboveSolid。
     * 如果起始位置在固体方块内，先向上移出固体，然后继续向上移动 aboveSolidAmount 格。
     * 如果在额外上升过程中遇到固体方块，则回退一格。
     *
     * @param pos 起始位置
     * @param aboveSolidAmount 需要在固体方块上方的格数
     * @param maxY 世界最大Y坐标
     * @param isSolid 判断方块是否固体的谓词
     * @return 移动后的位置
     */
    static BlockPos moveUpToAboveSolid(
        const BlockPos& pos, i32 aboveSolidAmount, i32 maxY, const std::function<bool(const BlockPos&)>& isSolid);

    /**
     * @brief 检查方块位置是否为固体
     */
    static bool isSolidAt(CreatureEntity* creature, const BlockPos& pos);

    /**
     * @brief 检查方块位置是否为水
     */
    static bool isWaterAt(CreatureEntity* creature, const BlockPos& pos);

    /**
     * @brief 检查位置是否有寻路惩罚
     */
    static bool hasPathfindingMalus(CreatureEntity* creature, const BlockPos& pos);

    /**
     * @brief 检查实体是否受家区域约束
     */
    static bool isMobRestricted(CreatureEntity* creature, f64 range);

    /**
     * @brief 检查位置是否超出建造高度
     */
    static bool isOutsideBuildHeight(CreatureEntity* creature, const BlockPos& pos);

    /**
     * @brief 生成随机偏移
     *
     * 对齐 vanilla RandomPos.generateRandomDirectionWithinRadians：偏移超界时本次尝试失败，
     * 返回 std::nullopt（而非 zero），由 findBestPosition 跳过——避免把"超界失败"误当"原地有效候选"
     * 致逃避位退化为实体自身位置（AvoidEntityGoal 原地不动被威胁源追上）。
     */
    static std::optional<Vector3> generateRandomOffset(
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
