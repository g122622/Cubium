#pragma once

#include "common/mod/bedrock/addon/event/ScriptEventBus.hpp"

#include <string>
#include <typeindex>
#include <vector>

namespace mc::mod::bedrock::addon {

class IScriptBindingContext;

/**
 * @brief 事件信号信息
 *
 * 描述一个可订阅的事件信号，用于注册JS绑定。
 */
struct EventSignalInfo {
    std::string name;        ///< JS属性名 (e.g. "blockBreak")
    std::type_index typeIdx; ///< C++类型索引
    bool isBefore;           ///< true=beforeEvent(可取消), false=afterEvent
};

/**
 * @brief 注册事件绑定到world对象
 *
 * 在world对象的beforeEvents和afterEvents属性上注册事件信号对象。
 *
 * @param ctx 绑定上下文
 * @param worldObj world对象句柄
 * @param eventBus 事件总线
 * @param signals 事件信号列表
 */
void registerEventBindings(
    IScriptBindingContext& ctx, void* worldObj, ScriptEventBus& eventBus, const std::vector<EventSignalInfo>& signals);

} // namespace mc::mod::bedrock::addon
