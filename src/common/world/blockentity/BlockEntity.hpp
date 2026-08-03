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
#include "resource/ResourceLocation.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/nbt/Nbt.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class BlockState;

/**
 * @brief 方块实体基类
 *
 * 方块实体是与特定方块位置关联的额外数据容器。
 * 用于存储方块状态无法表示的复杂数据，如：
 * - 容器内容（箱子、漏斗）
 * - 工作状态（熔炉、酿造台）
 * - 自定义文本（告示牌）
 * - 红石逻辑（命令方块）
 *
 * 生命周期：
 * 1. 方块放置时创建
 * 2. 存储在区块的方块实体列表中
 * 3. 方块移除时销毁
 *
 * 线程安全：
 * - tick() 方法可能在服务器线程调用
 * - load()/save() 可能在世界保存线程调用
 * - 需要子类自行处理线程同步
 */
class BlockEntity {
public:
    virtual ~BlockEntity() = default;

    /**
     * @brief 获取方块实体类型
     * @return 方块实体类型
     */
    [[nodiscard]] BlockEntityType getType() const noexcept { return m_type; }

    /**
     * @brief 获取方块位置
     * @return 世界坐标位置
     */
    [[nodiscard]] BlockPos getPos() const noexcept { return m_pos; }

    /**
     * @brief 获取世界引用
     * @return 世界引用，可能为nullptr
     */
    [[nodiscard]] IWorld* getWorld() const noexcept { return m_world; }

    /**
     * @brief 设置世界引用
     * @param world 世界引用
     */
    void setWorld(IWorld* world) { m_world = world; }

    /**
     * @brief 从JSON加载数据
     * @param data JSON数据
     * @return 是否成功
     *
     * 从区块数据加载方块实体状态。
     * 子类应重写此方法以加载自定义数据。
     */
    virtual bool load(const nlohmann::json& data)
    {
        MC_UNUSED(data);
        return true;
    }

    /**
     * @brief 保存数据到JSON
     * @param data 输出JSON数据
     *
     * 保存方块实体状态到区块数据。
     * 子类应重写此方法以保存自定义数据。
     */
    virtual void save(nlohmann::json& data) const
    {
        data["id"] = blockEntityTypeToId(m_type).toString();
        data["x"] = m_pos.x;
        data["y"] = m_pos.y;
        data["z"] = m_pos.z;
    }

    /**
     * @brief 从NBT加载数据
     * @param tag NBT复合标签
     * @return 是否成功
     *
     * 从结构模板加载方块实体状态。
     * 子类应重写此方法以加载自定义NBT数据。
     * 用于Jigsaw结构生成时恢复方块实体数据。
     */
    virtual bool loadFromNBT(const nbt::CompoundTag& tag);

    /**
     * @brief 保存数据到NBT
     * @param tag 输出NBT复合标签
     *
     * 保存方块实体状态到结构模板。
     * 子类应重写此方法以保存自定义NBT数据。
     */
    virtual void saveToNBT(nbt::CompoundTag& tag) const;

    /**
     * @brief 获取用于客户端同步的 NBT 数据快照
     * @return NBT 复合标签
     *
     * 服务端在方块实体数据变化后通过此方法生成发送给客户端的 NBT 快照。
     * 默认实现调用 saveToNBT() 写入完整数据（包括 id/x/y/z 公共字段），
     * 子类可重写以提供精简的更新数据（参考 MC Java 的 getUpdateTag()）。
     *
     * 参考 MC Java: BlockEntity.getUpdateTag(HolderLookup.Provider)
     */
    [[nodiscard]] virtual nbt::CompoundTag getUpdateTag() const;

    /**
     * @brief 每tick更新
     * @param world 所在世界
     *
     * 服务端每游戏tick调用一次。
     * 用于处理熔炉燃烧、漏斗传输等逻辑。
     */
    virtual void tick(IWorld& world) { MC_UNUSED(world); }

