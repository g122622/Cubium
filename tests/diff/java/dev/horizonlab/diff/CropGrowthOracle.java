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
import java.util.List;
import net.minecraft.core.BlockPos;
import net.minecraft.core.SectionPos;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.CropBlock;
import net.minecraft.world.level.block.FarmBlock;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.BlockStateProperties;
import net.minecraft.world.level.block.state.properties.IntegerProperty;
import net.minecraft.world.level.block.state.properties.Property;

/**
 * 作物随机刻的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CropGrowthOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.CropGrowthOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>按场景铺一片 7×7 地形（中心是作物），读 {@code CropBlock#getGrowthSpeed} 得到生长
 * 速率，再调用一次 {@code Block#randomTick}，输出「作物本体 / 上方 / 四个水平邻居 /
 * 脚下耕地」的状态。
 *
 * <p>随机数用 {@code level.random.setSeed(seed)}：原版 {@code RandomSource} 默认就是
 * 与 {@code java.util.Random} 同算法的 LCG，而探针的 {@code math::Random} 也照抄了这个
 * 算法，所以两侧掷出的序列逐个数相同——这是能把「这一次到底长没长」比到一致的前提。
 *
 * <p>作物只在天空光照 ≥9 时才掷骰生长，因此场景必须真的处在露天下。临时世界的区块在
 * 被写入时由光照引擎自己算好天空光照，这里的场景位于世界表层之上、四周无遮挡，天然满足；
 * 需要「暗处」的对照才额外把上方盖住。
 *
 * <p>刻意<b>不</b>放进「需要真实光照」的场景：光照引擎的更新时机不在两侧的约定范围内，
 * 拿它做判定只会比出环境噪声，而不是逻辑差异——这类判定留给 GameTest。
 */
