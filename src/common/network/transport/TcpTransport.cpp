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

#include "common/network/transport/TcpTransport.hpp"

#include "common/core/Constants.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <asio.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace mc::network::transport {
namespace {

// 网络与存档使用不同的执行资源，磁盘延迟不会阻塞 socket 的完成回调。
class _networkRuntime {
public:
    _networkRuntime()
        : m_work(asio::make_work_guard(context))
    {
        const auto count = std::clamp(std::thread::hardware_concurrency() / 4, 2u, 4u);
        for (u32 index = 0; index < count; ++index) {
            m_threads.emplace_back([this, index] {
                MC_TRACE_SET_THREAD_NAME("NetworkIO-" + std::to_string(index));
                context.run();
            });
        }
    }

    // jthread 的析构负责 join；停止事件循环后不存在等待自身的路径。
    //
    // NOLINTNEXTLINE(bugprone-exception-escape)
    ~_networkRuntime()
    {
        m_work.reset();
        context.stop();
        m_threads.clear();
    }

    asio::io_context context;

private:
    asio::executor_work_guard<asio::io_context::executor_type> m_work;
    std::vector<std::jthread> m_threads;
};

asio::io_context& _networkContext()
{
    static _networkRuntime runtime;
    return runtime.context;
}

} // namespace

struct TcpTransport::State : std::enable_shared_from_this<State> {
    State()
        : strand(asio::make_strand(_networkContext()))
        , socket(_networkContext())
        , timer(_networkContext())
    {}

    void _close()
    {
        MC_ASSERT_RELEASE(strand.running_in_this_thread());
        accepting.store(false);
        stopping.store(true);
        if (!connected.exchange(false)) return;
        asio::error_code ignored;
        (void)timer.cancel(ignored);
        if (const auto error = socket.cancel(ignored); error && error != asio::error::bad_descriptor) {
            spdlog::warn("TCP cancellation failed: {}", error.message());
        }
        if (const auto error = socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
            error && error != asio::error::not_connected && error != asio::error::bad_descriptor) {
            spdlog::warn("TCP shutdown failed: {}", error.message());
        }
        if (const auto error = socket.close(ignored); error) spdlog::warn("TCP close failed: {}", error.message());
        writes.clear();
        pendingBytes.store(0);
        DisconnectCallback disconnected;
        {
            std::lock_guard lock(callbackMutex);
            byteCallback = nullptr;
            disconnected = std::move(disconnectCallback);
        }
        if (disconnected) disconnected();
    }

    void _read()
    {
        if (!connected.load() || draining) return;
        socket.async_read_some(asio::buffer(readBuffer),
            asio::bind_executor(strand, [self = shared_from_this()](const asio::error_code& error, usize size) {
                if (!self->connected.load()) return;
                if (error) {
                    if (error != asio::error::eof && error != asio::error::operation_aborted &&
                        error != asio::error::connection_reset) {
                        spdlog::warn("TCP receive failed: {}", error.message());
                    }
                    self->_close();
                    return;
                }
                ByteCallback callback;
                {
                    std::lock_guard lock(self->callbackMutex);
                    callback = self->byteCallback;
                }
                if (callback) {
                    MC_TRACE_SCOPED_EVENT(::mc::trace::TraceEvents.Server.Network, "NetworkReceiveDecode");
                    callback(self->readBuffer.data(), size);
                }
                self->_read();
            }));
    }

    void _write()
    {
        MC_ASSERT_RELEASE(!writes.empty());
        auto bytes = writes.front();
        // 每批写入都有独立截止时间；旧批次取消的 timer 不得关闭后续批次。
        const auto generation = ++writeGeneration;
        timer.expires_after(std::chrono::milliseconds(WRITE_TIMEOUT_MS));
        timer.async_wait(
            asio::bind_executor(strand, [self = shared_from_this(), generation](const asio::error_code& error) {
                if (!error && generation == self->writeGeneration && self->connected.load()) {
                    spdlog::warn("TCP write timed out with {} pending bytes", self->pendingBytes.load());
                    self->_close();
                }
            }));
        asio::async_write(socket,
            asio::buffer(*bytes),
            asio::bind_executor(strand, [self = shared_from_this(), bytes](const asio::error_code& error, usize) {
                if (!self->connected.load()) return;
                asio::error_code ignored;
                (void)self->timer.cancel(ignored);
                if (error) {
                    if (error != asio::error::operation_aborted) {
                        spdlog::warn("TCP write failed: {}", error.message());
                    }
                    self->_close();
                    return;
                }
                self->pendingBytes.fetch_sub(bytes->size());
                self->writes.pop_front();
                if (!self->writes.empty())
                    self->_write();
                else if (self->draining)
                    self->_close();
            }));
    }

