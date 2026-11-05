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
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.synth.NormalNoise;

/**
 * 归一化噪声取值的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.NormalNoiseOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.NormalNoiseOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>{@code NormalNoise} 由两个 {@code PerlinNoise} 叠加：第二个的坐标乘以
 * {@code 1.0181268882175227} 错开避免相关性，两者取和后乘以
 * {@code 0.16666666666666666 / expectedDeviation(octaveRange)} 归一化。
 * 它是原版地形高度与气候参数采样的基础，比单独的 {@code PerlinNoise} 多三层易错点：
 *
 * <ol>
 *   <li>{@code octaveRange} 只统计<b>非零振幅</b>的最小/最大下标之差，不是振幅列表长度；
 *   <li>{@code expectedDeviation(n) = 0.1 * (1.0 + 1.0 / (n + 1))} 里的 {@code 1.0 / (n + 1)}；
 *   <li>归一化常数是十进制字面量 {@code 0.16666666666666666}，与 {@code 1.0 / 6.0} 在编译期
 *       常量折叠后同值，但若在运行期相除就会差 1 ulp。
 * </ol>
 *
 * <p>直接调用原版的 {@link NormalNoise#create}，不重写噪声算法。探针侧构造 Cubium 的
 * {@code NormalNoise} 并走同一条随机源，两侧用例表逐项一致。
 */
public final class NormalNoiseOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private NormalNoiseOracle() {}

    private static String doubleBits(double value) {
        return String.format("0x%016x", Double.doubleToRawLongBits(value));
    }

    /** 一条噪声参数，与探针侧逐项一致。 */
    private record NoiseCase(String id, long seed, int firstOctave, double[] amplitudes) {}

    private static List<NoiseCase> noiseCases() {
        List<NoiseCase> list = new ArrayList<>();
        list.add(new NoiseCase("single", 0L, 0, new double[] {1.0}));
        list.add(new NoiseCase("octaves_4", 42L, -4, new double[] {1.0, 0.5, 0.25, 0.125}));
        list.add(new NoiseCase("octaves_6", 42L, -4, new double[] {1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125}));
        list.add(new NoiseCase("gaps", 777L, -2, new double[] {1.0, 0.0, 0.5, 0.0, 0.25}));
        list.add(new NoiseCase("trimmed", 777L, -2, new double[] {0.0, 1.0, 0.5, 0.25, 0.0}));
        list.add(new NoiseCase("lonely", 5L, 0, new double[] {0.0, 0.0, 1.5, 0.0}));
        list.add(new NoiseCase("flat_weights", 12345L, -1, new double[] {1.0, 0.9, 0.8, 0.7}));
        return list;
    }

    /** 采样坐标，与探针侧逐项一致。 */
    private static final double[][] SAMPLE_POINTS = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.5, 0.5, 0.5},
        {0.25, 0.75, 0.125},
        {-1.0, -1.0, -1.0},
        {-0.5, 0.5, -0.5},
        {12.3456789, -7.654321, 3.14159265},
        {-100.0, 50.0, 200.0},
        {1000.0, -1000.0, 500.0},
        {1.0e-3, 2.0e-3, 3.0e-3},
        {123.456, 789.012, 345.678},
        {-0.123456789, 0.987654321, -0.5},
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

        throw new IllegalArgumentException("usage: NormalNoiseOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int at = requested.lastIndexOf('@');
        if (at < 0) {
            return missing(requested);
        }
        String caseId = requested.substring(0, at);
        int pointIndex;
        try {
            pointIndex = Integer.parseInt(requested.substring(at + 1));
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        if (pointIndex < 0 || pointIndex >= SAMPLE_POINTS.length) {
            return missing(requested);
        }

        for (NoiseCase noiseCase : noiseCases()) {
            if (!caseId.equals(noiseCase.id())) {
                continue;
            }
            RandomSource rng = new LegacyRandomSource(noiseCase.seed());
            NormalNoise noise = NormalNoise.create(rng, noiseCase.firstOctave(), noiseCase.amplitudes());
            double[] point = SAMPLE_POINTS[pointIndex];
            JsonObject output = new JsonObject();
            output.addProperty("unit", requested);
            output.addProperty("value", doubleBits(noise.getValue(point[0], point[1], point[2])));
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