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

#include "IWorldWriter.hpp"
#include "WorldConstants.hpp"
#include "block/BlockPos.hpp"
#include "border/WorldBorder.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "explosion/ExplosionContext.hpp"
#include "explosion/ExplosionMode.hpp"
#include "gameevent/GameEvent.hpp"
#include "lighting/InternalLightUtils.hpp"
#include "tick/base/TickPriority.hpp"
#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

// 前向声明
class Entity;
class DamageSource;
class BlockState;
namespace world::chunk {
class ChunkData;
}
using world::chunk::ChunkData;
class BlockPos;
class PhysicsEngine;
class Block;
class IRandom;
class BlockEntity;
class Player;
class ItemStack;
class INamedContainerProvider;
enum class Direction : u8;
enum class ContainerType : u8;
class EndDragonFight;

namespace world::tick {
class TickManager;
}

namespace world::explosion {
class Explosion; // 前向声明
}

namespace world::village {
class VillageManager; // 前向声明
}

namespace world::village::raid {
class RaidManager; // 前向声明
}

namespace world::map {
class MapDataManager; // 前向声明
}

namespace world::gamerule {
class GameRules; // 前向声明
}

namespace world::gen::structure {
class StructureSet; // 前向声明
}

namespace loot {
class LootTableManager; // 前向声明
}

namespace server {
class ServerWorld; // 前向声明，用于asServerWorld()
}

namespace ecs {
class EntityRegistry; // 前向声明，用于 entityRegistry()
} // namespace ecs

// 前向声明，用于createFeatureRegion()
class WorldGenRegion;

namespace fluid {
class Fluid;
class FluidState;
} // namespace fluid

namespace particle {
enum class ParticleTypeId : u16;
} // namespace particle

namespace gameevent {
class GameEvent; // 前向声明
} // namespace gameevent

/**
 * @brief 世界访问接口
 *
 * 为实体提供世界访问的抽象接口。
 * ServerWorld 和 ClientWorld 将实现此接口。
 *
 * 继承自 IWorldWriter，提供读写能力。
 */
class IWorld : public IWorldWriter {
public:
    virtual ~IWorld() = default;

    // IWorldWriter 的 setBlockState 方法已提供基础写入能力
    // IWorld 额外提供读取能力

    // ========== 方块访问 ==========

