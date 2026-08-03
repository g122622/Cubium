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

#include "CustomServerBossInfo.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponentFwd.hpp"
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

// 前向声明
class ServerPlayer;

namespace server {

// 前向声明
class IServer;

/**
 * @brief 自定义服务端 Boss 信息管理器
 *
 * 管理所有自定义 Boss 栏，提供创建、删除、查询功能。
 * 支持持久化存储和网络同步。
 */
class CustomServerBossInfoManager {
public:
    /**
     * @brief 构造函数
     *
     * @param server Minecraft 服务器实例
     */
    explicit CustomServerBossInfoManager(IServer& server);

    /**
     * @brief 析构函数
     */
    virtual ~CustomServerBossInfoManager();

    // 禁止拷贝
    CustomServerBossInfoManager(const CustomServerBossInfoManager&) = delete;
    CustomServerBossInfoManager& operator=(const CustomServerBossInfoManager&) = delete;

    // ========== Boss 栏管理 ==========

    /**
     * @brief 创建新的 Boss 栏
     *
     * @param id 资源位置 ID
     * @param name 显示名称
     * @return 创建的 Boss 栏，如果 ID 已存在则返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<CustomServerBossInfo> create(
        const ResourceLocation& id, std::unique_ptr<text::ITextComponent> name);

    /**
     * @brief 添加 Boss 栏到管理器
     *
     * @param bossInfo Boss 栏实例
     * @return 添加后的 Boss 栏指针
     */
    CustomServerBossInfo* add(std::unique_ptr<CustomServerBossInfo> bossInfo);

    /**
     * @brief 移除 Boss 栏
     *
     * @param bossInfo 要移除的 Boss 栏
     */
    void remove(CustomServerBossInfo& bossInfo);

    /**
     * @brief 通过 ID 获取 Boss 栏
     *
     * @param id 资源位置 ID
     * @return Boss 栏指针，不存在返回 nullptr
     */
    [[nodiscard]] CustomServerBossInfo* get(const ResourceLocation& id);

    /**
     * @brief 通过 ID 获取 Boss 栏（const 版本）
     *
     * @param id 资源位置 ID
     * @return Boss 栏指针，不存在返回 nullptr
     */
    [[nodiscard]] const CustomServerBossInfo* get(const ResourceLocation& id) const;

    /**
     * @brief 获取所有 Boss 栏 ID
     */
    [[nodiscard]] std::vector<ResourceLocation> getIds() const;

    /**
     * @brief 获取所有 Boss 栏
     */
    [[nodiscard]] std::vector<CustomServerBossInfo*> getBossBars();

    /**
     * @brief 获取 Boss 栏数量
     */
    [[nodiscard]] size_t size() const noexcept { return m_bossBars.size(); }

    /**
     * @brief 检查是否为空
     */
    [[nodiscard]] bool empty() const noexcept { return m_bossBars.empty(); }

    // ========== 玩家事件 ==========

    /**
     * @brief 玩家登录时调用
     *
     * 恢复玩家对之前可见的 Boss 栏的可见性。
     *
     * @param player 服务端玩家
     */
    void onPlayerLogin(::mc::ServerPlayer& player);

    /**
     * @brief 玩家登出时调用
     *
     * 清理玩家的 Boss 栏可见性。
     *
     * @param player 服务端玩家
     */
    void onPlayerLogout(::mc::ServerPlayer& player);

    // ========== 网络同步 ==========

    /**
     * @brief 发送 Boss 栏添加包给玩家
     *
     * @param bossInfo Boss 栏
     * @param player 玩家
     */
    virtual void sendAddPacket(CustomServerBossInfo& bossInfo, ::mc::ServerPlayer& player);

    /**
     * @brief 发送 Boss 栏移除包给玩家
     *
     * @param bossInfo Boss 栏
     * @param player 玩家
     */
    virtual void sendRemovePacket(CustomServerBossInfo& bossInfo, ::mc::ServerPlayer& player);

    /**
     * @brief 广播 Boss 栏更新给所有可见玩家
     *
     * @param bossInfo Boss 栏
     */
    virtual void broadcastUpdate(CustomServerBossInfo& bossInfo);

    // ========== 持久化 ==========

    /**
     * @brief 序列化为 NBT
     */
    [[nodiscard]] nbt::tags::compound_tag toNbt() const;

    /**
     * @brief 从 NBT 反序列化
     *
     * @param nbt NBT 数据
     */
    void fromNbt(const nbt::tags::compound_tag& nbt);

    /**
     * @brief 标记数据为脏
     */
    void markDirty() noexcept { m_dirty = true; }

    /**
     * @brief 检查数据是否需要保存
     */
    [[nodiscard]] bool isDirty() const noexcept { return m_dirty; }

    /**
     * @brief 清除脏标记
     */
    void clearDirty() noexcept { m_dirty = false; }

private:
    /// 服务器引用
    IServer& m_server;

    /// Boss 栏映射（ID -> Boss 栏）
    std::unordered_map<ResourceLocation, std::unique_ptr<CustomServerBossInfo>> m_bossBars;

    /// 数据是否需要保存
    bool m_dirty = false;
};

} // namespace server
} // namespace mc
