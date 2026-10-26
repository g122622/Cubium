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

package dev.horizonlab.diff;

import java.util.List;
import net.minecraft.SharedConstants;
import net.minecraft.core.registries.Registries;
import net.minecraft.server.Bootstrap;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.WorldLoader;
import net.minecraft.server.WorldStem;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.server.packs.repository.PackRepository;
import net.minecraft.server.packs.repository.ServerPacksSource;
import net.minecraft.world.level.storage.LevelStorageSource;

/**
 * 原版**战利品上下文需要的那一小块服务端**。
 *
 * <p>{@code LootParams} 构造函数只要求一个 {@code ServerLevel}，而 {@code ServerLevel}
 * 又要求一个 {@code MinecraftServer}。构建这两者必须走完整的 {@code WorldStem} 装载
 * （资源、注册表、标签），否则 {@code MinecraftServer} 的构造函数第一步就会读
 * {@code worldStem.registries()}。
 *
 * <p>所以这里只做「装载数据包 + 建一个世界」，**不跑 tick、不监听端口、不加载区块**：
 * 战利品掷骰用不到这些。
 */
final class LootLevel {
    private static LootLevel cached;

    final MinecraftServer server;
    final ServerLevel level;

    private LootLevel(MinecraftServer server, ServerLevel level) {
        this.server = server;
        this.level = level;
    }

    /** 进程内只建一次；建世界有开销，但远小于完整服务端启动。 */
    static synchronized LootLevel get() {
        if (cached == null) {
            cached = create();
        }
        return cached;
    }

    private static LootLevel create() {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        PackRepository packs = ServerPacksSource.createVanillaTrustedRepository();
        packs.reload();
        packs.setSelected(List.of("vanilla"));

        try {
            java.nio.file.Path runDirectory =
                java.nio.file.Path.of(System.getProperty("java.io.tmpdir"), "horizon-diff-loot");
            LevelStorageSource.LevelStorageAccess storage =
                LevelStorageSource.createDefault(runDirectory).createAccess("diff");

            // 与 IntegratedServer 同一套装载流程：WorldLoader 产出资源、可重载资源与
            // 分层注册表，再合成 WorldStem。
            var initConfig = new WorldLoader.InitConfig(
                new WorldLoader.PackConfig(packs, LootServerData.defaultConfiguration(), false, true),
                net.minecraft.commands.Commands.CommandSelection.ALL,
                net.minecraft.server.permissions.PermissionSet.ALL_PERMISSIONS);

            WorldStem stem = WorldLoader.load(
                initConfig,
                // 与 Main#createNewWorldData 一致：维度由 world_preset 烘焙出注册表，
                // 再取它的 dimensionsRegistryAccess；直接用 datapackDimensions 会缺主世界。
                LootServerData::newWorldData,
                (resourceManager, resources, registries, worldData) ->
                    new WorldStem(resourceManager, resources, registries, worldData),
                Runnable::run,
                Runnable::run)
                .join();

            var server = new HeadlessServer(storage, packs, stem);
            ServerLevel level = server.prepareLevel();
            return new LootLevel(server, level);
        } catch (Exception error) {
            throw new IllegalStateException("原版服务端世界构造失败：" + error, error);
        }
    }

    /**
     * 空的玩家列表：探针不接任何连接，但 {@code ServerLevel} 构造要读视距。
     *
     * <p>{@code PlayerList} 是抽象类，这里只需要它的「人数为 0、视距固定」语义。
     */
    private static final class EmptyPlayerList extends net.minecraft.server.players.PlayerList {
        EmptyPlayerList(
            MinecraftServer server,
            net.minecraft.core.LayeredRegistryAccess<net.minecraft.server.RegistryLayer> registries,
            net.minecraft.world.level.storage.PlayerDataStorage storage,
            net.minecraft.server.notifications.NotificationService notifications) {
            super(server, registries, storage, notifications);
        }
    }

