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
 * The copyright notice and this permission notice shall be included in all
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

#include "common/command/arguments/NbtPath.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// 前向声明
class IWorld;
class Entity;
class BlockEntity;

namespace command {

/**
 * @brief 数据访问器接口
 *
 * 为 /data 命令提供统一的数据访问接口。
 * 支持方块实体、实体和命令存储三种数据源。
 */
class IDataAccessor {
public:
    virtual ~IDataAccessor() = default;

    /**
     * @brief 获取目标的 NBT 数据
     * @return 包含所有数据的复合标签
     * @throws CommandException 如果无法获取数据
     */
    [[nodiscard]] virtual std::unique_ptr<nbt::tags::compound_tag> getData() const = 0;

    /**
     * @brief 合并 NBT 数据到目标
     * @param data 要合并的数据
     * @throws CommandException 如果无法合并数据或数据未改变
     */
    virtual void mergeData(const nbt::tags::compound_tag& data) = 0;

    /**
     * @brief 获取目标的显示名称
     * @return 显示名称（用于命令反馈）
     */
    [[nodiscard]] virtual std::string getDisplayName() const = 0;

    /**
     * @brief 获取数据修改成功的消息
     * @return 消息文本
     */
    [[nodiscard]] virtual std::string getModifiedMessage() const = 0;

    /**
     * @brief 获取查询结果的消息
     * @param nbt 查询到的 NBT 数据
     * @return 消息文本
     */
    [[nodiscard]] virtual std::string getQueryMessage(const nbt::tags::tag& nbt) const = 0;

    /**
     * @brief 获取带缩放的查询结果消息
     * @param path NBT 路径
     * @param scale 缩放因子
     * @param value 缩放后的值
     * @return 消息文本
     */
    [[nodiscard]] virtual std::string getGetMessage(const NbtPath& path, double scale, i32 value) const = 0;
};

/**
 * @brief 方块实体数据访问器
 *
 * 访问方块实体的 NBT 数据。
 */
class BlockDataAccessor : public IDataAccessor {
public:
    /**
     * @brief 构造函数
     * @param world 世界引用
     * @param pos 方块位置
     */
    BlockDataAccessor(IWorld* world, const BlockPos& pos);

    [[nodiscard]] std::unique_ptr<nbt::tags::compound_tag> getData() const override;
    void mergeData(const nbt::tags::compound_tag& data) override;
    [[nodiscard]] std::string getDisplayName() const override;
    [[nodiscard]] std::string getModifiedMessage() const override;
    [[nodiscard]] std::string getQueryMessage(const nbt::tags::tag& nbt) const override;
    [[nodiscard]] std::string getGetMessage(const NbtPath& path, double scale, i32 value) const override;

    /**
     * @brief 检查位置是否有方块实体
     * @return 如果有方块实体返回 true
     */
    [[nodiscard]] bool isValid() const noexcept { return m_blockEntity != nullptr; }

    /**
     * @brief 获取方块位置
     */
    [[nodiscard]] const BlockPos& getPosition() const noexcept { return m_pos; }

private:
    ::mc::IWorld* m_world;
    ::mc::BlockPos m_pos;
    ::mc::BlockEntity* m_blockEntity;
};

/**
 * @brief 实体数据访问器
 *
 * 访问实体的 NBT 数据。
 */
class EntityDataAccessor : public IDataAccessor {
public:
    /**
     * @brief 构造函数
     * @param entity 实体指针
     */
    explicit EntityDataAccessor(::mc::Entity* entity);

    [[nodiscard]] std::unique_ptr<nbt::tags::compound_tag> getData() const override;
    void mergeData(const nbt::tags::compound_tag& data) override;
    [[nodiscard]] std::string getDisplayName() const override;
    [[nodiscard]] std::string getModifiedMessage() const override;
    [[nodiscard]] std::string getQueryMessage(const nbt::tags::tag& nbt) const override;
    [[nodiscard]] std::string getGetMessage(const NbtPath& path, double scale, i32 value) const override;

    /**
     * @brief 检查实体是否有效
     */
    [[nodiscard]] bool isValid() const noexcept { return m_entity != nullptr; }

    /**
     * @brief 检查是否是玩家实体
     * 玩家实体不允许直接修改 NBT 数据
     */
    [[nodiscard]] bool isPlayer() const;

private:
    ::mc::Entity* m_entity;
};

/**
 * @brief 命令存储数据访问器
 *
 * 访问命令存储（/data storage）的 NBT 数据。
 * 数据存储在世界的维度数据中。
 */
class StorageDataAccessor : public IDataAccessor {
public:
    /**
     * @brief 构造函数
     * @param storage 存储管理器
     * @param id 存储 ID（如 "minecraft:my_storage"）
     */
    StorageDataAccessor(class CommandStorage* storage, const ResourceLocation& id);

    [[nodiscard]] std::unique_ptr<nbt::tags::compound_tag> getData() const override;
    void mergeData(const nbt::tags::compound_tag& data) override;
    [[nodiscard]] std::string getDisplayName() const override;
    [[nodiscard]] std::string getModifiedMessage() const override;
    [[nodiscard]] std::string getQueryMessage(const nbt::tags::tag& nbt) const override;
    [[nodiscard]] std::string getGetMessage(const NbtPath& path, double scale, i32 value) const override;

private:
    CommandStorage* m_storage;
    ResourceLocation m_id;
};

/**
 * @brief 命令存储管理器
 *
 * 管理 /data storage 命令的数据存储。
 * 数据存储在世界的维度数据中，持久化保存。
 */
class CommandStorage {
public:
    /**
     * @brief 获取存储数据
     * @param id 存储 ID
     * @return 存储数据，如果不存在返回空复合标签
     */
    [[nodiscard]] std::unique_ptr<nbt::tags::compound_tag> get(const ResourceLocation& id);

    /**
     * @brief 设置存储数据
     * @param id 存储 ID
     * @param data 数据
     */
    void set(const ResourceLocation& id, const nbt::tags::compound_tag& data);

    /**
     * @brief 检查存储是否存在
     * @param id 存储 ID
     */
    [[nodiscard]] bool exists(const ResourceLocation& id) const;

    /**
     * @brief 列出所有存储 ID
     */
    [[nodiscard]] std::vector<ResourceLocation> listAll() const;

    /**
     * @brief 清除存储
     * @param id 存储 ID
     */
    void clear(const ResourceLocation& id);

    /**
     * @brief 标记存储已修改（需要保存）
     */
    void markDirty() { m_dirty = true; }

    /**
     * @brief 检查是否需要保存
     */
    [[nodiscard]] bool isDirty() const { return m_dirty; }

    /**
     * @brief 保存到 JSON
     */
    void save(nlohmann::json& json) const;

    /**
     * @brief 从 JSON 加载
     */
    void load(const nlohmann::json& json);

private:
    std::unordered_map<std::string, std::unique_ptr<nbt::tags::compound_tag>> m_storage;
    bool m_dirty = false;
};

} // namespace command
} // namespace mc
