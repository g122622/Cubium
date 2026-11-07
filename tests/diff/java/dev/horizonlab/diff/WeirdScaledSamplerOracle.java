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
 * 奇异缩放采样器的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.WeirdScaledSamplerOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.WeirdScaledSamplerOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>覆盖原版 {@code DensityFunctions.WeirdScaledSampler} 的两种 rarity 映射。它不是纯算术——
 * 先拿输入值查表选出一个「稀有度」系数，再用 {@code 坐标 / 系数} 去采样噪声、结果取绝对值后
 * 乘回系数。主世界的 weirdness / ridges 用它把噪声在空间上分层。
 *
 * <p>查表用的是严格小于（{@code value < 阈值}），因此输入值在阈值上取等还是取不等，
 * 决定了落在哪个档。单元名前两段刻意写成阈值本身与它两侧的值。
 *
 * <p>{@code WeirdScaledSampler} 与其 {@code RarityValueMapper} 都是 protected 的嵌套类型，
 * 编译期不可引用，因此用反射构造。
 */
public final class WeirdScaledSamplerOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private WeirdScaledSamplerOracle() {}


    /** 与探针侧逐项一致。 */
    private record NoiseCase(long seed, int firstOctave, double[] amplitudes) {}

    private static List<NoiseCase> noiseCases() {
        List<NoiseCase> list = new ArrayList<>();
        list.add(new NoiseCase(1234567L, -8, new double[] {1.0, 1.0, 1.0, 1.0}));
        list.add(new NoiseCase(42L, -4, new double[] {1.0}));
        return list;
    }

    /** 与探针侧逐项一致。 */
    private static final double[] INPUT_VALUES = {
        -1.5, -0.7500001, -0.75, -0.7499999, -0.6,
        -0.5000001, -0.5, -0.4999999, -0.25,
        -0.0000001, 0.0, 0.0000001, 0.25,
        0.4999999, 0.5, 0.5000001, 0.6,
        0.7499999, 0.75, 0.7500001, 1.5,
    };

    /** 与探针侧逐项一致。 */
    private static final int[][] SAMPLE_POINTS = {
        {0, 0, 0},
        {1, 2, 3},
        {-3, 2, -1},
        {16, 64, -16},
        {-1000, 5, 1000},
    };

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (int typeIndex = 0; typeIndex < 2; typeIndex++) {
                for (double inputValue : INPUT_VALUES) {
                    for (int configIndex = 0; configIndex < noiseCases().size(); configIndex++) {
                        for (int i = 0; i < SAMPLE_POINTS.length; i++) {
                            text.append(typeIndex == 0 ? "type1" : "type2")
                                    .append('|')
                                    .append(String.format("%.7f", inputValue))
                                    .append('|')
                                    .append(configIndex)
                                    .append('@')
                                    .append(i)
                                    .append('\n');
                        }
                    }
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

        throw new IllegalArgumentException(
                "usage: WeirdScaledSamplerOracle list <units.txt> | run <in> <out>");
    }

    /**
     * 构造原版的 WeirdScaledSampler。
     *
     * 噪声通过双参 NoiseHolder 显式传入已建好的实例——单参构造的 noise 字段为 null，
     * getValue 会直接返回 0、maxValue 恒为 2。
     */
    private static DensityFunction buildFunction(String typeName, double inputValue, NoiseCase noiseCase) {
        it.unimi.dsi.fastutil.doubles.DoubleList amplitudes =
                new it.unimi.dsi.fastutil.doubles.DoubleArrayList(noiseCase.amplitudes());
        NormalNoise.NoiseParameters parameters =
                new NormalNoise.NoiseParameters(noiseCase.firstOctave(), amplitudes);
        RandomSource random = RandomSource.create(noiseCase.seed());
        NormalNoise noise = NormalNoise.create(random, parameters);
        DensityFunction.NoiseHolder holder =
                new DensityFunction.NoiseHolder(net.minecraft.core.Holder.direct(parameters), noise);

        String mapperName = typeName.equals("type1") ? "TYPE1" : "TYPE2";
        try {
            Class<?> mapperClass =
                    Class.forName("net.minecraft.world.level.levelgen.DensityFunctions$WeirdScaledSampler$RarityValueMapper");
            @SuppressWarnings({"unchecked", "rawtypes"})
            Object mapper = Enum.valueOf((Class<? extends Enum>) mapperClass.asSubclass(Enum.class), mapperName);

            Class<?> cls =
                    Class.forName("net.minecraft.world.level.levelgen.DensityFunctions$WeirdScaledSampler");
            var ctor = cls.getDeclaredConstructor(
                    DensityFunction.class, DensityFunction.NoiseHolder.class, mapperClass);
            ctor.setAccessible(true);
            return (DensityFunction) ctor.newInstance(
                    DensityFunctions.constant(inputValue), holder, mapper);
        } catch (ReflectiveOperationException error) {
            throw new IllegalStateException("failed to build WeirdScaledSampler", error);
        }
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int at = requested.lastIndexOf('@');
        int pipe1 = requested.indexOf('|');
        int pipe2 = pipe1 < 0 ? -1 : requested.indexOf('|', pipe1 + 1);
        if (at < 0 || pipe1 < 0 || pipe2 < 0) {
            return missing(requested);
        }

        String typeName = requested.substring(0, pipe1);
        double inputValue;
        int configIndex;
        int pointIndex;
        try {
            inputValue = Double.parseDouble(requested.substring(pipe1 + 1, pipe2));
            configIndex = Integer.parseInt(requested.substring(pipe2 + 1, at));
            pointIndex = Integer.parseInt(requested.substring(at + 1));
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        if (configIndex < 0
                || configIndex >= noiseCases().size()
                || pointIndex < 0
                || pointIndex >= SAMPLE_POINTS.length) {
            return missing(requested);
        }

        DensityFunction function = buildFunction(typeName, inputValue, noiseCases().get(configIndex));
        int[] point = SAMPLE_POINTS[pointIndex];
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