    /**
     * @brief 检查是否需要tick
     * @return 如果需要每tick更新返回true
     *
     * 用于优化性能，静态方块实体可以返回false。
     */
    [[nodiscard]] virtual bool needsTick() const noexcept { return false; }

    /**
     * @brief 获取方块实体的方块状态
     * @return 方块状态，如果不存在返回nullptr
     */
    [[nodiscard]] const BlockState* getBlockState() const;

    /**
     * @brief 检查是否已修改
     * @return 如果已修改返回true
     */
    [[nodiscard]] bool isChanged() const noexcept { return m_changed; }

    /**
     * @brief 清除修改标记
     *
     * 在保存后调用。
     */
    void clearChanged() noexcept { m_changed = false; }

    /**
     * @brief 检查方块实体是否已被移除
     * @return 如果已被移除返回true
     */
    [[nodiscard]] bool isRemoved() const noexcept { return m_removed; }

    /**
     * @brief 标记方块实体为已移除
     *
     * 当方块实体从世界中移除时调用。
     * 子类可重写此方法进行清理工作。
     */
    virtual void remove() { m_removed = true; }

    /**
     * @brief 验证方块实体是否有效
     *
     * 在方块实体添加到世界时调用。
     * 子类可重写此方法进行额外验证。
     */
    virtual void validate() { m_removed = false; }

    /**
     * @brief 标记方块实体已修改并更新世界
     *
     * 触发区块保存和红石比较器更新。
     * 子类在修改数据后应调用此方法。
     */
    void setChanged();

    /**
     * @brief 获取自定义名称
     * @return 自定义名称，如果没有返回空
     *
     * 用于重命名的方块实体（如重命名箱子）。
     */
    [[nodiscard]] virtual std::string getCustomName() const { return ""; }

    /**
     * @brief 设置自定义名称
     * @param name 名称
     */
    virtual void setCustomName(const std::string& name) { MC_UNUSED(name); }

    /**
     * @brief 处理客户端方块事件
     *
     * 当服务端调用 IWorld::blockEvent() 时，事件在服务端执行后广播到客户端。
     * 客户端收到 BlockEventPacket 后调用此方法来触发对应的视觉/动画效果。
     *
     * 子类应重写此方法以处理特定的事件ID和参数。
     * 默认实现返回 false（未处理）。
     *
     * 参考 MC Java: BlockEntity.triggerEvent(int, int)
     *
     * @param id 事件ID（含义因方块实体类型而异）
     * @param type 事件类型/数据（含义因方块实体类型而异）
     * @return 如果事件被成功处理返回 true
     */
    [[nodiscard]] virtual bool triggerEvent(i32 id, i32 type)
    {
        (void)id;
        (void)type;
        return false;
    }

    /**
     * @brief 检查方块实体是否仅允许OP玩家修改NBT数据
     *
     * 参考 MC Java: BlockEntityType.onlyOpCanSetNbt()
     * 在 MC Java 中，CommandBlock、StructureBlock、JigsawBlock、Sign、
     * HangingSign、TrialSpawner、Lectern 等方块实体需要OP权限才能通过
     * 物品NBT设置数据。
     *
     * @return 如果仅OP可修改NBT返回true，默认false
     */
    [[nodiscard]] virtual bool onlyOpsCanSetNbt() const noexcept { return false; }

    /**
     * @brief 创建方块实体的副本
     * @return 副本的unique_ptr
     */
    [[nodiscard]] virtual std::unique_ptr<BlockEntity> clone() const = 0;

protected:
    /**
     * @brief 构造函数
     * @param type 方块实体类型
     * @param pos 方块位置
     */
    BlockEntity(BlockEntityType type, const BlockPos& pos)
        : m_type(type)
        , m_pos(pos)
        , m_world(nullptr)
        , m_changed(false)
        , m_removed(false)
    {}

    BlockEntityType m_type;
    BlockPos m_pos;
    IWorld* m_world;
    bool m_changed;
    bool m_removed;
};

} // namespace mc
