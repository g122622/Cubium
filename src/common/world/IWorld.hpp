#pragma once

#include "../core/Types.hpp"
#include "block/BlockPos.hpp"
#include "../util/math/Vector3.hpp"
#include "../util/AxisAlignedBB.hpp"
#include "../resource/ResourceLocation.hpp"
#include "../sound/SoundCategory.hpp"
#include "tick/base/TickPriority.hpp"
#include "explosion/ExplosionMode.hpp"
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
class Player;
enum class ContainerType : u8;

namespace world::tick {
class TickManager;
}

namespace world::explosion {
class Explosion;  // 前向声明
}

namespace server {
class ServerWorld;  // 前向声明，用于asServerWorld()
}

namespace fluid {
class Fluid;
class FluidState;
}

namespace client::renderer::trident::particle {
enum class ParticleTypeId : u16;
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
     * @brief 获取方块状态（使用 BlockPos）
     * @param pos 方块位置
     * @return 方块状态指针，如果超出范围返回空气
     */
    [[nodiscard]] virtual const BlockState* getBlockState(const BlockPos& pos) const {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 设置方块状态
     * @param x, y, z 方块坐标
     * @param state 方块状态
     * @return 是否成功
     */
    virtual bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) = 0;

    /**
     * @brief 设置方块状态（使用 BlockPos）
     * @param pos 方块位置
     * @param state 方块状态
     * @return 是否成功
     */
    virtual bool setBlock(const BlockPos& pos, const BlockState* state) {
        return setBlock(pos.x, pos.y, pos.z, state);
    }

