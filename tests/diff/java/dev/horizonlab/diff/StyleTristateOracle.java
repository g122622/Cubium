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
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.mojang.serialization.JsonOps;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.ChatFormatting;
import net.minecraft.network.chat.Style;
import net.minecraft.network.chat.TextColor;

/**
 * {@code Style} 三态格式标志序列化的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.StyleTristateOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>用 {@link Style.Serializer#CODEC} 配合 {@link JsonOps#INSTANCE} 序列化，
 * 得到与原版存档一致的 JSON——这是**可观测行为**，而不是内部字段。
 *
 * <p>原版的 {@code Style#isBold()} 是 {@code bold == Boolean.TRUE}，无法区分
 * 「未设置」与「显式 false」；Cubium 在 getter 上暴露三态属于接口设计差异。
 * **必须一致的是序列化**：显式 {@code false} 要写出 {@code "bold": false}，
 * 未设置则不写键。
 *
 * <p>本能力需要注册表相关 bootstrap——{@code Style.Serializer.CODEC} 的静态初始化
 * 会触及 {@code TextColor} 的 codec。
 *
 * <p>单元表必须与 probes/style_tristate.cpp 逐项一致。
 */
public final class StyleTristateOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "count",
                    "flags",
                    "empty",
                    "colorSet|RED",
                    "colorSet|WHITE",
                    "set|bold|true",
                    "set|bold|false",
                    "set|bold|unset",
                    "set|italic|true",
                    "set|italic|false",
                    "set|underlined|true",
                    "set|underlined|false",
                    "set|strikethrough|true",
                    "set|strikethrough|false",
                    "set|obfuscated|true",
                    "set|obfuscated|false",
                    "setAll|t",
                    "setAll|f",
                    "setThenClear|bold|true",
                    "setThenClear|bold|false",
                    "setThenClear|italic|false",
                    "asBool|bold|unset",
                    "asBool|bold|false",
                    "asBool|bold|true",
                    "asBool|italic|unset",
                    "asBool|italic|false");

    /** 五个三态标志的名字，顺序必须与探针一致。 */
    private static final List<String> FLAGS =
            List.of("bold", "italic", "underlined", "strikethrough", "obfuscated");

    private StyleTristateOracle() {}

    public static void main(String[] args) throws IOException {
        // Style.Serializer.CODEC 的静态初始化会触及 TextColor 的 codec，必须先 bootstrap。
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
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
        System.err.println("usage: StyleTristateOracle list <units.txt> "
                + "| run <input.jsonl> <output.jsonl>");
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

    /** 用原版 codec 把 {@link Style} 序列化成 JSON 文本。 */
    private static String serialize(Style style) {
        JsonElement element = Style.Serializer.CODEC
                .encodeStart(JsonOps.INSTANCE, style)
                .getOrThrow(error -> new IllegalStateException("style_tristate: " + error));
        // 原版对空样式产出空对象；按 JsonOps 的实际结果原样输出。
        return GSON.toJson(element);
    }

    private static TextColor colorByName(String name) {
        ChatFormatting formatting = ChatFormatting.valueOf(name);
        Integer rgb = formatting.getColor();
        if (rgb == null) {
            throw new IllegalArgumentException("style_tristate: " + name + " 不是颜色");
        }
        return TextColor.fromRgb(rgb);
    }

    /** 设置单个标志（值为 null 表示清除）。 */
    private static Style withFlag(Style style, String flag, Boolean value) {
        return switch (flag) {
            case "bold" -> style.withBold(value);
            case "italic" -> style.withItalic(value);
            case "underlined" -> style.withUnderlined(value);
            case "strikethrough" -> style.withStrikethrough(value);
            case "obfuscated" -> style.withObfuscated(value);
            default -> throw new IllegalArgumentException("style_tristate: 未知标志: " + flag);
        };
    }

    /** 读取 {@code isXxx()} 的结果（原版只有 boolean，无三态访问器）。 */
    private static boolean asBool(Style style, String flag) {
        return switch (flag) {
            case "bold" -> style.isBold();
            case "italic" -> style.isItalic();
            case "underlined" -> style.isUnderlined();
            case "strikethrough" -> style.isStrikethrough();
            case "obfuscated" -> style.isObfuscated();
            default -> throw new IllegalArgumentException("style_tristate: 未知标志: " + flag);
        };
    }

    private static Boolean parseTriState(String text) {
        return switch (text) {
            case "true" -> Boolean.TRUE;
            case "false" -> Boolean.FALSE;
            default -> null;
        };
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        Style style = Style.EMPTY;

        if (parts.length == 1) {
            switch (parts[0]) {
                case "count" -> out.addProperty("value", String.valueOf(FLAGS.size()));
                case "flags" -> out.addProperty("value", String.join(",", FLAGS));
                case "empty" -> out.addProperty("value", serialize(style));
                default -> throw new IllegalArgumentException(
                        "style_tristate: 无法识别的单元: " + unit);
            }
            return out;
        }

        switch (parts[0]) {
            case "colorSet" -> {
                if (parts.length != 2) {
                    throw new IllegalArgumentException("style_tristate: 段数不符: " + unit);
                }
                out.addProperty("value", serialize(style.withColor(colorByName(parts[1]))));
            }
            case "set" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException("style_tristate: 段数不符: " + unit);
                }
                out.addProperty("value",
                        serialize(withFlag(style, parts[1], parseTriState(parts[2]))));
            }
            case "setAll" -> {
                if (parts.length != 2) {
                    throw new IllegalArgumentException("style_tristate: 段数不符: " + unit);
                }
                Boolean value = parts[1].equals("t") ? Boolean.TRUE : Boolean.FALSE;
                for (String flag : FLAGS) {
                    style = withFlag(style, flag, value);
                }
                out.addProperty("value", serialize(style));
            }
            case "setThenClear" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException("style_tristate: 段数不符: " + unit);
                }
                style = withFlag(style, parts[1], parseTriState(parts[2]));
                style = withFlag(style, parts[1], null);
                out.addProperty("value", serialize(style));
            }
            case "asBool" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException("style_tristate: 段数不符: " + unit);
                }
                style = withFlag(style, parts[1], parseTriState(parts[2]));
                out.addProperty("value", String.valueOf(asBool(style, parts[1])));
            }
            default -> throw new IllegalArgumentException(
                    "style_tristate: 无法识别的单元: " + unit);
        }

        return out;
    }
}