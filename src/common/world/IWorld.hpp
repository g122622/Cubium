#pragma once

#include "../core/Types.hpp"
#include "../util/math/Vector3.hpp"
#include "../util/AxisAlignedBB.hpp"
#include "tick/base/TickPriority.hpp"
#include <vector>
#include <memory>

namespace mc {

// 前向声明
class Entity;
class BlockState;
class ChunkData;
class BlockPos;
class PhysicsEngine;
class Block;
class IRandom;
class BlockEntity;

namespace server {
class ServerWorld;  // 前向声明，用于asServerWorld()
}

namespace fluid {
class Fluid;
class FluidState;
}

/**
 * @brief 世界访问接口
 *
 * 为实体提供世界访问的抽象接口。
 * ServerWorld 和 ClientWorld 将实现此接口。
 *
 * 参考 MC 1.16.5 IWorldReader / World
 */
class IWorld {
public:
    virtual ~IWorld() = default;

    // ========== 方块访问 ==========

    /**
     * @brief 获取方块状态
     * @param x, y, z 方块坐标
     * @return 方块状态指针，如果超出范围返回空气
     */
    [[nodiscard]] virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 设置方块状态
     * @param x, y, z 方块坐标
     * @param state 方块状态
     * @return 是否成功
     */
    virtual bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) = 0;

