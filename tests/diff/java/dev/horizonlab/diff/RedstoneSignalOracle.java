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
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.RedStoneWireBlock;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.BlockStateProperties;
import net.minecraft.world.level.block.state.properties.RedstoneSide;

/**
 * 红石信号的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RedstoneSignalOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.RedstoneSignalOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与探针共用同一张场景表（5×5×3 的薄片，主语固定在中心格，紧贴主语的六个邻居随场景
 * 变化），按原版自己的三条入口逐一提问，输出同名 JSON 字段：
 *
 * <ul>
 *   <li>{@code signal}       — {@code SignalGetter#getSignal(pos, dir)}，即电路真正「看到」的值：
 *       先问方块自己，若是红石导体再叠加 {@code getDirectSignalTo}。</li>
 *   <li>{@code weak}         — {@code BlockState#getSignal}（常说的「弱信号」）。</li>
 *   <li>{@code strong}       — {@code BlockState#getDirectSignal}（常说的「强信号」）。</li>
 *   <li>{@code bestNeighbor} — {@code SignalGetter#getBestNeighborSignal}。</li>
 * </ul>
 *
 * <p>「quiet」两段复刻 {@code RedStoneWireBlock#getBlockSignal} 的做法：先把红石线的
 * {@code shouldSignal} 置 false（这样它不再用自己的旧功率回灌），问完再置回 true。
 * 该字段是原版独有的瞬时开关，不存在于任何公开 API，只能通过反射读写。两侧的探针都
 * 显式表达同一个状态，比的是「关掉自我保护之后，邻居到底能给你多少」。
 *
 * <p>地形通过 {@code LevelChunk#setBlockState} 直接写进存储，绕开 {@code updateShape}：
 * 本能力要的是「给定地形下各方块怎么回答」，而不是「摆出这块地形会不会触发连锁更新」。
 * 连锁更新留给 GameTest，那里才有真实的 tick 与更新队列。
 */
public final class RedstoneSignalOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private RedstoneSignalOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (Scenario scenario : Scenario.all()) {
                text.append(scenario.unit()).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: RedstoneSignalOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String unit = request.get("unit").getAsString();

