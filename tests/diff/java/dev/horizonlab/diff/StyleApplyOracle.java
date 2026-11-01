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
import java.util.Locale;
import net.minecraft.ChatFormatting;
import net.minecraft.network.chat.Style;
import net.minecraft.network.chat.TextColor;

/**
 * {@code Style} 格式应用与合并的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.StyleApplyOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Style#applyFormat(ChatFormatting)} /
 * {@link Style#applyLegacyFormat(ChatFormatting)} / {@link Style#applyTo(Style)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/style_apply.cpp 逐项一致。
 */
public final class StyleApplyOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "applyFormat|-|-|-|-|-|-|-|bold",
                    "applyFormat|ff0000|-|-|-|-|-|-|bold",
                    "applyFormat|ff0000|1|1|-|-|-|-|green",
                    "applyFormat|-|1|1|1|1|1|-|green",
                    "applyFormat|ff0000|1|1|1|1|1|hello|reset",
                    "applyLegacyFormat|ff0000|1|1|1|1|1|-|green",
                    "applyLegacyFormat|-|1|1|1|1|1|-|green",
                    "applyLegacyFormat|ff0000|-|-|-|-|-|-|bold",
                    "applyLegacyFormat|ff0000|1|1|1|1|1|hello|reset",
                    "applyTo|ff0000|1|-|-|-|-|-|-|0|1|-|-|-|-",
                    "applyTo|-|-|-|-|-|-|-|ff0000|1|-|-|-|-|-",
                    "applyTo|ff0000|0|-|-|-|-|-|-|-|-|-|-|-|-",
                    "applyTo|-|-|-|-|-|-|-|-|-|-|-|-|-|-",
                    "applyTo|-|-|-|-|-|-|aaa|-|-|-|-|-|-|bbb");

    private StyleApplyOracle() {}

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
                "usage: StyleApplyOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /**
     * 读取 {@code Style} 的包级私有 {@code Boolean} 字段。
     *
     * <p>{@code isBold()} 等 getter 返回的是 {@code boolean}，**无法区分 {@code null}（继承）与
     * {@code false}（显式关闭）**——而 {@code applyTo} 的优先级正依赖这一区别。
     */
    private static Boolean readBooleanField(Style style, String name) {
        try {
            java.lang.reflect.Field field = Style.class.getDeclaredField(name);
            field.setAccessible(true);
            return (Boolean) field.get(style);
        } catch (ReflectiveOperationException exception) {
            throw new IllegalStateException("style_apply: 无法读取字段 " + name, exception);
        }
    }

    /** 渲染布尔三元值：`null` → `-`，`false` → `0`，`true` → `1`。 */
    private static String renderBoolean(Boolean value) {
        return value == null ? "-" : (value ? "1" : "0");
    }

    /** 渲染样式为 7 段紧凑形式。 */
    private static String render(Style style) {
        String color = "-";
        TextColor textColor = style.getColor();
        if (textColor != null) {
            color = String.format(Locale.ROOT, "%06x", textColor.getValue());
        }
        String insertion = style.getInsertion() == null ? "-" : style.getInsertion();
        return color + "|" + renderBoolean(readBooleanField(style, "bold")) + "|"
                + renderBoolean(readBooleanField(style, "italic")) + "|"
                + renderBoolean(readBooleanField(style, "underlined")) + "|"
                + renderBoolean(readBooleanField(style, "strikethrough")) + "|"
                + renderBoolean(readBooleanField(style, "obfuscated")) + "|" + insertion;
    }

    /** 解析布尔三元值。 */
    private static Boolean parseBoolean(String text) {
        return text.equals("-") ? null : Boolean.valueOf(text.equals("1"));
    }

    /** 从 7 段解析样式。 */
    private static Style parseStyle(String[] parts, int offset) {
        Style style = Style.EMPTY;
        if (!parts[offset].equals("-")) {
            style = style.withColor(Integer.parseInt(parts[offset], 16));
        }
        style = style.withBold(parseBoolean(parts[offset + 1]));
        style = style.withItalic(parseBoolean(parts[offset + 2]));
        style = style.withUnderlined(parseBoolean(parts[offset + 3]));
        style = style.withStrikethrough(parseBoolean(parts[offset + 4]));
        style = style.withObfuscated(parseBoolean(parts[offset + 5]));
        if (!parts[offset + 6].equals("-")) {
            style = style.withInsertion(parts[offset + 6]);
        }
        return style;
    }

    /** 按名字查格式化枚举。 */
    private static ChatFormatting formattingByName(String name) {
        return ChatFormatting.valueOf(name.toUpperCase(Locale.ROOT));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1 && parts[0].equals("empty")) {
            out.addProperty("value", render(Style.EMPTY));
            return out;
        }
        // applyFormat / applyLegacyFormat = 1 + 7 = 8 段；applyTo = 1 + 7 + 7 = 15 段。
        if (parts.length != 9 && parts.length != 15) {
            throw new IllegalArgumentException("style_apply: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "applyFormat" -> out.addProperty("value",
                    render(parseStyle(parts, 1).applyFormat(formattingByName(parts[8]))));
            case "applyLegacyFormat" -> out.addProperty("value",
                    render(parseStyle(parts, 1).applyLegacyFormat(formattingByName(parts[8]))));
            case "applyTo" -> out.addProperty("value",
                    render(parseStyle(parts, 1).applyTo(parseStyle(parts, 8))));
            default -> throw new IllegalArgumentException(
                    "style_apply: 无法识别的单元: " + unit);
        }

        return out;
    }
}
