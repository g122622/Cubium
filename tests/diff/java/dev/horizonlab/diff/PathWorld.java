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

import java.lang.reflect.Field;
import java.util.List;
import net.minecraft.SharedConstants;
import net.minecraft.commands.Commands;
import net.minecraft.server.Bootstrap;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.WorldLoader;
import net.minecraft.server.WorldStem;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.server.packs.repository.PackRepository;
import net.minecraft.server.packs.repository.ServerPacksSource;
import net.minecraft.server.permissions.LevelBasedPermissionSet;
import net.minecraft.server.permissions.PermissionSet;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.gamerules.GameRules;
import net.minecraft.world.level.LevelSettings;
import net.minecraft.world.level.WorldDataConfiguration;
import net.minecraft.world.level.storage.LevelStorageSource;

/**
 * 寻路 oracle 共用的**空**超平坦世界。
 *
 * <p>寻路求值器要的 {@code PathNavigationRegion} 必须以真实 {@code Level} 构造（它要读
 * {@code getChunkSource()} 与建造高度），而 {@code Mob} 也必须挂在真实世界上，因此这里建一个
 * 无头服务端世界。与原版一致的三点：
 *
 * <ol>
 *   <li>世界是**平的、空的**：所有地形由场景脚本按需用 {@code ServerLevel#setBlock} 摆出来，
 *       保证两侧（原版 jar 与 Cubium 探针）看到的是同一批方块；
 *   <li>世界只建一次：建世界比跑一次寻路贵几个数量级；
 *   <li>不推进任何 tick：没有方块刻、没有天气、没有刷怪，寻路结果因此完全确定。
 * </ol>
 *
 * <p>与 {@link LootLevel} 的区别：那个用默认（含地形生成配置的）世界数据，只为拿
 * {@code LootTable}；这里要**可写**方块，所以用超平坦预设并直接给区块写方块。
 */
final class PathWorld {
    private static PathWorld cached;

    /** 空世界的主世界。 */
    final ServerLevel level;

    private PathWorld(ServerLevel level) {
        this.level = level;
    }

    /** 进程内只建一次。 */
    static synchronized PathWorld get() {
        if (cached == null) {
            cached = create();
        }
        return cached;
    }

    private static PathWorld create() {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        PackRepository packs = ServerPacksSource.createVanillaTrustedRepository();
        packs.reload();
        packs.setSelected(List.of("vanilla"));

        try {
            java.nio.file.Path runDirectory = java.nio.file.Path.of(System.getProperty("java.io.tmpdir"), "horizon-diff-path");
            LevelStorageSource.LevelStorageAccess storage =
                LevelStorageSource.createDefault(runDirectory).createAccess("path");
            var initConfig = new WorldLoader.InitConfig(
                new WorldLoader.PackConfig(packs, WorldDataConfiguration.DEFAULT, false, true),
                Commands.CommandSelection.ALL,
                PermissionSet.ALL_PERMISSIONS);
            WorldStem stem = WorldLoader.load(
                initConfig,
                PathServerData::newWorldData,
                (resourceManager, resources, registries, worldData) ->
                    new WorldStem(resourceManager, resources, registries, worldData),
                Runnable::run,
                Runnable::run)
                .join();
            var server = new HeadlessServer(storage, packs, stem);
            return new PathWorld(server.prepareLevel());
        }
        catch (Exception error) {
            throw new IllegalStateException("原版服务端世界构造失败：" + error, error);
        }
    }

    /**
     * 清掉一片区域（把方块还原成空气），使每个场景都从同一块空白场地开始。
     *
     * <p>只清 {@code y ∈ [minY, minY + height)} 的一小块：场景都在这个范围里布置。
     */
    /**
     * 直接写入一格的方块状态，绕过方块的邻居更新与「能否存活」检查。
     *
     * <p>{@code ServerLevel#setBlock} 会触发 {@code updateNeighbourForOutputSignal} 与
     * 方块的 {@code neighborChanged}：火这类「需要支撑」的方块在没有实际 tick 的世界里会被
     * 立即移除，读回来仍是空气。差分要的是「把方块摆在那儿」，不是模拟放置流程，所以这里
     * 落到区块存储层直接写。
     */
    void setBlockDirect(int x, int y, int z, net.minecraft.world.level.block.state.BlockState state) {
        net.minecraft.core.BlockPos pos = new net.minecraft.core.BlockPos(x, y, z);
        level.getChunk(pos).setBlockState(pos, state);
    }