    Result<void> _send(std::shared_ptr<std::vector<u8>> bytes)
    {
        MC_ASSERT_RELEASE(strand.running_in_this_thread());
        if (!connected.load()) return Error(ErrorCode::ConnectionClosed, "TCP is closed", "TcpTransport::send");
        const usize count = pendingBytes.load();
        if (bytes->size() > MAX_PENDING_WRITE_BYTES - count) {
            spdlog::warn("TCP pending write byte limit exceeded");
            _close();
            return Error(ErrorCode::ResourceExhausted, "TCP pending write byte limit exceeded", "TcpTransport::send");
        }
        pendingBytes.store(count + bytes->size());
        const bool idle = writes.empty();
        writes.push_back(std::move(bytes));
        if (idle) _write();
        return Result<void>::ok();
    }

    asio::strand<asio::io_context::executor_type> strand;
    asio::ip::tcp::socket socket;
    asio::steady_timer timer;
    std::array<u8, 65536> readBuffer{};
    std::deque<std::shared_ptr<std::vector<u8>>> writes;
    std::atomic<bool> connected{false};
    std::atomic<bool> accepting{false};
    std::atomic<bool> stopping{false};
    u64 writeGeneration = 0;
    std::atomic<usize> pendingBytes{0};
    std::mutex submitMutex;
    usize pendingTasks = 0;
    bool receiving = false;
    bool draining = false;
    std::string peer;
    ByteCallback byteCallback;
    DisconnectCallback disconnectCallback;
    std::mutex callbackMutex;
};

TcpTransport::TcpTransport()
    : m_state(std::make_shared<State>())
{}
// 析构必须同步取消回调；此路径仅可能因 Asio 调度的内存分配失败而抛异常。
//
// NOLINTNEXTLINE(bugprone-exception-escape)
TcpTransport::~TcpTransport()
{
    close();
}

Result<void> TcpTransport::connect(const Endpoint& endpoint)
{
    asio::error_code error;
    asio::ip::tcp::resolver resolver(_networkContext());
    const auto endpoints = resolver.resolve(endpoint.host, std::to_string(endpoint.port), error);
    if (!error) asio::connect(m_state->socket, endpoints, error);
    if (error) return Error(ErrorCode::ConnectionFailed, error.message(), "TcpTransport::connect");
    m_state->peer = remoteAddress();
    m_state->connected.store(true);
    m_state->accepting.store(true);
    return Result<void>::ok();
}

Result<void> TcpTransport::attachConnectedSocket(asio::ip::tcp::socket socket)
{
    asio::error_code error;
    const auto endpoint = socket.remote_endpoint(error);
    if (error) return Error(ErrorCode::ConnectionFailed, error.message(), "TcpTransport::attachConnectedSocket");
    m_state->peer = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    const auto handle = socket.release(error);
    if (error) return Error(ErrorCode::ConnectionFailed, error.message(), "TcpTransport::attachConnectedSocket");
    const auto assigned = m_state->socket.assign(endpoint.protocol(), handle, error);
    if (assigned) {
        // assign 失败时原 socket 仍负责回收已释放的原生句柄。
        asio::error_code ignored;
        const auto restored = socket.assign(endpoint.protocol(), handle, ignored);
        MC_ASSERT_RELEASE_MSG(!restored, "Cannot restore ownership of the accepted TCP socket");
        return Error(ErrorCode::ConnectionFailed, error.message(), "TcpTransport::attachConnectedSocket");
    }
    m_state->connected.store(true);
    m_state->accepting.store(true);
    return Result<void>::ok();
}

Result<void> TcpTransport::startReceiving()
{
    {
        std::lock_guard lock(m_state->callbackMutex);
        if (!m_state->byteCallback) {
            return Error(ErrorCode::InvalidState, "TCP byte callback missing", "TcpTransport::startReceiving");
        }
    }
    return execute([state = m_state] {
        if (state->receiving)
            return Result<void>(
                Error(ErrorCode::InvalidState, "TCP receive already started", "TcpTransport::startReceiving"));
        state->receiving = true;
        state->_read();
        return Result<void>::ok();
    });
}

