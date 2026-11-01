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
import net.minecraft.world.item.DyeColor;

/**
 * {@code DyeColor} 染色值与查找的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DyeColorOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link DyeColor#byId(int)} / {@link DyeColor#byName(String, DyeColor)} /
 * {@link DyeColor#byFireworkColor(int)}，以及实例方法 `getId` / `getTextureDiffuseColor` /
 * `getFireworkColor` / `getTextColor` / `getSerializedName`。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/dye_color.cpp 逐项一致。
 */
public final class DyeColorOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "byId|0", "byId|1", "byId|2", "byId|3", "byId|4",
                    "byId|5", "byId|6", "byId|7", "byId|8", "byId|9",
                    "byId|10", "byId|11", "byId|12", "byId|13", "byId|14", "byId|15",
                    "byId|-1", "byId|16", "byId|999", "byId|-999",
                    "byName|white", "byName|light_blue", "byName|WHITE",
                    "byName|Light_Blue", "byName|orange", "byName|not_a_color",
                    "byFirework|15790320", "byFirework|16777215", "byFirework|0",
                    "byFirework|12345", "byFirework|15435844",
                    "info|WHITE", "info|ORANGE", "info|LIGHT_BLUE",
                    "info|BLACK", "info|RED", "info|CYAN");

    private DyeColorOracle() {}

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
                "usage: DyeColorOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static String nameOf(DyeColor color) {
        return color == null ? "null" : color.name();
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("dye_color: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "byId" -> out.addProperty(
                    "value", DyeColor.byId(Integer.parseInt(parts[1])).name());
            case "byName" -> out.addProperty(
                    "value", nameOf(DyeColor.byName(parts[1], null)));
            case "byFirework" -> out.addProperty(
                    "value", nameOf(DyeColor.byFireworkColor(Integer.parseInt(parts[1]))));
            case "info" -> {
                DyeColor color = matchByName(parts[1]);
                if (color == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value",
                            color.getId() + "," + rgbOf(color) + ","
                                    + color.getTextureDiffuseColor() + ","
                                    + color.getFireworkColor() + ","
                                    + color.getTextColor() + ","
                                    + color.getSerializedName());
                }
            }
            default -> throw new IllegalArgumentException("dye_color: 无法识别的单元: " + unit);
        }

        return out;
    }

    /**
     * 从 `textureDiffuseColor` 反推原始 `rgb`（去掉 alpha 高位）。
     *
     * <p>`textureDiffuseColor = ARGB.opaque(rgb) = 0xFF000000 | rgb`，所以
     * `rgb = textureDiffuseColor & 0x00FFFFFF`。
     */
    private static int rgbOf(DyeColor color) {
        return color.getTextureDiffuseColor() & 0x00FFFFFF;
    }

    /** 按枚举名精确匹配。 */
    private static DyeColor matchByName(String name) {
        for (DyeColor color : DyeColor.values()) {
            if (color.name().equals(name)) {
                return color;
            }
        }
        return null;
    }
}