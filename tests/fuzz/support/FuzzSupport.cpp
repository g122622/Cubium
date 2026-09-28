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

#include "support/FuzzSupport.hpp"

#include "common/item/Items.hpp"
#include "common/network/backend/java/JavaProtocolTables.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <mutex>

namespace mc::fuzz {

namespace {

std::once_flag g_initOnce;
std::shared_ptr<FuzzTables> g_tables;

void _initializeRegistriesAndTables()
{
    // 方块与物品注册表是 RegistryByteBuf 解码 BlockState / ItemStack holder 的前置。
    // 两者均带 s_initialized 守卫、均为纯内存注册（不触碰资源包与数据包目录）。
    VanillaBlocks::initialize();
    Items::initialize();

    // 五阶段 × 两流向共 10 张包表。build() 内部按 GameProtocols.java 的注册顺序
    // 依次 addPacket，显式 id 即 wire packet id。
    g_tables = network::backend::java::JavaProtocolTables::build();
}

} // namespace

void initializeOnce()
{
    std::call_once(g_initOnce, _initializeRegistriesAndTables);
}

const std::shared_ptr<FuzzTables>& tables()
{
    initializeOnce();
    return g_tables;
}

} // namespace mc::fuzz
