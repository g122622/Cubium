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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CrashHandler.hpp"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

#ifdef _WIN32
// clang-format off
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
// clang-format on
#pragma comment(lib, "dbghelp.lib")
#elif defined(__linux__) || defined(__APPLE__)
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace mc::assert {

bool CrashHandler::s_installed = false;
CrashCleanupCallback CrashHandler::s_cleanupCallback;

#ifdef _WIN32
static bool s_symInitialized = false;
#endif

// ============================================================================
// 平台特定：符号初始化与调用栈捕获
// ============================================================================

#ifdef _WIN32

namespace {

/**
 * @brief 格式化单个符号（局部变量或参数），追加到输出流
 */
void formatSymbolEntry(PSYMBOL_INFO symbolInfo, std::ostringstream& oss, bool& hasAny)
{
    hasAny = true;
    oss << "        " << symbolInfo->Name;

    // 尝试输出类型名
    if (symbolInfo->TypeIndex != 0) {
        WCHAR* typeName = nullptr;
        if (SymGetTypeInfo(
                GetCurrentProcess(), symbolInfo->ModBase, symbolInfo->TypeIndex, TI_GET_SYMNAME, &typeName)) {
            char narrowName[MAX_SYM_NAME];
            size_t converted = 0;
            wcstombs_s(&converted, narrowName, typeName, MAX_SYM_NAME - 1);
            oss << " : " << narrowName;
            LocalFree(typeName);
        }
    }

    // 标记变量类别
    if (symbolInfo->Flags & SYMFLAG_PARAMETER) {
        oss << " [param]";
    } else if (symbolInfo->Flags & SYMFLAG_LOCAL) {
        oss << " [local]";
    }

    oss << "\n";
}

/**
 * @brief 初始化 DbgHelp 符号处理器
 *
 * 只调用一次（在 install() 中），避免多线程竞争和重复初始化。
 */
bool initializeSymbols()
{
    if (s_symInitialized) {
        return true;
    }

    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

    if (!SymInitialize(process, nullptr, TRUE)) {
        return false;
    }

    s_symInitialized = true;
    return true;
}

/**
 * @brief 枚举单个栈帧的局部变量和参数
 *
 * 先尝试 SymSetContext + SymEnumSymbols（需要准确的帧指针），
 * 如果失败则回退到搜索函数地址范围内的所有符号。
 *
 * 注意：Clang -O2 会省略帧指针，导致 SymSetContext 失败。
 * 回退方案能输出函数参数（this 等），但局部变量可能丢失。
 */
std::string enumerateLocalVariables(HANDLE process, DWORD64 address, STACKFRAME64& stackFrame)
{
    struct EnumContext {
        std::ostringstream oss;
        bool hasAny = false;
    };

    // 方法1：SymSetContext + SymEnumSymbols（需要准确帧指针）
    {
        IMAGEHLP_STACK_FRAME frame{};
        frame.InstructionOffset = address;
        frame.FrameOffset = stackFrame.AddrFrame.Offset;
        frame.StackOffset = stackFrame.AddrStack.Offset;

        if (SymSetContext(process, &frame, nullptr)) {
            EnumContext ctx;
            SymEnumSymbols(
                process,
                0,
                nullptr,
                [](PSYMBOL_INFO symbolInfo, ULONG symbolSize, PVOID userContext) -> BOOL {
                    (void)symbolSize;
                    auto* ctx = static_cast<EnumContext*>(userContext);

                    // 包含局部变量和参数
                    if (symbolInfo->Flags & (SYMFLAG_LOCAL | SYMFLAG_PARAMETER)) {
                        formatSymbolEntry(symbolInfo, ctx->oss, ctx->hasAny);
                    }

                    return TRUE;
                },
                &ctx);

            if (ctx.hasAny) {
                return "    Locals:\n" + ctx.oss.str();
            }
        }
    }

    // 方法2：回退方案 - 查找该地址所属函数，搜索函数范围内的符号
    {
        SYMBOL_INFO symInfo{};
        symInfo.SizeOfStruct = sizeof(SYMBOL_INFO);
        // 需要为名称分配空间
        constexpr ULONG NAME_BUFFER_SIZE = 256;
        auto* symbolBuffer = static_cast<SYMBOL_INFO*>(malloc(sizeof(SYMBOL_INFO) + NAME_BUFFER_SIZE));
        symbolBuffer->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbolBuffer->MaxNameLen = NAME_BUFFER_SIZE;

        DWORD64 displacement = 0;
        if (SymFromAddr(process, address, &displacement, symbolBuffer)) {
            EnumContext ctx;

            // 构造搜索掩码：函数名!*
            std::string mask = std::string(symbolBuffer->Name) + "!*";

            SymEnumSymbols(
                process,
                0,
                mask.c_str(),
                [](PSYMBOL_INFO symbolInfo, ULONG symbolSize, PVOID userContext) -> BOOL {
                    (void)symbolSize;
                    auto* ctx = static_cast<EnumContext*>(userContext);

                    // 只包含局部变量和参数
                    if (symbolInfo->Flags & (SYMFLAG_LOCAL | SYMFLAG_PARAMETER)) {
                        formatSymbolEntry(symbolInfo, ctx->oss, ctx->hasAny);
                    }

                    return TRUE;
                },
                &ctx);

            if (ctx.hasAny) {
                free(symbolBuffer);
                return "    Locals:\n" + ctx.oss.str();
            }
        }

        free(symbolBuffer);
    }

    return {};
}

} // namespace

std::string CrashHandler::captureStackTrace(i32 skipFrames, i32 maxFrames)
{
    std::ostringstream oss;

    if (!s_symInitialized) {
        initializeSymbols();
    }

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    CONTEXT context{};
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);

    STACKFRAME64 stackFrame{};
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Mode = AddrModeFlat;

#ifdef _M_X64
    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrFrame.Offset = context.Rbp;
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_IX86)
    stackFrame.AddrPC.Offset = context.Eip;
    stackFrame.AddrStack.Offset = context.Esp;
    stackFrame.AddrFrame.Offset = context.Ebp;
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
#elif defined(_M_ARM64)
    stackFrame.AddrPC.Offset = context.Pc;
    stackFrame.AddrStack.Offset = context.Sp;
    stackFrame.AddrFrame.Offset = context.Fp;
    DWORD machineType = IMAGE_FILE_MACHINE_ARM64;