    /** 只装载资源、建一个世界的最小服务端。 */
    private static final class HeadlessServer extends MinecraftServer {
        HeadlessServer(LevelStorageSource.LevelStorageAccess storage, PackRepository packs, WorldStem stem) {
            super(Thread.currentThread(), storage, packs, stem, java.net.Proxy.NO_PROXY, null, null,
                new net.minecraft.server.level.progress.LevelLoadListener() {
                    @Override
                    public void start(net.minecraft.server.level.progress.LevelLoadListener.Stage stage, int total) {}

                    @Override
                    public void update(net.minecraft.server.level.progress.LevelLoadListener.Stage stage, int done, int total) {}

                    @Override
                    public void finish(net.minecraft.server.level.progress.LevelLoadListener.Stage stage) {}

                    @Override
                    public void updateFocus(
                        net.minecraft.resources.ResourceKey<net.minecraft.world.level.Level> dimension,
                        net.minecraft.world.level.ChunkPos pos) {}
                });
        }

        ServerLevel prepareLevel() {
            // ServerLevel 构造时会读 getPlayerList().getViewDistance()/getSimulationDistance()，
            // 所以先塞一个空的玩家列表；探针不接玩家，视距取最小值降低内存占用。
            var playerList = new EmptyPlayerList(this, registries(), storageSource.createPlayerStorage(),
                new net.minecraft.server.notifications.NotificationService() {
                    @Override
                    public void playerJoined(net.minecraft.server.level.ServerPlayer player) {}

                    @Override
                    public void playerLeft(net.minecraft.server.level.ServerPlayer player) {}

                    @Override
                    public void serverStarted() {}

                    @Override
                    public void serverShuttingDown() {}

                    @Override
                    public void serverSaveStarted() {}

                    @Override
                    public void serverSaveCompleted() {}

                    @Override
                    public void serverActivityOccured() {}

                    @Override
                    public void playerOped(net.minecraft.server.players.ServerOpListEntry entry) {}

                    @Override
                    public void playerDeoped(net.minecraft.server.players.ServerOpListEntry entry) {}

                    @Override
                    public void playerAddedToAllowlist(net.minecraft.server.players.NameAndId profile) {}

                    @Override
                    public void playerRemovedFromAllowlist(net.minecraft.server.players.NameAndId profile) {}

                    @Override
                    public void ipBanned(net.minecraft.server.players.IpBanListEntry entry) {}

                    @Override
                    public void ipUnbanned(String address) {}

                    @Override
                    public void playerBanned(net.minecraft.server.players.UserBanListEntry entry) {}

                    @Override
                    public void playerUnbanned(net.minecraft.server.players.NameAndId profile) {}

                    @Override
                    public <T> void onGameRuleChanged(net.minecraft.world.level.gamerules.GameRule<T> rule, T value) {}

                    @Override
                    public void statusHeartbeat() {}
                });
            playerList.setViewDistance(2);
            playerList.setSimulationDistance(2);
            setPlayerList(playerList);

            // loadLevel() 内部调用 createLevels()，把主世界装进 m_levels。
            loadLevel();
            return overworld();
        }

        @Override
        protected boolean initServer() {
            return true;
        }

        @Override
        public boolean isDedicatedServer() {
            return false;
        }

        @Override
        public boolean useNativeTransport() {
            return false;
        }

        @Override
        public boolean isPublished() {
            return false;
        }

        @Override
        public int getMaxPlayers() {
            return 1;
        }

        @Override
        public boolean isSingleplayerOwner(net.minecraft.server.players.NameAndId profile) {
            return false;
        }

        @Override
        public net.minecraft.server.permissions.LevelBasedPermissionSet operatorUserPermissions() {
            return net.minecraft.server.permissions.LevelBasedPermissionSet.OWNER;
        }

        @Override
        public net.minecraft.server.permissions.PermissionSet getFunctionCompilationPermissions() {
            return net.minecraft.server.permissions.PermissionSet.ALL_PERMISSIONS;
        }

        @Override
        public boolean shouldRconBroadcast() {
            return false;
        }

        @Override
        protected net.minecraft.util.debugchart.SampleLogger getTickTimeLogger() {
            return null;
        }

        @Override
        public boolean isTickTimeLoggingEnabled() {
            return false;
        }

        @Override
        public int getRateLimitPacketsPerSecond() {
            return 0;
        }

        @Override
        public boolean shouldInformAdmins() {
            return false;
        }

        @Override
        public net.minecraft.SystemReport fillServerSystemReport(net.minecraft.SystemReport report) {
            return report;
        }
    }
}