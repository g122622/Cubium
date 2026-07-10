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

#include "../BenchmarkRegistry.hpp"

#include "common/perfetto/TraceEvents.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#else
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <filesystem>
#include <string>
#include <vector>

using namespace mc::trace;

namespace mc::benchmark {
namespace {

class ClientInitializeBenchmark final : public IBenchmarkCase {
public:
    [[nodiscard]] std::string name() const override { return "client_initialize"; }

    [[nodiscard]] Result<void> validateConfig(const CaseRuntimeConfig& config) const override
    {
        if (!config.parameters.contains("clientExecutable") || !config.parameters.at("clientExecutable").is_string()) {
            return Error(ErrorCode::InvalidArgument, "client_initialize requires string parameter: clientExecutable");
        }

        if (config.parameters.contains("timeoutMs") && !config.parameters.at("timeoutMs").is_number_integer()) {
            return Error(ErrorCode::InvalidArgument, "client_initialize optional parameter timeoutMs must be integer");
        }

        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> setUp(const CaseRuntimeConfig& config) override
    {
        m_clientExecutable = std::filesystem::path(config.parameters.at("clientExecutable").get<std::string>());
        if (!std::filesystem::exists(m_clientExecutable)) {
            return Error(ErrorCode::NotFound, "client_initialize executable not found: " + m_clientExecutable.string());
        }

        m_timeoutMs = 300000;
        if (config.parameters.contains("timeoutMs")) {
            m_timeoutMs = config.parameters.at("timeoutMs").get<i32>();
        }
        if (m_timeoutMs <= 0) {
            return Error(ErrorCode::InvalidArgument, "client_initialize timeoutMs must be > 0");
        }

        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> runOnce() override
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "ClientInitializeBenchmark::runOnce");

#ifdef _WIN32
        std::wstring commandLine = L"\"" + m_clientExecutable.wstring() + L"\" --benchmark-exit-after-initialize";
        std::vector<wchar_t> commandBuffer(commandLine.begin(), commandLine.end());
        commandBuffer.push_back(L'\0');

        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInfo{};
        const std::wstring workingDirectory = m_clientExecutable.parent_path().wstring();

        const BOOL createResult = CreateProcessW(m_clientExecutable.c_str(),
            commandBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            workingDirectory.c_str(),
            &startupInfo,
            &processInfo);
        if (createResult == FALSE) {
            return Error(ErrorCode::OperationFailed,
                "client_initialize failed to start process, win32=" + std::to_string(GetLastError()));
        }

        const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, static_cast<DWORD>(m_timeoutMs));
        if (waitResult == WAIT_TIMEOUT) {
            TerminateProcess(processInfo.hProcess, 124);
            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);
            return Error(
                ErrorCode::OperationFailed, "client_initialize timed out after " + std::to_string(m_timeoutMs) + " ms");
        }

        if (waitResult != WAIT_OBJECT_0) {
            const DWORD waitError = GetLastError();
            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);
            return Error(
                ErrorCode::OperationFailed, "client_initialize wait failed, win32=" + std::to_string(waitError));
        }

        DWORD exitCode = 0;
        if (GetExitCodeProcess(processInfo.hProcess, &exitCode) == FALSE) {
            const DWORD exitError = GetLastError();
            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);
            return Error(ErrorCode::OperationFailed,
                "client_initialize failed to query exit code, win32=" + std::to_string(exitError));
        }

        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);

        if (exitCode != 0) {
            return Error(
                ErrorCode::OperationFailed, "client_initialize process exited with code " + std::to_string(exitCode));
        }
#else
        std::string command = "\"" + m_clientExecutable.string() + "\" --benchmark-exit-after-initialize";
        int status = std::system(command.c_str());
        if (status == -1) {
            return Error(ErrorCode::OperationFailed, "client_initialize failed to start process");
        }
        int exitCode = WEXITSTATUS(status);
        if (exitCode != 0) {
            return Error(
                ErrorCode::OperationFailed, "client_initialize process exited with code " + std::to_string(exitCode));
        }
#endif

        return Result<void>::ok();
    }

    void tearDown() override
    {
        m_clientExecutable.clear();
        m_timeoutMs = 0;
    }

private:
    std::filesystem::path m_clientExecutable;
    i32 m_timeoutMs = 0;
};

const bool g_registered = []() {
    BenchmarkRegistry::instance().registerCase(
        "client_initialize", []() { return std::make_unique<ClientInitializeBenchmark>(); });
    return true;
}();

} // namespace
} // namespace mc::benchmark