#else
    constexpr i32 MAX_FRAMES = 64;
    void* stack[MAX_FRAMES];
    USHORT frames = CaptureStackBackTrace(skipFrames + 1, MAX_FRAMES, stack, nullptr);

    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(malloc(sizeof(SYMBOL_INFO) + 256));
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (USHORT i = 0; i < frames; ++i) {
        DWORD64 addr = reinterpret_cast<DWORD64>(stack[i]);
        if (SymFromAddr(process, addr, nullptr, symbol)) {
            oss << "  [" << std::setw(2) << i << "] " << symbol->Name;
            IMAGEHLP_LINE64 line;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD displacement;
            if (SymGetLineFromAddr64(process, addr, &displacement, &line)) {
                oss << " at " << line.FileName << ":" << line.LineNumber;
            }
            oss << "\n";
        }
    }

    free(symbol);
    return oss.str();
#endif

    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(malloc(sizeof(SYMBOL_INFO) + 256));
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement = 0;

    i32 frameIndex = 0;
    i32 displayedIndex = 0;

    while (StackWalk64(machineType,
        process,
        thread,
        &stackFrame,
        &context,
        nullptr,
        SymFunctionTableAccess64,
        SymGetModuleBase64,
        nullptr)) {
        if (stackFrame.AddrPC.Offset == 0) {
            break;
        }

        if (frameIndex < skipFrames) {
            frameIndex++;
            continue;
        }

        if (displayedIndex >= maxFrames) {
            oss << "  ... (" << maxFrames << " frames limit reached)\n";
            break;
        }

        DWORD64 address = stackFrame.AddrPC.Offset;

        if (SymFromAddr(process, address, nullptr, symbol)) {
            oss << "  [" << std::setw(2) << displayedIndex << "] " << symbol->Name;

            if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
                oss << " at " << line.FileName << ":" << line.LineNumber;
            }

            oss << "\n";

            // 尝试枚举该帧的局部变量
            oss << enumerateLocalVariables(process, address, stackFrame);
        }

        frameIndex++;
        displayedIndex++;
    }

    free(symbol);
    return oss.str();
}