            Scenario scenario = Scenario.byName(unit);
            JsonObject result = new JsonObject();
            result.addProperty("unit", unit);
            if (scenario == null) {
                result.addProperty("ok", false);
                result.addProperty("error", "unknown scenario");
            } else {
                runScenario(scenario, result);
            }
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text.toString(), StandardCharsets.UTF_8);
    }

    private static void runScenario(Scenario scenario, JsonObject result) {
        // scenario.subject() 是「方块ID[属性=值,...]」的完整记法，注册表只认前半段。
        String blockId = scenario.subject().substring(0, indexOfBracket(scenario.subject()));
        Block block = BuiltInRegistries.BLOCK.getValue(Identifier.parse(blockId));
        if (block == null || block == Blocks.AIR) {
            result.addProperty("ok", false);
            result.addProperty("error", "unknown block");
            return;
        }

        ServerLevel level = LootLevel.get().level;
        BlockPos center = new BlockPos(scenario.baseX() + 8, 64, 8);
        ensureChunkLoaded(level, center);
        clearArea(level, center);

        BlockState subject = parseState(block, scenario.subject());
        buildTerrain(level, scenario, center);
        place(level, center, subject);

        result.addProperty("ok", true);

        // 第 1 段：不动开关。必须先把答案全部取完，否则第 2 段关掉的开关会污染这里。
        result.add("signal", directionMap(direction -> level.getSignal(center, direction)));
        result.add("weak", directionMap(direction -> subject.getSignal(level, center, direction)));
        result.add("strong", directionMap(direction -> subject.getDirectSignal(level, center, direction)));
        result.addProperty("bestNeighbor", level.getBestNeighborSignal(center));

        // 第 2 段：关掉红石线自身的输出，对应原版 getBlockSignal。
        RedStoneWireBlock wireBlock =
            block instanceof RedStoneWireBlock wire ? wire : (RedStoneWireBlock) Blocks.REDSTONE_WIRE;
        setShouldSignal(wireBlock, false);
        try {
            result.add("quietSignal", directionMap(direction -> level.getSignal(center, direction)));
            result.addProperty("quietBestNeighbor", level.getBestNeighborSignal(center));
        } finally {
            // 开关是全局瞬时状态：无论成功与否都必须恢复，否则会污染后续场景。
            setShouldSignal(wireBlock, true);
        }

        if (block instanceof RedStoneWireBlock) {
            JsonObject wire = new JsonObject();
            // 与探针同一口径：在 shouldSignal 关闭的状态下取，才不会被红石线自己的旧功率回灌。
            wire.addProperty("neighborPower", quietBestNeighbor(result));
            // 红石线自身的功率：原版 updatePowerStrength 里 calculateTargetStrength =
            // max(getBlockSignal, getIncomingWireSignal)，此刻 shouldSignal 仍为 false，
            // getBlockSignal 正好等于 quietBestNeighbor。
            wire.addProperty("input", Math.max(quietBestNeighbor(result), incomingWireSignal(level, center)));

            JsonObject links = new JsonObject();
            for (Direction direction : Direction.Plane.HORIZONTAL) {
                BlockState state = level.getBlockState(center);
                RedstoneSide side = state.getValue(RedStoneWireBlock.PROPERTY_BY_DIRECTION.get(direction));
                links.addProperty(propertyName(direction), side == RedstoneSide.NONE ? "none" : "side");
            }
            wire.add("links", links);
            result.add("wire", wire);
        }
    }

    private static int indexOfBracket(String text) {
        int bracket = text.indexOf('[');
        return bracket < 0 ? text.length() : bracket;
    }

    private static int quietBestNeighbor(JsonObject result) {
        return result.has("quietBestNeighbor") ? result.get("quietBestNeighbor").getAsInt() : 0;
    }

    /**
     * 相邻红石线朝本格提供的功率（原版 {@code getIncomingWireSignal}）。
     *
     * <p>只取「水平连接 + 上坡」的邻居：原版对每个可见方向先看同层的红石线，再看比它高的
     * 那一格（信号沿斜坡向上传输）。这里逐条复刻，避免两侧对「上坡」的理解出现偏差。
     */
    private static int incomingWireSignal(ServerLevel level, BlockPos pos) {
        int best = 0;
        for (Direction direction : Direction.Plane.HORIZONTAL) {
            BlockPos side = pos.relative(direction);
            BlockState sideState = level.getBlockState(side);
            if (sideState.is(Blocks.REDSTONE_WIRE)) {
                best = Math.max(best, sideState.getValue(RedStoneWireBlock.POWER));
            }
            if (sideState.isRedstoneConductor(level, side)) {
                BlockPos up = side.above();
                BlockState upState = level.getBlockState(up);
                if (upState.is(Blocks.REDSTONE_WIRE)) {
                    best = Math.max(best, upState.getValue(RedStoneWireBlock.POWER));
                }
            }
        }
        return best == 0 ? 0 : best - 1;
    }

    private static JsonObject directionMap(java.util.function.ToIntFunction<Direction> query) {
        JsonObject map = new JsonObject();
        for (Direction direction : Direction.values()) {
            map.addProperty(propertyName(direction), query.applyAsInt(direction));
        }
        return map;
    }

    private static String propertyName(Direction direction) {
        return direction.getSerializedName();
    }

    /**
     * 反射读写 {@code RedStoneWireBlock#shouldSignal}。
     *
     * <p>它是 {@code private boolean}：原版只在重算功率时临时置 false，没有 getter/setter。
     * oracle 不改原版代码，用反射复刻同一条查询路径，保证比的是原版自己的算法。
     */
    private static void setShouldSignal(RedStoneWireBlock block, boolean value) {
        try {
            Field field = RedStoneWireBlock.class.getDeclaredField("shouldSignal");
            field.setAccessible(true);
            field.setBoolean(block, value);
        } catch (ReflectiveOperationException error) {
            throw new IllegalStateException("RedStoneWireBlock#shouldSignal 不可访问", error);
        }
    }

    /**
     * 把 {@code 方块ID[属性=值,...]} 解析成完整状态。
     *
     * <p>与探针的 {@code parseSubject} 同一口径：从默认状态出发，逐条覆盖属性。属性名找不到
     * 或值非法时直接抛错——静默退回默认状态会让两侧地形悄悄不同，差分也就失去意义。
     */
    @SuppressWarnings({"unchecked", "rawtypes"})
    private static BlockState parseState(Block block, String text) {
        BlockState state = block.defaultBlockState();
        int bracket = text.indexOf('[');
        if (bracket < 0) {
            return state;
        }
        String body = text.substring(bracket + 1, text.length() - 1);
        for (String pair : body.split(",")) {
            int equals = pair.indexOf('=');
            String name = pair.substring(0, equals);
            String value = pair.substring(equals + 1);
            var property = state.getProperties().stream()
                .filter(candidate -> candidate.getName().equals(name))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("属性不存在：" + name));
            String serialized = property.getName();
            state = state.setValue((net.minecraft.world.level.block.state.properties.Property) property,
                (Comparable) property.getValue(value).orElseThrow(
                    () -> new IllegalArgumentException("属性值非法：" + serialized + "=" + value)));
        }
        return state;
    }

    private static void ensureChunkLoaded(ServerLevel level, BlockPos center) {
        int cx = center.getX() >> 4;
        int cz = center.getZ() >> 4;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                level.getChunk(cx + dx, cz + dz);
            }
        }
    }

    /**
     * 直接写进区块的方块存储，不触发连锁更新（理由见类注释）。
     *
     * <p>标志取 {@code UPDATE_ALL & ~UPDATE_NEIGHBORS}：既让方块真的落进区块存储，又屏蔽
     * 「我的变化会传导给邻居」——地形是逐格铺的，传导会让先摆的红石线按未必最终的邻居
     * 提前改写自己的连接属性，两侧的中间态一旦不同，比出来的差异就与地形无关了。
     */
    private static void place(ServerLevel level, BlockPos pos, BlockState state) {
        level.getChunk(pos.getX() >> 4, pos.getZ() >> 4)
            // UPDATE_SKIP_ALL_SIDEEFFECTS = 816：只把方块写进存储，不触发 onPlace /
            // affectNeighborsAfterRemoval / 邻居更新。红石线的 onPlace 会调 updatePowerStrength，
            // 而它第一条就检查「脚下有没有支撑」——逐格铺地形时支撑可能还没摆上，红石线会被
            // 自己判为非法、原地变回空气。本能力要的是「给定地形下各方块怎么回答」，
            // 不是「摆放过程中发生了什么」。
            .setBlockState(pos, state, Block.UPDATE_SKIP_ALL_SIDEEFFECTS);
    }

    private static void clearArea(ServerLevel level, BlockPos center) {
        for (int dx = -3; dx <= 3; dx++) {
            for (int dz = -3; dz <= 3; dz++) {
                // 清空范围必须比地形本身更大一圈：地形占 y = -2 .. +2（下层中心格在 -2，
                // 上层中心格在 +2），少清一层就会把上一次场景遗留的方块留在原地。
                for (int dy = -3; dy <= 3; dy++) {
                    place(level, center.offset(dx, dy, dz), Blocks.AIR.defaultBlockState());
                }
            }
        }
    }

    private static void buildTerrain(ServerLevel level, Scenario scenario, BlockPos center) {
        // 三层按「中心 / 上 / 下」逐层铺：显式列出层与 y 的对应，不能靠数组引用比较——
        // 上一层若有内容、下一层也有，比较写错就会把下层内容堆到上层去。
        String[][] layers = {scenario.plane(), scenario.above(), scenario.below()};
        int[] heights = {0, 1, -1};
        for (int index = 0; index < layers.length; index++) {
            String[] layer = layers[index];
            int y = heights[index];
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 5; col++) {
                    if (y == 0 && row == 2 && col == 2) {
                        // 只有中心层（y=0）的中心格由调用方单独放主语；
                        // 上下两层的中心格是真正的邻居（例如火把上方的石头），必须照常写。
                        continue;
                    }
                    String cell = layer[row * 5 + col];
                    BlockState state = cellToState(cell);
                    if (state == null) {
                        continue;
                    }
                    // 行号增大对应 +Z（南），列号增大对应 +X（东），与探针一致。
                    place(level, center.offset(col - 2, y, row - 2), state);
                }
            }
        }
    }

    /** 地形字符 → 方块状态。未知字符返回 null（该格留空）。 */
    private static BlockState cellToState(String cell) {
        return switch (cell) {
            case ".", "" -> null;
            case "S" -> Blocks.STONE.defaultBlockState();
            case "G" -> Blocks.GLASS.defaultBlockState();
            case "s" -> Blocks.SMOOTH_STONE_SLAB.defaultBlockState();
            case "R" -> Blocks.REDSTONE_BLOCK.defaultBlockState();
            case "T" -> Blocks.REDSTONE_TORCH.defaultBlockState();
            case "t" -> Blocks.REDSTONE_TORCH.defaultBlockState().setValue(BlockStateProperties.LIT, false);
            case "L" -> Blocks.LEVER.defaultBlockState().setValue(BlockStateProperties.POWERED, true);
            // 地形里的红石线按场景表写死「四边都是 side 连接」，与探针的 `w` 记法一致。
            // 不能靠默认状态：地形是直接写存储的，没有 updateShape 去推连接属性。
            case "w" -> redstoneSide();
            case "x" -> redstoneDot();
            default -> null;
        };
    }

    /** 四边都是 side 连接的红石线（场景表的 `w`）。 */
    @SuppressWarnings({"unchecked", "rawtypes"})
    private static BlockState redstoneSide() {
        BlockState state = Blocks.REDSTONE_WIRE.defaultBlockState();
        for (Direction direction : Direction.Plane.HORIZONTAL) {
            state = state.setValue(
                (net.minecraft.world.level.block.state.properties.Property) RedStoneWireBlock.PROPERTY_BY_DIRECTION
                    .get(direction),
                (Comparable) RedstoneSide.SIDE);
        }
        return state;
    }

    /** 点状红石线：四边都不连接。 */
    @SuppressWarnings({"unchecked", "rawtypes"})
    private static BlockState redstoneDot() {
        BlockState state = Blocks.REDSTONE_WIRE.defaultBlockState();
        for (Direction direction : Direction.Plane.HORIZONTAL) {
            state = state.setValue(
                (net.minecraft.world.level.block.state.properties.Property) RedStoneWireBlock.PROPERTY_BY_DIRECTION
                    .get(direction),
                (Comparable) RedstoneSide.NONE);
        }
        return state;
    }

    /**
     * 一个场景。字段与探针的 {@code Scenario} 一一对应：地形两侧各自写死，任何一侧改了
     * 地形都会在差分里立刻暴露成不一致。
     *
     * <p>{@code plane} / {@code above} / {@code below} 是 5×5 的扁平薄片（行优先，共 25 格），
     * 用与探针相同的单字符记法。
     */
    private record Scenario(
        String unit, String subject, String[] plane, String[] above, String[] below, int baseX) {
        private static List<Scenario> cached;

        static List<Scenario> all() {
            if (cached == null) {
                cached = build();
            }
            return cached;
        }

        static Scenario byName(String unit) {
            for (Scenario scenario : all()) {
                if (scenario.unit().equals(unit)) {
                    return scenario;
                }
            }
            return null;
        }

        private static List<Scenario> build() {
            List<Scenario> list = new ArrayList<>();
            int x = 16;
            list.add(new Scenario("lever_off", "minecraft:lever[face=floor,facing=north,powered=false]",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("lever_floor_north", "minecraft:lever[face=floor,facing=north,powered=true]",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("lever_floor_south", "minecraft:lever[face=floor,facing=south,powered=true]",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("lever_wall_north", "minecraft:lever[face=wall,facing=north,powered=true]",
                plane(2, "S.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("lever_wall_south", "minecraft:lever[face=wall,facing=south,powered=true]",
                plane(2, "S.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("lever_ceiling", "minecraft:lever[face=ceiling,facing=south,powered=true]",
                empty(), plane(2, "S.."), empty(), x));
            x += 16;
            list.add(new Scenario("lever_on_stone_side", "minecraft:lever[face=wall,facing=north,powered=true]",
                plane(2, "S.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("lever_island_in_stone", "minecraft:lever[face=wall,facing=north,powered=true]",
                plane(2, "SSS"), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("redstone_block_alone", "minecraft:redstone_block",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("redstone_block_beside_stone", "minecraft:redstone_block",
                plane(2, "S.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("redstone_block_under_stone", "minecraft:redstone_block",
                plane(2, "SSS"), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("torch_lit_alone", "minecraft:redstone_torch[lit=true]",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("torch_unlit_alone", "minecraft:redstone_torch[lit=false]",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("torch_lit_under_stone", "minecraft:redstone_torch[lit=true]",
                plane(2, "SSS"), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("torch_lit_above_stone", "minecraft:redstone_torch[lit=true]",
                empty(), plane(2, "SSS"), empty(), x));
            x += 16;
            list.add(new Scenario("wall_torch_north", "minecraft:redstone_wall_torch[lit=true,facing=north]",
                plane(2, "S.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("wall_torch_south", "minecraft:redstone_wall_torch[lit=true,facing=south]",
                plane(2, "S.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("wall_torch_unlit", "minecraft:redstone_wall_torch[lit=false,facing=north]",
                plane(2, "S.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("observer_powered_north", "minecraft:observer[facing=north,powered=true]",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("observer_unpowered", "minecraft:observer[facing=north,powered=false]",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("observer_facing_up", "minecraft:observer[facing=up,powered=true]",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("observer_beside_stone", "minecraft:observer[facing=north,powered=true]",
                plane(2, "S.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("conductor_alone", "minecraft:stone",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("conductor_torch_side", "minecraft:stone",
                plane(2, "T.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("conductor_two_torches", "minecraft:stone",
                plane(2, "T.T"), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("conductor_torch_below", "minecraft:stone",
                empty(), empty(), plane(2, ".T."), x));
            x += 16;
            list.add(new Scenario("conductor_torch_above", "minecraft:stone",
                empty(), plane(2, ".T."), empty(), x));
            x += 16;
            list.add(new Scenario("conductor_lever_side", "minecraft:stone",
                plane(2, "L.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("conductor_redstone_block_side", "minecraft:stone",
                plane(2, "R.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("glass_torch_below", "minecraft:glass",
                empty(), empty(), plane(2, ".T."), x));
            x += 16;
            list.add(new Scenario("glass_lever_side", "minecraft:glass",
                plane(2, "L.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("slab_torch_below", "minecraft:smooth_stone_slab[type=bottom]",
                empty(), empty(), plane(2, ".T."), x));
            x += 16;
            list.add(new Scenario("slab_lever_side", "minecraft:smooth_stone_slab[type=bottom]",
                plane(2, "L.."), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("wire_isolated", "minecraft:redstone_wire[north=side,south=side,east=side,west=side]",
                empty(), empty(), empty(), x));
            x += 16;
            list.add(new Scenario("wire_beside_wire", "minecraft:redstone_wire[north=side,south=side,east=side,west=side]",
                plane(2, "w.."), empty(), plane(2, "SSS"), x));
            x += 16;
            list.add(new Scenario("wire_beside_torch", "minecraft:redstone_wire[north=side,south=side,east=side,west=side]",
                plane(2, "T.."), empty(), plane(2, "SSS"), x));
            x += 16;
            list.add(new Scenario("wire_beside_redstone_block", "minecraft:redstone_wire[north=side,south=side,east=side,west=side]",
                plane(2, "R.."), empty(), plane(2, "SSS"), x));
            x += 16;
            list.add(new Scenario("wire_beside_lever", "minecraft:redstone_wire[north=side,south=side,east=side,west=side]",
                plane(2, "L.."), empty(), plane(2, "SSS"), x));
            x += 16;
            list.add(new Scenario("wire_over_glass", "minecraft:redstone_wire[north=side,south=side,east=side,west=side]",
                empty(), empty(), plane(2, "GGG"), x));
            x += 16;
            list.add(new Scenario("wire_above_wire_on_stone", "minecraft:redstone_wire[north=side,south=side,east=side,west=side]",
                empty(), plane(2, ".w."), plane(2, "SSS"), x));
            x += 16;
            list.add(new Scenario("wire_dot_beside_wire", "minecraft:redstone_wire[north=none,south=none,east=none,west=none]",
                plane(2, "w.."), empty(), plane(2, "SSS"), x));
            return list;
        }

        /** 25 格全空。 */
        private static String[] empty() {
            return box("...", "...", "...", "...", "...");
        }

        /** 只有第 {@code row} 行的中间三格有内容，其余为空。 */
        private static String[] plane(int row, String cells) {
            String[] rows = new String[5];
            for (int i = 0; i < 5; i++) {
                rows[i] = i == row ? cells : "...";
            }
            return box(rows);
        }

        private static String[] box(String... rows) {
            String[] cells = new String[25];
            for (int i = 0; i < 5; i++) {
                String row = rows[i];
                java.util.Arrays.fill(cells, i * 5, i * 5 + 5, ".");
                for (int col = 0; col < Math.min(3, row.length()); col++) {
                    cells[i * 5 + 1 + col] = String.valueOf(row.charAt(col));
                }
            }
            return cells;
        }
    }

    static {
        // 触发 bootstrap 后再碰 BuiltInRegistries。
        LootServerData.get();
    }
}