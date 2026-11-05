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
import java.util.List;
import net.minecraft.util.Mth;

/**
 * {@code Mth} 插值函数族的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthLerpOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Mth#lerp(double, double, double)} /
 * {@link Mth#lerp(float, float, float)} / {@link Mth#clampedLerp(double, double, double)} /
 * {@link Mth#lerpInt(float, int, int)} / {@link Mth#lerpDiscrete(float, int, int)} /
 * {@link Mth#lerp2}，不重写公式。
 *
 * <p>单元表必须与 probes/mth_lerp.cpp 逐项一致。
 */
public final class MthLerpOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "lerpd|0|10|20",
                    "lerpd|1|10|20",
                    "lerpd|0.5|10|20",
                    "lerpd|0.25|100|200",
                    "lerpd|-0.5|10|20",
                    "lerpd|1.5|10|20",
                    "lerpd|0.5|20|10",
                    "lerpf|0|10|20",
                    "lerpf|1|10|20",
                    "lerpf|0.5|10|20",
                    "lerpf|0.25|100|200",
                    "lerpf|-0.5|10|20",
                    "lerpf|1.5|10|20",
            // 能区分「FMA 融合」与「逐步舍入」的样本，与探针侧逐项一致。
            // 上面的对称整数用例（10/20、0/10）在两种算法下结果相同，无法暴露
            // `a + (b - a) * t` 被编译成单条 fma 后少一次舍入的问题。
            "lerpf|0.843400002|2.2530005|-6.13999987",
            "lerpf|0.0476999991|-0.416999549|8.59300041",
            "lerpf|0.525900006|-0.289999545|1.87100053",
            "lerpf|0.206599995|0.864000499|-8.08599949",
            "lerpf|0.176499993|-0.224999532|4.9890008",
            "lerpf|0.613599956|-3.81899977|5.65500069",
            "lerpf|0.612500012|5.7060008|-2.33499956",
            "lerpf|0.958999991|2.24900055|-6.52499962",
                    "clamppd|-0.5|10|20",
                    "clamppd|-1e-09|10|20",
                    "clamppd|0|10|20",
                    "clamppd|0.5|10|20",
                    "clamppd|1|10|20",
                    "clamppd|1.000000001|10|20",
                    "clamppd|1.5|10|20",
                    "clamppd|-0.5|0.1|0.7",
                    "clamppd|1.5|0.1|0.7",
                    "clamppd|1|0.1|0.7",
                    "lerpint|0.5|0|10",
                    "lerpint|0|0|10",
                    "lerpint|1|0|10",
                    "lerpint|0.5|10|0",
                    "lerpint|0.25|0|10",
                    "lerpint|0.55|10|0",
                    "lerpint|-0.5|0|10",
                    "lerpdisc|0|0|10",
                    "lerpdisc|1|0|10",
                    "lerpdisc|0.5|0|10",
                    "lerpdisc|0.0001|0|10",
                    "lerpdisc|0.9999|0|10",
                    "lerpdisc|0|100|200",
                    "lerpdisc|1|100|200",
                    "lerp2|0|0|0|10|20|30",
                    "lerp2|1|0|0|10|20|30",
                    "lerp2|0|1|0|10|20|30",
                    "lerp2|1|1|0|10|20|30",
                    "lerp2|0.5|0.5|0|10|20|30");

    private MthLerpOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: MthLerpOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            text.append(GSON.toJson(summarise(unit))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|");
        if (parts.length < 2) {
            throw new IllegalArgumentException("mth_lerp: 单元至少两段: " + unit);
        }

        String mode = parts[0];
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (mode.equals("lerpd") && parts.length == 4) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.lerp(
                            Double.parseDouble(parts[1]),
                            Double.parseDouble(parts[2]),
                            Double.parseDouble(parts[3]))));
            return result;
        }
        if (mode.equals("lerpf") && parts.length == 4) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.lerp(
                            Float.parseFloat(parts[1]),
                            Float.parseFloat(parts[2]),
                            Float.parseFloat(parts[3]))));
            return result;
        }
        if (mode.equals("clamppd") && parts.length == 4) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.clampedLerp(
                            Double.parseDouble(parts[1]),
                            Double.parseDouble(parts[2]),
                            Double.parseDouble(parts[3]))));
            return result;
        }
        if (mode.equals("lerpint") && parts.length == 4) {
            result.addProperty(
                    "value",
                    Mth.lerpInt(
                            Float.parseFloat(parts[1]),
                            Integer.parseInt(parts[2]),
                            Integer.parseInt(parts[3])));
            return result;
        }
        if (mode.equals("lerpdisc") && parts.length == 4) {
            result.addProperty(
                    "value",
                    Mth.lerpDiscrete(
                            Float.parseFloat(parts[1]),
                            Integer.parseInt(parts[2]),
                            Integer.parseInt(parts[3])));
            return result;
        }
        if (mode.equals("lerp2") && parts.length == 7) {
            result.addProperty(
                    "value",
                    formatNumber(Mth.lerp2(
                            Double.parseDouble(parts[1]),
                            Double.parseDouble(parts[2]),
                            Double.parseDouble(parts[3]),
                            Double.parseDouble(parts[4]),
                            Double.parseDouble(parts[5]),
                            Double.parseDouble(parts[6]))));
            return result;
        }

        throw new IllegalArgumentException("mth_lerp: 无法识别的单元: " + unit);
    }

    private static String formatNumber(double value) {
        String text = String.format("%.9g", value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            while (text.endsWith("0")) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }
}