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

#include "server/world/chunk/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/chunk/lifecycle/ChunkLifecycleController.hpp"
#include <cmath>

namespace mc::server {
ServerChunkManager::ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator)
    : m_lifecycle(std::make_unique<ChunkLifecycleController>(&world, std::move(generator)))
{}

ServerChunkManager::ServerChunkManager(std::unique_ptr<IChunkGenerator> generator)
    : m_lifecycle(std::make_unique<ChunkLifecycleController>(nullptr, std::move(generator)))
{}

ServerChunkManager::~ServerChunkManager() = default;

ChunkData* ServerChunkManager::tryToGetChunkInMem(ChunkCoord x, ChunkCoord z)
{
    return m_lifecycle->registry.chunk(x, z).get();
}

const ChunkData* ServerChunkManager::tryToGetChunkInMem(ChunkCoord x, ChunkCoord z) const
{
    return m_lifecycle->registry.chunk(x, z).get();
}

std::shared_ptr<ChunkData> ServerChunkManager::tryToGetChunkSharedInMem(ChunkCoord x, ChunkCoord z)
{
    return m_lifecycle->registry.chunk(x, z);
}

std::shared_ptr<const ChunkData> ServerChunkManager::tryToGetChunkSharedInMem(ChunkCoord x, ChunkCoord z) const
{
    return m_lifecycle->registry.chunk(x, z);
}

bool ServerChunkManager::hasChunkInMem(ChunkCoord x, ChunkCoord z) const
{
    return tryToGetChunkInMem(x, z) != nullptr;
}

void ServerChunkManager::_registerTicket(ChunkPos pos, world::chunk::ChunkLoadTicket ticket)
{
    m_lifecycle->demand.registerTicket(pos, std::move(ticket));
}

void ServerChunkManager::shutdown()
{
    m_lifecycle->shutdown();
}

void ServerChunkManager::stopScheduling()
{
    m_lifecycle->stopScheduling();
}

void ServerChunkManager::setWorkerPool(util::UniversalWorkerPool* pool)
{
    m_lifecycle->setWorkerPool(pool);
}

ChunkData* ServerChunkManager::requestChunkSync(ChunkCoord x, ChunkCoord z, const world::chunk::ChunkStatus& status)
{
    return m_lifecycle->requestChunkSync(x, z, status);
}

ChunkData* ServerChunkManager::requestFullChunkSync(ChunkCoord x, ChunkCoord z)
{
    return requestChunkSync(x, z, world::chunk::ChunkStatuses::FULL);
}

std::future<ChunkData*> ServerChunkManager::requestChunkAsync(
    ChunkCoord x, ChunkCoord z, const world::chunk::ChunkStatus& status)
{
    auto promise = std::make_shared<std::promise<ChunkData*>>();
    auto future = promise->get_future();
    m_lifecycle->submit(x, z, status, {}, promise);
    return future;
}

void ServerChunkManager::requestChunkAsync(
    ChunkCoord x, ChunkCoord z, const world::chunk::ChunkStatus& status, ChunkCallback callback)
{
    m_lifecycle->submit(x, z, status, std::move(callback), {});
}

void ServerChunkManager::requestChunkUnload(ChunkCoord x, ChunkCoord z)
{
    m_lifecycle->unloader.request(x, z);
}

void ServerChunkManager::updatePlayerPosition(PlayerId player, f64 x, f64 z)
{
    m_lifecycle->demand.updatePlayerPosition(player,
        static_cast<ChunkCoord>(std::floor(x / world::CHUNK_WIDTH)),
        static_cast<ChunkCoord>(std::floor(z / world::CHUNK_WIDTH)));
}

void ServerChunkManager::removePlayer(PlayerId player)
{
    m_lifecycle->demand.removePlayer(player);
}

void ServerChunkManager::forceChunk(ChunkCoord x, ChunkCoord z, bool force)
{
    m_lifecycle->demand.forceChunk(x, z, force);
}

void ServerChunkManager::addLightTicket(ChunkCoord x, ChunkCoord z)
{
    m_lifecycle->demand.registerTicket(
        world::chunk::TicketTypes::LIGHT, x, z, static_cast<i32>(world::chunk::ChunkLoadLevel::Full), ChunkPos(x, z));
    processTicketUpdates();
}

void ServerChunkManager::removeLightTicket(ChunkCoord x, ChunkCoord z)
{
    m_lifecycle->demand.releaseTicket(
        world::chunk::TicketTypes::LIGHT, x, z, static_cast<i32>(world::chunk::ChunkLoadLevel::Full), ChunkPos(x, z));
}

