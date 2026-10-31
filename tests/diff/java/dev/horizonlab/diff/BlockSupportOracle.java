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
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.function.Consumer;
import java.util.function.Predicate;
import net.minecraft.commands.arguments.blocks.BlockStateParser;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.core.Holder;
import net.minecraft.core.particles.ExplosionParticleInfo;
import net.minecraft.core.particles.ParticleOptions;
import net.minecraft.core.particles.ParticleTypes;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.server.MinecraftServer;
import net.minecraft.sounds.SoundEvent;
import net.minecraft.sounds.SoundSource;
import net.minecraft.util.AbortableIterationConsumer;
import net.minecraft.util.random.WeightedList;
import net.minecraft.world.TickRateManager;
import net.minecraft.world.attribute.EnvironmentAttributeSystem;
import net.minecraft.world.damagesource.DamageSource;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.boss.enderdragon.EnderDragonPart;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.flag.FeatureFlagSet;
import net.minecraft.world.flag.FeatureFlags;
import net.minecraft.world.item.alchemy.PotionBrewing;
import net.minecraft.world.item.crafting.RecipeAccess;
import net.minecraft.world.level.ColorResolver;
import net.minecraft.world.level.ExplosionDamageCalculator;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.biome.Biome;
import net.minecraft.world.level.biome.BiomeManager;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.entity.BlockEntity;
import net.minecraft.world.level.block.entity.FuelValues;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.border.WorldBorder;
import net.minecraft.world.level.chunk.ChunkAccess;
import net.minecraft.world.level.chunk.ChunkSource;
import net.minecraft.world.level.chunk.status.ChunkStatus;
import net.minecraft.world.level.dimension.DimensionType;
import net.minecraft.world.level.entity.EntityTypeTest;
import net.minecraft.world.level.entity.LevelEntityGetter;
import net.minecraft.world.level.gameevent.GameEvent;
import net.minecraft.world.level.levelgen.Heightmap;
import net.minecraft.world.level.lighting.LevelLightEngine;
import net.minecraft.world.level.material.FluidState;
import net.minecraft.world.level.saveddata.maps.MapId;
import net.minecraft.world.level.saveddata.maps.MapItemSavedData;
import net.minecraft.world.level.storage.LevelData;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;

/**
 * 方块支撑判定的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockSupportOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/block_support.cpp 同构：对每个方块、每个底座，回答「它能否站在这个底座上」
 * （原版 `BlockState#canSurvive`，对应 Cubium 的 `Block::isValidPosition`）。
 *
 * <p>世界只回答「这个坐标是什么方块」：底座放在待测位置的正下方，其余全为空气。
 * 继承 `Level` 以复用 `LevelReader` 的全部默认实现，只覆写 `getBlockState` 一族——
 * `canSurvive` 的 63 个覆写只读方块，不触碰世界 tick 与实体。
 */
