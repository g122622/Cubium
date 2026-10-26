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

import com.mojang.serialization.Lifecycle;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import net.minecraft.core.LayeredRegistryAccess;
import net.minecraft.core.RegistryAccess;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.server.RegistryLayer;
import net.minecraft.server.ReloadableServerResources;
import net.minecraft.server.WorldLoader;
import net.minecraft.server.packs.resources.CloseableResourceManager;
import net.minecraft.server.packs.repository.PackRepository;
import net.minecraft.server.packs.repository.ServerPacksSource;
import net.minecraft.world.level.gamerules.GameRules;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.LevelSettings;
import net.minecraft.world.level.WorldDataConfiguration;
import net.minecraft.world.level.levelgen.WorldOptions;
import net.minecraft.world.level.levelgen.presets.WorldPresets;
import net.minecraft.world.level.storage.PrimaryLevelData;
import net.minecraft.world.level.storage.WorldData;
import net.minecraft.world.level.storage.loot.LootTable;

/**
 * 原版**服务端形态**的数据：战利品表、进阶、配方都挂在
 * {@link ReloadableServerResources} 上，光加载注册表拿不到。
 *
 * <p>{@link DataPackRegistries} 只做「注册表 + 标签」这一步，够生物群系、附魔这类
 * 注册表驱动数据用；战利品表在 1.21 之后是**资源**而不是注册表，必须走
 * {@link ReloadableServerResources#loadResources} 才存在。
 *
 * <p>这里照搬 <i>IntegratedServer</i> / <i>MinecraftServer</i> 的启动骨架：
 *
 * <ol>
 *   <li>打开原版数据包（{@code ServerPacksSource.createVanillaTrustedRepository}）；
 *   <li>{@link WorldLoader#load} 一次；它的 {@code ResultFactory} 里能同时拿到
 *       资源管理器、{@link ReloadableServerResources} 与分层注册表；
 *   <li>用这两样构造一个 {@link WorldStem}，并以它为准取战利品表。
 * </ol>
 *
 * <p>不构造服务端实例、不建世界：{@code LootTable#getRandomItems} 需要的是
 * {@code LootContext.Builder(LootParams)}，而 {@code LootParams} 需要一个
 * {@code ServerLevel} 只用于「查询世界状态」。本 harness 的三类上下文都不查询世界，
 * 因此改用 {@link HeadlessLevelAccess} 提供一个最小 {@code ServerLevel} 替身，
 * 避免为此启动整个服务端。
 */
final class LootServerData {
    private static LootServerData cached;

    final CloseableResourceManager resourceManager;
    final ReloadableServerResources resources;
    final LayeredRegistryAccess<RegistryLayer> registries;

    private LootServerData(
        CloseableResourceManager resourceManager,
        ReloadableServerResources resources,
        LayeredRegistryAccess<RegistryLayer> registries) {
        this.resourceManager = resourceManager;
        this.resources = resources;
        this.registries = registries;
    }

    /** 该进程内只加载一次：1300 多张表 + 全部资源的加载开销远大于掷骰。 */
    static synchronized LootServerData get() {
        if (cached == null) {
            cached = load();
        }
        return cached;
    }

    /** 取一张战利品表；不存在时返回 null。 */
    LootTable table(String id) {
        ResourceKey<LootTable> key = ResourceKey.create(Registries.LOOT_TABLE, Identifier.parse(id));
        return resources.fullRegistries()
            .lookup()
            .lookupOrThrow(Registries.LOOT_TABLE)
            .get(key)
            .map(holder -> holder.value())
            .orElse(null);
    }

    /** 全部战利品表 ID（排序后）。 */
    List<String> tableIds() {
        return resources.fullRegistries()
            .lookup()
            .lookupOrThrow(Registries.LOOT_TABLE)
            .listElementIds()
            .map(key -> key.identifier().toString())
            .sorted()
            .toList();
    }

    /** 注册表访问（世界生成、生物群系等 oracle 复用）。 */
    RegistryAccess registryAccess() {
        return registries.compositeAccess();
    }

