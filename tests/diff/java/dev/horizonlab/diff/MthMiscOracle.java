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
 * {@code Mth} 里一批纯计算函数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthMiscOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.MthMiscOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>覆盖 murmur3 混洗、快速立方根倒数、三态符号、向下量化、棋盘距离、整数倍判断、
 * 正方形边长、Catmull-Rom 样条、整数平方。这些函数都不依赖世界状态。
 */
public final class MthMiscOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private MthMiscOracle() {}

    /** 整数输入，与探针侧逐项一致。 */
    private static final int[] INT_INPUTS = {
        0, 1, -1, 2, -2, 7, -7, 13, 255, 256, -256, 65535, 65536, -65536,
        Integer.MAX_VALUE, Integer.MIN_VALUE, 0x55555555, 0x7FFFFFF, -123456789, 123456789,
    };

    /** f32 输入，与探针侧逐项一致。 */
    private static final float[] FLOAT_INPUTS = {
        0.0f, -0.0f, 1.0f, -1.0f, 2.0f, 0.5f, -0.5f, 3.0f, 8.0f, 27.0f,
        -27.0f, 0.001f, -0.001f, 1.0e-30f, 1.0e30f, 123.456f,
    };

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            Files.writeString(Path.of(args[1]), buildListing(), StandardCharsets.UTF_8);
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

        throw new IllegalArgumentException("usage: MthMiscOracle list <units.txt> | run <in> <out>");
    }

    /** 与探针侧 writeList 逐项一致。 */
    private static String buildListing() {
        StringBuilder text = new StringBuilder();
        for (int value : INT_INPUTS) {
            text.append("murmur|").append(value).append('\n');
            text.append("square|").append(value).append('\n');
        }
        for (float value : FLOAT_INPUTS) {
            text.append("invCube|").append(javaToHex(value)).append('\n');
            text.append("sign|").append(javaToHex((double) value)).append('\n');
        }
        int[] steps = {1, 2, 3, 7, 16, 64};
        double[] quantizeValues = {-100.5, -64.0, -1.0, -0.5, 0.0, 0.5, 1.0, 63.9, 64.0, 100.5, 1234.5678};
        for (int step : steps) {
            for (double value : quantizeValues) {
                text.append("quantize|").append(step).append('|').append(javaToHex(value)).append('\n');
            }
        }
        int[][] chess = {{0, 0, 0, 0}, {0, 0, 5, 3}, {0, 0, -5, 3}, {0, 0, 3, 5},
                         {10, -10, -10, 10}, {100, 200, 100, 200}, {0, 0, 1, 0}, {0, 0, 0, 1}};
        for (int[] c : chess) {
            text.append("chess|").append(c[0]).append('|').append(c[1]).append('|')
                .append(c[2]).append('|').append(c[3]).append('\n');
        }
        int[] divisors = {1, 2, 3, 4, 7, 16};
        int[] multiples = {0, 1, -1, 6, -6, 7, -7, 16, -16, 100, -100};
        for (int divisor : divisors) {
            for (int value : multiples) {
                text.append("multiple|").append(value).append('|').append(divisor).append('\n');
            }
        }
        int[] counts = {0, 1, 2, 3, 4, 5, 8, 9, 10, 15, 16, 17, 63, 64, 65, 1000, 1000000};
        for (int count : counts) {
            text.append("squareSide|").append(count).append('\n');
        }
        double[] deltas = {0.0, 0.25, 0.5, 0.75, 1.0, 0.1, 1.0 / 3.0, 0.9999999};
        for (double delta : deltas) {
            text.append("catmull|").append(javaToHex(delta)).append("|1.5|-2.25|3.125|-0.0625\n");
            text.append("catmull|").append(javaToHex(delta)).append("|-1.0|1.0|-1.0|1.0\n");
        }
        return text.toString();
    }

    /**
     * 把 double 格式化成两侧一致的十进制串。
     *
     * C 的 `%a` 与 Java 的 `Double.toHexString` 在指数符号、尾数省略上写法不同，直接对
     * 字符串会误判。这里统一用 `%.17g` 等价的十进制，保证单元名逐字相同。
     */
    private static String javaToHex(double value) {
        return String.format(java.util.Locale.ROOT, "%.17g", value);
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String text = input.get("unit").getAsString();

        String[] parts = text.split("\\|", -1);
        JsonObject result = new JsonObject();
        result.addProperty("unit", text);

        switch (parts[0]) {
            case "murmur" -> result.addProperty("value", Mth.murmurHash3Mixer(Integer.parseInt(parts[1])));
            case "square" -> {
                int x = Integer.parseInt(parts[1]);
                result.addProperty("value", x * x);
            }
            case "invCube" -> {
                JsonFloats.add(result, "value", (double) Mth.fastInvCubeRoot((float) Double.parseDouble(parts[1])));
            }
            case "sign" -> result.addProperty("value", Mth.sign(Double.parseDouble(parts[1])));
            case "quantize" -> result.addProperty(
                    "value", Mth.quantize(Double.parseDouble(parts[2]), Integer.parseInt(parts[1])));
            case "chess" -> result.addProperty(
                    "value",
                    Mth.chessboardDistance(
                            Integer.parseInt(parts[1]),
                            Integer.parseInt(parts[2]),
                            Integer.parseInt(parts[3]),
                            Integer.parseInt(parts[4])));
            case "multiple" -> result.addProperty(
                    "value", Mth.isMultipleOf(Integer.parseInt(parts[1]), Integer.parseInt(parts[2])));
            case "squareSide" -> result.addProperty("value", Mth.smallestSquareSide(Integer.parseInt(parts[1])));
            case "catmull" -> {
                JsonFloats.add(result, "value", (double) Mth.catmullrom(
                        (float) Double.parseDouble(parts[1]),
                        (float) Double.parseDouble(parts[2]),
                        (float) Double.parseDouble(parts[3]),
                        (float) Double.parseDouble(parts[4]),
                        (float) Double.parseDouble(parts[5])));
            }
            default -> result.addProperty("missing", true);
        }
        return GSON.toJson(result);
    }
}