/**
 * @brief 从崩溃上下文捕获调用栈（SEH 异常专用）
 *
 * 使用崩溃线程的真实 CONTEXT 而非当前线程，确保栈帧准确。
 */
std::string captureStackTraceFromContext(EXCEPTION_POINTERS* exceptionInfo, i32 skipFrames, i32 maxFrames)
{
    std::ostringstream oss;

    if (!s_symInitialized) {
        initializeSymbols();
    }

    if (!exceptionInfo || !exceptionInfo->ContextRecord) {
        return CrashHandler::captureStackTrace(skipFrames, maxFrames);
    }

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    // 使用崩溃线程的真实上下文
    CONTEXT context{};
    std::memcpy(&context, exceptionInfo->ContextRecord, sizeof(CONTEXT));

    STACKFRAME64 stackFrame{};
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Mode = AddrModeFlat;

#ifdef _M_X64
    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrFrame.Offset = context.Rbp;
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_IX86)
    stackFrame.AddrPC.Offset = context.Eip;
    stackFrame.AddrStack.Offset = context.Esp;
    stackFrame.AddrFrame.Offset = context.Ebp;
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
#elif defined(_M_ARM64)
    stackFrame.AddrPC.Offset = context.Pc;
    stackFrame.AddrStack.Offset = context.Sp;
    stackFrame.AddrFrame.Offset = context.Fp;
    DWORD machineType = IMAGE_FILE_MACHINE_ARM64;
#else
    return CrashHandler::captureStackTrace(skipFrames, maxFrames);
#endif

    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(malloc(sizeof(SYMBOL_INFO) + 256));
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement = 0;

    i32 frameIndex = 0;
    i32 displayedIndex = 0;

    while (StackWalk64(machineType,
        process,
        thread,
        &stackFrame,
        &context,
        nullptr,
        SymFunctionTableAccess64,
        SymGetModuleBase64,
        nullptr)) {
        if (stackFrame.AddrPC.Offset == 0) {
            break;
        }

        if (frameIndex < skipFrames) {
            frameIndex++;
            continue;
        }

        if (displayedIndex >= maxFrames) {
            oss << "  ... (" << maxFrames << " frames limit reached)\n";
            break;
        }

        DWORD64 address = stackFrame.AddrPC.Offset;

        if (SymFromAddr(process, address, nullptr, symbol)) {
            oss << "  [" << std::setw(2) << displayedIndex << "] " << symbol->Name;

            if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
                oss << " at " << line.FileName << ":" << line.LineNumber;
            }

            oss << "\n";

            enumerateLocalVariables(process, address, stackFrame);
        }

        frameIndex++;
        displayedIndex++;
    }

    free(symbol);
    return oss.str();
}

#else // Linux / macOS

std::string CrashHandler::captureStackTrace(i32 skipFrames, i32 maxFrames)
{
    std::ostringstream oss;

    constexpr i32 MAX_FRAMES = 128;
    void* buffer[MAX_FRAMES];
    int frames = backtrace(buffer, MAX_FRAMES);

    i32 start = skipFrames + 1;
    if (start >= frames) {
        return oss.str();
    }

    char** symbols = backtrace_symbols(buffer, frames);
    if (!symbols) {
        return oss.str();
    }

    i32 displayedIndex = 0;
    for (int i = start; i < frames && displayedIndex < maxFrames; ++i) {
        oss << "  [" << std::setw(2) << displayedIndex << "] ";

        std::string sym(symbols[i]);

#ifdef __GNUC__
        size_t startParen = sym.find('(');
        size_t endParen = sym.find('+', startParen);

        if (startParen != std::string::npos && endParen != std::string::npos) {
            std::string mangled = sym.substr(startParen + 1, endParen - startParen - 1);
            if (!mangled.empty()) {
                int status = 0;
                char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);
                if (status == 0 && demangled) {
                    oss << demangled;
                    free(demangled);
                } else {
                    oss << mangled;
                }
            } else {
                oss << sym;
            }
        } else {
            oss << sym;
        }
#else
        oss << sym;
#endif

        oss << "\n";
        displayedIndex++;
    }

    if (displayedIndex >= maxFrames) {
        oss << "  ... (" << maxFrames << " frames limit reached)\n";
    }

    free(symbols);
    return oss.str();
}

