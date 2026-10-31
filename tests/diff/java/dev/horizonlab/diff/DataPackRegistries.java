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

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.resources.ResourceKey;
import java.util.stream.Stream;
import net.minecraft.SharedConstants;
import net.minecraft.core.HolderLookup;
import net.minecraft.core.Registry;
import net.minecraft.core.RegistryAccess;
import net.minecraft.resources.RegistryDataLoader;
import net.minecraft.server.Bootstrap;
import net.minecraft.server.RegistryLayer;
import net.minecraft.server.ReloadableServerRegistries;
import net.minecraft.server.packs.PackType;
import net.minecraft.server.packs.repository.ServerPacksSource;
import net.minecraft.server.packs.resources.MultiPackResourceManager;
import net.minecraft.tags.TagLoader;

/**
 * 把原版数据包加载成服务端形态的注册表，供数据驱动的 oracle 使用。
 *
 * <p>1.21 起附魔、生物变种等注册表不再写在代码里，而是数据包 JSON。只调
 * {@link Bootstrap#bootStrap()} 拿不到它们，必须像服务端启动那样：
 *
 * <ol>
 *   <li>打开 jar 内置的 vanilla 数据包；
 *   <li>{@link RegistryDataLoader} 载入同步注册表层（含附魔、生物变种……）；
 *   <li>{@link ReloadableServerRegistries#reload} 载入可重载层（含战利品表、世界生成）。
 * </ol>
 *
 * <p>缺了第 3 步，同步层里引用「标签必须非空」的注册表（生物变种等）会校验失败；
 * 这些注册表只有打上标签之后才算加载完成。
 */
public final class DataPackRegistries {
    /** 服务端形态的注册表（同步层 + 可重载层合成）。 */
    public final RegistryAccess access;

    /** 合并后的查找视图（带标签）。 */
    public final HolderLookup.Provider lookup;

    /**
     * 原版数据包本身。
     *
     * 结构模板（`.nbt`）与其它非 JSON 资源不在注册表里，只能按路径从数据包读；
     * 结构生成的参照实现靠它构造模板管理器。
     */
    public final MultiPackResourceManager resourceManager;

    /**
     * 世界生成层单独加载的那一份注册表。
     *
     * 结构生成会从 {@code RegistryAccess} 里反复取模板池、结构处理器等注册表；同步层与
     * 可重载层都不含它们，只有这份 WORLDGEN 层视图才有。
     */
    public final RegistryAccess worldGenAccess;

    private DataPackRegistries(RegistryAccess access, HolderLookup.Provider lookup,
            MultiPackResourceManager resourceManager, RegistryAccess worldGenAccess) {
        this.access = access;
        this.lookup = lookup;
        this.resourceManager = resourceManager;
        this.worldGenAccess = worldGenAccess;
    }

    /** 按服务端启动的方式加载原版数据包。只应调用一次。 */
    public static DataPackRegistries load() {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        var repo = ServerPacksSource.createVanillaTrustedRepository();
        repo.reload();
        repo.setSelected(List.of("vanilla"));
        var resources = new MultiPackResourceManager(PackType.SERVER_DATA, repo.openAllSelected());
        var layers = RegistryLayer.createRegistryAccess();
        var staticTags = TagLoader.loadTagsForExistingRegistries(resources, layers.getLayer(RegistryLayer.STATIC));
        var context = TagLoader.buildUpdatedLookups(layers.getAccessForLoading(RegistryLayer.WORLDGEN), staticTags);
        RegistryAccess.Frozen world = RegistryDataLoader.load(resources, context, RegistryDataLoader.SYNCHRONIZED_REGISTRIES);
        // 世界生成层（结构集、模板池……）不在同步层里，要按 WORLDGEN 层单独加载一次；
        // 可重载层只含战利品、配方等。参照实现会同时用到两层，因此合并成一份查找视图。
        RegistryAccess.Frozen worldGen =
                RegistryDataLoader.load(resources, context, RegistryDataLoader.WORLDGEN_REGISTRIES);
        var worldLayers = layers.replaceFrom(RegistryLayer.WORLDGEN, world);
        var reloaded = ReloadableServerRegistries.reload(worldLayers, staticTags, resources, Runnable::run).join();
        var reloadedLookup = reloaded.lookupWithUpdatedTags();
        var worldGenLookup = TagLoader.buildUpdatedLookups(worldGen, staticTags);
        Map<ResourceKey<?>, HolderLookup.RegistryLookup<?>> merged = new LinkedHashMap<>();
        reloadedLookup.listRegistries().forEach(registry -> merged.put(registry.key(), registry));
        // 世界生成层优先：同一注册表两层都有时，取带世界生成内容的那一份。
        worldGenLookup.forEach(registry -> merged.put(registry.key(), registry));
        var lookup = HolderLookup.Provider.create(merged.values().stream());
        staticTags.forEach(Registry.PendingTags::apply);
        return new DataPackRegistries(reloaded.layers().compositeAccess(), lookup, resources, worldGen);
    }
}