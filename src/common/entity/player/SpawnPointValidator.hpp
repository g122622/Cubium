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

#include "../../core/Types.hpp"
#include "../../util/Direction.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../world/GlobalPos.hpp"
#include "../../world/block/BlockPos.hpp"
#include <optional>

namespace mc {

// 前向声明
class IWorld;
class ServerWorld;
class BlockState;

/**
 * @brief 重生点验证结果
 *
 * 描述重生点验证的结果和原因
 */
enum class SpawnPointValidationResult : u8 {
    /// 重生点有效
    Valid,
    /// 重生点无效 - 床不存在或被破坏
    BedMissing,
    /// 重生点无效 - 床在错误的维度（非主世界）
    BedWrongDimension,
    /// 重生点无效 - 床被阻挡
    BedObstructed,
    /// 重生点无效 - 重生锚不存在或被破坏
    RespawnAnchorMissing,
    /// 重生点无效 - 重生锚无能量
    RespawnAnchorNoCharge,
    /// 重生点无效 - 重生锚在错误的维度（非下界）
    RespawnAnchorWrongDimension,
    /// 重生点无效 - 重生锚周围无安全位置
    RespawnAnchorNoSafePosition,
    /// 重生点无效 - 方块不允许在内部生成
    BlockCannotSpawnIn,
    /// 重生点无效 - 维度不存在
    DimensionNotFound,
    /// 重生点无效 - 世界不存在
    WorldNotFound
};

/**
 * @brief 重生点验证器
 *
 * 验证玩家的重生点（床/重生锚）是否仍然有效。
 * 参考 MC 1.16.5 PlayerEntity.func_242374_a_()
 */
class SpawnPointValidator {
public:
    /**
     * @brief 验证重生点
     *
     * 执行完整的重生点验证流程：
     * 1. 检查世界和维度是否存在
     * 2. 检查方块是否存在
     * 3. 根据方块类型（床/重生锚/其他）执行特定验证
     * 4. 查找安全生成位置
     *
     * @param world 世界引用
     * @param spawnPoint 重生点位置（包含维度和坐标）
     * @param spawnForced 是否强制重生点
     * @param consumeCharge 是否消耗重生锚能量（死亡重生时为 true）
     * @return 验证结果
     */
    [[nodiscard]] static SpawnPointValidationResult validate(
        IWorld& world, const GlobalPos& spawnPoint, bool spawnForced, bool consumeCharge);

    /**
     * @brief 查找重生点的安全生成位置
     *
     * 如果重生点有效，返回安全的生成位置。
     *
     * @param world 世界引用
     * @param spawnPoint 重生点位置
     * @param spawnForced 是否强制重生点
     * @param consumeCharge 是否消耗重生锚能量
     * @return 安全生成位置，如果无效返回 nullopt
     */
    [[nodiscard]] static std::optional<Vector3> findSafeSpawnPosition(
        IWorld& world, const GlobalPos& spawnPoint, bool spawnForced, bool consumeCharge);

    /**
     * @brief 验证床重生点
     *
     * 检查：
     * - 床是否存在
     * - 是否在正确维度（主世界）
     * - 床周围是否有安全生成位置
     *
     * @param world 世界引用
     * @param bedPos 床头位置
     * @return 如果床有效返回 true
     */
    [[nodiscard]] static bool validateBedSpawn(IWorld& world, const BlockPos& bedPos);

    /**
     * @brief 获取床的安全生成位置
     *
     * 在床周围查找安全的生成位置。
     * 参考 MC 1.16.5 BedBlock.func_242652_a_()
     *
     * @param world 世界引用
     * @param bedPos 床头位置
     * @return 安全生成位置，如果无返回 nullopt
     */
    [[nodiscard]] static std::optional<Vector3> findBedSpawnPosition(IWorld& world, const BlockPos& bedPos);

    /**
     * @brief 验证重生锚重生点
     *
     * 检查：
     * - 重生锚是否存在
     * - 是否在正确维度（下界）
     * - 是否有能量
     *
     * @param world 世界引用
     * @param anchorPos 重生锚位置
     * @return 如果重生锚有效返回 true
     */
    [[nodiscard]] static bool validateRespawnAnchorSpawn(IWorld& world, const BlockPos& anchorPos);

    /**
     * @brief 获取重生锚的安全生成位置
     *
     * 在重生锚周围查找安全的生成位置。
     * 参考 MC 1.16.5 RespawnAnchorBlock.func_235560_a_()
     *
     * @param world 世界引用
     * @param anchorPos 重生锚位置
     * @param consumeCharge 是否消耗能量
     * @return 安全生成位置，如果无返回 nullopt
     */
    [[nodiscard]] static std::optional<Vector3> findRespawnAnchorSpawnPosition(
        IWorld& world, const BlockPos& anchorPos, bool consumeCharge);

    /**
     * @brief 验证强制重生点
     *
     * 对于强制重生点，检查方块是否允许在内部生成。
     * 参考 MC 1.16.5 Block.canSpawnInBlock()
     *
     * @param world 世界引用
     * @param pos 重生点位置
     * @return 如果可以在该位置生成返回 true
     */
    [[nodiscard]] static bool validateForcedSpawn(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查方块是否为床
     *
     * @param state 方块状态
     * @return 如果是床返回 true
     */
    [[nodiscard]] static bool isBed(const BlockState& state);

    /**
     * @brief 检查方块是否为重生锚
     *
     * @param state 方块状态
     * @return 如果是重生锚返回 true
     */
    [[nodiscard]] static bool isRespawnAnchor(const BlockState& state);

    /**
     * @brief 获取重生锚的充能等级
     *
     * @param state 重生锚方块状态
     * @return 充能等级 (0-4)，如果不是重生锚返回 0
     */
    [[nodiscard]] static i32 getRespawnAnchorCharges(const BlockState& state);

private:
    /**
     * @brief 检查位置是否有站立空间
     *
     * 需要两个方块高的空间（玩家高度约 1.8 格）。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return 如果有足够空间返回 true
     */
    [[nodiscard]] static bool hasStandingSpace(const IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取床的朝向
     *
     * @param state 床方块状态
     * @return 床的朝向，如果不是床返回 Direction::None
     */
    [[nodiscard]] static Direction getBedFacing(const BlockState& state);

    /**
     * @brief 检查位置是否安全可站立
     *
     * 参考 MC 1.16.5 TransportationHelper.func_242379_a_()
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param requireSafe 是否要求位置安全（不会窒息）
     * @return 如果位置安全可站立返回 true
     */
    [[nodiscard]] static bool isSafeSpawnPosition(const IWorld& world, const BlockPos& pos, bool requireSafe);
};

} // namespace mc