void ServerChunkManager::setViewDistance(i32 distance)
{
    m_lifecycle->demand.setViewDistance(distance);
}

void ServerChunkManager::setPlayerViewDistance(PlayerId player, i32 distance)
{
    m_lifecycle->demand.setPlayerViewDistance(player, distance);
}

void ServerChunkManager::setSuppressAutoUnloadForBenchmark(bool suppress)
{
    m_lifecycle->unloader.suppressAutomatic(suppress);
}

void ServerChunkManager::processTicketUpdates()
{
    m_lifecycle->processTicketUpdates();
}

void ServerChunkManager::processPendingChunkTasks()
{
    m_lifecycle->processPendingChunkTasks();
}

void ServerChunkManager::tick()
{
    m_lifecycle->tick();
}

void ServerChunkManager::setEntitySpawnCallback(EntitySpawnCallback callback)
{
    m_lifecycle->integration.setSpawnCallback(std::move(callback));
}

void ServerChunkManager::setChunkLoadedCallback(ChunkLoadedCallback callback)
{
    m_lifecycle->integration.setLoadedCallback(std::move(callback));
}

void ServerChunkManager::setChunkUnloadedCallback(ChunkLoadedCallback callback)
{
    m_lifecycle->integration.setUnloadedCallback(std::move(callback));
}

void ServerChunkManager::setTrackingChangeCallback(TrackingChangeCallback callback)
{
    m_lifecycle->demand.setTrackingChangeCallback(std::move(callback));
}

void ServerChunkManager::setChunkCacheCenterCallback(ChunkCacheCenterCallback callback)
{
    m_lifecycle->demand.setChunkCacheCenterCallback(std::move(callback));
}

bool ServerChunkManager::shouldChunkLoad(ChunkCoord x, ChunkCoord z) const
{
    return m_lifecycle->demand.shouldChunkLoad(x, z);
}

i32 ServerChunkManager::viewDistance() const
{
    return m_lifecycle->demand.viewDistance();
}

i32 ServerChunkManager::getChunkLevel(ChunkCoord x, ChunkCoord z) const
{
    return m_lifecycle->demand.getChunkLevel(x, z);
}

bool ServerChunkManager::isTickingChunk(ChunkCoord x, ChunkCoord z, i32 distance) const
{
    return m_lifecycle->demand.isTickingChunk(x, z, distance);
}

bool ServerChunkManager::isPlayerTracking(PlayerId player, ChunkCoord x, ChunkCoord z) const
{
    return m_lifecycle->demand.isPlayerTracking(player, x, z);
}

std::vector<PlayerId> ServerChunkManager::getTrackingPlayers(ChunkCoord x, ChunkCoord z) const
{
    return m_lifecycle->demand.getTrackingPlayers(x, z);
}

bool ServerChunkManager::isForcedChunk(ChunkCoord x, ChunkCoord z) const
{
    return m_lifecycle->demand.isForcedChunk(x, z);
}

std::vector<ChunkPos> ServerChunkManager::getForcedChunks() const
{
    return m_lifecycle->demand.getForcedChunks();
}

size_t ServerChunkManager::loadedChunkCount() const
{
    return m_lifecycle->registry.loadedCount();
}

size_t ServerChunkManager::lifecycleManagerCount() const
{
    return m_lifecycle->registry.holderCount();
}

size_t ServerChunkManager::pendingTaskCount() const
{
    return m_lifecycle->area.pendingTaskCount();
}

void ServerChunkManager::forEachLoadedChunk(const std::function<bool(ChunkData&)>& callback)
{
    m_lifecycle->forEachLoadedChunk(callback);
}

void ServerChunkManager::forEachLoadedChunk(const std::function<bool(const ChunkData&)>& callback) const
{
    m_lifecycle->forEachLoadedChunk([&](ChunkData& data) {
        return callback(data);
    });
}

void ServerChunkManager::forEachTickingChunk(const std::function<bool(ChunkData&)>& callback)
{
    m_lifecycle->forEachTickingChunk(callback);
}

IChunkGenerator* ServerChunkManager::generator()
{
    return m_lifecycle->steps.generator();
}

const IChunkGenerator* ServerChunkManager::generator() const
{
    return m_lifecycle->steps.generator();
}

void ServerChunkManager::_debugDumpStuckHolders()
{
    ChunkDiagnostics::dump(m_lifecycle->registry);
}

std::vector<ChunkDiagnosticSnapshot> ServerChunkManager::debugCollectPrimerStats() const
{
    return ChunkDiagnostics::collect(m_lifecycle->registry);
}
} // namespace mc::server