#endif

// ============================================================================
// 崩溃信息输出（全部在互斥锁内，防止多线程输出交织）
// ============================================================================

namespace {

std::mutex& getCrashMutex()
{
    static std::mutex s_crashMutex;
    return s_crashMutex;
}

#ifdef _WIN32

std::string formatExceptionCode(DWORD exceptionCode)
{
    switch (exceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
            return "ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return "ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT:
            return "BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            return "DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            return "FLT_DENORMAL_OPERAND";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            return "FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_INEXACT_RESULT:
            return "FLT_INEXACT_RESULT";
        case EXCEPTION_FLT_INVALID_OPERATION:
            return "FLT_INVALID_OPERATION";
        case EXCEPTION_FLT_OVERFLOW:
            return "FLT_OVERFLOW";
        case EXCEPTION_FLT_STACK_CHECK:
            return "FLT_STACK_CHECK";
        case EXCEPTION_FLT_UNDERFLOW:
            return "FLT_UNDERFLOW";
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            return "ILLEGAL_INSTRUCTION";
        case EXCEPTION_IN_PAGE_ERROR:
            return "IN_PAGE_ERROR";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            return "INT_DIVIDE_BY_ZERO";
        case EXCEPTION_INT_OVERFLOW:
            return "INT_OVERFLOW";
        case EXCEPTION_INVALID_DISPOSITION:
            return "INVALID_DISPOSITION";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            return "NONCONTINUABLE_EXCEPTION";
        case EXCEPTION_PRIV_INSTRUCTION:
            return "PRIV_INSTRUCTION";
        case EXCEPTION_SINGLE_STEP:
            return "SINGLE_STEP";
        case EXCEPTION_STACK_OVERFLOW:
            return "STACK_OVERFLOW";
        default:
            return "UNKNOWN(0x" + std::to_string(exceptionCode) + ")";
    }
}

void printRegisterDump(CONTEXT* ctx)
{
    if (!ctx) {
        return;
    }

    std::cerr << "\nRegister dump:\n";
#ifdef _M_X64
    std::cerr << "  RAX: 0x" << std::hex << ctx->Rax << "\n";
    std::cerr << "  RBX: 0x" << std::hex << ctx->Rbx << "\n";
    std::cerr << "  RCX: 0x" << std::hex << ctx->Rcx << "\n";
    std::cerr << "  RDX: 0x" << std::hex << ctx->Rdx << "\n";
    std::cerr << "  RSI: 0x" << std::hex << ctx->Rsi << "\n";
    std::cerr << "  RDI: 0x" << std::hex << ctx->Rdi << "\n";
    std::cerr << "  RBP: 0x" << std::hex << ctx->Rbp << "\n";
    std::cerr << "  RSP: 0x" << std::hex << ctx->Rsp << "\n";
    std::cerr << "  RIP: 0x" << std::hex << ctx->Rip << "\n";
    std::cerr << std::dec;
#elif defined(_M_IX86)
    std::cerr << "  EAX: 0x" << std::hex << ctx->Eax << "\n";
    std::cerr << "  EBX: 0x" << std::hex << ctx->Ebx << "\n";
    std::cerr << "  ECX: 0x" << std::hex << ctx->Ecx << "\n";
    std::cerr << "  EDX: 0x" << std::hex << ctx->Edx << "\n";
    std::cerr << "  ESI: 0x" << std::hex << ctx->Esi << "\n";
    std::cerr << "  EDI: 0x" << std::hex << ctx->Edi << "\n";
    std::cerr << "  EBP: 0x" << std::hex << ctx->Ebp << "\n";
    std::cerr << "  ESP: 0x" << std::hex << ctx->Esp << "\n";
    std::cerr << "  EIP: 0x" << std::hex << ctx->Eip << "\n";
    std::cerr << std::dec;
#endif
    std::cerr << "\n";
}

#endif // _WIN32

/**
 * @brief 输出崩溃信息到 stderr（调用者已持锁）
 */
void printCrashInfoLocked(const std::string& reason
#ifdef _WIN32
    ,
    EXCEPTION_POINTERS* exceptionInfo = nullptr
#endif
)
{
    std::cerr << "\n";
    std::cerr << "============================================\n";
    std::cerr << "          FATAL CRASH DETECTED              \n";
    std::cerr << "============================================\n";
    std::cerr << "\n";
    std::cerr << "Reason: " << reason << "\n";

#ifdef _WIN32
    // 寄存器转储（仅在 SEH 异常时）
    if (exceptionInfo && exceptionInfo->ContextRecord) {
        printRegisterDump(exceptionInfo->ContextRecord);
    }

    // 从崩溃上下文捕获调用栈（跳过 SEH 过滤器帧）
    std::cerr << "\nStack trace:\n";
    std::cerr << captureStackTraceFromContext(exceptionInfo, 0, 64);
#else
    // Linux/macOS：从当前上下文捕获
    std::cerr << "\nStack trace:\n";
    std::cerr << CrashHandler::captureStackTrace(2);
#endif

    std::cerr << "\n";
    std::cerr << "============================================\n";
    std::cerr << std::flush;

    // 执行清理回调（如刷新 Perfetto 跟踪数据等）
    if (CrashHandler::s_cleanupCallback) {
        CrashHandler::s_cleanupCallback();
    }
}

} // namespace