    private static LootServerData load() {
        // 必须先探测版本：ServerPacksSource / KnownPack 的静态初始化会读 SharedConstants。
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
        try {
            PackRepository packs = vanillaRepository();
            packs.reload();
            packs.setSelected(List.of("vanilla"));

            WorldDataConfiguration configuration = WorldDataConfiguration.DEFAULT;
            var initConfig = new WorldLoader.InitConfig(
                new WorldLoader.PackConfig(packs, configuration, false, true),
                net.minecraft.commands.Commands.CommandSelection.ALL,
                net.minecraft.server.permissions.PermissionSet.ALL_PERMISSIONS);

            AtomicReference<ReloadableServerResources> resourcesRef = new AtomicReference<>();
            AtomicReference<LayeredRegistryAccess<RegistryLayer>> registriesRef = new AtomicReference<>();
            AtomicReference<CloseableResourceManager> managerRef = new AtomicReference<>();

            WorldLoader.load(
                initConfig,
                // 「世界数据」这一步：只需要数据包配置与维度注册表。
                context -> new WorldLoader.DataLoadOutput<>(context.dataConfiguration(), context.datapackDimensions()),
                (resourceManager, resources, registries, cookie) -> {
                    managerRef.set(resourceManager);
                    resourcesRef.set(resources);
                    registriesRef.set(registries);
                    return cookie;
                },
                Runnable::run,
                Runnable::run)
                .join();

            ReloadableServerResources resources = resourcesRef.get();
            LayeredRegistryAccess<RegistryLayer> registries = registriesRef.get();
            CloseableResourceManager resourceManager = managerRef.get();
            if (resources == null || registries == null || resourceManager == null) {
                throw new IllegalStateException("WorldLoader 未返回完整的服务端资源");
            }
            // 标签必须回写到静态注册表，否则 LootTable 解析时按标签查方块/物品会得到空集。
            resources.updateStaticRegistryTags();
            return new LootServerData(resourceManager, resources, registries);
        } catch (RuntimeException error) {
            throw new IllegalStateException("原版服务端数据加载失败：" + error, error);
        }
    }

    /**
     * 原版数据包仓库。
     *
     * <p>与 C++ 探针必须指向**同一份**数据：环境变量 `MC_DIFF_VANILLA_DATAPACK` 由
     * harness 两侧都设置好；未设置时回落到 jar 内置的 vanilla 数据包（纯 Java 侧调试用）。
     *
     * <p>用 `ServerPacksSource.createPackRepository(Path, DirectoryValidator)` 从目录建仓库：
     * 目录布局与正式服务端一致（父目录下每个子目录是一个数据包），因此直接传数据包的
     * 父目录即可。
     */
    private static PackRepository vanillaRepository() {
        String directory = System.getenv("MC_DIFF_VANILLA_DATAPACK");
        if (directory == null || directory.isBlank()) {
            return ServerPacksSource.createVanillaTrustedRepository();
        }
        var dataPackDirectory = java.nio.file.Path.of(directory);
        var parent = dataPackDirectory.getParent();
        if (parent == null) {
            throw new IllegalStateException("MC_DIFF_VANILLA_DATAPACK 必须是数据包目录：" + directory);
        }
        // 与 `createVanillaTrustedRepository` 相同的「信任一切」校验器：探针读的是本地
        // 已校验过的原版数据包，不需要符号链接安全策略。
        return ServerPacksSource.createPackRepository(
            parent, new net.minecraft.world.level.validation.DirectoryValidator(ignored -> true));
    }

    /** 原版默认数据配置（vanilla 数据包）。 */
    static WorldDataConfiguration defaultConfiguration() {
        return WorldDataConfiguration.DEFAULT;
    }

    /**
     * 像 {@code Main#createNewWorldData} 那样造一份新世界：normal 预设烘焙成维度注册表。
     *
     * @param context WorldLoader 提供的装载上下文（资源、注册表、数据包配置）
     */
    static WorldLoader.DataLoadOutput<WorldData> newWorldData(WorldLoader.DataLoadContext context) {
        WorldDataConfiguration configuration = context.dataConfiguration();
        var settings = new LevelSettings(
            "diff", GameType.CREATIVE, false, net.minecraft.world.Difficulty.NORMAL, false,
            new GameRules(configuration.enabledFeatures()), configuration);
        var dimensions = net.minecraft.world.level.levelgen.presets.WorldPresets
            .createNormalWorldDimensions(context.datapackWorldgen());
        var complete = dimensions.bake(context.datapackDimensions().lookupOrThrow(Registries.LEVEL_STEM));
        var lifecycle = complete.lifecycle().add(context.datapackWorldgen().allRegistriesLifecycle());
        var worldData = new PrimaryLevelData(
            settings, WorldOptions.testWorldWithRandomSeed(), complete.specialWorldProperty(), lifecycle);
        return new WorldLoader.DataLoadOutput<>(worldData, complete.dimensionsRegistryAccess());
    }
}
