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
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.levelgen.DensityFunction;
import net.minecraft.world.level.levelgen.DensityFunctions;
import net.minecraft.world.level.levelgen.synth.NormalNoise;

/**
 * 顶点位移噪声的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ShiftNoiseOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.ShiftNoiseOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>覆盖原版的 {@code ShiftA} / {@code ShiftB} / {@code Shift} 三种形态。它们共享同一套算术——
 * 三个坐标各乘 0.25 后喂给噪声、结果再乘 4——区别只在坐标如何映射到噪声的三个轴：
 *
 * <pre>
 *   ShiftA.compute(ctx) = compute(ctx.blockX(), 0.0, ctx.blockZ())
 *   ShiftB.compute(ctx) = compute(ctx.blockZ(), ctx.blockX(), 0.0)
 *   Shift.compute(ctx)  = compute(ctx.blockX(), ctx.blockY(), ctx.blockZ())
 *   compute(x, y, z)    = offsetNoise.getValue(x * 0.25, y * 0.25, z * 0.25) * 4.0
 * </pre>
 *
 * <p>这三种是 {@code ShiftedNoise} 位移函数的实际提供方，两者组合起来才是完整的地形扭曲链路。
 *
 * <p>三个 record 与它们实现的 {@code ShiftNoise} 接口都是 protected 的嵌套类型，编译期不可引用，
 * 因此用反射构造。
 */
public final class ShiftNoiseOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ShiftNoiseOracle() {}

    private static String doubleBits(double value) {
        return String.format("0x%016x", Double.doubleToRawLongBits(value));
    }

    /** 与探针侧逐项一致。 */
    private record ShiftCase(
            String id, String kind, long seed, int firstOctave, double[] amplitudes) {}

    private static List<ShiftCase> shiftCases() {
        List<ShiftCase> list = new ArrayList<>();
        list.add(new ShiftCase("a_main", "A", 1234567L, -8, new double[] {1.0, 1.0, 1.0, 1.0}));
        list.add(new ShiftCase("b_main", "B", 1234567L, -8, new double[] {1.0, 1.0, 1.0, 1.0}));
        list.add(new ShiftCase("c_main", "C", 1234567L, -8, new double[] {1.0, 1.0, 1.0, 1.0}));
        list.add(new ShiftCase("a_single", "A", 42L, -4, new double[] {1.0}));
        list.add(new ShiftCase("b_single", "B", 42L, -4, new double[] {1.0}));
        list.add(new ShiftCase("c_single", "C", 42L, -4, new double[] {1.0}));
        list.add(new ShiftCase("a_mixed", "A", 987654321L, -6, new double[] {1.0, 0.5, 0.0, 0.25}));
        list.add(new ShiftCase("c_mixed", "C", 987654321L, -6, new double[] {1.0, 0.5, 0.0, 0.25}));
        list.add(new ShiftCase("a_oct", "A", -42L, -7, new double[] {1.0, 1.0, 1.0}));
        return list;
    }

    /** 采样坐标，与探针侧逐项一致。三轴互不相等才能区分 ShiftA 与 ShiftB 的轴置换。 */
    private static final int[][] SAMPLE_POINTS = {
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
        {1, 2, 3},
        {3, 2, 1},
        {-1, -1, -1},
        {16, 64, -16},
        {100, -32, 200},
        {-1000, 5, 1000},
        {12345, 200, -6789},
    };

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (ShiftCase shiftCase : shiftCases()) {
                for (int i = 0; i < SAMPLE_POINTS.length; i++) {
                    text.append(shiftCase.id()).append('@').append(i).append('\n');
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

        throw new IllegalArgumentException("usage: ShiftNoiseOracle list <units.txt> | run <in> <out>");
    }

    /**
     * 构造原版的 ShiftA / ShiftB / Shift。
     *
     * 噪声通过双参 NoiseHolder 显式传入已建好的实例——单参构造的 noise 字段为 null，
     * getValue 会直接返回 0、maxValue 恒为 2。
     */
    private static DensityFunction buildFunction(ShiftCase shiftCase) {
        it.unimi.dsi.fastutil.doubles.DoubleList amplitudes =
                new it.unimi.dsi.fastutil.doubles.DoubleArrayList(shiftCase.amplitudes());
        NormalNoise.NoiseParameters parameters =
                new NormalNoise.NoiseParameters(shiftCase.firstOctave(), amplitudes);
        RandomSource random = RandomSource.create(shiftCase.seed());
        NormalNoise noise = NormalNoise.create(random, parameters);
        DensityFunction.NoiseHolder holder =
                new DensityFunction.NoiseHolder(net.minecraft.core.Holder.direct(parameters), noise);

        String className =
                switch (shiftCase.kind()) {
                    case "A" -> "net.minecraft.world.level.levelgen.DensityFunctions$ShiftA";
                    case "B" -> "net.minecraft.world.level.levelgen.DensityFunctions$ShiftB";
                    case "C" -> "net.minecraft.world.level.levelgen.DensityFunctions$Shift";
                    default -> throw new IllegalArgumentException("unknown shift kind: " + shiftCase.kind());
                };
        try {
            Class<?> cls = Class.forName(className);
            var ctor = cls.getDeclaredConstructor(DensityFunction.NoiseHolder.class);
            ctor.setAccessible(true);
            return (DensityFunction) ctor.newInstance(holder);
        } catch (ReflectiveOperationException error) {
            throw new IllegalStateException("failed to build " + className, error);
        }
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int at = requested.lastIndexOf('@');
        if (at < 0) {
            return missing(requested);
        }
        String caseId = requested.substring(0, at);
        int index;
        try {
            index = Integer.parseInt(requested.substring(at + 1));
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        if (index < 0 || index >= SAMPLE_POINTS.length) {
            return missing(requested);
        }

        for (ShiftCase shiftCase : shiftCases()) {
            if (!caseId.equals(shiftCase.id())) {
                continue;
            }
            DensityFunction function = buildFunction(shiftCase);
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
            output.addProperty("value", doubleBits(function.compute(context)));
            output.addProperty("min", doubleBits(function.minValue()));
            output.addProperty("max", doubleBits(function.maxValue()));
            return GSON.toJson(output);
        }
        return missing(requested);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}