    /**
     * @brief 获取方块状态
     * @param x, y, z 方块坐标
     * @return 方块状态指针；如果区块未加载、坐标无效或方块状态不可用，可能返回 nullptr
     */
    [[nodiscard]] virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取方块状态（使用 BlockPos）
     * @param pos 方块位置
     * @return 方块状态指针；如果区块未加载、坐标无效或方块状态不可用，可能返回 nullptr
     */
    [[nodiscard]] virtual const BlockState* getBlockState(const BlockPos& pos) const
    {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    // 注意：setBlockState 方法已从 IWorldWriter 继承

    /**
     * @brief 设置方块状态（使用 BlockPos，带标志）
     * @param pos 方块位置
     * @param state 方块状态
     * @param flags 更新标志
     * @return 是否成功
     */
    bool setBlockState(const BlockPos& pos, const BlockState* state, i32 flags)
    {
        return setBlockState(pos.x, pos.y, pos.z, state, flags);
    }

    // 使用基类的 setBlockState(const BlockPos&, const BlockState*) 非虚函数
    using IWorldWriter::setBlockState;

    /**
     * @brief 获取方块实体
     * @param pos 方块位置
     * @return 方块实体指针，如果不存在返回 nullptr
     */
    [[nodiscard]] virtual BlockEntity* getBlockEntity(const BlockPos& pos)
    {
        (void)pos;
        return nullptr;
    }
    [[nodiscard]] virtual const BlockEntity* getBlockEntity(const BlockPos& pos) const
    {
        (void)pos;
        return nullptr;
    }

    /**
     * @brief 打开方块容器
     * @param type 容器类型
     * @param pos 方块位置
     * @param player 发起交互的玩家
     * @return 如果成功打开返回true
     */
    [[nodiscard]] virtual bool openContainer(ContainerType type, const BlockPos& pos, Player& player)
    {
        (void)type;
        (void)pos;
        (void)player;
        return false;
    }

    /**
     * @brief 打开实体容器
     *
     * 用于旁观者模式玩家与实体容器交互（如村民交易、矿车容器）。
     *
     * @param provider 命名容器提供者（实体）
     * @param player 发起交互的玩家
     * @return 如果成功打开返回true
     */
    [[nodiscard]] virtual bool openEntityContainer(INamedContainerProvider& provider, Player& player)
    {
        (void)provider;
        (void)player;
        return false;
    }

    /**
     * @brief 设置方块实体
     * @param pos 方块位置
     * @param entity 方块实体指针（获取所有权）
     */
    virtual void setBlockEntity(const BlockPos& pos, BlockEntity* entity)
    {
        (void)pos;
        (void)entity;
    }

    /**
     * @brief 移除方块实体
     * @param pos 方块位置
     */
    virtual void removeBlockEntity(const BlockPos& pos) { (void)pos; }

    // ========== 方块更新 ==========

    /**
     * @brief 通知单个邻居方块更新
     *
     * 通知指定位置的方块其邻居发生了变化。
     * 这是对 Block::neighborChanged 的封装，处理 const Block& 到 Block& 的转换。
     *
     * @param neighborPos 邻居方块位置
     * @param neighborState 邻居方块的当前状态
     * @param sourceBlock 触发更新的源方块
     * @param sourcePos 源方块位置
     * @param isMoving 是否正在移动（活塞等）
     */
    void notifyNeighborChanged(const BlockPos& neighborPos,
        const BlockState& neighborState,
        Block& sourceBlock,
        const BlockPos& sourcePos,
        bool isMoving = false);

    /**
     * @brief 通知相邻方块更新
     *
     * 通知指定位置的所有6个相邻方块发生变化
     *
     * @param pos 发生变化的位置
     * @param sourceBlock 触发变化的方块
     */
    virtual void updateNeighbors(const BlockPos& pos, Block& sourceBlock);

    /**
     * @brief 通知相邻方块更新（排除指定方向）
     *
     * 通知指定位置的所有相邻方块发生变化，排除指定方向
     *
     * @param pos 发生变化的位置
     * @param sourceBlock 触发变化的方块
     * @param except 排除的方向
     */
    virtual void updateNeighborsExcept(const BlockPos& pos, Block& sourceBlock, Direction except);

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
    [[nodiscard]] virtual const fluid::FluidState* getFluidState(const BlockPos& pos) const
    {
        return getFluidState(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 检查位置是否有流体
     */
    [[nodiscard]] bool hasFluid(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否有流体（使用 BlockPos）
     */
    [[nodiscard]] virtual bool hasFluid(const BlockPos& pos) const { return hasFluid(pos.x, pos.y, pos.z); }

    /**
     * @brief 检查位置是否为水
     */
    [[nodiscard]] bool isWaterAt(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否为水（使用 BlockPos）
     */
    [[nodiscard]] virtual bool isWaterAt(const BlockPos& pos) const { return isWaterAt(pos.x, pos.y, pos.z); }

    /**
     * @brief 检查位置是否为岩浆
     */
    [[nodiscard]] bool isLavaAt(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否为岩浆（使用 BlockPos）
     */
    [[nodiscard]] virtual bool isLavaAt(const BlockPos& pos) const { return isLavaAt(pos.x, pos.y, pos.z); }

    /**
     * @brief 检查碰撞箱范围内是否包含任何流体
     *
     * 遍历碰撞箱覆盖的所有方块位置，检查是否存在流体方块。
     * 用于判断实体生成位置是否在液体中（如僵尸增援生成时排除水中位置）。
     *
     * @param box 碰撞箱
     * @return 是否包含流体
     */
    [[nodiscard]] bool containsAnyLiquid(const AxisAlignedBB& box) const;

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

    /**
     * @brief 同步获取或加载区块
     *
     * 对应 MC Java 的 Level.getChunk(x, z, require=true)：
     * 如果区块已加载则直接返回，否则同步触发区块加载/生成。
     *
     * 仅在服务端主线程调用安全（与 requestFullChunkSync 同样的约束）。
     * 客户端和其他不支持的实现返回 nullptr。
     *
     * @param x, z 区块坐标
     * @return 区块数据指针，如果无法加载返回 nullptr
     */
    [[nodiscard]] virtual const ChunkData* getOrLoadChunk(ChunkCoord x, ChunkCoord z) { return getChunk(x, z); }

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
        f32 pitch)
    {
        (void)soundEventId;
        (void)category;
        (void)position;
        (void)volume;
        (void)pitch;
    }

    // ========== 世界事件 ==========

    /**
     * @brief 播放世界事件
     *
     * 世界事件是服务端广播给客户端的游戏事件，用于触发音效和粒子效果。
     * 例如：门开关音效、铁砧使用、方块破坏粒子等。
     *
     * @param eventId 事件ID，参见 WorldEvents 命名空间
     * @param pos 事件位置
     * @param data 事件数据（含义因事件而异）
     */
    virtual void playEvent(i32 eventId, const BlockPos& pos, i32 data)
    {
        (void)eventId;
        (void)pos;
        (void)data;
    }

    /**
     * @brief 设置方块破坏进度动画
     *
     * 向客户端广播方块破坏进度动画。
     * 对应 MC Java 中的 Level.destroyBlockProgress(breakerId, pos, progress)。
     * 发送 ClientboundBlockDestructionPacket / BlockBreakAnimPacket。
     *
     * @param breakerId 破坏者实体ID
     * @param pos 方块位置
     * @param progress 破坏进度 (0-9 表示阶段，-1 表示移除动画)
     */
    virtual void destroyBlockProgress(EntityInstanceId breakerId, const BlockPos& pos, i32 progress)
    {
        (void)breakerId;
        (void)pos;
        (void)progress;
    }

    // ========== 游戏事件 ==========

    /**
     * @brief 触发游戏事件
     *
     * 游戏事件是服务端内部事件分发机制，通知附近的 GameEventListener（如幽匿感测体）
     * 有振动信号产生。与 playEvent（世界事件/levelEvent）不同，gameEvent 不会发送
     * 网络包给客户端，而是用于服务端内部的信号传播。
     *
     * 参考 MC: LevelAccessor.gameEvent(Holder<GameEvent>, Vec3, GameEvent.Context)
     *
     * @param event 游戏事件，参见 GameEvents 命名空间
     * @param pos 事件位置
     * @param context 事件上下文（源实体和受影响方块状态）
     */
    virtual void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context)
    {
        (void)event;
        (void)pos;
        (void)context;
    }

    /**
     * @brief 触发游戏事件（仅传入方块状态）
     *
     * 便捷方法，等价于 gameEvent(event, pos, GameEvent::Context::of(blockState))。
     *
     * @param event 游戏事件
     * @param pos 事件位置
     * @param blockState 受影响的方块状态
     */
    void gameEvent(const gameevent::GameEvent& event, const BlockPos& pos, const BlockState* blockState)
    {
        gameEvent(event, pos, gameevent::GameEvent::Context::of(blockState));
    }

    // ========== 方块更新通知 ==========

    /**
     * @brief 通知客户端指定位置的方块（含方块实体）已更新
     *
     * 即使方块状态未改变，也会触发客户端同步。用于方块实体数据变化后
     * 通知客户端刷新显示，例如营火烹饪物品变化、箱子开合状态等。
     *
     * 参考 MC: Level.sendBlockUpdated(pos, oldState, newState, flags)
     *
     * @param pos 方块位置
     */
    virtual void notifyBlockUpdate(const BlockPos& pos) { (void)pos; }

    // ========== 方块事件 ==========

    /**
     * @brief 触发方块事件
     *
     * 方块事件用于服务端向客户端同步方块动画和状态变化。
     * 服务端将事件加入队列，每tick处理时验证方块是否仍匹配，
     * 匹配则执行事件并广播给附近客户端。
     *
     * 客户端收到 BlockEventPacket 后，调用 Block::triggerEvent() 处理事件，
     * 默认实现委托给 BlockEntity::triggerEvent()。
     *
     * 参考 MC Java: Level.blockEvent(BlockPos, Block, int, int)
     *
     * 典型用途：
     * - 箱子开合动画：blockEvent(pos, block, 1, openerCount)
     * - 音符盒播放：blockEvent(pos, block, 0, 0)
     * - 活塞伸缩：blockEvent(pos, block, 0/1/2, direction)
     * - 陶罐摇晃：blockEvent(pos, block, 1, wobbleStyle)
     * - 末地折跃门冷却：blockEvent(pos, block, 1, 0)
     *
     * @param pos 方块位置
     * @param block 方块类型（用于验证方块是否仍存在）
     * @param paramA 事件参数A（含义因方块类型而异）
     * @param paramB 事件参数B（含义因方块类型而异）
     */
    virtual void blockEvent(const BlockPos& pos, const Block& block, i32 paramA, i32 paramB)
    {
        (void)pos;
        (void)block;
        (void)paramA;
        (void)paramB;
    }

    // ========== 高度查询 ==========

    /**
     * @brief 获取最低建筑高度
     *
     * 对应 MC Java 版 LevelHeightAccessor.getMinY()。
     * 主世界和末地返回 -64，下界返回 0。
     */
    [[nodiscard]] virtual i32 getMinBuildHeight() const { return world::MIN_BUILD_HEIGHT; }

    /**
     * @brief 获取最高建筑高度（不含）
     *
     * 对应 MC Java 版 LevelHeightAccessor.getMaxY() + 1。
     * 主世界和末地返回 320，下界返回 128。
     * 注意：此值为独占上界，有效方块 Y 范围是 [getMinBuildHeight(), getMaxBuildHeight())。
     */
    [[nodiscard]] virtual i32 getMaxBuildHeight() const { return world::MAX_BUILD_HEIGHT; }

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
    [[nodiscard]] virtual u8 getBlockLight(const BlockPos& pos) const { return getBlockLight(pos.x, pos.y, pos.z); }

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
    [[nodiscard]] virtual u8 getSkyLight(const BlockPos& pos) const { return getSkyLight(pos.x, pos.y, pos.z); }

    /**
     * @brief 获取综合光照等级
     *
     * 计算方块位置的实际光照等级，考虑天空光照衰减。
     * 这是用于作物生长判断的标准方法。
     *
     * @param pos 方块位置
     * @param skyDarkening 天空光照衰减值（0-15，用于天气/时间影响）
     * @return 综合光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getLightSubtracted(const BlockPos& pos, u32 skyDarkening) const
    {
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
     * @brief 获取邻居感知的综合光照等级
     *
     * 这个方法在敌对生物生成检测时使用，特别是在雷暴天气。
     * 如果当前位置可以看到天空（天空光照 >= 15），会检查邻居方块的天空光照。
     * 如果邻居的天空光照更低，则使用更小的天空减暗因子。
     *
     * @param pos 方块位置
     * @param skyDarkening 天空光照衰减值（0-15）
     * @return 综合光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getNeighborAwareLightSubtracted(const BlockPos& pos, u32 skyDarkening) const
    {
        // 检查坐标是否在世界边界内
        if (pos.x < -world::WORLD_BORDER || pos.x >= world::WORLD_BORDER || pos.z < -world::WORLD_BORDER ||
            pos.z >= world::WORLD_BORDER) {
            return 15; // 世界边界外返回最大亮度
        }
        return getLightSubtracted(pos, skyDarkening);
    }

    /**
     * @brief 获取当前位置的综合光照等级
     *
     * 使用当前时间和天气计算的天空减暗因子。
     *
     * @param pos 方块位置
     * @return 综合光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getLight(const BlockPos& pos) const
    {
        return getNeighborAwareLightSubtracted(pos, static_cast<u32>(getSkyDarkening()));
    }

    /**
     * @brief 获取当前天空减暗因子
     *
     * 根据当前时间和天气计算天空减暗因子。
     * 用于计算综合光照等级。
     *
     * @return 天空减暗因子 (0-11)
     */
    [[nodiscard]] virtual i32 getSkyDarkening() const
    {
        // 使用 InternalLightUtils 计算天空减暗因子
        // 根据当前时间和天气状态
        // 注意：InternalLightUtils 函数内部会取模，但传入 dayTimeOfDay() 更清晰
        return InternalLightUtils::calculateSkyDarkening(dayTimeOfDay(), isRaining(), isThundering());
    }

    /**
     * @brief 检查位置是否可以看到天空
     *
     * 检查该位置的天空光照等级是否达到最大值。
     * 只有有天空光照的维度才能看到天空。
     *
     * @param pos 方块位置
     * @return 如果该位置可以看到天空返回 true
     */
    [[nodiscard]] virtual bool canSeeSky(const BlockPos& pos) const
    {
        if (!hasSkyLight()) {
            return false;
        }
        return getSkyLight(pos) >= 15;
    }

    /**
     * @brief 获取位置的最大局部原始亮度
     *
     * 等效于 MC 的 World.getMaxLocalRawBrightness(pos)。
     * 在有天空光照的维度（主世界）中，考虑天气衰减后的天空光照；
     * 在无天空光照的维度（末地）中，仅使用方块光照。
     *
     * 用于霜冰等方块判断是否融化。
     *
     * @param pos 方块位置
     * @return 最大局部原始亮度 (0-15)
     */
    [[nodiscard]] virtual i32 getMaxLocalRawBrightness(const BlockPos& pos) const
    {
        u8 blockLight = getBlockLight(pos);
        if (!hasSkyLight()) {
            // 末地等无天空光照的维度：仅使用方块光照
            return static_cast<i32>(blockLight);
        }
        u8 skyLight = getSkyLight(pos);
        i32 skyDarkening = getSkyDarkening();
        return InternalLightUtils::calculateRawBrightness(blockLight, skyLight, skyDarkening);
    }

    /**
     * @brief 获取位置的亮度因子
     *
     * 根据位置的光照等级计算亮度因子（0.0-1.0）。
     * 用于阴影渲染、生物生成等。
     *
     * @param pos 方块位置
     * @return 亮度因子 (0.0-1.0)
     */
    [[nodiscard]] virtual f32 getBrightness(const BlockPos& pos) const
    {
        // 默认实现：使用 getLightSubtracted(pos, 0) 计算亮度
        u8 light = getLightSubtracted(pos, 0);
        return static_cast<f32>(light) / 15.0f;
    }

    // ========== 方块射线遍历 ==========

    /**
     * @brief 沿直线遍历方块，检查是否有匹配谓词的方块
     *
     * 使用 DDA 算法从 from 到 to 逐格遍历，对每个经过的方块调用谓词检查。
     * 如果谓词返回 true，则返回 true（找到匹配方块）。
     * 如果遍历完成未找到匹配方块，返回 false。
     *
     * 注意：起点所在的方块也会被检查。
     *
     * @param from 起点（世界坐标）
     * @param to 终点（世界坐标）
     * @param predicate 方块状态谓词，返回 true 表示匹配目标方块
     * @return 如果沿路径找到匹配谓词的方块返回 true，否则返回 false
     */
    [[nodiscard]] virtual bool isBlockInLine(
        const Vector3d& from, const Vector3d& to, std::function<bool(const BlockState&)> predicate) const
    {
        (void)from;
        (void)to;
        (void)predicate;
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
    [[nodiscard]] virtual bool isWithinWorldBounds(const BlockPos& pos) const
    {
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
     * @brief 获取本世界的 ECS 实体注册表
     *
     * 实体工厂 EntityType::create(world, registry) 构造实体时，经此 registry 在 ECS 层
     * create 实体并 attach 高频组件。entt 实体不可跨 registry 迁移，故构造时 registry
     * 必须就位——common 层代码（实体自身方法/反序列化/方块实体）无法下转 ServerWorld，
     * 统一经此虚方法取 registry，避免 common 反向依赖 server。
     *
     * ServerWorld 返回其 m_entityRegistry；ClientWorld 首批不接入 ECS，返回 nullptr。
     * 调用方在服务端上下文可断言非空。
     *
     * @return ECS 实体注册表指针，未接入 ECS 的世界返回 nullptr
     */
    [[nodiscard]] virtual ecs::EntityRegistry* entityRegistry() { return nullptr; }
    [[nodiscard]] virtual const ecs::EntityRegistry* entityRegistry() const { return nullptr; }

    /**
     * @brief 生成实体到世界中
     * @param entity 实体实例
     * @return 实体ID，如果失败返回 0
     *
     * 默认实现返回 0（不支持生成实体）。
     * ServerWorld 会重写此方法以实际生成实体。
     */
    virtual EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity);

    /**
     * @brief 通过ID获取实体
     * @param id 实体ID
     * @return 实体指针，如果不存在返回 nullptr
     *
     * 默认实现返回 nullptr。
     */
    [[nodiscard]] virtual Entity* getEntity(EntityInstanceId id)
    {
        (void)id;
        return nullptr;
    }
    [[nodiscard]] virtual const Entity* getEntity(EntityInstanceId id) const
    {
        (void)id;
        return nullptr;
    }

    /**
     * @brief 通过UUID获取实体
     *
     * 利用 EntityManager 的 UUID 索引进行 O(1) 查找，避免全量遍历。
     *
     * 默认实现返回 nullptr。
     *
     * @param uuid 实体UUID字符串
     * @return 实体指针，如果不存在返回 nullptr
     */
    [[nodiscard]] virtual Entity* getEntityByUuid(const std::string& uuid)
    {
        (void)uuid;
        return nullptr;
    }
    [[nodiscard]] virtual const Entity* getEntityByUuid(const std::string& uuid) const
    {
        (void)uuid;
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
        const AxisAlignedBB& box, const Entity* except = nullptr) const = 0;

    /**
     * @brief 获取范围内的所有实体
     * @param pos 中心位置
     * @param range 范围
     * @param except 排除的实体（可选）
     * @return 实体列表
     */
    [[nodiscard]] virtual std::vector<Entity*> getEntitiesInRange(
        const Vector3& pos, f32 range, const Entity* except = nullptr) const = 0;

    /**
     * @brief 获取所有玩家实体
     * @return 玩家实体列表
     */
    [[nodiscard]] virtual std::vector<Entity*> getPlayers() const { return {}; }

    /**
     * @brief 获取指定类型的所有实体
     *
     * 返回世界中所有匹配指定实体类型ID的存活实体。
     * ServerWorld 通过 EntityManager 实现此方法。
     *
     * @param typeId 实体类型字符串（来自 EntityTypeKeys，如 "minecraft:ender_dragon"）
     * @return 匹配类型的实体列表
     */
    [[nodiscard]] virtual std::vector<Entity*> getEntitiesByType(const std::string& typeId) const
    {
        (void)typeId;
        return {};
    }

    // ========== 最近玩家查询 ==========

    /**
     * @brief 获取最近的玩家
     *
     * @param pos 中心位置
     * @param maxDistance 最大距离（-1 表示无限制）
     * @return 最近的玩家指针，如果没有玩家返回 nullptr
     */
    [[nodiscard]] virtual Player* getClosestPlayer(const Vector3& pos, f32 maxDistance = -1.0f)
    {
        (void)pos;
        (void)maxDistance;
        return nullptr;
    }
    [[nodiscard]] virtual const Player* getClosestPlayer(const Vector3& pos, f32 maxDistance = -1.0f) const
    {
        (void)pos;
        (void)maxDistance;
        return nullptr;
    }

    /**
     * @brief 获取最近的玩家（排除特定玩家）
     *
     * @param pos 中心位置
     * @param maxDistance 最大距离（-1 表示无限制）
     * @param exclude 排除的玩家（可以是 nullptr）
     * @return 最近的玩家指针，如果没有玩家返回 nullptr
     */
    [[nodiscard]] virtual Player* getClosestPlayer(const Vector3& pos, f32 maxDistance, const Entity* exclude)
    {
        (void)pos;
        (void)maxDistance;
        (void)exclude;
        return nullptr;
    }
    [[nodiscard]] virtual const Player* getClosestPlayer(
        const Vector3& pos, f32 maxDistance, const Entity* exclude) const
    {
        (void)pos;
        (void)maxDistance;
        (void)exclude;
        return nullptr;
    }

    /**
     * @brief 获取最近玩家距离的平方
     *
     * 这是一个便捷方法，用于快速检查实体与最近玩家的距离。
     * 如果没有玩家，返回 std::numeric_limits<f64>::max()。
     *
     * @param pos 中心位置
     * @return 最近玩家距离的平方，如果没有玩家返回最大值
     */
    [[nodiscard]] virtual f64 getClosestPlayerDistanceSq(const Vector3& pos) const
    {
        (void)pos;
        return std::numeric_limits<f64>::max();
    }

    // ========== 维度信息 ==========

    /**
     * @brief 获取维度 ID
     */
    [[nodiscard]] virtual DimensionId dimension() const = 0;

    /**
     * @brief 是否为超热维度
     *
     * 下界是超热维度，水会蒸发。
     */
    [[nodiscard]] virtual bool isUltraWarm() const
    {
        return dimension() == -1; // NETHER
    }

    /**
     * @brief 是否允许火焰蔓延
     *
     * 当前默认开启，后续接入游戏规则后可由具体世界覆盖。
     */
    [[nodiscard]] virtual bool doFireTick() const { return true; }

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
     * @brief 获取累积的日光时间（可能超过 24000）
     *
     * 注意：此值可能超过 24000。如需一天内的时间 (0-23999)，请使用 dayTimeOfDay()。
     * 用于存储、保存到存档等场景。
     */
    [[nodiscard]] virtual i64 dayTime() const = 0;

    /**
     * @brief 获取当前一天内的时间 (0-23999)
     *
     * 用于天体角度计算、时间显示、睡眠检测等场景。
     */
    [[nodiscard]] virtual i64 dayTimeOfDay() const { return dayTime() % 24000; }

    /**
     * @brief 检查是否为白天
     *
     * dayTimeOfDay() < 12000 为白天 (0-11999 = 白天, 12000-23999 = 夜晚)
     */
    [[nodiscard]] virtual bool isDaytime() const { return dayTimeOfDay() < 12000; }

    /**
     * @brief 检查外面是否明亮
     *
     * MC原版对应: Level.isBrightOutside()
     * 当天空减暗因子小于4时返回true，即天空足够明亮。
     * 与isDaytime()不同，此方法还考虑了天气（雷暴时白天也会变暗），
     * 以及维度类型（下界和末地总是返回false，因为它们有固定时间）。
     * 用于亡灵生物的阳光相关AI（RestrictSunGoal、FleeSunGoal等）。
     */
    [[nodiscard]] virtual bool isBrightOutside() const
    {
        // 没有天空光照的维度（下界、末地）不算明亮
        if (!hasSkyLight()) {
            return false;
        }
        return getSkyDarkening() < 4;
    }

    /**
     * @brief 获取游戏时间 (总tick数)
     *
     * 与 currentTick() 相同，提供更明确的语义。
     */
    [[nodiscard]] virtual u64 getGameTime() const { return currentTick(); }

    /**
     * @brief 检查是否为客户端世界
     * @return 如果是客户端返回true，服务端返回false
     */
    [[nodiscard]] virtual bool isClientSide() const = 0;

    /**
     * @brief 检查维度是否有天空光照
     * @return 如果维度有天空光照返回true（主世界），否则返回false（下界、末地）
     */
    [[nodiscard]] virtual bool hasSkyLight() const
    {
        return dimension() == 0; // OVERWORLD
    }

    // ========== 难度 ==========

    /**
     * @brief 是否困难模式
     */
    [[nodiscard]] virtual bool isHardcore() const = 0;

    /**
     * @brief 获取难度
     */
    [[nodiscard]] virtual Difficulty difficulty() const = 0;

    /**
     * @brief 是否允许玩家对玩家造成伤害（PvP）
     *
     * 读取 PVP 游戏规则判断是否允许 PvP。客户端世界默认返回 true。
     *
     * @return 如果允许 PvP 返回 true
     */
    [[nodiscard]] virtual bool isPvpAllowed() const { return true; }

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
    [[nodiscard]] virtual f32 rainStrength(f32 partialTick = 0.0f) const
    {
        (void)partialTick;
        return 0.0f;
    }

    /**
     * @brief 获取雷暴强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)，用于插值
     * @return 雷暴强度 (0.0 - 1.0)
     */
    [[nodiscard]] virtual f32 thunderStrength(f32 partialTick = 0.0f) const
    {
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
    [[nodiscard]] virtual bool canRainAt(const BlockPos& pos) const
    {
        (void)pos;
        return false;
    }

    // ========== 闪电闪烁效果 ==========

    /**
     * @brief 设置闪电闪烁时间
     *
     * 当闪电击中时调用，产生天空闪烁效果。
     * 注意：只有客户端世界需要实现此方法。
     *
     * @param time 闪烁时间（ticks），通常为 2
     */
    virtual void setTimeLightningFlash(i32 time)
    {
        (void)time;
        // 默认实现：无操作（服务端世界不需要实现）
    }

    /**
     * @brief 获取当前闪电闪烁时间
     *
     * @return 当前闪烁时间（ticks），0表示无闪烁
     */
    [[nodiscard]] virtual i32 lightningFlashTime() const { return 0; }

    /**
     * @brief 检查碰撞箱是否无碰撞
     *
     * 检查指定碰撞箱是否与世界中的方块或实体发生碰撞。
     *
     * @param box 碰撞箱
     * @return 如果无碰撞返回true
     */
    [[nodiscard]] virtual bool hasNoCollisions(const AxisAlignedBB& box) const
    {
        (void)box;
        return true;
    }

    /**
     * @brief 获取随机数生成器
     *
     * 返回世界的随机数生成器，用于生成随机数。
     *
     * @return 随机数生成器引用
     */
    [[nodiscard]] virtual math::Random& getRandom() = 0;
    [[nodiscard]] virtual const math::Random& getRandom() const = 0;

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
    virtual void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity)
    {
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
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count)
    {
        (void)type;
        (void)pos;
        (void)velocity;
        (void)offset;
        (void)count;
    }

    /**
     * @brief 生成带颜色的实体效果粒子
     *
     * 用于 BellBlockEntity 等需要携带 ARGB 颜色的 EntityEffect 粒子场景。
     * 服务端：广播给附近玩家（携带颜色数据）
     * 客户端：默认回退到普通 addParticle（忽略颜色）
     *
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param offset 随机偏移范围
     * @param count 粒子数量
     * @param color 粒子颜色（ARGB 格式，如 0xFFFF0000 为红色）
     */
    virtual void addEntityEffectParticle(
        const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color)
    {
        (void)pos;
        (void)velocity;
        (void)offset;
        (void)count;
        (void)color;
    }

    /**
     * @brief 生成方块粒子（携带方块状态）
     *
     * 用于 Block/Breaking/FallingDust 等需要方块纹理的粒子。
     * 服务端：广播 LevelParticles（ParticleOptions(Block) 携带 blockStateId）给附近玩家
     * 客户端：调用 ClientWorld::addBlockParticle 直接生成
     *
     * @param type 粒子类型（必须为 requiresBlockState 返回 true 的类型）
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param blockState 方块状态（用于粒子纹理和颜色）
     */
    virtual void addBlockParticle(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const BlockState& blockState)
    {
        (void)type;
        (void)pos;
        (void)velocity;
        (void)blockState;
    }

    /**
     * @brief 生成物品粒子（携带物品堆）
     *
     * 用于 Item/ItemSlime/ItemCobweb/ItemSnowball 等需要物品纹理的粒子。
     * 服务端：广播 LevelParticles（ParticleOptions(Item) 携带 ItemStack）给附近玩家
     * 客户端：通过粒子数据管线调用 ItemParticle::createWithItemStack 生成
     *
     * @param type 粒子类型（必须为 requiresItemData 返回 true 的类型）
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param itemStack 物品堆（用于粒子纹理）
     */
    virtual void addItemParticle(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const ItemStack& itemStack)
    {
        (void)type;
        (void)pos;
        (void)velocity;
        (void)itemStack;
    }

    /**
     * @brief 生成轨迹粒子（Trail Particle）
     *
     * 用于眼眸花状态切换等场景。Trail 粒子会从 pos 飞向 targetPosition，
     * 颜色与持续时间由调用方指定。
     *
     * 服务端：广播给附近玩家（携带目标位置、颜色、持续时间）
     * 客户端：默认无操作（客户端粒子由 animateTick 自行处理）
     *
     * @param pos 粒子起始位置
     * @param targetPosition 粒子飞向的目标位置
     * @param color 粒子颜色（ARGB 格式）
     * @param durationInTicks 飞行持续时间（tick 数）
     */
    virtual void addTrailParticle(const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks)
    {
        (void)pos;
        (void)targetPosition;
        (void)color;
        (void)durationInTicks;
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
    [[nodiscard]] virtual bool shouldSpawnParticleAt(const Vector3& pos, f32 maxDistance = 256.0f) const
    {
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
    virtual void createExplosion(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode = world::explosion::ExplosionMode::Destroy,
        bool causesFire = false,
        Entity* source = nullptr)
    {
        // 默认空操作，ServerWorld 会重写以实际执行爆炸
        (void)position;
        (void)radius;
        (void)mode;
        (void)causesFire;
        (void)source;
    }

    /**
     * @brief 创建带自定义伤害来源的爆炸
     *
     * 与 createExplosion 相同，但允许指定自定义 DamageSource，
     * 用于爆炸伤害归因（如 TNT 矿车将引爆者归因为间接爆炸来源）。
     *
     * @param position 爆炸中心位置
     * @param radius 爆炸半径
     * @param mode 爆炸模式
     * @param causesFire 是否生成火焰
     * @param source 爆炸源实体（可选）
     * @param damageSource 自定义伤害来源（可选，为 nullptr 时使用默认爆炸伤害）。
     *                     调用者保留所有权，实现会在内部 clone 一份。
     */
    virtual void createExplosionWithSource(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode,
        bool causesFire,
        Entity* source,
        const DamageSource* damageSource)
    {
        // 默认实现：忽略 damageSource，退回到无自定义伤害来源版本
        (void)damageSource;
        createExplosion(position, radius, mode, causesFire, source);
    }

    /**
     * @brief 创建带自定义爆炸上下文的爆炸
     *
     * 允许调用者传入自定义的 ExplosionContext，以控制爆炸对方块的行为。
     * 例如蓝色凋灵之首使用 WitherSkullExplosionContext 来穿透高抗性方块。
     *
     * @param position 爆炸中心位置
     * @param radius 爆炸半径
     * @param mode 爆炸模式
     * @param causesFire 是否生成火焰
     * @param source 爆炸源实体（可选）
     * @param context 自定义爆炸上下文（必须非空）
     */
    virtual void createExplosionWithContext(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode,
        bool causesFire,
        Entity* source,
        std::unique_ptr<world::explosion::ExplosionContext> context)
    {
        // 默认实现：忽略自定义 context，退回到普通爆炸
        (void)context;
        createExplosion(position, radius, mode, causesFire, source);
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

    // ========== 按需特征放置 ==========

    /**
     * @brief 从已加载区块构建临时 WorldGenRegion
     *
     * 在指定位置周围收集 3x3 区块窗口，构建 WorldGenRegion，
     * 用于 SaplingBlock::grow() 等按需特征放置场景。
     *
     * 只有 ServerWorld 会返回有效的 WorldGenRegion，
     * 客户端和其他实现返回 nullptr。
     *
     * @param position 中心位置
     * @return 创建的 WorldGenRegion，如果区块未加载则返回 nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<WorldGenRegion> createFeatureRegion(const BlockPos& /*position*/)
    {
        return nullptr;
    }

    // ========== 村庄管理 ==========

    /**
     * @brief 获取村庄管理器
     *
     * 只有ServerWorld会返回有效的指针，其他实现返回nullptr。
     *
     * @return VillageManager指针，如果不存在返回nullptr
     */
    [[nodiscard]] virtual world::village::VillageManager* villageManager() { return nullptr; }
    [[nodiscard]] virtual const world::village::VillageManager* villageManager() const { return nullptr; }

    // ========== 袭击管理 ==========

    /**
     * @brief 获取袭击管理器
     *
     * 只有ServerWorld会返回有效的指针，其他实现返回nullptr。
     *
     * @return RaidManager指针，如果不存在返回nullptr
     */
    [[nodiscard]] virtual world::village::raid::RaidManager* raidManager() { return nullptr; }
    [[nodiscard]] virtual const world::village::raid::RaidManager* raidManager() const { return nullptr; }

    // ========== 末影龙战斗管理 ==========

    /**
     * @brief 获取末影龙战斗管理器
     *
     * 只有末地维度的ServerWorld会返回有效的指针，其他实现返回nullptr。
     * 用于末影龙死亡后放置龙蛋、生成折跃门等战斗奖励逻辑。
     *
     * @return EndDragonFight指针，如果不存在返回nullptr
     */
    [[nodiscard]] virtual class EndDragonFight* dragonFight() { return nullptr; }
    [[nodiscard]] virtual const class EndDragonFight* dragonFight() const { return nullptr; }

    // ========== 战利品表管理 ==========

    /**
     * @brief 获取战利品表管理器
     *
     * 只有ServerWorld会返回有效的指针，其他实现返回nullptr。
     * 用于方块实体填充战利品表。
     *
     * @return LootTableManager指针，如果不存在返回nullptr
     */
    [[nodiscard]] virtual const loot::LootTableManager* lootTableManager() const { return nullptr; }

    // ========== 地图数据管理 ==========

    /**
     * @brief 获取地图数据管理器
     *
     * 只有ServerWorld会返回有效的指针，其他实现返回nullptr。
     * 用于地图物品获取和更新地图数据。
     *
     * @return MapDataManager指针，如果不存在返回nullptr
     */
    [[nodiscard]] virtual world::map::MapDataManager* mapDataManager() { return nullptr; }
    [[nodiscard]] virtual const world::map::MapDataManager* mapDataManager() const { return nullptr; }

    // ========== 世界边界 ==========

    /**
     * @brief 获取世界边界
     *
     * 返回世界边界对象，用于边界检测和伤害计算。
     *
     * @return 世界边界引用
     */
    [[nodiscard]] virtual world::border::WorldBorder& worldBorder() = 0;
    [[nodiscard]] virtual const world::border::WorldBorder& worldBorder() const = 0;

    // ========== 实体状态广播 ==========

    /**
     * @brief 广播实体状态事件
     *
     * 向所有追踪该实体的玩家发送实体状态事件。
     * 用于触发客户端的动画、音效等效果。
     *
     * @param entityId 实体ID
     * @param status 状态码（如 network::EntityStatus::GuardianAttack）
     */
    virtual void broadcastEntityStatus(EntityInstanceId entityId, u8 status)
    {
        (void)entityId;
        (void)status;
    }

    /**
     * @brief 广播实体动画事件
     *
     * 向所有追踪该实体的玩家发送实体动画事件。
     * 用于触发客户端的动画效果，如暴击粒子、挥动手臂等。
     *
     * @param entityId 实体ID
     * @param animation 动画类型（如 network::EntityAnimation::CriticalEffect）
     */
    virtual void broadcastEntityAnimation(EntityInstanceId entityId, u8 animation)
    {
        (void)entityId;
        (void)animation;
    }

    /**
     * @brief 广播实体受伤动画（携带受伤方向，对应 MC ClientboundHurtAnimationPacket）
     *
     * 向所有追踪该实体的玩家发送 TakeDamage 动画包并附带 hurtDir，
     * 客户端据此设置 damageTilt 的 hurtDir（屏幕倾斜方向）。
     *
     * @param entityId 受伤实体ID
     * @param hurtDir 受伤方向角（度，相对实体朝向）
     */
    virtual void broadcastHurtAnimation(EntityInstanceId entityId, f32 hurtDir)
    {
        (void)entityId;
        (void)hurtDir;
    }

    // ========== 实体拴绳链接广播 ==========

    /**
     * @brief 广播实体拴绳链接变更事件
     *
     * 向所有追踪该实体的玩家发送 ir::play::SetEntityLink，
     * 用于客户端拴绳绳索的渲染同步。
     *
     * @param entityId 被拴实体的ID
     * @param linkedEntityId 拴绳持有者实体ID（0 表示解除拴绳）
     */
    virtual void broadcastSetEntityLink(EntityInstanceId entityId, EntityInstanceId linkedEntityId)
    {
        (void)entityId;
        (void)linkedEntityId;
    }

    // ========== 实体乘客广播 ==========

    /**
     * @brief 广播实体乘客列表变更事件
     *
     * 向所有追踪该载具的玩家发送 ir::play::SetPassengers，用于客户端骑乘关系
     * （船载人、骑马等）的渲染同步。在 Entity::addPassenger/removePassenger
     * 改变载具乘客列表后调用。
     *
     * @param vehicleId 载具实体ID
     */
    virtual void broadcastPassengersChanged(EntityInstanceId vehicleId) { (void)vehicleId; }

    // ========== 爆炸事件广播 ==========

    /**
     * @brief 广播爆炸事件给附近玩家
     *
     * 向爆炸点附近（默认 64 格）的玩家发送 1.21.11 Explosion IR，
     * 包含爆炸位置、威力、受影响方块列表以及每个玩家的击退向量。
     *
     * ServerWorld 重写此方法，通过 `m_onBroadcastExplosion` 回调委托给
     * MinecraftServer::broadcastExplosionInRange 进行实际的网络发送；
     * 其他实现（如 WorldGenRegion）默认为空操作。
     *
     * 与 MC Java 的 ServerLevel.explode() 行为对应：爆炸完成后，
     * 遍历 64 格内的玩家，根据 hitPlayers 映射发送 ClientboundExplodePacket。
     *
     * @param position 爆炸中心
     * @param strength 爆炸威力（半径）
     * @param affectedBlocks 受爆炸影响的方块位置列表
     * @param playerKnockback 每个玩家对应的击退向量，键为玩家实体ID
     */
    virtual void broadcastExplosion(const Vector3& position,
        f32 strength,
        const std::vector<BlockPos>& affectedBlocks,
        const std::unordered_map<u64, Vector3>& playerKnockback)
    {
        (void)position;
        (void)strength;
        (void)affectedBlocks;
        (void)playerKnockback;
    }

    // ========== 睡眠系统 ==========

    /**
     * @brief 通知世界玩家睡眠状态变化
     *
     * 当玩家开始或停止睡眠时调用。默认空实现。
     * 全员睡眠判定已上提到 MinecraftServer::checkAllPlayersSleeping（每 tick 跨维度聚合轮询），
     * 各 ServerWorld 不再重写此方法，本接口保留供 Player 基类调用兼容。
     */
    virtual void onPlayerSleepingChanged()
    {
        // 默认空实现
    }

    // ========== 进度触发 ==========

    /**
     * @brief 通知世界方块被放置
     *
     * 当玩家成功放置方块时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 BlockPlaceEvent。
     * ClientWorld 和其他实现返回空实现。
     *
     * @param playerId 放置方块的玩家ID（可能为0表示非玩家放置）
     * @param pos 方块位置
     * @param state 放置的方块状态
     * @param item 用于放置的物品（可能为null）
     */
    virtual void onBlockPlaced(PlayerId playerId, const BlockPos& pos, const BlockState* state, const ItemStack* item)
    {
        (void)playerId;
        (void)pos;
        (void)state;
        (void)item;
        // 默认空实现
    }

    /**
     * @brief 通知世界僵尸村民被治愈
     *
     * 当僵尸村民被治愈时调用，用于触发进度检测和村庄声望更新。
     * ServerWorld 重写此方法来发布 CuredZombieVillagerEvent。
     * ClientWorld 和其他实现返回空实现。
     *
     * @param starterUuid 治愈发起者玩家UUID（可能为空）
     * @param zombie 治愈前的僵尸村民实体
     * @param villager 治愈后的村民实体
     */
    virtual void onZombieVillagerCured(const std::string& starterUuid, Entity* zombie, Entity* villager)
    {
        (void)starterUuid;
        (void)zombie;
        (void)villager;
        // 默认空实现
    }

    /**
     * @brief 通知世界引雷附魔触发
     *
     * 当玩家使用引雷附魔的三叉戟召唤闪电击中实体时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 ChanneledLightningEvent。
     * ClientWorld 和其他实现返回空实现。
     *
     * @param casterId 施法者ID（引雷附魔的玩家）
     * @param victims 被闪电击中的实体列表
     */
    virtual void onChanneledLightning(PlayerId casterId, const std::vector<Entity*>& victims)
    {
        (void)casterId;
        (void)victims;
        // 默认空实现
    }

    /**
     * @brief 通知世界玩家物品销毁
     *
     * 当玩家物品因使用而损坏或消耗完毕时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 PlayerDestroyItemEvent。
     * ClientWorld 和其他实现返回空实现。
     *
     * @param playerId 玩家ID
     * @param item 销毁前的物品副本
     * @param slot 物品所在槽位（主手=0，副手=40，其他为物品栏槽位，-1表示未知）
     * @param hand 使用的手（MainHand 或 OffHand）
     */
    virtual void onPlayerDestroyItem(PlayerId playerId, const ItemStack& item, i32 slot, Hand hand)
    {
        (void)playerId;
        (void)item;
        (void)slot;
        (void)hand;
        // 默认空实现
    }

    /**
     * @brief 通知世界玩家消耗物品
     *
     * 当玩家消耗物品（如吃食物、喝药水）时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 ConsumeItemEvent。
     *
     * @param playerId 玩家ID
     * @param item 消耗的物品
     */
    virtual void onConsumeItem(PlayerId playerId, const ItemStack& item)
    {
        (void)playerId;
        (void)item;
        // 默认空实现
    }

    /**
     * @brief 通知世界物品耐久度变化
     *
     * 当物品耐久度变化时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 ItemDurabilityEvent。
     *
     * @param playerId 玩家ID
     * @param item 物品
     * @param oldDurability 旧耐久度
     * @param newDurability 新耐久度
     */
    virtual void onItemDurabilityChange(PlayerId playerId, const ItemStack& item, i32 oldDurability, i32 newDurability)
    {
        (void)playerId;
        (void)item;
        (void)oldDurability;
        (void)newDurability;
        // 默认空实现
    }

    /**
     * @brief 通知世界附魔完成
     *
     * 当玩家附魔物品时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 EnchantItemEvent。
     *
     * @param playerId 玩家ID
     * @param item 附魔的物品
     * @param levels 消耗的经验等级
     */
    virtual void onEnchantItem(PlayerId playerId, const ItemStack& item, i32 levels)
    {
        (void)playerId;
        (void)item;
        (void)levels;
        // 默认空实现
    }

    /**
     * @brief 通知世界桶填充完成
     *
     * 当玩家用桶装液体时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 FilledBucketEvent。
     *
     * @param playerId 玩家ID
     * @param bucket 填充后的桶物品
     */
    virtual void onFilledBucket(PlayerId playerId, const ItemStack& bucket)
    {
        (void)playerId;
        (void)bucket;
        // 默认空实现
    }

    /**
     * @brief 玩家进入方块事件回调
     *
     * 当玩家进入方块碰撞箱时调用。
     * ServerWorld 重写此方法来发布 EnterBlockEvent。
     *
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param state 方块状态
     */
    virtual void onEnterBlock(PlayerId playerId, const BlockPos& pos, const BlockState* state)
    {
        (void)playerId;
        (void)pos;
        (void)state;
        // 默认空实现
    }

    /**
     * @brief 玩家在方块上滑落事件回调
     *
     * 当玩家在方块上滑落（如蜂蜜块）时调用。
     * ServerWorld 重写此方法来发布 SlideDownBlockEvent。
     *
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param state 方块状态
     */
    virtual void onSlideDownBlock(PlayerId playerId, const BlockPos& pos, const BlockState* state)
    {
        (void)playerId;
        (void)pos;
        (void)state;
        // 默认空实现
    }

    /**
     * @brief 蜂巢破坏事件回调
     *
     * 当玩家破坏蜂巢/蜂箱时调用。
     * ServerWorld 重写此方法来发布 BeeNestDestroyedEvent。
     *
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param state 方块状态
     * @param tool 使用的工具
     * @param numBeesInside 蜂巢内的蜜蜂数量
     */
    virtual void onBeeNestDestroyed(
        PlayerId playerId, const BlockPos& pos, const BlockState* state, const ItemStack& tool, i32 numBeesInside)
    {
        (void)playerId;
        (void)pos;
        (void)state;
        (void)tool;
        (void)numBeesInside;
        // 默认空实现
    }

    /**
     * @brief 通知世界动物繁殖
     *
     * 当动物繁殖产生幼体时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 BredAnimalsEvent。
     * ClientWorld 和其他实现返回空实现。
     *
     * @param playerId 繁殖发起者玩家ID（喂食动物的玩家）
     * @param child 幼体实体
     * @param parent1 父母1
     * @param parent2 父母2
     */
    virtual void onBredAnimals(PlayerId playerId, Entity* child, Entity* parent1, Entity* parent2)
    {
        (void)playerId;
        (void)child;
        (void)parent1;
        (void)parent2;
        // 默认空实现
    }

    /**
     * @brief 通知世界玩家与村民完成交易
     *
     * 当玩家与村民（或流浪商人）完成交易时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 VillagerTradeEvent。
     * ClientWorld 和其他实现返回空实现。
     *
     * @param playerId 交易玩家ID
     * @param villager 商人实体（村民或流浪商人）
     * @param resultItem 交易结果物品（玩家获得的物品，即玩家买到的）
     * @param paymentItem 交易支付物品（玩家付出的物品，即玩家卖出的）
     */
    virtual void onVillagerTrade(
        PlayerId playerId, Entity* villager, const ItemStack& resultItem, const ItemStack& paymentItem)
    {
        (void)playerId;
        (void)villager;
        (void)resultItem;
        (void)paymentItem;
        // 默认空实现
    }

    /**
     * @brief 通知世界动物被驯服
     *
     * 当玩家成功驯服动物时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 TameAnimalEvent。
     * ClientWorld 和其他实现返回空实现。
     *
     * @param playerId 驯服动物的玩家ID
     * @param animal 被驯服的动物实体
     */
    virtual void onTameAnimal(PlayerId playerId, Entity* animal)
    {
        (void)playerId;
        (void)animal;
        // 默认空实现
    }

    /**
     * @brief 通知世界实体被召唤
     *
     * 当玩家通过建造或命令召唤实体时调用，用于触发进度检测。
     * ServerWorld 重写此方法来发布 SummonedEntityEvent。
     * ClientWorld 和其他实现返回空实现。
     *
     * 参考 MC: CriteriaTriggers.SUMMONED_ENTITY.trigger()
     * 触发场景：建造铁傀儡/雪傀儡、建造凋灵、重生末影龙、/summon 命令
     *
     * @param playerId 召唤实体的玩家ID
     * @param entity 被召唤的实体
     */
    virtual void onSummonedEntity(PlayerId playerId, Entity* entity)
    {
        (void)playerId;
        (void)entity;
        // 默认空实现
    }

    // ========== 结构定位 ==========

    /**
     * @brief 查找最近的结构
     *
     * 在指定范围内搜索指定类型结构的最近位置。
     * 服务端世界实现此方法，客户端世界返回空。
     *
     * @param center 搜索中心位置
     * @param structureId 结构资源位置 ID（如 minecraft:village_plains）
     * @param maxDistance 最大搜索距离（格）
     * @param skipExisting 是否跳过已找到的结构（用于定位命令的多次搜索）
     * @return 最近结构位置，如果未找到返回空
     */
    [[nodiscard]] virtual std::optional<BlockPos> findNearestStructure(
        const BlockPos& center, const ResourceLocation& structureId, i32 maxDistance, bool skipExisting = false)
    {
        (void)center;
        (void)structureId;
        (void)maxDistance;
        (void)skipExisting;
        return std::nullopt;
    }

    /**
     * @brief 按结构标签查找最近的结构
     *
     * 对应 MC 1.21.11 ServerLevel.findNearestMapStructure(TagKey<Structure>, BlockPos, int, boolean)。
     *
     * 与 findNearestStructure 不同，此方法接受结构标签（如 minecraft:dolphin_located），
     * 遍历标签中的所有结构 ID，对每个结构调用 findNearestStructure，返回最近的位置。
     *
     * 服务端世界实现此方法，客户端世界返回空。
     *
     * @param center 搜索中心位置
     * @param tagId 结构标签资源位置（如 minecraft:dolphin_located）
     * @param maxDistance 最大搜索距离（格）
     * @param skipExisting 是否跳过已找到的结构
     * @return 最近结构位置，如果未找到返回空
     */
    [[nodiscard]] virtual std::optional<BlockPos> findNearestMapStructure(
        const BlockPos& center, const ResourceLocation& tagId, i32 maxDistance, bool skipExisting = false)
    {
        (void)center;
        (void)tagId;
        (void)maxDistance;
        (void)skipExisting;
        return std::nullopt;
    }

    // ========== 命令执行 ==========

    /**
     * @brief 执行命令
     *
     * 在世界中以指定位置和权限级别执行命令。
     * ServerWorld 会通过 CommandRegistry 执行命令。
     * ClientWorld 和其他实现返回空实现（返回0）。
     *
     * @param command 命令字符串（可包含或不包含 '/' 前缀）
     * @param position 命令执行位置
     * @param permissionLevel 权限级别（0-4，命令方块矿车使用2）
     * @param rotation 命令源朝向 (pitch, yaw)，用于 `^` 局部坐标解析。
     *                 命令方块传 (0,0)（基岩命令方块 `^` forward 固定朝南 +Z），
     *                 实体（矿车/玩家）应传自身朝向，控制台/无朝向源传 (0,0)。
     * @param player 执行命令的玩家实体（可选）。非空时命令源 isPlayer()=true，
     *               解锁需玩家源的命令（/tp <coords> 传自己、/effect give @s、/give @s 等）。
     *               命令方块/告示牌/控制台传 nullptr。SimulatedPlayer::chat 传自身。
     * @return 命令执行结果码（成功返回正整数，失败返回0）
     */
    [[nodiscard]] virtual i32 executeCommand(const std::string& command,
        const Vector3d& position,
        i32 permissionLevel,
        const Vector2f& rotation = Vector2f(0.0f, 0.0f),
        Player* player = nullptr)
    {
        (void)command;
        (void)position;
        (void)permissionLevel;
        (void)rotation;
        (void)player;
        return 0;
    }

    // ========== 游戏规则 ==========

    /**
     * @brief 获取游戏规则管理器（只读）
     *
     * 游戏规则控制世界行为，如 mobGriefing、naturalRegeneration 等。
     * ServerWorld 返回有效的 GameRules 实例。
     * ClientWorld 和其他实现应返回默认规则。
     *
     * @return GameRules 常引用
     */
    [[nodiscard]] virtual const world::gamerule::GameRules& getGameRules() const;

    /**
     * @brief 获取游戏规则管理器（可变）
     *
     * @return GameRules 引用
     */
    [[nodiscard]] virtual world::gamerule::GameRules& getGameRules();

protected:
    IWorld() = default;
};

/**
 * @brief 区块读取器接口
 *
 * IBlockReader 继承自 IWorld，用于表示只读的方块访问接口。
 */
class IBlockReader : public IWorld {};

/**
 * @brief 世界读取器接口
 *
 * IWorldReader 是 IWorld 的别名，用于表示只读的世界访问接口。
 */
using IWorldReader = IWorld;

} // namespace mc
