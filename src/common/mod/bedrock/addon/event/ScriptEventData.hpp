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

#include <functional>
#include <string>
#include <unordered_map>

namespace mc {
class Entity;
class IWorld;
class ItemStack;
class BlockPos;
class BlockState;
} // namespace mc

namespace mc::mod::bedrock::addon {
class IScriptBindingContext;

/** @brief 引擎无关的事件快照，队列持有值副本及受生命周期追踪的实体引用。 */
class ScriptEventData {
public:
    using ValueBuilder = std::function<void*(IScriptBindingContext&)>;

    /** @brief 写入一个可在目标插件上下文中构造的属性。 */
    void setValue(const std::string& name, ValueBuilder builder);
    /** @brief 保存字符串属性的值副本。 */
    void setString(const std::string& name, std::string value);
    /** @brief 保存数字属性。 */
    void setNumber(const std::string& name, f64 value);
    /** @brief 保存布尔属性。 */
    void setBoolean(const std::string& name, bool value);
    /** @brief 保存生命周期受追踪的实体属性。 */
    void setEntity(const std::string& name, mc::Entity* entity);
    /** @brief 保存维度属性。 */
    void setDimension(const std::string& name, mc::IWorld* world);
    /** @brief 保存物品堆副本。 */
    void setItem(const std::string& name, const mc::ItemStack& item);
    /** @brief 保存方块位置及状态。 */
    void setBlock(const std::string& name, mc::IWorld* world, const mc::BlockPos& pos, const mc::BlockState* state);
    /** @brief 保存嵌套的事件对象快照。 */
    void setObject(const std::string& name, ScriptEventData object);
    /** @brief 在目标上下文内构造属性完整的事件对象。 */
    [[nodiscard]] void* createObject(IScriptBindingContext& context) const;

    bool cancelled = false;

private:
    std::unordered_map<std::string, ValueBuilder> m_values;
};

/** @brief 脚本包注册自定义内容的启动信号类型。 */
struct ScriptStartupEvent {};

/** @brief 脚本消息传递信号类型。 */
struct ScriptMessageEvent {};

/** @brief 所有行为包启动后的世界就绪信号类型。 */
struct ScriptWorldLoadEvent {};

/** @brief 玩家首次加入与重生的统一脚本信号。 */
struct ScriptPlayerSpawnEvent {};

/** @brief 数据驱动实体定义事件完成计算后的通知。 */
struct ScriptDataDrivenEntityEvent {};

} // namespace mc::mod::bedrock::addon
