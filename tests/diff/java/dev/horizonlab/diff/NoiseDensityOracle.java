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
 * 噪声型密度函数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.NoiseDensityOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.NoiseDensityOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>覆盖 {@code NoiseDensity}（{@code noise * scale}）与 {@code MappedNoise}（把噪声重映射到
 * {@code [from, to]}）。两者都直接决定主世界与下界的洞穴形状。
 *
 * <p>缩放特意用非 2 的幂（0.1、0.011、0.31），因为 {@code x * xzScale} 这一步的舍入会直接
 * 传到噪声输入。
 */
public final class NoiseDensityOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private NoiseDensityOracle() {}


    /** 与探针侧逐项一致。 */
    private record NoiseCase(
            String id,
            long seed,
            int firstOctave,
            double[] amplitudes,
            double xzScale,
            double yScale,
            double fromValue,
            double toValue) {}

    private static List<NoiseCase> noiseCases() {
        List<NoiseCase> list = new ArrayList<>();
        list.add(new NoiseCase("single_unit", 0L, -7, new double[] {1.0}, 1.0, 1.0, 0.0, 0.0));
        list.add(new NoiseCase("octaves_4", 1234567L, -8, new double[] {1.0, 1.0, 1.0, 1.0}, 0.25, 0.25, 0.0, 0.0));
        list.add(new NoiseCase("scale_0_1", 987654321L, -6, new double[] {1.0, 0.5}, 0.1, 0.1, 0.0, 0.0));
        list.add(new NoiseCase("scale_small", -42L, -7, new double[] {1.0, 1.0, 1.0}, 0.011, 0.011, 0.0, 0.0));
        list.add(new NoiseCase(
                "anisotropic", 555L, -8, new double[] {1.0, 1.0, 0.5, 0.25}, 0.5, 0.125, 0.0, 0.0));
        list.add(new NoiseCase("mapped_sym", 24680L, -7, new double[] {1.0, 1.0}, 0.5, 0.5, -0.5, 0.5));
        list.add(new NoiseCase("mapped_pos", 13579L, -6, new double[] {1.0, 0.5, 0.25}, 0.25, 0.25, 0.0, 1.0));
        list.add(new NoiseCase("mapped_asym", -100L, -7, new double[] {1.0, 1.0, 1.0}, 0.31, 0.17, -2.5, 0.75));
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

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (NoiseCase noiseCase : noiseCases()) {
                for (int i = 0; i < SAMPLE_POINTS.length; i++) {
                    text.append(noiseCase.id()).append('@').append(i).append('\n');
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

        throw new IllegalArgumentException("usage: NoiseDensityOracle list <units.txt> | run <in> <out>");
    }

    /** 用原版 NormalNoise.create 构造噪声，再包成密度函数。 */
    private static DensityFunction buildFunction(NoiseCase noiseCase) {
        // 原版 NormalNoise.NoiseParameters 用 DoubleList 存振幅。
        // 原版 NoiseHolder 的单参构造只存参数、noise 字段为 null，getValue 会直接返回 0 且
        // maxValue 恒为 2。要真正取值必须用双参构造显式传入已建好的 NormalNoise。
        it.unimi.dsi.fastutil.doubles.DoubleList amplitudes =
                new it.unimi.dsi.fastutil.doubles.DoubleArrayList(noiseCase.amplitudes());
        NormalNoise.NoiseParameters parameters =
                new NormalNoise.NoiseParameters(noiseCase.firstOctave(), amplitudes);
        RandomSource random = RandomSource.create(noiseCase.seed());
        NormalNoise noise = NormalNoise.create(random, parameters);
        // 原版 mappedNoise/noise 工厂内部用的是单参 NoiseHolder（noise 为 null），
        // 在测试里会直接得到 0。必须直接构造 Noise record 并传入已建好的 NormalNoise。
        DensityFunction.NoiseHolder holder = new DensityFunction.NoiseHolder(
                net.minecraft.core.Holder.direct(parameters), noise);

        DensityFunction noiseFunction;
        try {
            Class<?> noiseClass = Class.forName("net.minecraft.world.level.levelgen.DensityFunctions$Noise");
            var ctor = noiseClass.getDeclaredConstructor(
                    DensityFunction.NoiseHolder.class, double.class, double.class);
            ctor.setAccessible(true);
            noiseFunction = (DensityFunction) ctor.newInstance(
                    holder, noiseCase.xzScale(), noiseCase.yScale());
        } catch (ReflectiveOperationException error) {
            throw new IllegalStateException("failed to build Noise density function", error);
        }

        if (!noiseCase.id().startsWith("mapped_")) {
            return noiseFunction;
        }
        // MappedNoise = 把 Noise 的输出从 [-1,1] 线性映射到 [from, to]。
        double mid = (noiseCase.fromValue() + noiseCase.toValue()) * 0.5;
        double half = (noiseCase.toValue() - noiseCase.fromValue()) * 0.5;
        return DensityFunctions.add(
                DensityFunctions.mul(DensityFunctions.constant(half), noiseFunction),
                DensityFunctions.constant(mid));
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

        for (NoiseCase noiseCase : noiseCases()) {
            if (!caseId.equals(noiseCase.id())) {
                continue;
            }
            DensityFunction function = buildFunction(noiseCase);
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