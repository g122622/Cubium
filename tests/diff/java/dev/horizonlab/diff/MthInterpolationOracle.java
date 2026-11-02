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
import java.util.ArrayList;
import java.util.List;
import net.minecraft.SharedConstants;
import net.minecraft.server.Bootstrap;
import net.minecraft.util.Mth;

/**
 * {@code Mth} 插值与平滑函数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthInterpolationOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用 {@link Mth#lerp2} / {@link Mth#lerp3} / {@link Mth#catmullrom} /
 * {@link Mth#smoothstep}，不重写公式。
 *
 * <p>需要 bootstrap：{@code Mth} 的静态初始化会触碰 {@code SharedConstants}。
 *
 * <p>单元表必须与 probes/mth_interpolation.cpp 逐项一致。
 */
public final class MthInterpolationOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] DELTAS = {"0", "0.25", "0.5", "0.75", "1"};
    private static final String[] SMOOTH_SAMPLES = {"0", "0.1", "0.25", "0.5", "0.75", "0.9", "1"};

    private MthInterpolationOracle() {}

    private static List<String> buildUnits() {
        List<String> units = new ArrayList<>();
        for (String delta : DELTAS) {
            units.add("catmullrom|" + delta + ",0.1,0.4,0.7,1.0");
        }
        for (String sample : SMOOTH_SAMPLES) {
            units.add("smoothstep|" + sample);
        }
        return List.copyOf(units);
    }

    public static void main(String[] args) throws IOException {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : buildUnits()) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: MthInterpolationOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            JsonObject row = summarise(unit);
            row.addProperty("unit", unit);
            text.append(GSON.toJson(row)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /** 与 C++ 侧一致：14 位有效数字，含 `e` 时不剥零。 */
    private static String formatDouble(double value) {
        if (value == 0.0) {
            return "0";
        }
        String text = String.format("%.14g", value);
        // 剥掉尾随零，与 C++ 的 %g 行为一致（C++ 自动剥离，Java 不剥）。
        if (text.indexOf('e') < 0) {
            while (text.endsWith("0") && text.indexOf('.') >= 0) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }

    private static double[] parseDoubles(String text) {
        String[] pieces = text.split(",", -1);
        double[] values = new double[pieces.length];
        for (int i = 0; i < pieces.length; i++) {
            values[i] = Double.parseDouble(pieces[i]);
        }
        return values;
    }

    private static JsonObject summarise(String unit) {
        int bar = unit.indexOf('|');
        if (bar < 0) {
            throw new IllegalArgumentException("mth_interpolation: 单元应为 <模式>|<参数>: " + unit);
        }
        String mode = unit.substring(0, bar);
        double[] a = parseDoubles(unit.substring(bar + 1));

        double result;
        switch (mode) {
            case "lerp2" -> {
                if (a.length != 6) {
                    throw new IllegalArgumentException("mth_interpolation: lerp2 需要 6 个参数: " + unit);
                }
                result = Mth.lerp2(a[0], a[1], a[2], a[3], a[4], a[5]);
            }
            case "lerp3" -> {
                if (a.length != 11) {
                    throw new IllegalArgumentException("mth_interpolation: lerp3 需要 11 个参数: " + unit);
                }
                result = Mth.lerp3(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10]);
            }
            case "catmullrom" -> {
                if (a.length != 5) {
                    throw new IllegalArgumentException("mth_interpolation: catmullrom 需要 5 个参数: " + unit);
                }
                result = Mth.catmullrom((float) a[0], (float) a[1], (float) a[2], (float) a[3], (float) a[4]);
            }
            case "smoothstep" -> {
                if (a.length != 1) {
                    throw new IllegalArgumentException("mth_interpolation: smoothstep 需要 1 个参数: " + unit);
                }
                result = Mth.smoothstep(a[0]);
            }
            default -> throw new IllegalArgumentException("mth_interpolation: 未知模式: " + mode);
        }

        JsonObject out = new JsonObject();
        out.addProperty("result", formatDouble(result));
        return out;
    }
}