public final class BlockSupportOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockSupportOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (var key : BuiltInRegistries.BLOCK.keySet()) {
                names.add(key.toString());
            }
            names.sort(Comparator.naturalOrder());
            Files.writeString(Path.of(args[1]), String.join("\n", names) + "\n", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockSupportOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        SupportWorld world = SupportWorld.create();
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            text.append(GSON.toJson(resolve(world, JsonParser.parseString(line).getAsJsonObject()))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject resolve(SupportWorld world, JsonObject caseObject) throws IOException {
        String blockName = caseObject.get("block").getAsString();
        BlockState state = parse(blockName);
        if (state == null) {
            JsonObject missing = new JsonObject();
            missing.addProperty("block", blockName);
            missing.addProperty("missing", true);
            return missing;
        }

        BlockPos origin = new BlockPos(0, 0, 0);
        JsonObject support = new JsonObject();
        for (var baseEntry : caseObject.getAsJsonArray("bases")) {
            String baseName = baseEntry.getAsString();
            world.cells.clear();
            BlockState base = parse(baseName);
            if (base == null) {
                base = Blocks.AIR.defaultBlockState();
            }
            // 与探针完全一致：底座同时放在正下方与四个水平侧面，
            // 以便同时评估「下方附着」（植物/压力板/站立告示牌）与「侧面附着」
            // （墙上告示牌、按钮、火把、灯笼、珊瑚）。
            world.cells.put(origin.below(), base);
            world.cells.put(origin.north(), base);
            world.cells.put(origin.south(), base);
            world.cells.put(origin.west(), base);
            world.cells.put(origin.east(), base);
            support.addProperty(baseName, state.canSurvive(world, origin));
        }

        JsonObject result = new JsonObject();
        result.addProperty("block", blockName);
        result.add("support", support);
        return result;
    }

    private static BlockState parse(String text) throws IOException {
        try {
            return BlockStateParser.parseForBlock(BuiltInRegistries.BLOCK, text, false).blockState();
        } catch (Exception error) {
            throw new IOException("无法解析方块状态：" + text, error);
        }
    }

    /** 只回答方块查询的无头关卡（与 `PistonMovementOracle.JanitorLevel` 同一思路）。 */
    private static final class SupportWorld extends Level {
        private static final int MIN_Y = -64;

        private static final int HEIGHT = 384;

        private static DataPackRegistries registries;

        final Map<BlockPos, BlockState> cells = new HashMap<>();

        static SupportWorld create() {
            registries = DataPackRegistries.load();
            return new SupportWorld();
        }

        private SupportWorld() {
            super(null, Level.OVERWORLD, registries.access, null, true, false, 0L, 0);
        }

        @Override
        public BlockState getBlockState(BlockPos pos) {
            return cells.getOrDefault(pos, Blocks.AIR.defaultBlockState());
        }

        @Override
        public FluidState getFluidState(BlockPos pos) {
            return getBlockState(pos).getFluidState();
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
        public WorldBorder getWorldBorder() {
            return new WorldBorder();
        }

        @Override
        public BlockEntity getBlockEntity(BlockPos pos) {
            return null;
        }

        @Override
        public int getHeight(Heightmap.Types type, int x, int z) {
            return getMinY();
        }

        @Override
        public int getSkyDarken() {
            return 0;
        }

        @Override
        public float getShade(Direction direction, boolean shade) {
            return 1.0f;
        }

        @Override
        public int getBlockTint(BlockPos pos, ColorResolver resolver) {
            return 0;
        }

        @Override
        public ChunkAccess getChunk(int x, int z, ChunkStatus status, boolean load) {
            return null;
        }

        @Override
        public boolean hasChunk(int x, int z) {
            return false;
        }

        @Override
        public BiomeManager getBiomeManager() {
            throw unsupported("getBiomeManager");
        }

        @Override
        public Holder<Biome> getUncachedNoiseBiome(int x, int y, int z) {
            throw unsupported("getUncachedNoiseBiome");
        }

        @Override
        public DimensionType dimensionType() {
            throw unsupported("dimensionType");
        }

        @Override
        public LevelLightEngine getLightEngine() {
            throw unsupported("getLightEngine");
        }

        /**
         * 光照统一约定为「晴天全亮」（15）。
         *
         * <p>作物等方块的 `canSurvive` 会查光照（`CropBlock.hasSufficientLight`）；
         * 直接覆写这个默认方法即可，不必构造真实光照引擎。探针侧 SupportWorld 同样返回 15。
         */
        @Override
        public int getRawBrightness(BlockPos pos, int amount) {
            return 15;
        }

        @Override
        public int getBrightness(net.minecraft.world.level.LightLayer layer, BlockPos pos) {
            return 15;
        }

        @Override
        public void sendBlockUpdated(BlockPos pos, BlockState old, BlockState updated, int flags) {}

        @Override
        public void playSeededSound(Entity except, double x, double y, double z, Holder<SoundEvent> sound,
                SoundSource source, float volume, float pitch, long seed) {}

        @Override
        public void playSeededSound(Entity except, Entity source, Holder<SoundEvent> sound,
                SoundSource category, float volume, float pitch, long seed) {}

        @Override
        public void explode(Entity source, DamageSource damageSource, ExplosionDamageCalculator calculator,
                double x, double y, double z, float radius, boolean fire, ExplosionInteraction interaction,
                ParticleOptions smallParticle, ParticleOptions largeParticle,
                WeightedList<ExplosionParticleInfo> blockParticles, Holder<SoundEvent> sound) {}

        @Override
        public String gatherChunkSourceStats() {
            return "block_support";
        }

        @Override
        public void setRespawnData(LevelData.RespawnData data) {}

        @Override
        public LevelData.RespawnData getRespawnData() {
            throw unsupported("getRespawnData");
        }

        @Override
        public Entity getEntity(int id) {
            return null;
        }

        @Override
        public Collection<EnderDragonPart> dragonParts() {
            return List.of();
        }

        @Override
        public TickRateManager tickRateManager() {
            return new TickRateManager();
        }

        @Override
        public MapItemSavedData getMapData(MapId id) {
            return null;
        }

        @Override
        public void destroyBlockProgress(int breakerId, BlockPos pos, int progress) {}

        @Override
        public net.minecraft.world.ticks.LevelTickAccess<net.minecraft.world.level.block.Block> getBlockTicks() {
            throw unsupported("getBlockTicks");
        }

        @Override
        public net.minecraft.world.ticks.LevelTickAccess<net.minecraft.world.level.material.Fluid> getFluidTicks() {
            throw unsupported("getFluidTicks");
        }

        @Override
        public net.minecraft.world.scores.Scoreboard getScoreboard() {
            return new net.minecraft.world.scores.Scoreboard();
        }

        @Override
        public RecipeAccess recipeAccess() {
            throw unsupported("recipeAccess");
        }

        @Override
        public EnvironmentAttributeSystem environmentAttributes() {
            throw unsupported("environmentAttributes");
        }

        @Override
        public PotionBrewing potionBrewing() {
            throw unsupported("potionBrewing");
        }

        @Override
        public FuelValues fuelValues() {
            throw unsupported("fuelValues");
        }

        @Override
        protected LevelEntityGetter<Entity> getEntities() {
            return new LevelEntityGetter<Entity>() {
                @Override
                public Entity get(int id) {
                    return null;
                }

                @Override
                public Entity get(UUID id) {
                    return null;
                }

                @Override
                public Iterable<Entity> getAll() {
                    return List.of();
                }

                @Override
                public <U extends Entity> void get(EntityTypeTest<Entity, U> type,
                        AbortableIterationConsumer<U> consumer) {}

                @Override
                public void get(AABB box, Consumer<Entity> consumer) {}

                @Override
                public <U extends Entity> void get(EntityTypeTest<Entity, U> type, AABB box,
                        AbortableIterationConsumer<U> consumer) {}
            };
        }

        @Override
        public void gameEvent(Holder<GameEvent> event, Vec3 pos, GameEvent.Context context) {}

        @Override
        public long nextSubTickCount() {
            return 0L;
        }

        @Override
        public FeatureFlagSet enabledFeatures() {
            return FeatureFlags.DEFAULT_FLAGS;
        }

        @Override
        public LevelData getLevelData() {
            throw unsupported("getLevelData");
        }

        @Override
        public MinecraftServer getServer() {
            return null;
        }

        @Override
        public ChunkSource getChunkSource() {
            throw unsupported("getChunkSource");
        }

        @Override
        public void playSound(Entity except, BlockPos pos, SoundEvent sound, SoundSource source,
                float volume, float pitch) {}

        @Override
        public void addParticle(ParticleOptions particle, double x, double y, double z,
                double vx, double vy, double vz) {}

        @Override
        public void levelEvent(Entity source, int type, BlockPos pos, int data) {}

        @Override
        public List<Entity> getEntities(Entity except, AABB box, Predicate<? super Entity> predicate) {
            return List.of();
        }

        @Override
        public <T extends Entity> List<T> getEntities(EntityTypeTest<Entity, T> type, AABB box,
                Predicate<? super T> predicate) {
            return List.of();
        }

        @Override
        public List<? extends Player> players() {
            return List.of();
        }

        private static UnsupportedOperationException unsupported(String member) {
            return new UnsupportedOperationException("block_support 不涉及 Level." + member);
        }
    }
}