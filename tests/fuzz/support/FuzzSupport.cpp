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

#include <spdlog/spdlog.h>

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

/// 当前正在执行的 fuzz 输入（见 setCurrentInput）。
const std::uint8_t* g_currentInput = nullptr;
std::size_t g_currentInputSize = 0;

/**
 * @brief 把当前输入落盘为可复现文件
 *
 * 刻意走 Win32 API 而非 CRT 的 fopen/fwrite：故障现场 CRT/堆可能已经损坏（实测表现为
 * fwrite 写出 0 字节），只有内核态的 CreateFile/WriteFile 还可靠。
 */
void _dumpCurrentInput(DWORD code)
{
    if (g_currentInput == nullptr || g_currentInputSize == 0) {
        return;
    }
    char name[128];
    std::snprintf(name,
        sizeof(name),
        "fuzz-crash-%08lX-%lu.bin",
        static_cast<unsigned long>(code),
        static_cast<unsigned long>(GetCurrentProcessId()));

    HANDLE file = CreateFileA(name, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD written = 0;
    (void)WriteFile(file, g_currentInput, static_cast<DWORD>(g_currentInputSize), &written, nullptr);
    CloseHandle(file);

    std::fprintf(stderr, "[fuzz] 当前输入已落盘: %s (%lu 字节)\n", name, static_cast<unsigned long>(written));
    std::fflush(stderr);
}

/// 异常现场报告器。
LONG CALLBACK _exceptionReporter(EXCEPTION_POINTERS* info)
{
    const DWORD code = info->ExceptionRecord->ExceptionCode;

    // 只报告两类"Sanitizer 死回调覆盖不到、进程会静默死亡"的异常：
    //   0x80000003 STATUS_BREAKPOINT（int3）
    //   0xC0000005 访问违例（ASan 未及报告时）
    // 其余异常一律原样放行——尤其 0xE06D7363 是 MSVC C++ 抛出点，属存量库的正常控制流；
    // 把不该拦的拦下来放行会与 ASan 的处理器相互干扰，表现为反复故障（hang）。
    if (code != 0x80000003u && code != 0xC0000005u) {
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
    std::fprintf(stderr,
        "\n[fuzz] 异常 0x%08lX at %p (module base %p)\n"
        "       用 llvm-symbolizer --obj=<该可执行文件> 0x%llX 符号化\n",
        static_cast<unsigned long>(code),
        reinterpret_cast<const void*>(addr),
        reinterpret_cast<const void*>(base),
        static_cast<unsigned long long>(addr - base));
    std::fflush(stderr);

    _dumpCurrentInput(code);

    // 刻意不做栈回溯：在故障现场调用 StackWalk64 可能再次故障，从而递归重入本处理器。

    _reporting = 0;
    return EXCEPTION_CONTINUE_SEARCH;
}

} // namespace

void setCurrentInput(const std::uint8_t* data, std::size_t size)
{
    g_currentInput = data;
    g_currentInputSize = size;
}

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
    // fuzz 吞吐优先：静音到 critical。
    // 解码路径上有若干"偏离正常路径"的 warn 留痕（如帧结构非法、单包解码失败被跳过），
    // 在乱码输入下会高频触发；spdlog 默认写控制台，光是格式化与写盘就能让吞吐掉一个
    // 数量级。日志与解析逻辑正交，故 harness 侧统一静音，只保留 critical。
    spdlog::set_level(spdlog::level::critical);

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
