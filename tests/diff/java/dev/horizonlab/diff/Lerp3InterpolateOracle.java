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
import net.minecraft.util.Mth;

/**
 * 三线性插值的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.Lerp3InterpolateOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.Lerp3InterpolateOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>覆盖 {@code Mth.lerp3}。它是区块内噪声插值的核心：{@code NoiseInterpolator} 把密度函数
 * 在 4x8x4 的网格上采样，再用它把网格值插到任意方块坐标——密度函数之上最后一层生成逻辑。
 *
 * <p>插值顺序 X → Y → Z，三步 lerp 各自独立舍入。任何一步被编译器融合成 FMA 都会让结果
 * 偏若干 ulp，且偏差随插值层级放大。
 */
public final class Lerp3InterpolateOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private Lerp3InterpolateOracle() {}

    private static String doubleBits(double value) {
        return String.format("0x%016x", Double.doubleToRawLongBits(value));
    }

    /** 八个角值，顺序与探针侧一致。 */
    private static final double[][] LATTICE_CASES = {
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0},
        {0.123456789, -0.987654321, 0.5, -0.25, 0.75, -0.125, 0.0625, -0.03125},
        {1.0e12, -3.0e12, 7.0e11, -1.5e11, 2.5e13, -8.0e12, 4.0e10, -6.0e10},
        {1.0, 1.0e-16, 1.0e16, -1.0e16, 0.0, 1.0, -1.0, 1.0e-300},
        {1.0, 1.0000000000000002, 0.9999999999999999, 1.0, 1.0000000000000004, 0.9999999999999998, 1.0, 1.0},
        {-1.5, -2.5, -3.5, -4.5, -5.5, -6.5, -7.5, -8.5},
    };

    /** 插值因子三元组，与探针侧逐项一致。 */
    private static final double[][] FACTOR_CASES = {
        {0.0, 0.0, 0.0},
        {1.0, 1.0, 1.0},
        {0.5, 0.5, 0.5},
        {0.25, 0.75, 0.125},
        {1.0 / 3.0, 1.0 / 7.0, 1.0 / 11.0},
        {0.1, 0.2, 0.3},
        {0.7, 0.9, 0.11},
        {0.5, 0.0, 0.0},
        {0.0, 0.5, 0.0},
        {0.0, 0.0, 0.5},
        {0.9999999999, 0.0000000001, 0.5},
    };

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (int ci = 0; ci < LATTICE_CASES.length; ci++) {
                for (int fi = 0; fi < FACTOR_CASES.length; fi++) {
                    text.append(ci).append('|').append(fi).append('@').append(ci).append('\n');
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
                "usage: Lerp3InterpolateOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int pipe = requested.indexOf('|');
        int at = requested.lastIndexOf('@');
        if (pipe < 0 || at < 0) {
            return missing(requested);
        }
        int cornerIndex;
        int factorIndex;
        try {
            cornerIndex = Integer.parseInt(requested.substring(0, pipe));
            factorIndex = Integer.parseInt(requested.substring(pipe + 1, at));
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        if (cornerIndex < 0
                || cornerIndex >= LATTICE_CASES.length
                || factorIndex < 0
                || factorIndex >= FACTOR_CASES.length) {
            return missing(requested);
        }

        double[] c = LATTICE_CASES[cornerIndex];
        double[] f = FACTOR_CASES[factorIndex];
        double value = Mth.lerp3(f[0], f[1], f[2], c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);

        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("value", doubleBits(value));
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}