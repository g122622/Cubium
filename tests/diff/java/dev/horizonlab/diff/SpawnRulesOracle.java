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
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.mojang.authlib.GameProfile;
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Locale;
import java.util.UUID;
import net.minecraft.commands.arguments.blocks.BlockStateParser;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Holder;
import net.minecraft.core.RegistryAccess;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.util.RandomSource;
import net.minecraft.world.Difficulty;
import net.minecraft.world.attribute.EnvironmentAttributeMap;
import net.minecraft.world.attribute.EnvironmentAttributeSystem;
import net.minecraft.world.attribute.EnvironmentAttributes;
import net.minecraft.world.entity.EntitySpawnReason;
import net.minecraft.world.entity.SpawnPlacements;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.flag.FeatureFlagSet;
import net.minecraft.world.flag.FeatureFlags;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.LightLayer;
import net.minecraft.world.level.biome.Biome;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.border.WorldBorder;
import net.minecraft.world.level.dimension.DimensionType;
import net.minecraft.world.level.levelgen.Heightmap;
import net.minecraft.world.level.material.FluidState;

/** 与 Horizon 一样，用受控 ServerLevel 直接运行原版谓词，同时记录其后一次随机抽样。 */
public final class SpawnRulesOracle {
    private static final Gson GSON = new Gson();

    private SpawnRulesOracle() {}

    private static final class TestLevel extends ServerLevel {
        TestLevel() { super(null, null, null, null, null, null, false, 0L, List.of(), false, null); }
        RegistryAccess access;
        Map<Integer, BlockState> column;
        int x, y, z, sky, block, darken, sea, surface;
        long seed;
        boolean thunder;
        Difficulty difficulty;
        DimensionType type;
        Holder<Biome> biome;
        EnvironmentAttributeSystem attributes;
        List<Player> players;
        WorldBorder border;
        @Override public RegistryAccess registryAccess() { return access; }
        @Override public BlockState getBlockState(BlockPos pos) {
            return pos.getX() == x && pos.getZ() == z
                ? column.getOrDefault(pos.getY(), Blocks.AIR.defaultBlockState()) : Blocks.AIR.defaultBlockState();
        }
        @Override public FluidState getFluidState(BlockPos pos) { return getBlockState(pos).getFluidState(); }
        @Override public int getBrightness(LightLayer layer, BlockPos pos) { return layer == LightLayer.SKY ? sky : block; }
        @Override public int getRawBrightness(BlockPos pos, int darkening) { return Math.max(block, sky - darkening); }
        @Override public int getSkyDarken() { return darken; }
        @Override public int getSeaLevel() { return sea; }
        @Override public boolean isThundering() { return thunder; }
        @Override public int getHeight(Heightmap.Types heightmap, int x, int z) { return surface; }
        @Override public long getSeed() { return seed; }
        @Override public long getGameTime() { return 0; }
        @Override public Difficulty getDifficulty() { return difficulty; }
        @Override public DimensionType dimensionType() { return type; }
        @Override public Holder<Biome> getBiome(BlockPos pos) { return biome; }
        @Override public EnvironmentAttributeSystem environmentAttributes() { return attributes; }
        @Override public WorldBorder getWorldBorder() { return border; }
        @Override public FeatureFlagSet enabledFeatures() { return FeatureFlags.REGISTRY.allFlags(); }
        @SuppressWarnings({"unchecked", "rawtypes"})
        @Override public List players() { return players; }
    }

    private static final class TestPlayer extends Player {
        private final GameType mode;
        TestPlayer(Level level, GameType mode) {
            super(level, new GameProfile(new UUID(1, 2), "oracle"));
            this.mode = mode;
        }
        @Override public GameType gameMode() { return mode; }
    }