    /**
     * @brief 设置方块状态（带标志）
     * @param x, y, z 方块坐标
     * @param state 方块状态
     * @param flags 更新标志（2=通知邻居，3=通知邻居+更新客户端）
     * @return 是否成功
     */
    virtual bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) {
        (void)flags;
        return setBlock(x, y, z, state);
    }

    /**
     * @brief 获取方块实体
     * @param pos 方块位置
     * @return 方块实体指针，如果不存在返回 nullptr
     */
    [[nodiscard]] virtual BlockEntity* getBlockEntity(const BlockPos& pos) {
        (void)pos;
        return nullptr;
    }
    [[nodiscard]] virtual const BlockEntity* getBlockEntity(const BlockPos& pos) const {
        (void)pos;
        return nullptr;
    }

    /**
     * @brief 设置方块实体
     * @param pos 方块位置
     * @param entity 方块实体指针（调用者保留所有权）
     */
    virtual void setBlockEntity(const BlockPos& pos, BlockEntity* entity) {
        (void)pos;
        (void)entity;
    }

    /**
     * @brief 移除方块实体
     * @param pos 方块位置
     */
    virtual void removeBlockEntity(const BlockPos& pos) {
        (void)pos;
    }

    // ========== 流体访问 ==========

    /**
     * @brief 获取流体状态
     * @param x, y, z 方块坐标
     * @return 流体状态指针，如果无流体返回空流体状态
     */
    [[nodiscard]] virtual const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 检查位置是否有流体
     */
    [[nodiscard]] bool hasFluid(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否为水
     */
    [[nodiscard]] bool isWaterAt(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否为岩浆
     */
    [[nodiscard]] bool isLavaAt(i32 x, i32 y, i32 z) const;

    // ========== 区块访问 ==========

    /**
     * @brief 获取区块
     * @param x, z 区块坐标
     * @return 区块数据指针，如果未加载返回 nullptr
     */
    [[nodiscard]] virtual const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const = 0;

    /**
     * @brief 检查区块是否存在
     */
    [[nodiscard]] virtual bool hasChunk(ChunkCoord x, ChunkCoord z) const = 0;

    // ========== 高度查询 ==========

    /**
     * @brief 获取最高可站立方块高度
     * @param x, z 水平坐标
     * @return 最高方块 Y 坐标
     */
    [[nodiscard]] virtual i32 getHeight(i32 x, i32 z) const = 0;

    // ========== 光照查询 ==========

    /**
     * @brief 获取方块光照
     * @param x, y, z 方块坐标
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getBlockLight(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取天空光照
     * @param x, y, z 方块坐标
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getSkyLight(i32 x, i32 y, i32 z) const = 0;

    // ========== 碰撞检测 ==========

    /**
     * @brief 检查碰撞箱是否与方块碰撞
     * @param box 碰撞箱
     * @return 是否碰撞
     */
    [[nodiscard]] virtual bool hasBlockCollision(const AxisAlignedBB& box) const = 0;

    /**
     * @brief 获取碰撞箱内的所有方块碰撞箱
     * @param box 碰撞箱
     * @return 碰撞箱列表
     */
    [[nodiscard]] virtual std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const = 0;

    /**
     * @brief 检查位置是否在世界边界内
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 是否在世界边界内
     */
    [[nodiscard]] virtual bool isWithinWorldBounds(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 检查碰撞箱是否与实体碰撞
     * @param box 碰撞箱
     * @param except 排除的实体（通常是自身）
     * @return 是否碰撞
     */
    [[nodiscard]] virtual bool hasEntityCollision(const AxisAlignedBB& box, const Entity* except = nullptr) const = 0;

    /**
     * @brief 获取碰撞箱内的所有实体碰撞箱
     * @param box 碰撞箱
     * @param except 排除的实体（通常是自身）
     * @return 实体碰撞箱列表
     */
    [[nodiscard]] virtual std::vector<AxisAlignedBB> getEntityCollisions(
        const AxisAlignedBB& box, const Entity* except = nullptr) const = 0;

    // ========== 物理引擎 ==========

    /**
     * @brief 获取物理引擎
     * @return 物理引擎指针（可能为 nullptr）
     */
    [[nodiscard]] virtual PhysicsEngine* physicsEngine() = 0;
    [[nodiscard]] virtual const PhysicsEngine* physicsEngine() const = 0;

    // ========== 实体管理 ==========

    /**
     * @brief 生成实体到世界中
     * @param entity 实体实例
     * @return 实体ID，如果失败返回 0
     *
     * 默认实现返回 0（不支持生成实体）。
     * ServerWorld 会重写此方法以实际生成实体。
     */
    virtual EntityId spawnEntity(std::unique_ptr<Entity> entity);

    /**
     * @brief 通过ID获取实体
     * @param id 实体ID
     * @return 实体指针，如果不存在返回 nullptr
     *
     * 默认实现返回 nullptr。
     */
    [[nodiscard]] virtual Entity* getEntity(EntityId id) { return nullptr; }
    [[nodiscard]] virtual const Entity* getEntity(EntityId id) const { return nullptr; }

    // ========== 实体查询 ==========

    /**
     * @brief 获取碰撞箱内的所有实体
     * @param box 碰撞箱
     * @param except 排除的实体（可选）
     * @return 实体列表
     */
    [[nodiscard]] virtual std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box,
        const Entity* except = nullptr) const = 0;

    /**
     * @brief 获取范围内的所有实体
     * @param pos 中心位置
     * @param range 范围
     * @param except 排除的实体（可选）
     * @return 实体列表
     */
    [[nodiscard]] virtual std::vector<Entity*> getEntitiesInRange(
        const Vector3& pos,
        f32 range,
        const Entity* except = nullptr) const = 0;

    // ========== 维度信息 ==========

    /**
     * @brief 获取维度 ID
     */
    [[nodiscard]] virtual DimensionId dimension() const = 0;

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] virtual u64 seed() const = 0;

    // ========== 时间 ==========

    /**
     * @brief 获取当前 tick
     */
    [[nodiscard]] virtual u64 currentTick() const = 0;

    /**
     * @brief 获取一天内的时间 (0-23999)
     */
    [[nodiscard]] virtual i64 dayTime() const = 0;

    // ========== 难度 ==========

    /**
     * @brief 是否困难模式
     */
    [[nodiscard]] virtual bool isHardcore() const = 0;

    /**
     * @brief 获取难度
     */
    [[nodiscard]] virtual i32 difficulty() const = 0;

    // ========== 天气 ==========

    /**
     * @brief 是否正在降雨（强度检查）
     *
     * 使用强度阈值判断，rainStrength > 0.2 返回 true
     *
     * @return 是否正在下雨
     */
    [[nodiscard]] virtual bool isRaining() const { return false; }

    /**
     * @brief 是否正在雷暴（强度检查）
     *
     * 使用强度阈值判断，thunderStrength > 0.9 返回 true
     *
     * @return 是否正在雷暴
     */
    [[nodiscard]] virtual bool isThundering() const { return false; }

    /**
     * @brief 获取降雨强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)，用于插值
     * @return 降雨强度 (0.0 - 1.0)
     */
    [[nodiscard]] virtual f32 rainStrength(f32 partialTick = 0.0f) const {
        (void)partialTick;
        return 0.0f;
    }

    /**
     * @brief 获取雷暴强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)，用于插值
     * @return 雷暴强度 (0.0 - 1.0)
     */
    [[nodiscard]] virtual f32 thunderStrength(f32 partialTick = 0.0f) const {
        (void)partialTick;
        return 0.0f;
    }

    /**
     * @brief 判断指定位置是否可以降雨
     *
     * 需要满足：可以看到天空 + 生物群系允许降水
     *
     * @param pos 方块位置
     * @return 是否可以降雨
     */
    [[nodiscard]] virtual bool canRainAt(const BlockPos& pos) const {
        (void)pos;
        return false;
    }

    // ========== Tick调度 ==========

    /**
     * @brief 调度方块tick
     *
     * 方便方法，委托给TickManager。
     * 默认实现为空操作，ServerWorld会重写以实际调度。
     *
     * @param pos 方块位置
     * @param block 方块引用
     * @param delay 延迟tick数
     * @param priority 优先级（默认Normal）
     */
    virtual void scheduleBlockTick(const BlockPos& pos, Block& block, i32 delay,
                                   world::tick::TickPriority priority = world::tick::TickPriority::Normal) {
        // 默认空操作，客户端不需要调度tick
        (void)pos;
        (void)block;
        (void)delay;
        (void)priority;
    }

    /**
     * @brief 调度流体tick
     *
     * 方便方法，委托给TickManager。
     * 默认实现为空操作，ServerWorld会重写以实际调度。
     *
     * @param pos 流体位置
     * @param fluid 流体引用
     * @param delay 延迟tick数
     * @param priority 优先级（默认Normal）
     */
    virtual void scheduleFluidTick(const BlockPos& pos, fluid::Fluid& fluid, i32 delay,
                                   world::tick::TickPriority priority = world::tick::TickPriority::Normal) {
        // 默认空操作，客户端不需要调度tick
        (void)pos;
        (void)fluid;
        (void)delay;
        (void)priority;
    }

    // ========== 类型转换 ==========

    /**
     * @brief 转换为ServerWorld指针
     *
     * 只有ServerWorld会返回有效的指针，其他实现返回nullptr。
     * 用于需要访问ServerWorld特有功能（如Brain系统）的场景。
     *
     * @return ServerWorld指针，如果不是ServerWorld返回nullptr
     */
    [[nodiscard]] virtual server::ServerWorld* asServerWorld() { return nullptr; }
    [[nodiscard]] virtual const server::ServerWorld* asServerWorld() const { return nullptr; }

protected:
    IWorld() = default;
};

/**
 * @brief 区块读取器接口
 *
 * IBlockReader 继承自 IWorld，用于表示只读的方块访问接口。
 * 参考 MC 1.16.5 IBlockReader
 */
class IBlockReader : public IWorld {};

/**
 * @brief 世界读取器接口
 *
 * IWorldReader 是 IWorld 的别名，用于表示只读的世界访问接口。
 */
using IWorldReader = IWorld;

} // namespace mc
