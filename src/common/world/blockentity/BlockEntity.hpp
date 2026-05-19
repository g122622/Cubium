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

#include "resource/ResourceLocation.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include <memory>
#include <nlohmann/json.hpp>

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
    [[nodiscard]] BlockEntityType getType() const { return m_type; }

    /**
     * @brief 获取方块位置
     * @return 世界坐标位置
     */
    [[nodiscard]] BlockPos getPos() const { return m_pos; }

    /**
     * @brief 获取世界引用
     * @return 世界引用，可能为nullptr
     */
    [[nodiscard]] IWorld* getWorld() const { return m_world; }

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
        (void)data;
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
     * @brief 每tick更新
     * @param world 所在世界
     *
     * 服务端每游戏tick调用一次。
     * 用于处理熔炉燃烧、漏斗传输等逻辑。
     */
    virtual void tick(IWorld& world) { (void)world; }

    /**
     * @brief 检查是否需要tick
     * @return 如果需要每tick更新返回true
     *
     * 用于优化性能，静态方块实体可以返回false。
     */
    [[nodiscard]] virtual bool needsTick() const { return false; }

    /**
     * @brief 获取方块实体的方块状态
     * @return 方块状态，如果不存在返回nullptr
     */
    [[nodiscard]] const BlockState* getBlockState() const;

    /**
     * @brief 检查是否已修改
     * @return 如果已修改返回true
     */
    [[nodiscard]] bool isChanged() const { return m_changed; }

    /**
     * @brief 清除修改标记
     *
     * 在保存后调用。
     */
    void clearChanged() { m_changed = false; }

    /**
     * @brief 检查方块实体是否已被移除
     * @return 如果已被移除返回true
     */
    [[nodiscard]] bool isRemoved() const { return m_removed; }

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
    virtual void setCustomName(const std::string& name) { (void)name; }

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