    public static void main(String[] args) throws Exception {
        var registries = DataPackRegistries.load();
        if (args.length == 2 && args[0].equals("list")) {
            Files.write(Path.of(args[1]), BuiltInRegistries.ENTITY_TYPE.keySet().stream().map(Object::toString).sorted().toList(), StandardCharsets.UTF_8);
            return;
        }
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: SpawnRulesOracle list <units> | run <input> <output>");
        }
        // 构造函数会启动真实区块系统；这里只分配对象，并显式装入谓词可读取的全部状态。
        Field field = Class.forName("sun.misc.Unsafe").getDeclaredField("theUnsafe");
        field.setAccessible(true);
        Object unsafe = field.get(null);
        TestLevel level = (TestLevel) unsafe.getClass().getMethod("allocateInstance", Class.class).invoke(unsafe, TestLevel.class);
        var randomField = Level.class.getDeclaredField("random");
        long offset = (long) unsafe.getClass().getMethod("objectFieldOffset", Field.class).invoke(unsafe, randomField);
        unsafe.getClass().getMethod("putObject", Object.class, long.class, Object.class).invoke(unsafe, level, offset, RandomSource.create(0));
        level.access = registries.access;
        level.column = new HashMap<>();
        level.players = new ArrayList<>();
        level.border = new WorldBorder();
        var blocks = registries.lookup.lookupOrThrow(Registries.BLOCK);
        var biomes = registries.access.lookupOrThrow(Registries.BIOME);
        var dimensions = registries.access.lookupOrThrow(Registries.DIMENSION_TYPE);
        StringBuilder output = new StringBuilder();
        for (String line : Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            var input = JsonParser.parseString(line).getAsJsonObject();
            String id = input.get("entity").getAsString();
            var entity = BuiltInRegistries.ENTITY_TYPE.getOptional(Identifier.parse(id)).orElseThrow();
            JsonArray cases = new JsonArray();
            for (var element : input.getAsJsonArray("cases")) {
                var c = element.getAsJsonObject();
                level.x = c.get("x").getAsInt(); level.y = c.get("y").getAsInt(); level.z = c.get("z").getAsInt();
                level.sky = c.get("sky").getAsInt(); level.block = c.get("light").getAsInt(); level.darken = c.get("darken").getAsInt();
                level.sea = c.get("sea").getAsInt(); level.surface = c.get("surface").getAsInt(); level.seed = c.get("worldSeed").getAsLong();
                level.thunder = c.get("thunder").getAsBoolean(); level.difficulty = Difficulty.byName(c.get("difficulty").getAsString());
                level.type = dimensions.getValue(Identifier.parse(c.get("dimension").getAsString()));
                level.biome = biomes.get(Identifier.parse(c.get("biome").getAsString())).orElseThrow();
                level.attributes = EnvironmentAttributeSystem.builder().addConstantLayer(EnvironmentAttributeMap.builder()
                    .set(EnvironmentAttributes.SURFACE_SLIME_SPAWN_CHANCE, c.get("slimeChance").getAsFloat()).build()).build();
                level.column.clear();
                for (var entry : c.getAsJsonObject("blocks").entrySet()) {
                    level.column.put(level.y + Integer.parseInt(entry.getKey()), BlockStateParser.parseForBlock(blocks, entry.getValue().getAsString(), false).blockState());
                }
                level.players.clear();
                if (!c.get("player").isJsonNull()) {
                    var p = c.getAsJsonObject("player");
                    var player = new TestPlayer(level, GameType.byName(p.get("mode").getAsString()));
                    player.setPos(level.x + p.get("x").getAsDouble(), level.y + p.get("y").getAsDouble(), level.z + p.get("z").getAsDouble());
                    level.players.add(player);
                }
                var random = RandomSource.create(c.get("seed").getAsLong());
                var reason = EntitySpawnReason.valueOf(c.get("reason").getAsString().toUpperCase(Locale.ROOT));
                var pos = new BlockPos(level.x, level.y, level.z);
                JsonObject answer = new JsonObject();
                answer.addProperty("name", c.get("name").getAsString());
                answer.addProperty("positionOk", SpawnPlacements.isSpawnPositionOk(entity, level, pos));
                answer.addProperty("rulesOk", SpawnPlacements.checkSpawnRules(entity, level, reason, pos, random));
                answer.addProperty("nextRandom", random.nextInt(1000));
                cases.add(answer);
            }
            JsonObject result = new JsonObject(); result.addProperty("entity", id); result.add("cases", cases);
            output.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(Path.of(args[2]), output, StandardCharsets.UTF_8);
    }
}
