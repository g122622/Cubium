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
import net.minecraft.util.KeyDispatchDataCodec;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.levelgen.DensityFunction;
import net.minecraft.world.level.levelgen.DensityFunctions;
import net.minecraft.world.level.levelgen.synth.NormalNoise;

/**
 * 带坐标平移的噪声密度函数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ShiftedNoiseOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.ShiftedNoiseOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>覆盖原版 {@code DensityFunctions.ShiftedNoise}：坐标先缩放、再叠加位移，然后把结果喂给噪声。
 * 这是主世界洞穴与下界地形里最常见的写法之一——位移让噪声在空间上扭曲，形成蜿蜒的洞穴。
 *
 * <p>位移函数用两种形态：常量（覆盖「缩放 + 常量」的加法顺序）与线性（覆盖非零位移路径）。
 * 两者都只用加乘，不引入噪声，便于把差异定位到 {@code ShiftedNoise} 自身的坐标运算。
 *
 * <p>{@code ShiftedNoise} 是 protected 的嵌套 record，编译期不可引用，因此用反射构造。
 */
public final class ShiftedNoiseOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ShiftedNoiseOracle() {}


    /** 平移函数的形态，与探针侧一致。 */
    private enum ShiftKind {
        CONSTANT,
        LINEAR
    }

    /** 与探针侧逐项一致。 */
    private record ShiftCase(
            String id,
            long seed,
            int firstOctave,
            double[] amplitudes,
            double xzScale,
            double yScale,
            ShiftKind shiftKind,
            double shiftBase,
            double shiftAx,
            double shiftAy,
            double shiftAz) {}

    private static List<ShiftCase> shiftCases() {
        List<ShiftCase> list = new ArrayList<>();
        list.add(new ShiftCase(
                "shift_zero", 1234567L, -8, new double[] {1.0, 1.0, 1.0, 1.0}, 0.25, 0.25, ShiftKind.CONSTANT, 0.0, 0.0, 0.0, 0.0));
        list.add(new ShiftCase(
                "shift_const", 1234567L, -8, new double[] {1.0, 1.0, 1.0, 1.0}, 0.25, 0.25, ShiftKind.CONSTANT, 0.5, 0.0, 0.0, 0.0));
        list.add(new ShiftCase(
                "shift_tiny", 987654321L, -6, new double[] {1.0, 0.5}, 0.1, 0.1, ShiftKind.CONSTANT, 1.0e-9, 0.0, 0.0, 0.0));
        list.add(new ShiftCase(
                "shift_linear", -42L, -7, new double[] {1.0, 1.0, 1.0}, 0.011, 0.011, ShiftKind.LINEAR, 0.125, 0.75, -0.5, 0.25));
        list.add(new ShiftCase(
                "shift_aniso", 555L, -8, new double[] {1.0, 1.0, 0.5, 0.25}, 0.5, 0.125, ShiftKind.LINEAR, -0.375, 1.5, 0.0, -1.0));
        return list;
    }

    /** 采样坐标，与探针侧逐项一致。 */
    private static final int[][] SAMPLE_POINTS = {
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
        {-1, -1, -1},
        {16, 64, -16},
        {100, -32, 200},
        {-1000, 5, 1000},
        {12345, 200, -6789},
    };

    /** 线性位移密度函数：value = base + ax*x + ay*y + az*z。 */
    private static final class LinearShift implements DensityFunction.SimpleFunction {
        private final double ax;
        private final double ay;
        private final double az;
        private final double base;

        LinearShift(double ax, double ay, double az, double base) {
            this.ax = ax;
            this.ay = ay;
            this.az = az;
            this.base = base;
        }

        @Override
        public double compute(DensityFunction.FunctionContext context) {
            double x = context.blockX() * this.ax;
            double y = context.blockY() * this.ay;
            double z = context.blockZ() * this.az;
            return this.base + x + y + z;
        }

        @Override
        public double minValue() {
            return -1.0e6;
        }

        @Override
        public double maxValue() {
            return 1.0e6;
        }

        @Override
        public KeyDispatchDataCodec<? extends DensityFunction> codec() {
            throw new UnsupportedOperationException("oracle-only density function");
        }
    }

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

        throw new IllegalArgumentException("usage: ShiftedNoiseOracle list <units.txt> | run <in> <out>");
    }

    /** 构造位移函数。只有对应轴的位移生效，其余两轴为 0，便于区分每条轴。 */
    private static DensityFunction buildShift(ShiftCase shiftCase, int axis) {
        if (shiftCase.shiftKind() == ShiftKind.CONSTANT) {
            double base = (axis == 0) ? shiftCase.shiftBase() : 0.0;
            return DensityFunctions.constant(base);
        }
        double ax = (axis == 0) ? shiftCase.shiftAx() : 0.0;
        double ay = (axis == 1) ? shiftCase.shiftAy() : 0.0;
        double az = (axis == 2) ? shiftCase.shiftAz() : 0.0;
        return new LinearShift(ax, ay, az, shiftCase.shiftBase());
    }

    /**
     * 构造原版的 ShiftedNoise。
     *
     * 该 record 是 protected 的嵌套类型，公开 API 里没有对应的工厂方法，因此用反射构造。
     * 噪声通过双参 NoiseHolder 显式传入已建好的实例——单参构造的 noise 字段为 null，
     * getValue 会直接返回 0。
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

        try {
            Class<?> cls = Class.forName("net.minecraft.world.level.levelgen.DensityFunctions$ShiftedNoise");
            var ctor = cls.getDeclaredConstructor(
                    DensityFunction.class,
                    DensityFunction.class,
                    DensityFunction.class,
                    double.class,
                    double.class,
                    DensityFunction.NoiseHolder.class);
            ctor.setAccessible(true);
            return (DensityFunction) ctor.newInstance(
                    buildShift(shiftCase, 0),
                    buildShift(shiftCase, 1),
                    buildShift(shiftCase, 2),
                    shiftCase.xzScale(),
                    shiftCase.yScale(),
                    holder);
        } catch (ReflectiveOperationException error) {
            throw new IllegalStateException("failed to build ShiftedNoise", error);
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
            JsonFloats.add(output, "value", function.compute(context));
            JsonFloats.add(output, "min", function.minValue());
            JsonFloats.add(output, "max", function.maxValue());
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