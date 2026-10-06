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

#include "common/resource/ResourceLocation.hpp"

#include <unordered_set>
#include <vector>

namespace mc::gameevent {

class GameEvent;

/**
 * @brief 游戏事件标签
 *
 * 用于将具有相同特性的游戏事件分组（如"哪些事件可被监听的振动"、"哪些事件
 * 可被幽匿尖啸体监听"），对应 MC 1.21.11 的 GameEventTags 系统。
 *
 * 成员以 GameEvent 指针存储（事件为常量单例，指针稳定），
 * 与 BlockTag（ResourceLocation）/ DamageTypeTag（枚举）的设计模式一致。
 *
 * 用法示例:
 * @code
 * if (GameEventTags::IGNORE_VIBRATIONS_SNEAKING().contains(event)) {
 *     // 该事件在源实体潜行时被忽略
 * }
 * @endcode
 */
class GameEventTag {
public:
    /**
     * @brief 构造游戏事件标签
     * @param id 标签资源位置
     */
    explicit GameEventTag(ResourceLocation id) noexcept;

    /**
     * @brief 获取标签ID
     */
    [[nodiscard]] const ResourceLocation& getId() const { return m_id; }

    /**
     * @brief 添加游戏事件到标签
     * @param event 游戏事件指针（不能为 nullptr）
     */
    void add(const GameEvent* event);

    /**
     * @brief 批量添加游戏事件
     * @param events 游戏事件指针列表
     */
    void addAll(const std::vector<const GameEvent*>& events);

    /**
     * @brief 检查游戏事件是否在标签中
     * @param event 游戏事件指针
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const GameEvent* event) const noexcept;

    /**
     * @brief 检查游戏事件是否在标签中
     * @param event 游戏事件引用
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const GameEvent& event) const noexcept;

    /**
     * @brief 清空标签中的所有游戏事件
     *
     * 用于数据包加载的 replace 语义。
     */
    void clear();

    /**
     * @brief 获取标签中的所有游戏事件
     */
    [[nodiscard]] const std::unordered_set<const GameEvent*>& getEvents() const noexcept { return m_events; }

private:
    ResourceLocation m_id;
    std::unordered_set<const GameEvent*> m_events;
};

} // namespace mc::gameevent