// ============================================================================
// Windows 平台：SEH 异常处理器
// ============================================================================

#ifdef _WIN32

static LPTOP_LEVEL_EXCEPTION_FILTER s_previousFilter = nullptr;

LONG WINAPI crashExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
    // 整个输出过程在互斥锁内，防止多线程交织
    std::lock_guard<std::mutex> lock(getCrashMutex());

    if (!exceptionInfo) {
        printCrashInfoLocked("Unknown exception (no exception info)");
        return EXCEPTION_CONTINUE_SEARCH;
    }

    DWORD code = exceptionInfo->ExceptionRecord->ExceptionCode;
    std::string reason = formatExceptionCode(code);

    // 对于 ACCESS_VIOLATION，输出具体的读写地址
    if (code == EXCEPTION_ACCESS_VIOLATION && exceptionInfo->ExceptionRecord->NumberParameters >= 2) {
        ULONG_PTR operation = exceptionInfo->ExceptionRecord->ExceptionInformation[0];
        ULONG_PTR address = exceptionInfo->ExceptionRecord->ExceptionInformation[1];
        reason += " - ";
        reason += (operation == 0) ? "Read" : "Write";
        reason += " access at address 0x";

        std::ostringstream addrOss;
        addrOss << std::hex << address;
        reason += addrOss.str();
    }

    printCrashInfoLocked(reason, exceptionInfo);

    // 调用之前的处理器（如果有）
    if (s_previousFilter) {
        return s_previousFilter(exceptionInfo);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

// 纯虚函数调用处理器
void __cdecl pureCallHandler()
{
    std::lock_guard<std::mutex> lock(getCrashMutex());
    printCrashInfoLocked("Pure virtual function call");
    _exit(1);
}

// 无效参数处理器
void __cdecl invalidParameterHandler(
    const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved)
{
    (void)reserved;

    std::lock_guard<std::mutex> lock(getCrashMutex());

    std::ostringstream oss;
    oss << "Invalid parameter detected";

    if (expression) {
        char narrowExpr[256];
        size_t converted = 0;
        wcstombs_s(&converted, narrowExpr, expression, 255);
        oss << "\n  Expression: " << narrowExpr;
    }
    if (function) {
        char narrowFunc[256];
        size_t converted = 0;
        wcstombs_s(&converted, narrowFunc, function, 255);
        oss << "\n  Function: " << narrowFunc;
    }
    if (file) {
        char narrowFile[256];
        size_t converted = 0;
        wcstombs_s(&converted, narrowFile, file, 255);
        oss << "\n  File: " << narrowFile << ":" << line;
    }

    printCrashInfoLocked(oss.str());
    _exit(1);
}

#else // Linux / macOS

static struct sigaction s_oldSigSegv{};
static struct sigaction s_oldSigAbrt{};
static struct sigaction s_oldSigFpe{};
static struct sigaction s_oldSigBus{};
static struct sigaction s_oldSigIll{};
static struct sigaction s_oldSigTerm{};

void crashSignalHandler(int signal, siginfo_t* info, void* context)
{
    (void)context;

    // 整个输出过程在互斥锁内
    std::lock_guard<std::mutex> lock(getCrashMutex());

    std::string reason;
    switch (signal) {
        case SIGSEGV:
            reason = "SIGSEGV - Segmentation fault";
            break;
        case SIGABRT:
            reason = "SIGABRT - Abort signal";
            break;
        case SIGFPE:
            reason = "SIGFPE - Floating point exception";
            break;
        case SIGBUS:
            reason = "SIGBUS - Bus error";
            break;
        case SIGILL:
            reason = "SIGILL - Illegal instruction";
            break;
        case SIGTERM:
            reason = "SIGTERM - Termination signal";
            break;
        default:
            reason = "Signal " + std::to_string(signal);
            break;
    }

    if (info) {
        std::ostringstream addrOss;
        addrOss << " at address 0x" << std::hex << reinterpret_cast<uintptr_t>(info->si_addr);
        reason += addrOss.str();
    }

    printCrashInfoLocked(reason);

    // 恢复默认处理器并重新发送信号，生成 core dump
    struct sigaction sa{};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(signal, &sa, nullptr);
    raise(signal);
}

#endif // _WIN32

// std::terminate 处理器
void terminateHandler()
{
    std::lock_guard<std::mutex> lock(getCrashMutex());
    printCrashInfoLocked("std::terminate() called (unhandled exception or noexcept violation)");
    _exit(1);
}

// std::set_new_handler 处理器
void newHandler()
{
    std::lock_guard<std::mutex> lock(getCrashMutex());
    printCrashInfoLocked("Memory allocation failed (operator new returned nullptr)");
    _exit(1);
}

// ============================================================================
// 公共接口
// ============================================================================

void CrashHandler::install()
{
    if (s_installed) {
        return;
    }
    s_installed = true;

#ifdef _WIN32
    // 提前初始化 DbgHelp 符号处理器，避免崩溃时才初始化可能失败
    initializeSymbols();

    // 注册 SEH 异常过滤器
    s_previousFilter = SetUnhandledExceptionFilter(crashExceptionFilter);

    // 注册纯虚函数调用处理器
    _set_purecall_handler(pureCallHandler);

    // 注册无效参数处理器
    _set_invalid_parameter_handler(invalidParameterHandler);

#else // Linux / macOS
    struct sigaction sa{};
    sa.sa_sigaction = crashSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;

    sigaction(SIGSEGV, &sa, &s_oldSigSegv);
    sigaction(SIGABRT, &sa, &s_oldSigAbrt);
    sigaction(SIGFPE, &sa, &s_oldSigFpe);
    sigaction(SIGBUS, &sa, &s_oldSigBus);
    sigaction(SIGILL, &sa, &s_oldSigIll);
#endif

    // 注册 std::terminate 处理器
    std::set_terminate(terminateHandler);

    // 注册内存分配失败处理器
    std::set_new_handler(newHandler);
}

void CrashHandler::uninstall()
{
    if (!s_installed) {
        return;
    }
    s_installed = false;

#ifdef _WIN32
    SetUnhandledExceptionFilter(s_previousFilter);
    _set_purecall_handler(nullptr);
    _set_invalid_parameter_handler(nullptr);

    // 清理 DbgHelp 符号
    if (s_symInitialized) {
        SymCleanup(GetCurrentProcess());
        s_symInitialized = false;
    }
#else
    sigaction(SIGSEGV, &s_oldSigSegv, nullptr);
    sigaction(SIGABRT, &s_oldSigAbrt, nullptr);
    sigaction(SIGFPE, &s_oldSigFpe, nullptr);
    sigaction(SIGBUS, &s_oldSigBus, nullptr);
    sigaction(SIGILL, &s_oldSigIll, nullptr);
#endif

    std::set_terminate(nullptr);
    std::set_new_handler(nullptr);
}

bool CrashHandler::isInstalled()
{
    return s_installed;
}

void CrashHandler::setCleanupCallback(CrashCleanupCallback callback)
{
    s_cleanupCallback = std::move(callback);
}

} // namespace mc::assert
