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

#include "ProcessorLists.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include "common/world/gen/jigsaw/TemplatePool.hpp"
#include "common/world/gen/jigsaw/TemplatePoolRegistry.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

using jigsaw::JigsawPlacementBehaviour;
using jigsaw::TemplatePool;
using jigsaw::TemplatePoolRegistry;

/**
 * @brief 模板池统一注册入口
 *
 * 此命名空间提供所有模板池的注册功能。
 * 使用方式：
 *   Pools::initialize();  // 启动时调用一次
 *
 * 初始化顺序：
 * 1. ProcessorLists::initialize()
 * 2. VillagePools::registerAll()
 * 3. PillagerOutpostPools::registerAll()
 * 4. BastionPools::registerAll()
 * 5. (可选) 从数据包加载覆盖
 */
namespace Pools {

/**
 * @brief 初始化所有模板池
 *
 * 必须在使用任何模板池之前调用。
 * 通常在服务器启动时调用一次。
 *
 * 初始化顺序：
 * 1. 初始化处理器列表
 * 2. 注册空模板池
 * 3. 注册公共模板池
 * 4. 注册村庄模板池
 * 5. 注册掠夺者前哨站模板池
 * 6. 注册堡垒遗迹模板池
 */
void initialize();

/**
 * @brief 检查是否已初始化
 */
bool isInitialized();

/**
 * @brief 注册空模板池
 *
 * 用于模板池的 fallback 终止链。
 */
void registerEmptyPool(TemplatePoolRegistry& registry);

} // namespace Pools

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
