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
#include "common/network/backend/java/mappings/JavaBlockStateIdMap.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/JavaBiomeRegistryIdMap.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <mutex>

#ifdef _WIN32
#include <windows.h>

#include <cstdint>
#include <cstdio>
#endif

namespace mc::fuzz {

#ifdef _WIN32

namespace {

/// VEH 句柄，用于首次报告后卸载自身。
PVOID _reporterHandle = nullptr;

/// 防重入：报告期间发生的任何二次故障一律直接放行。
volatile LONG _reporting = 0;

/// 异常现场报告器。
LONG CALLBACK _exceptionReporter(EXCEPTION_POINTERS* info)
{
    const DWORD code = info->ExceptionRecord->ExceptionCode;

    // 只报告 int3/断点类异常（STATUS_BREAKPOINT）。这是唯一一类 Sanitizer 死回调覆盖
    // 不到的异常：进程会静默死亡，既不打印栈回溯也不落盘复现用例。
    // 其余异常一律原样放行，交由 Sanitizer/libFuzzer 正常处理——
    //   - 0xE06D7363：MSVC C++ 抛出点，属存量库的正常控制流；
    //   - 0xC0000005：访问违例，由 ASan 的处理器负责报告。
    // 若把这些也拦下来放行，会与 ASan 的处理器相互干扰，表现为反复故障（hang）。
    if (code != 0x80000003) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (InterlockedExchange(&_reporting, 1) != 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (_reporterHandle != nullptr) {
        RemoveVectoredExceptionHandler(_reporterHandle);
        _reporterHandle = nullptr;
    }

    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    const auto addr = reinterpret_cast<std::uintptr_t>(info->ExceptionRecord->ExceptionAddress);
    const auto rva = static_cast<unsigned long long>(addr - base);
    std::fprintf(stderr,
        "\n[fuzz] int3/breakpoint at %p (module base %p)\n"
        "       用 llvm-symbolizer --obj=<该可执行文件> 0x%llX 符号化\n",
        reinterpret_cast<const void*>(addr),
        reinterpret_cast<const void*>(base),
        rva);
    std::fflush(stderr);
    // 刻意不做栈回溯：在故障现场调用 StackWalk64 可能再次故障，从而递归重入本处理器。

    _reporting = 0;
    return EXCEPTION_CONTINUE_SEARCH;
}

} // namespace

void installExceptionReporter()
{
    static bool installed = false;
    if (installed) {
        return;
    }
    installed = true;
    // 1 = 最先被调用（在所有其他 VEH 之前）。
    _reporterHandle = AddVectoredExceptionHandler(1, _exceptionReporter);
}

#endif // _WIN32

namespace {

std::once_flag g_initOnce;
std::shared_ptr<FuzzTables> g_tables;

void _initializeRegistriesAndTables()
{
    // 方块与物品注册表是 RegistryByteBuf 解码 BlockState / ItemStack holder 的前置。
    // 两者均带 s_initialized 守卫、均为纯内存注册（不触碰资源包与数据包目录）。
    VanillaBlocks::initialize();
    Items::initialize();

    // 生物群系注册表与两张 id 映射表（内部 id ↔ Java wire id）是 VanillaChunkWire
    // 区块线格式翻译的前置。均为纯内存初始化（映射表用构建期烘焙的静态表）。
    mc::world::biome::BiomeRegistry::instance().initialize();
    (void)mc::network::backend::java::JavaBlockStateIdMap::instance().initialize();
    (void)mc::world::biome::JavaBiomeRegistryIdMap::instance().initialize();

    // 五阶段 × 两流向共 10 张包表。build() 内部按 GameProtocols.java 的注册顺序
    // 依次 addPacket，显式 id 即 wire packet id。
    g_tables = network::backend::java::JavaProtocolTables::build();

#ifdef _WIN32
    installExceptionReporter();
#endif
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
