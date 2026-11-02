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
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.SharedConstants;
import net.minecraft.server.Bootstrap;
import net.minecraft.util.EasingType;

/**
 * 缓动函数 {@code EasingType} / {@code Ease} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EasingTypeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用 {@link EasingType} 扩展接口的 {@code apply(float)}，不重写公式。
 *
 * <p>需要 bootstrap：{@code EasingType} 的静态初始化会注册到
 * {@code ExtraCodecs.LateBoundIdMapper}，那条路径触碰 {@code Codec} 相关静态字段。
 *
 * <p>单元表必须与 probes/easing_type.cpp 逐项一致。
 */
public final class EasingTypeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 32 个具名缓动，键为序列化名（与探针一致）。 */
    private static final Map<String, EasingType> EASINGS = buildEasings();

    /** 采样点，与探针的 SAMPLES 一致。 */
    private static final String[] SAMPLES = {"0", "0.1", "0.25", "0.3333", "0.5", "0.6667", "0.75", "0.9", "1"};

    private static Map<String, EasingType> buildEasings() {
        Map<String, EasingType> map = new LinkedHashMap<>();
        map.put("constant", EasingType.CONSTANT);
        map.put("linear", EasingType.LINEAR);
        map.put("in_back", EasingType.IN_BACK);
        map.put("in_bounce", EasingType.IN_BOUNCE);
        map.put("in_circ", EasingType.IN_CIRC);
        map.put("in_cubic", EasingType.IN_CUBIC);
        map.put("in_elastic", EasingType.IN_ELASTIC);
        map.put("in_expo", EasingType.IN_EXPO);
        map.put("in_quad", EasingType.IN_QUAD);
        map.put("in_quart", EasingType.IN_QUART);
        map.put("in_quint", EasingType.IN_QUINT);
        map.put("in_sine", EasingType.IN_SINE);
        map.put("in_out_back", EasingType.IN_OUT_BACK);
        map.put("in_out_bounce", EasingType.IN_OUT_BOUNCE);
        map.put("in_out_circ", EasingType.IN_OUT_CIRC);
        map.put("in_out_cubic", EasingType.IN_OUT_CUBIC);
        map.put("in_out_elastic", EasingType.IN_OUT_ELASTIC);
        map.put("in_out_expo", EasingType.IN_OUT_EXPO);
        map.put("in_out_quad", EasingType.IN_OUT_QUAD);
        map.put("in_out_quart", EasingType.IN_OUT_QUART);
        map.put("in_out_quint", EasingType.IN_OUT_QUINT);
        map.put("in_out_sine", EasingType.IN_OUT_SINE);
        map.put("out_back", EasingType.OUT_BACK);
        map.put("out_bounce", EasingType.OUT_BOUNCE);
        map.put("out_circ", EasingType.OUT_CIRC);
        map.put("out_cubic", EasingType.OUT_CUBIC);
        map.put("out_elastic", EasingType.OUT_ELASTIC);
        map.put("out_expo", EasingType.OUT_EXPO);
        map.put("out_quad", EasingType.OUT_QUAD);
        map.put("out_quart", EasingType.OUT_QUART);
        map.put("out_quint", EasingType.OUT_QUINT);
        map.put("out_sine", EasingType.OUT_SINE);
        return Map.copyOf(map);
    }

    private static List<String> buildUnits() {
        List<String> units = new ArrayList<>();
        for (String name : EASINGS.keySet()) {
            for (String sample : SAMPLES) {
                units.add(name + "|" + sample);
            }
        }
        return List.copyOf(units);
    }

    private EasingTypeOracle() {}

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
        System.err.println("usage: EasingTypeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 与探测侧一致：float 的 9 位有效数字往返表示。 */
    private static String formatFloat(float value) {
        if (value == 0.0F) {
            return "0";
        }
        String text = String.format("%.9g", (double) value);
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

    private static JsonObject summarise(String unit) {
        int bar = unit.indexOf('|');
        if (bar < 0) {
            throw new IllegalArgumentException("easing_type: 单元应为 <name>|<input>: " + unit);
        }
        String name = unit.substring(0, bar);
        float input = Float.parseFloat(unit.substring(bar + 1));
        EasingType easing = EASINGS.get(name);
        if (easing == null) {
            throw new IllegalArgumentException("easing_type: 未知缓动名: " + name);
        }
        JsonObject out = new JsonObject();
        out.addProperty("result", formatFloat(easing.apply(input)));
        return out;
    }
}