std::string TcpTransport::remoteAddress() const
{
    if (!m_state->peer.empty()) return m_state->peer;
    asio::error_code error;
    const auto endpoint = m_state->socket.remote_endpoint(error);
    return error ? std::string{} : endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
}

Result<void> TcpTransport::execute(Task task)
{
    auto state = m_state;
    if (state->strand.running_in_this_thread()) {
        if (!state->accepting.load())
            return Error(ErrorCode::ConnectionClosed, "TCP is closing", "TcpTransport::execute");
        return task();
    }
    std::lock_guard lock(state->submitMutex);
    if (!state->accepting.load()) {
        return Error(ErrorCode::ConnectionClosed, "TCP is not accepting tasks", "TcpTransport::execute");
    }
    if (state->pendingTasks >= MAX_PENDING_TASKS) {
        state->accepting.store(false);
        state->stopping.store(true);
        asio::post(state->strand, [state] {
            spdlog::warn("TCP outbound task limit exceeded");
            state->_close();
        });
        return Error(ErrorCode::ResourceExhausted, "TCP outbound task limit exceeded", "TcpTransport::execute");
    }
    ++state->pendingTasks;
    asio::post(state->strand, [state, task = std::move(task)] {
        {
            std::lock_guard taskLock(state->submitMutex);
            --state->pendingTasks;
        }
        if (!state->connected.load() || state->stopping.load()) return;
        MC_TRACE_SCOPED_EVENT(::mc::trace::TraceEvents.Server.Network, "NetworkPipelineTask");
        auto result = task();
        if (!result.success()) {
            spdlog::warn("TCP pipeline task failed: {}", result.error().toString());
            state->_close();
        }
    });
    return Result<void>::ok();
}

void TcpTransport::synchronize()
{
    if (m_state->strand.running_in_this_thread()) return;
    std::mutex mutex;
    std::condition_variable completed;
    bool ready = false;
    std::unique_lock lock(mutex);
    asio::post(m_state->strand, [&] {
        std::lock_guard callbackLock(mutex);
        ready = true;
        completed.notify_one();
    });
    completed.wait(lock, [&] { return ready; });
}

Result<void> TcpTransport::send(const u8* data, usize size, DeliveryHint /*hint*/)
{
    if (!m_state->connected.load()) return Error(ErrorCode::ConnectionClosed, "TCP is closed", "TcpTransport::send");
    if (size == 0) return Result<void>::ok();
    if (size > MAX_PENDING_WRITE_BYTES) {
        close();
        return Error(ErrorCode::ResourceExhausted, "TCP write exceeds pending byte limit", "TcpTransport::send");
    }
    auto bytes = std::make_shared<std::vector<u8>>(data, data + size);
    if (m_state->strand.running_in_this_thread()) return m_state->_send(std::move(bytes));
    return execute([state = m_state, bytes] { return state->_send(bytes); });
}

void TcpTransport::onBytes(ByteCallback callback)
{
    std::lock_guard lock(m_state->callbackMutex);
    m_state->byteCallback = std::move(callback);
}

void TcpTransport::onDisconnect(DisconnectCallback callback)
{
    std::lock_guard lock(m_state->callbackMutex);
    m_state->disconnectCallback = std::move(callback);
}

bool TcpTransport::isConnected() const noexcept
{
    return m_state->connected.load();
}
usize TcpTransport::pendingWriteBytes() const noexcept
{
    return m_state->pendingBytes.load();
}

void TcpTransport::closeAfterFlush()
{
    auto state = m_state;
    std::lock_guard lock(state->submitMutex);
    state->accepting.store(false);
    asio::post(state->strand, [state] {
        state->draining = true;
        if (state->writes.empty()) state->_close();
    });
}

void TcpTransport::close()
{
    auto state = m_state;
    state->accepting.store(false);
    state->stopping.store(true);
    if (state->strand.running_in_this_thread()) {
        state->_close();
        return;
    }
    asio::post(state->strand, [state] { state->_close(); });
    synchronize();
}

} // namespace mc::network::transport
