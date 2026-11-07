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
import net.minecraft.world.level.levelgen.DensityFunction;
import net.minecraft.world.level.levelgen.DensityFunctions;

/**
 * 末地岛屿密度的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EndIslandsOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.EndIslandsOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>覆盖原版 {@code DensityFunctions.EndIslandDensityFunction}——末地地形的主密度函数。
 * 把方块坐标除以 8 得到岛屿网格坐标，在该网格上求一个「岛屿高度」，再映射到
 * {@code [-0.84375, 0.5625]}。
 *
 * <p>内部有一个 25x25 的邻域搜索，分支条件是 {@code k1² + l1² > 4096}（距中心超过 64 格）
 * 且噪声小于 -0.9。因此采样点必须同时覆盖中心区与远区，否则外岛分支一次都不会进入。
 *
 * <p>该类是 protected 的嵌套类型，编译期不可引用，因此用反射构造。
 */
public final class EndIslandsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EndIslandsOracle() {}


    /** 与探针侧逐项一致。 */
    private static final long[] SEEDS = {
        0L, 1L, 1234567L, 987654321L, 0x0123456789ABCDEFL,
    };

    /** 采样坐标，与探针侧逐项一致。 */
    private static final int[][] SAMPLE_POINTS = {
        {0, 0, 0},
        {1, 0, 0},
        {0, 0, 1},
        {1, 0, 1},
        {7, 0, 7},
        {8, 0, 8},
        {16, 64, 16},
        {520, 0, 0},
        {0, 0, 520},
        {-520, 0, 0},
        {512, 0, 512},
        {-512, 0, -512},
        {4000, 0, -4000},
        {16384, 128, 16384},
        {100000, 0, -100000},
    };

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (long seed : SEEDS) {
                for (int i = 0; i < SAMPLE_POINTS.length; i++) {
                    text.append(seed).append('@').append(i).append('\n');
                }
            }
            Files.writeString(Path.of(args[1]), text.toString(), StandardCharsets.UTF_8);
            return;
        }

        if (args.length == 3 && args[0].equals("run")) {
            List<String> inputs = Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8);
            List<String> outputs = new ArrayList<>(inputs.size());
            for (String line : inputs) {
                if (line.isBlank()) {
                    continue;
                }
                outputs.add(runLine(line));
            }
            Files.write(Path.of(args[2]), outputs, StandardCharsets.UTF_8);
            return;
        }

        throw new IllegalArgumentException("usage: EndIslandsOracle list <units.txt> | run <in> <out>");
    }

    /** 构造原版的 EndIslandDensityFunction。 */
    private static DensityFunction buildFunction(long seed) {
        try {
            Class<?> cls =
                    Class.forName("net.minecraft.world.level.levelgen.DensityFunctions$EndIslandDensityFunction");
            var ctor = cls.getDeclaredConstructor(long.class);
            ctor.setAccessible(true);
            return (DensityFunction) ctor.newInstance(seed);
        } catch (ReflectiveOperationException error) {
            throw new IllegalStateException("failed to build EndIslandDensityFunction", error);
        }
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int at = requested.lastIndexOf('@');
        if (at < 0) {
            return missing(requested);
        }
        long seed;
        int index;
        try {
            seed = Long.parseUnsignedLong(requested.substring(0, at));
            index = Integer.parseInt(requested.substring(at + 1));
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        if (index < 0 || index >= SAMPLE_POINTS.length) {
            return missing(requested);
        }

        DensityFunction function = buildFunction(seed);
        int[] point = SAMPLE_POINTS[index];
        DensityFunction.FunctionContext context = new DensityFunction.FunctionContext() {
            @Override
            public int blockX() {
                return point[0];
            }

            @Override
            public int blockY() {
                return point[1];
            }

            @Override
            public int blockZ() {
                return point[2];
            }
        };
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        JsonFloats.add(output, "value", function.compute(context));
        JsonFloats.add(output, "min", function.minValue());
        JsonFloats.add(output, "max", function.maxValue());
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}