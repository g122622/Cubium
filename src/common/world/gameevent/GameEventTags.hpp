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

#include "GameEventTag.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

namespace mc::gameevent {

/**
 * @brief 游戏事件标签注册表
 *
 * 负责集中管理 GameEventTag 的创建、查询和遍历。
 * 对应 MC 1.21.11 的 GameEventTags 系统。
 */
class GameEventTags {
public:
    // ========== 内置游戏事件标签 ==========

    /**
     * @brief 所有可被监听的振动事件标签（幽匿感测体默认监听集合）
     * 对应 MC 原版标签 minecraft:vibrations。
     */
    static GameEventTag& ALL_VIBRATIONS();

    /**
     * @brief 源实体潜行时被忽略的振动事件标签
     * 对应 MC 原版标签 minecraft:ignore_vibrations_sneaking。
     */
    static GameEventTag& IGNORE_VIBRATIONS_SNEAKING();

    /**
     * @brief 悦灵可监听的事件标签
     * 对应 MC 原版标签 minecraft:allay_can_listen。
     */
    static GameEventTag& ALLAY_CAN_LISTEN();

    /**
     * @brief 幽匿尖啸体可监听的事件标签
     * 对应 MC 原版标签 minecraft:shrieker_can_listen。
     */
    static GameEventTag& SHRIEKER_CAN_LISTEN();

    /**
     * @brief 监守者可监听的振动事件标签
     * 对应 MC 原版标签 minecraft:warden_can_listen。
     */
    static GameEventTag& WARDEN_CAN_LISTEN();

    /**
     * @brief 初始化所有内置游戏事件标签
     *
     * 在 GameEvents 常量可用之后调用。数据包标签加载器在其上追加/替换。
     */
    static void initialize();

    /**
     * @brief 检查游戏事件标签系统是否已初始化
     */
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

    /**
     * @brief 注册或获取指定ID的标签
     * @param id 标签ID
     * @return 标签引用
     */
    static GameEventTag& registerTag(const ResourceLocation& id);

    /**
     * @brief 根据ID获取标签
     * @param id 标签ID
     * @return 标签指针，不存在返回 nullptr
     */
    [[nodiscard]] static GameEventTag* getTag(const ResourceLocation& id);

    /**
     * @brief 遍历全部标签
     * @param callback 回调函数
     */
    static void forEachTag(std::function<void(GameEventTag&)> callback);

private:
    GameEventTags() = delete;

    static std::unordered_map<ResourceLocation, std::unique_ptr<GameEventTag>>& _getTags();
    static bool s_initialized;
};

} // namespace mc::gameevent