    /**
     * @brief 设置方块状态（带标志）
     * @param x, y, z 方块坐标
     * @param state 方块状态
     * @param flags 更新标志（2=通知邻居，3=通知邻居+更新客户端）
     * @return 是否成功
     */
    virtual bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) {
        // TODO: 处理标志（目前直接调用不带标志的 setBlock）
        (void)flags;
        return setBlock(x, y, z, state);
    }

    /**
     * @brief 设置方块状态（使用 BlockPos）
     * @param pos 方块位置
     * @param state 方块状态
     * @param flags 更新标志
     * @return 是否成功
     */
    virtual bool setBlockState(const BlockPos& pos, const BlockState* state, i32 flags) {
        return setBlockState(pos.x, pos.y, pos.z, state, flags);
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
     * @brief 打开容器
     * @param type 容器类型
     * @param pos 方块位置
     * @param player 发起交互的玩家
     * @return 如果成功打开返回true
     */
    [[nodiscard]] virtual bool openContainer(ContainerType type, const BlockPos& pos, Player& player) {
        (void)type;
        (void)pos;
        (void)player;
        return false;
    }

    /**
     * @brief 设置方块实体
     * @param pos 方块位置
     * @param entity 方块实体指针（获取所有权）
     */
    virtual void setBlockEntity(const BlockPos& pos, BlockEntity* entity) {
        (void)pos;
        (void)entity;
        // TODO : 存储方块实体
    }

    /**
     * @brief 移除方块实体
     * @param pos 方块位置
     */
    virtual void removeBlockEntity(const BlockPos& pos) {
        // TODO : 移除方块实体
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
     * @brief 获取流体状态（使用 BlockPos）
     * @param pos 方块位置
     * @return 流体状态指针，如果无流体返回空流体状态
     */
    [[nodiscard]] virtual const fluid::FluidState* getFluidState(const BlockPos& pos) const {
        return getFluidState(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 检查位置是否有流体
     */
    [[nodiscard]] bool hasFluid(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否有流体（使用 BlockPos）
     */
    [[nodiscard]] virtual bool hasFluid(const BlockPos& pos) const {
        return hasFluid(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 检查位置是否为水
     */
    [[nodiscard]] bool isWaterAt(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否为水（使用 BlockPos）
     */
    [[nodiscard]] virtual bool isWaterAt(const BlockPos& pos) const {
        return isWaterAt(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 检查位置是否为岩浆
     */
    [[nodiscard]] bool isLavaAt(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否为岩浆（使用 BlockPos）
     */
    [[nodiscard]] virtual bool isLavaAt(const BlockPos& pos) const {
        return isLavaAt(pos.x, pos.y, pos.z);
    }

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

    // ========== 声音播放 ==========

    /**
     * @brief 播放声音
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param position 声音位置
     * @param volume 音量倍率
     * @param pitch 音调倍率
     */
    virtual void playSound(const ResourceLocation& soundEventId,
                           sound::SoundCategory category,
                           const Vector3& position,
                           f32 volume,
                           f32 pitch) {
        (void)soundEventId;
        (void)category;
        (void)position;
        (void)volume;
        (void)pitch;
    }

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
     * @brief 获取方块光照（使用 BlockPos）
     * @param pos 方块位置
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getBlockLight(const BlockPos& pos) const {
        return getBlockLight(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 获取天空光照
     * @param x, y, z 方块坐标
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getSkyLight(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取天空光照（使用 BlockPos）
     * @param pos 方块位置
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getSkyLight(const BlockPos& pos) const {
        return getSkyLight(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 获取综合光照等级
     *
     * 计算方块位置的实际光照等级，考虑天空光照衰减。
     * 这是 MC 1.16.5 中用于作物生长判断的标准方法。
     *
     * 参考: net.minecraft.world.World#getLightSubtracted
     *
     * @param pos 方块位置
     * @param skyDarkening 天空光照衰减值（0-15，用于天气/时间影响）
     * @return 综合光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getLightSubtracted(const BlockPos& pos, u32 skyDarkening) const {
        // 默认实现：返回方块光照和（天空光照-衰减）的最大值
        u8 blockLight = getBlockLight(pos);
        u8 skyLight = getSkyLight(pos);
        if (skyLight > skyDarkening) {
            skyLight = skyLight - static_cast<u8>(skyDarkening);
        } else {
            skyLight = 0;
        }
        return std::max(blockLight, skyLight);
    }

    /**
     * @brief 检查位置是否可以看到天空
     *
     * 参考: net.minecraft.world.IWorldReader#canSeeSky
     *
     * @param pos 方块位置
     * @return 如果该位置可以看到天空返回 true
     */
    [[nodiscard]] virtual bool canSeeSky(const BlockPos& pos) const {
        (void)pos;
        // 默认实现返回 false，具体世界需要重写
        return false;
    }

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
     * @brief 检查位置是否在世界边界内（使用 BlockPos）
     * @param pos 方块位置
     * @return 是否在世界边界内
     */
    [[nodiscard]] virtual bool isWithinWorldBounds(const BlockPos& pos) const {
        return isWithinWorldBounds(pos.x, pos.y, pos.z);
    }

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

    /**
     * @brief 获取 Tick 管理器
     *
     * 服务端世界将返回有效的 `world::tick::TickManager` 实例，
     * 客户端或不支持调度的实现应覆盖或实现为不支持。
     */
    [[nodiscard]] virtual world::tick::TickManager& tickManager() = 0;
    [[nodiscard]] virtual const world::tick::TickManager& tickManager() const = 0;

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
    [[nodiscard]] virtual Entity* getEntity(EntityId id) {
        (void)id;
        return nullptr;
    }
    [[nodiscard]] virtual const Entity* getEntity(EntityId id) const {
        (void)id;
        return nullptr;
    }

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
     * @brief 是否为超热维度
     */
    [[nodiscard]] virtual bool isUltraWarm() const {
        return dimension() == 1;
    }

    /**
     * @brief 是否允许火焰蔓延
     *
     * 当前默认开启，后续接入游戏规则后可由具体世界覆盖。
     */
    [[nodiscard]] virtual bool doFireTick() const {
        return true;
    }

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

    /**
     * @brief 获取游戏时间 (总tick数)
     *
     * 与 currentTick() 相同，提供更明确的语义。
     */
    [[nodiscard]] virtual u64 getGameTime() const {
        return currentTick();
    }

    /**
     * @brief 检查是否为客户端世界
     * @return 如果是客户端返回true，服务端返回false
     */
    [[nodiscard]] virtual bool isRemote() const { return true; }

    /**
     * @brief 检查维度是否有天空光照
     * @return 如果维度有天空光照返回true（主世界），否则返回false（下界、末地）
     */
    [[nodiscard]] virtual bool hasSkyLight() const { return dimension() == 0; }

    // ========== 难度 ==========

    /**
     * @brief 是否困难模式
     */
    [[nodiscard]] virtual bool isHardcore() const = 0;

    /**
     * @brief 获取难度
     */
    [[nodiscard]] virtual Difficulty difficulty() const = 0;

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

    // ========== 粒子生成 ==========

    /**
     * @brief 生成粒子
     *
     * 服务端：广播给附近玩家
     * 客户端：本地生成粒子
     *
     * @param type 粒子类型
     * @param pos 粒子位置
     * @param velocity 粒子速度
     */
    virtual void addParticle(
        client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity) {
        (void)type;
        (void)pos;
        (void)velocity;
    }

    /**
     * @brief 生成粒子（带数量和偏移）
     *
     * 在指定位置附近随机生成多个粒子。
     *
     * @param type 粒子类型
     * @param pos 粒子中心位置
     * @param velocity 粒子基础速度
     * @param offset 随机偏移范围
     * @param count 粒子数量
     */
    virtual void addParticle(
        client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count) {
        (void)type;
        (void)pos;
        (void)velocity;
        (void)offset;
        (void)count;
    }

    /**
     * @brief 检查是否应在指定位置生成粒子
     *
     * 用于距离裁剪，避免在玩家视野外生成粒子。
     *
     * @param pos 粒子位置
     * @param maxDistance 最大距离（默认 256 格）
     * @return 是否应生成粒子
     */
    [[nodiscard]] virtual bool shouldSpawnParticleAt(
        const Vector3& pos,
        f32 maxDistance = 256.0f) const {
        (void)pos;
        (void)maxDistance;
        return true;
    }

    // ========== 爆炸 ==========

    /**
     * @brief 创建爆炸
     *
     * 在指定位置创建爆炸，破坏方块、造成伤害和击退。
     *
     * @param position 爆炸中心位置
     * @param radius 爆炸半径
     * @param mode 爆炸模式（默认 Destroy）
     * @param causesFire 是否生成火焰（默认 false）
     * @param source 爆炸源实体（可选）
     */
    virtual void createExplosion(
        const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode = world::explosion::ExplosionMode::Destroy,
        bool causesFire = false,
        Entity* source = nullptr) {
        // 默认空操作，ServerWorld 会重写以实际执行爆炸
        (void)position;
        (void)radius;
        (void)mode;
        (void)causesFire;
        (void)source;
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
