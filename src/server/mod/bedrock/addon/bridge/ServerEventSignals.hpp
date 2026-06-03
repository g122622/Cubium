#pragma once

#include "common/mod/bedrock/addon/modules/ScriptEventBinding.hpp"

#include <vector>

namespace mc::server {

/**
 * @brief 获取所有beforeEvent信号定义
 *
 * 返回可取消的事件信号列表，包含事件名称和C++类型索引。
 * 这些信号注册到world.beforeEvents上。
 */
std::vector<mc::mod::bedrock::addon::EventSignalInfo> getBeforeEventSignals();

/**
 * @brief 获取所有afterEvent信号定义
 *
 * 返回不可取消的事件信号列表，包含事件名称和C++类型索引。
 * 这些信号注册到world.afterEvents上。
 */
std::vector<mc::mod::bedrock::addon::EventSignalInfo> getAfterEventSignals();

} // namespace mc::server