    void clear(int centerX, int centerZ, int radiusXZ, int minY, int height) {
        for (int x = centerX - radiusXZ; x <= centerX + radiusXZ; x++) {
            for (int z = centerZ - radiusXZ; z <= centerZ + radiusXZ; z++) {
                for (int y = minY; y < minY + height; y++) {
                setBlockDirect(x, y, z, net.minecraft.world.level.block.Blocks.AIR.defaultBlockState());
                }
            }
        }
    }

        /**
     * 清空服务端级别的「坐标 → 路径类型」缓存。
     *
     * <p>{@code PathfindingContext} 在 {@code ServerLevel} 上挂了一个 4096 项的
     * {@link net.minecraft.world.level.pathfinder.PathTypeCache}，按坐标取整格类型。差分里
     * 每个场景都复用同一批坐标（原点固定为 0），于是第二个场景会直接命中第一个场景的结果，
     * 表现为「摆了新方块却判定成旧类型」。原版世界里这张表靠方块更新失效，探针不改方块时
     * 它不会自己清——所以每个场景开始前必须显式清空。
     */
    void invalidatePathTypeCache() {
        // PathTypeCache 只有按坐标失效，没有整体清空；直接换一个新的实例最省事也最彻底。
        try {
            java.lang.reflect.Field field = ServerLevel.class.getDeclaredField("pathTypesByPosCache");
            field.setAccessible(true);
            field.set(level, new net.minecraft.world.level.pathfinder.PathTypeCache());
        }
        catch (ReflectiveOperationException error) {
            throw new IllegalStateException("无法重置路径类型缓存：" + error, error);
        }
    }

    /** 最小服务端：只装载资源并准备一个世界，不监听、不接玩家。 */
    private static final class HeadlessServer extends MinecraftServer {
        HeadlessServer(LevelStorageSource.LevelStorageAccess storage, PackRepository packs, WorldStem stem) {
            super(java.lang.Thread.currentThread(), storage, packs, stem, java.net.Proxy.NO_PROXY, null, null,
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
        public LevelBasedPermissionSet operatorUserPermissions() {
            return LevelBasedPermissionSet.OWNER;
        }

        @Override
        public PermissionSet getFunctionCompilationPermissions() {
            return PermissionSet.ALL_PERMISSIONS;
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

    /** 空玩家列表：{@code ServerLevel} 构造要读视距。 */
    private static final class EmptyPlayerList extends net.minecraft.server.players.PlayerList {
        EmptyPlayerList(
            MinecraftServer server,
            net.minecraft.core.LayeredRegistryAccess<net.minecraft.server.RegistryLayer> registries,
            net.minecraft.world.level.storage.PlayerDataStorage storage,
            net.minecraft.server.notifications.NotificationService notifications) {
            super(server, registries, storage, notifications);
        }
    }

    /** 反射读私有字段；诊断信息用。 */
    static Object field(Object owner, String name) {
        try {
            Field field = owner.getClass().getDeclaredField(name);
            field.setAccessible(true);
            return field.get(owner);
        }
        catch (ReflectiveOperationException error) {
            throw new IllegalStateException("无法读取字段 " + name + "：" + error, error);
        }
    }

    /** 世界设置里的难度（寻路的下落距离依赖它）。 */
    static LevelSettings settings() {
        return new LevelSettings("diff-path", GameType.CREATIVE, false, net.minecraft.world.Difficulty.NORMAL, false,
            new net.minecraft.world.level.gamerules.GameRules(WorldDataConfiguration.DEFAULT.enabledFeatures()),
            WorldDataConfiguration.DEFAULT);
    }
}
