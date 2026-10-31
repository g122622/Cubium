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

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.commands.arguments.blocks.BlockStateParser;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.piston.PistonStructureResolver;

/**
 * 活塞推动结构的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.PistonMovementOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/piston_movement.cpp 同构：两侧读同一份输入（方块描述字符串 + 活塞位置 + 朝向 +
 * 是否伸出），各自调用生产的结构求解器，输出可推动标志、移动链与破坏链。
 *
 * <p>方块用**描述字符串**而不是数值 ID 表示：`block_state_names` 已经保证了方块名与属性在两侧
 * 一致，因此不必依赖状态 ID 映射，也不会把 ID 映射的偏差混进本能力的结论。
 *
 * <p>`list` 不枚举场景（场景完全由输入行携带），只写一个空文件，与探针的 `list` 行为对称。
 */
public final class PistonMovementOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private PistonMovementOracle() {}

    public static void main(String[] args) throws IOException {
        // `Level` 构造器要经 `registryAccess()` 建调色板工厂，注册表必须先加载。
        JanitorLevel.registries = DataPackRegistries.load();

        if (args.length == 2 && args[0].equals("list")) {
            Files.writeString(Path.of(args[1]), "", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: PistonMovementOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        JanitorLevel level = new JanitorLevel();
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            text.append(GSON.toJson(resolve(level, JsonParser.parseString(line).getAsJsonObject()))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject resolve(JanitorLevel level, JsonObject input) throws IOException {
        level.cells.clear();
        for (var cell : input.getAsJsonArray("cells")) {
            JsonArray entry = cell.getAsJsonArray();
            BlockPos pos = new BlockPos(entry.get(0).getAsInt(), entry.get(1).getAsInt(), entry.get(2).getAsInt());
            level.cells.put(pos, parse(entry.get(3).getAsString()));
        }

        JsonArray pistonEntry = input.getAsJsonArray("piston");
        BlockPos piston = new BlockPos(
                pistonEntry.get(0).getAsInt(), pistonEntry.get(1).getAsInt(), pistonEntry.get(2).getAsInt());
        Direction facing = Direction.from3DDataValue(input.get("dir").getAsInt());
        boolean extending = input.get("extending").getAsBoolean();

        PistonStructureResolver resolver = new PistonStructureResolver(level, piston, facing, extending);
        boolean canMove = resolver.resolve();

        JsonObject result = new JsonObject();
        result.addProperty("unit", input.get("unit").getAsString());
        result.addProperty("canMove", canMove);
        result.add("toMove", positions(resolver.getToPush()));
        result.add("toDestroy", positions(resolver.getToDestroy()));
        return result;
    }

    private static JsonArray positions(List<BlockPos> list) {
        JsonArray array = new JsonArray();
        for (BlockPos pos : list) {
            JsonArray entry = new JsonArray();
            entry.add(pos.getX());
            entry.add(pos.getY());
            entry.add(pos.getZ());
            array.add(entry);
        }
        return array;
    }

    /** 用原版命令的方块状态语法解析方块（与探针侧 `BlockStateArgumentType` 同一套语法）。 */
    private static BlockState parse(String text) throws IOException {
        try {
            // `Registry` 本身实现了 `HolderLookup.RegistryLookup`，可直接作为解析用的查找视图。
            return BlockStateParser.parseForBlock(BuiltInRegistries.BLOCK, text, false).blockState();
        } catch (Exception error) {
            throw new IOException("无法解析方块状态：" + text, error);
        }
    }

    /**
     * 承载方块表的无头关卡。
     *
     * <p>和 Horizon 的 map-backed 世界同思路：不生成地形、不加载区块，只回答「这个坐标是什么
     * 方块」。用反射分配实例以跳过构造器里的服务端设施——本能力只用到 `getBlockState` 与
     * `getMinBuildHeight`/`getMaxBuildHeight`/`getWorldBorder`，不触碰世界 tick 与实体。
     */
    private static final class JanitorLevel extends Level {
        /** 主世界的高度范围（-64..319），与原版 `DimensionType.OVERWORLD` 一致。 */
        private static final int MIN_Y = -64;

        private static final int HEIGHT = 384;

        /** 原版数据包注册表：`Level` 构造器要用它建立调色板工厂。 */
        private static DataPackRegistries registries;

        final Map<BlockPos, BlockState> cells = new HashMap<>();

        private JanitorLevel() {
            // Level 构造器内部会经 `registryAccess()` 建立 PalettedContainerFactory，
            // 因此注册表必须在构造前就位（用静态字段传给 super）。
            super(null, Level.OVERWORLD, registries.access, null, true, false, 0L, 0);
        }

        @Override
        public int getMinY() {
            return MIN_Y;
        }

        @Override
        public int getHeight() {
            return HEIGHT;
        }

        @Override
        public int getSeaLevel() {
            return 63;
        }

        @Override
        public BlockState getBlockState(BlockPos pos) {
            return cells.getOrDefault(pos, net.minecraft.world.level.block.Blocks.AIR.defaultBlockState());
        }

        // ---- 以下抽象成员本能力全部用不到，给出最简实现（被调用即为设计错误） ----

        @Override
        public void sendBlockUpdated(BlockPos pos, BlockState old, BlockState updated, int flags) {}

        @Override
        public void gameEvent(net.minecraft.core.Holder<net.minecraft.world.level.gameevent.GameEvent> event,
                net.minecraft.world.phys.Vec3 pos,
                net.minecraft.world.level.gameevent.GameEvent.Context context) {}

        @Override
        public long nextSubTickCount() {
            return 0L;
        }

        @Override
        public net.minecraft.world.flag.FeatureFlagSet enabledFeatures() {
            return net.minecraft.world.flag.FeatureFlags.DEFAULT_FLAGS;
        }

        @Override
        public net.minecraft.world.level.storage.LevelData getLevelData() {
            throw unsupported("getLevelData");
        }

        @Override
        public net.minecraft.server.MinecraftServer getServer() {
            return null;
        }

        @Override
        public net.minecraft.world.level.chunk.ChunkSource getChunkSource() {
            throw unsupported("getChunkSource");
        }

        @Override
        public net.minecraft.core.Holder<net.minecraft.world.level.biome.Biome> getUncachedNoiseBiome(
                int x, int y, int z) {
            throw unsupported("getUncachedNoiseBiome");
        }

        // ---- LevelReader / BlockGetter / BlockAndTintGetter 剩余抽象成员 ----

        @Override
        public net.minecraft.world.level.chunk.ChunkAccess getChunk(
                int x, int z, net.minecraft.world.level.chunk.status.ChunkStatus status, boolean load) {
            return null;
        }

        @Override
        public net.minecraft.world.level.border.WorldBorder getWorldBorder() {
            // canPush 会查询边界是否包含目标位置：给出默认（原点 29999984 半径）边界即可。
            return new net.minecraft.world.level.border.WorldBorder();
        }

        @Override
        public net.minecraft.world.ticks.LevelTickAccess<net.minecraft.world.level.block.Block> getBlockTicks() {
            throw unsupported("getBlockTicks");
        }

        @Override
        public net.minecraft.world.ticks.LevelTickAccess<net.minecraft.world.level.material.Fluid> getFluidTicks() {
            throw unsupported("getFluidTicks");
        }

        @Override
        public boolean hasChunk(int x, int z) {
            return false;
        }

        @Override
        public int getHeight(
                net.minecraft.world.level.levelgen.Heightmap.Types type, int x, int z) {
            return getMinY();
        }

        @Override
        public int getSkyDarken() {
            return 0;
        }

        @Override
        public net.minecraft.world.level.biome.BiomeManager getBiomeManager() {
            throw unsupported("getBiomeManager");
        }

        @Override
        public net.minecraft.world.level.dimension.DimensionType dimensionType() {
            throw unsupported("dimensionType");
        }

        @Override
        public net.minecraft.core.RegistryAccess registryAccess() {
            return registries.access;
        }

        @Override
        public net.minecraft.world.level.block.entity.BlockEntity getBlockEntity(BlockPos pos) {
            return null;
        }

        @Override
        public net.minecraft.world.level.material.FluidState getFluidState(BlockPos pos) {
            return getBlockState(pos).getFluidState();
        }

        @Override
        public net.minecraft.world.level.lighting.LevelLightEngine getLightEngine() {
            throw unsupported("getLightEngine");
        }

        @Override
        public float getShade(net.minecraft.core.Direction direction, boolean shade) {
            return 1.0f;
        }

        @Override
        public int getBlockTint(BlockPos pos, net.minecraft.world.level.ColorResolver resolver) {
            return 0;
        }

        // ---- EntityGetter：结构求解从不查询实体，一律返回空 ----

        @Override
        public List<net.minecraft.world.entity.Entity> getEntities(
                net.minecraft.world.entity.Entity except,
                net.minecraft.world.phys.AABB box,
                java.util.function.Predicate<? super net.minecraft.world.entity.Entity> predicate) {
            return List.of();
        }

        @Override
        public <T extends net.minecraft.world.entity.Entity> List<T> getEntities(
                net.minecraft.world.level.entity.EntityTypeTest<net.minecraft.world.entity.Entity, T> type,
                net.minecraft.world.phys.AABB box,
                java.util.function.Predicate<? super T> predicate) {
            return List.of();
        }

        @Override
        public List<? extends net.minecraft.world.entity.player.Player> players() {
            return List.of();
        }

        @Override
        public void playSound(net.minecraft.world.entity.Entity except, BlockPos pos,
                net.minecraft.sounds.SoundEvent sound, net.minecraft.sounds.SoundSource source,
                float volume, float pitch) {}

        @Override
        public void addParticle(net.minecraft.core.particles.ParticleOptions particle,
                double x, double y, double z, double vx, double vy, double vz) {}

        @Override
        public void levelEvent(net.minecraft.world.entity.Entity source, int type, BlockPos pos, int data) {}

        @Override
        public void playSeededSound(net.minecraft.world.entity.Entity except, double x, double y, double z,
                net.minecraft.core.Holder<net.minecraft.sounds.SoundEvent> sound,
                net.minecraft.sounds.SoundSource source, float volume, float pitch, long seed) {}

        @Override
        public void playSeededSound(net.minecraft.world.entity.Entity except, net.minecraft.world.entity.Entity source,
                net.minecraft.core.Holder<net.minecraft.sounds.SoundEvent> sound,
                net.minecraft.sounds.SoundSource category, float volume, float pitch, long seed) {}

        @Override
        public void explode(net.minecraft.world.entity.Entity source,
                net.minecraft.world.damagesource.DamageSource damageSource,
                net.minecraft.world.level.ExplosionDamageCalculator calculator, double x, double y, double z,
                float radius, boolean fire, net.minecraft.world.level.Level.ExplosionInteraction interaction,
                net.minecraft.core.particles.ParticleOptions smallParticle,
                net.minecraft.core.particles.ParticleOptions largeParticle,
                net.minecraft.util.random.WeightedList<net.minecraft.core.particles.ExplosionParticleInfo> blockParticles,
                net.minecraft.core.Holder<net.minecraft.sounds.SoundEvent> sound) {}

        @Override
        public String gatherChunkSourceStats() {
            return "piston_movement";
        }

        @Override
        public void setRespawnData(net.minecraft.world.level.storage.LevelData.RespawnData data) {}

        @Override
        public net.minecraft.world.level.storage.LevelData.RespawnData getRespawnData() {
            throw unsupported("getRespawnData");
        }

        @Override
        public net.minecraft.world.entity.Entity getEntity(int id) {
            return null;
        }

        @Override
        public java.util.Collection<net.minecraft.world.entity.boss.enderdragon.EnderDragonPart> dragonParts() {
            return List.of();
        }

        @Override
        protected net.minecraft.world.level.entity.LevelEntityGetter<net.minecraft.world.entity.Entity> getEntities() {
            // 空实体视图：结构求解从不查询实体。
            return new net.minecraft.world.level.entity.LevelEntityGetter<net.minecraft.world.entity.Entity>() {
                @Override
                public net.minecraft.world.entity.Entity get(int id) {
                    return null;
                }

                @Override
                public net.minecraft.world.entity.Entity get(java.util.UUID id) {
                    return null;
                }

                @Override
                public Iterable<net.minecraft.world.entity.Entity> getAll() {
                    return List.of();
                }

                @Override
                public <U extends net.minecraft.world.entity.Entity> void get(
                        net.minecraft.world.level.entity.EntityTypeTest<net.minecraft.world.entity.Entity, U> type,
                        net.minecraft.util.AbortableIterationConsumer<U> consumer) {}

                @Override
                public void get(
                        net.minecraft.world.phys.AABB box, java.util.function.Consumer<net.minecraft.world.entity.Entity> consumer) {}

                @Override
                public <U extends net.minecraft.world.entity.Entity> void get(
                        net.minecraft.world.level.entity.EntityTypeTest<net.minecraft.world.entity.Entity, U> type,
                        net.minecraft.world.phys.AABB box,
                        net.minecraft.util.AbortableIterationConsumer<U> consumer) {}
            };
        }

        @Override
        public net.minecraft.world.TickRateManager tickRateManager() {
            return new net.minecraft.world.TickRateManager();
        }

        @Override
        public net.minecraft.world.level.saveddata.maps.MapItemSavedData getMapData(
                net.minecraft.world.level.saveddata.maps.MapId id) {
            return null;
        }

        @Override
        public void destroyBlockProgress(int breakerId, BlockPos pos, int progress) {}

        @Override
        public net.minecraft.world.scores.Scoreboard getScoreboard() {
            return new net.minecraft.world.scores.Scoreboard();
        }

        @Override
        public net.minecraft.world.item.crafting.RecipeAccess recipeAccess() {
            throw unsupported("recipeAccess");
        }

        @Override
        public net.minecraft.world.attribute.EnvironmentAttributeSystem environmentAttributes() {
            throw unsupported("environmentAttributes");
        }

        @Override
        public net.minecraft.world.item.alchemy.PotionBrewing potionBrewing() {
            throw unsupported("potionBrewing");
        }

        @Override
        public net.minecraft.world.level.block.entity.FuelValues fuelValues() {
            throw unsupported("fuelValues");
        }

        private static UnsupportedOperationException unsupported(String member) {
            return new UnsupportedOperationException("piston_movement 不涉及 Level." + member);
        }
    }
}