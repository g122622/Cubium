package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import it.unimi.dsi.fastutil.doubles.DoubleArrayList;
import it.unimi.dsi.fastutil.doubles.DoubleList;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.synth.PerlinNoise;

/**
 * Perlin 噪声取值的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.PerlinNoiseOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.PerlinNoiseOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版的 {@link PerlinNoise#create} 与 {@code getValue}，不重写噪声算法——
 * 噪声里每一步插值都会累积舍入，重写一份几乎必然与实现分叉，测出来的差异说明不了任何问题。
 * 探针侧同样直接构造 Cubium 的 {@code PerlinNoise}，两侧用例必须逐项一致。
 *
 * <p>噪声直接决定地形高度，是实现价值最高的差分目标之一：一旦有偏差，表现是「地形形状
 * 看起来对但具体高度不同」，人工几乎无法定位。
 */
public final class PerlinNoiseOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private PerlinNoiseOracle() {}

    private static String doubleBits(double value) {
        return String.format("0x%016x", Double.doubleToRawLongBits(value));
    }

    /** 一条噪声配置，与探针侧逐项一致。 */
    private record NoiseCase(String id, long seed, int firstOctave, double[] amplitudes) {}

    private static List<NoiseCase> noiseCases() {
        List<NoiseCase> list = new ArrayList<>();
        list.add(new NoiseCase("single", 0L, 0, new double[] {1.0}));
        list.add(new NoiseCase("single_neg", 12345L, -3, new double[] {1.0}));
        list.add(new NoiseCase("two_halves", 0L, 0, new double[] {1.0, 0.5}));
        list.add(new NoiseCase("two_halves_seed", 987654321L, -2, new double[] {1.5, 0.75}));
        list.add(new NoiseCase("octaves_4", 42L, -4, new double[] {1.0, 0.5, 0.25, 0.125}));
        list.add(new NoiseCase("octaves_6", 42L, -4, new double[] {1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125}));
        list.add(new NoiseCase("flat_weights", 777L, 0, new double[] {1.0, 0.9, 0.8, 0.7}));
        list.add(new NoiseCase("fo_m7", -42L, -7, new double[] {1.0, 1.0, 1.0}));
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

        throw new IllegalArgumentException("usage: PerlinNoiseOracle list <units.txt> | run <in> <out>");
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
            // 对应 Cubium 的 PositionalRandomFactory 构造：
            // create 内部走 rng.forkPositional() + fromHashOf("octave_N")。
            RandomSource rng = new LegacyRandomSource(noiseCase.seed());
            DoubleList amplitudes = new DoubleArrayList(noiseCase.amplitudes());
            PerlinNoise noise = PerlinNoise.create(rng, noiseCase.firstOctave(), amplitudes);
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