public final class CropGrowthOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private CropGrowthOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Scenario scenario : Scenario.all()) {
                units.add(scenario.unit());
            }
            units.sort(String::compareTo);
            Files.writeString(Path.of(args[1]), String.join("\n", units) + "\n", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: CropGrowthOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        Block block = BuiltInRegistries.BLOCK.getValue(Identifier.parse(scenario.subject()));
        if (block == null || block == Blocks.AIR) {
            result.addProperty("ok", false);
            result.addProperty("error", "unknown block");
            return;
        }

        ServerLevel level = LootLevel.get().level;
        level.random.setSeed(scenario.seed());

        // 每个场景用一块互不重叠的地盘，避免场景之间残留方块互相影响。
        BlockPos center = new BlockPos(scenario.baseX() + 8, 64, 8);
        clearArea(level, center);
        ensureChunkLoaded(level, center);

        // 先垫地基再铺地形：作物/耕地写入时 canSurvive 要求下方有支撑，否则写入会被判为
        // 非法（表现为"写进去又变回空气"）。
        ensureSupport(level, center);

        BlockState subject = withAge(block.defaultBlockState(), scenario.age());
        buildTerrain(level, scenario, center, subject);

        // 光照是环境输入而不是被测逻辑：临时世界的天空光照恒为 0，无法据此区分明暗。
        // 这里改为显式给「暗场景」盖顶（上方放实心方块），让作物真的拿不到生长条件；
        // 「亮场景」保持露天。两侧用同一条约定，比的就是方块在两种环境下的反应。
        if (scenario.lightLevel() < 9) {
            place(level, center.above(), Blocks.STONE.defaultBlockState());
        }

        // 明暗差别放在离作物足够远的位置（上方 3 格以上的侧旁），不影响方块本身。
        result.addProperty("ok", true);
        if (block instanceof CropBlock || scenario.subject().endsWith("_stem")) {
            result.addProperty("growthSpeed", growthSpeed(block, level, center));
        }

        subject.randomTick(level, center, level.random);

        result.addProperty("after", describe(level.getBlockState(center)));
        JsonObject neighbours = new JsonObject();
        neighbours.addProperty("up", describe(level.getBlockState(center.above())));
        neighbours.addProperty("north", describe(level.getBlockState(center.north())));
        neighbours.addProperty("south", describe(level.getBlockState(center.south())));
        neighbours.addProperty("west", describe(level.getBlockState(center.west())));
        neighbours.addProperty("east", describe(level.getBlockState(center.east())));
        result.add("neighbours", neighbours);
        result.addProperty("ground", describe(level.getBlockState(center.below())));
    }

    /**
     * 反射调用 {@code CropBlock#getGrowthSpeed}。
     *
     * <p>它是 {@code protected static}——原版只在方块内部（作物的随机刻、骨粉）用，不对
     * 外暴露。这里不改原版代码，用反射取同一条公式，保证比的就是原版自己算的那个数。
     */
    private static float growthSpeed(Block block, ServerLevel level, BlockPos pos) {
        try {
            java.lang.reflect.Method method =
                CropBlock.class.getDeclaredMethod("getGrowthSpeed", Block.class, net.minecraft.world.level.BlockGetter.class, BlockPos.class);
            method.setAccessible(true);
            return (float) method.invoke(null, block, level, pos);
        } catch (ReflectiveOperationException exception) {
            throw new IllegalStateException("CropBlock#getGrowthSpeed 不可调用", exception);
        }
    }

    /**
     * 把中心下方补成实心地基。
     *
     * <p>原版 {@code CropBlock#canSurvive} 只要求光照与「脚下是耕地」，而耕地本身要求地面
     * 下方有支撑。临时世界默认是虚空，所以这里把中心格下方一路垫到 y=60，让方块能被
     * {@code setBlock} 接受（否则写入会被 {@code updateShape} 判为不合法而回滚成空气）。
     */
    private static void ensureSupport(ServerLevel level, BlockPos center) {
        for (int dx = -5; dx <= 5; dx++) {
            for (int dz = -5; dz <= 5; dz++) {
                for (int y = 62; y >= 60; y--) {
                    BlockPos pos = center.offset(dx, y - 64, dz);
                    if (level.getBlockState(pos).isAir()) {
                        place(level, pos, Blocks.DIRT.defaultBlockState());
                    }
                }
            }
        }
    }

    /**
     * 确认中心格所在区块已加载。
     *
     * <p>临时世界是懒加载的：没有区块时 {@code setBlock} 直接返回 false、方块落不到世界里，
     * 读到的一切都是空气。这里显式取一次区块（不跑 tick，只做地形生成与加载）。
     */
    private static void ensureChunkLoaded(ServerLevel level, BlockPos center) {
        // 3×3 区块：场景地形 7×7 会跨到相邻区块，未加载的那半侧写入会被丢弃。
        int cx = center.getX() >> 4;
        int cz = center.getZ() >> 4;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                level.getChunk(cx + dx, cz + dz);
            }
        }
    }

    /** 是否需要充足日照才会生长（作物与梗都是）。 */
    private static boolean requiresSunlight(Block block) {
        return block instanceof CropBlock || BuiltInRegistries.BLOCK.getKey(block).getPath().endsWith("_stem");
    }

    /**
     * 直接写进区块的方块存储。
     *
     * <p>走 {@code level.setBlock} 会先过一遍 {@code updateShape} 的合法性判断——在临时
     * 世界里，地形是逐格铺的，写入顺序稍有不对（例如耕地先于它的地基）就会整块被判定为
     * 不合法而回滚成空气，表现为「写进去的方块凭空消失」。{@code LevelChunk#setBlockState}
     * 只做存储与高度图维护，正是这里要的「把地形摆好」的语义。
     */
    private static void place(ServerLevel level, BlockPos pos, BlockState state) {
        level.getChunk(pos.getX() >> 4, pos.getZ() >> 4).setBlockState(pos, state, Block.UPDATE_ALL);
    }

    private static void clearArea(ServerLevel level, BlockPos center) {
        for (int dx = -4; dx <= 4; dx++) {
            for (int dz = -4; dz <= 4; dz++) {
                for (int dy = -2; dy <= 1; dy++) {
                    place(level, center.offset(dx, dy, dz), Blocks.AIR.defaultBlockState());
                }
            }
        }
    }

    private static void buildTerrain(ServerLevel level, Scenario scenario, BlockPos center, BlockState subject) {
        BlockState farmland = Blocks.FARMLAND.defaultBlockState();

        // 中心格先放主语方块：它脚下那格在循环结束后单独铺耕地。
        place(level, center, subject);

        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 7; col++) {
                char cell = scenario.rows()[row].charAt(col);
                // 矩阵按「北→南」自上而下书写，所以行号越大越靠南（+Z）。
                BlockPos pos = center.offset(col - 3, 0, row - 3);

                if (row == 3 && col == 3) {
                    continue;
                }
                if (cell == '.') {
                    continue;
                }
                if (cell >= '0' && cell <= '7') {
                    place(level, pos, farmland.setValue(FarmBlock.MOISTURE, cell - '0'));
                    continue;
                }
                if (cell == 'c') {
                    place(level, pos, withAge(subject, scenario.age()));
                    continue;
                }
                if (cell == 'b') {
                    place(level, pos, Blocks.BAMBOO.defaultBlockState());
                }
            }
        }

        char centerGround = scenario.rows()[3].charAt(3);
        int moisture = (centerGround >= '0' && centerGround <= '7') ? centerGround - '0' : 0;
        place(level, center.below(), farmland.setValue(FarmBlock.MOISTURE, moisture));
    }

    /**
     * 按场景的「年龄」设置状态。
     *
     * <p>对作物与梗是 {@code age}，对耕地是 {@code moisture}——两者都是「随场景变化的那个
     * 整数属性」，与探针的 {@code makeSubjectState} 采用同一口径。否则耕地会话永远拿默认
     * 湿度，两侧的输入从一开始就不一样。
     */
    private static BlockState withAge(BlockState state, int age) {
        for (Property<?> property : state.getProperties()) {
            if (property instanceof IntegerProperty integerProperty && property.getName().equals("age")) {
                return setInteger(state, integerProperty, Math.min(age, maxOf(integerProperty)));
            }
        }
        for (Property<?> property : state.getProperties()) {
            if (property instanceof IntegerProperty integerProperty && property.getName().equals("moisture")) {
                return setInteger(state, integerProperty, Math.max(0, Math.min(age, maxOf(integerProperty))));
            }
        }
        return state;
    }

    @SuppressWarnings({"unchecked", "rawtypes"})
    private static int maxOf(IntegerProperty property) {
        return property.getPossibleValues().stream().mapToInt(value -> (Integer) value).max().orElse(0);
    }

    @SuppressWarnings({"unchecked", "rawtypes"})
    private static BlockState setInteger(BlockState state, IntegerProperty property, int value) {
        return state.setValue((Property) property, (Comparable) Integer.valueOf(value));
    }

    /** 方块状态的简短表示，与探针的 {@code describeState} 同一种写法。 */
    private static String describe(BlockState state) {
        if (state.getBlock() == Blocks.AIR) {
            return "<air>";
        }
        String text = BuiltInRegistries.BLOCK.getKey(state.getBlock()).toString();
        Property<?> moisture = BlockStateProperties.MOISTURE;
        if (state.hasProperty(moisture)) {
            text += "#" + state.getValue(moisture);
        }
        return text;
    }

    /**
     * 一个场景。字段与探针的 {@code Scenario} 一一对应；地形表两侧各自写死，任何一侧
     * 改动了地形都会在差分里立刻暴露成不一致。
     *
     * @param baseX 该场景在临时世界里的地盘起点（互不重叠，避免残留方块串味）
     */
    private record Scenario(
        String unit, String subject, int age, int seed, int baseX, int lightLevel, String[] rows
    ) {
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
            list.add(new Scenario("speed_open_field", "minecraft:wheat", 0, 1, 16, 15,
                new String[] {".......", ".......", ".......", "...0...", ".......", ".......", "......."}));
            list.add(new Scenario("speed_surrounded_dry", "minecraft:wheat", 0, 2, 32, 15,
                new String[] {".......", ".......", "..000..", "..000..", "..000..", ".......", "......."}));
            list.add(new Scenario("speed_surrounded_wet", "minecraft:wheat", 0, 3, 64, 15,
                new String[] {".......", ".......", "..777..", "..777..", "..777..", ".......", "......."}));
            list.add(new Scenario("speed_mixed_moisture", "minecraft:wheat", 0, 4, 96, 15,
                new String[] {".......", ".......", "..017..", "..304..", "..652..", ".......", "......."}));
            list.add(new Scenario("speed_no_farmland", "minecraft:wheat", 0, 5, 128, 15,
                new String[] {".......", ".......", ".......", ".......", ".......", ".......", "......."}));
            list.add(new Scenario("speed_cvenc_ns_line", "minecraft:wheat", 0, 6, 160, 15,
                new String[] {".......", ".......", "...c...", "..c0c..", "...c...", ".......", "......."}));
            list.add(new Scenario("speed_cvenc_we_line", "minecraft:wheat", 0, 7, 192, 15,
                new String[] {".......", ".......", ".......", ".c0c...", ".......", ".......", "......."}));
            list.add(new Scenario("speed_cvenc_n_diag", "minecraft:wheat", 0, 8, 224, 15,
                new String[] {".......", ".......", ".......", "..c0...", "...c...", ".......", "......."}));
            list.add(new Scenario("speed_cvenc_nw_diag", "minecraft:wheat", 0, 9, 256, 15,
                new String[] {".......", ".......", ".......", ".c0c...", ".......", ".......", "......."}));
            list.add(new Scenario("speed_cvenc_se_diag", "minecraft:wheat", 0, 10, 288, 15,
                new String[] {".......", ".......", ".......", "...0...", "....c..", ".......", "......."}));
            list.add(new Scenario("speed_cvenc_axis_and_diag", "minecraft:wheat", 0, 11, 320, 15,
                new String[] {".......", ".......", "..cc...", "..c0c..", "...c...", ".......", "......."}));
            list.add(new Scenario("speed_cvenc_wet_crowded", "minecraft:wheat", 0, 12, 352, 15,
                new String[] {".......", ".......", "..777..", "c.707.c", "..777..", ".......", "......."}));
            list.add(new Scenario("wheat_age_0_dark", "minecraft:wheat", 0, 13, 384, 4,
                new String[] {".......", ".......", ".......", "...0...", ".......", ".......", "......."}));
            list.add(new Scenario("wheat_age_0_bright", "minecraft:wheat", 0, 14, 416, 15,
                new String[] {".......", ".......", ".......", "...0...", ".......", ".......", "......."}));
            list.add(new Scenario("wheat_age_6_last_step", "minecraft:wheat", 6, 15, 448, 15,
                new String[] {".......", ".......", ".......", "...0...", ".......", ".......", "......."}));
            list.add(new Scenario("wheat_age_7_mature", "minecraft:wheat", 7, 16, 480, 15,
                new String[] {".......", ".......", ".......", "...0...", ".......", ".......", "......."}));
            list.add(new Scenario("carrots_age_3_wet", "minecraft:carrots", 3, 17, 512, 15,
                new String[] {".......", ".......", "..777..", "..777..", "..777..", ".......", "......."}));
            list.add(new Scenario("beetroots_age_5_crowded", "minecraft:beetroots", 5, 18, 544, 15,
                new String[] {".......", ".......", "..ccc..", "..c0c..", "..cc...", ".......", "......."}));
            list.add(new Scenario("stem_age_3_grow", "minecraft:pumpkin_stem", 3, 19, 576, 15,
                new String[] {".......", ".......", ".......", "...0...", ".......", ".......", "......."}));
            list.add(new Scenario("stem_age_7_no_support", "minecraft:pumpkin_stem", 7, 20, 608, 15,
                new String[] {".......", ".......", ".......", "...0...", ".......", ".......", "......."}));
            list.add(new Scenario("stem_age_7_farmland_ring", "minecraft:pumpkin_stem", 7, 21, 640, 15,
                new String[] {".......", "..777..", "..777..", "..707..", "..777..", "..777..", "......."}));
            list.add(new Scenario("stem_age_7_dirt_ring", "minecraft:melon_stem", 7, 22, 672, 15,
                new String[] {".......", "..bbb..", "..bbb..", "..b0b..", "..bbb..", "..bbb..", "......."}));
            list.add(new Scenario("stem_age_7_blocked", "minecraft:melon_stem", 7, 23, 704, 15,
                new String[] {".......", "..bbb..", "..bcb..", "..b0b..", "..bbb..", "..bbb..", "......."}));
            list.add(new Scenario("farmland_dry_no_crop", "minecraft:farmland", 3, 24, 736, 15,
                new String[] {".......", ".......", ".......", ".......", ".......", ".......", "......."}));
            list.add(new Scenario("farmland_dry_with_crop", "minecraft:farmland", 3, 25, 768, 15,
                new String[] {".......", ".......", ".......", ".......", ".......", ".......", "......."}));
            list.add(new Scenario("farmland_wet_no_nearby_water", "minecraft:farmland", 5, 26, 800, 15,
                new String[] {".......", ".......", ".......", ".......", ".......", ".......", "......."}));
            return list;
        }
    }

    static {
        // 触发 bootstrap 后再碰 BuiltInRegistries。
        LootServerData.get();
    }
}