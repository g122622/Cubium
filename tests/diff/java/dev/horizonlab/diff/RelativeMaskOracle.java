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
import java.util.EnumSet;
import java.util.List;
import java.util.Set;
import net.minecraft.world.entity.Relative;

/**
 * {@code Relative} 位掩码编解码与集合工厂的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RelativeMaskOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Relative#pack(Set)} / {@link Relative#unpack(int)} /
 * {@link Relative#rotation(boolean, boolean)} / {@link Relative#position(boolean, boolean, boolean)} /
 * {@link Relative#direction(boolean, boolean, boolean)} / {@link Relative#union(Set[])}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>集合输出按枚举序数排序，逗号分隔；空集输出 `-`。
 *
 * <p>单元表必须与 probes/relative_mask.cpp 逐项一致。
 */
public final class RelativeMaskOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "pack|x",
                    "pack|y_rot",
                    "pack|x_rot",
                    "pack|x,y",
                    "pack|x,y,z",
                    "pack|-",
                    "pack|x,y,z,y_rot,x_rot,delta_x,delta_y,delta_z,rotate_delta",
                    "unpack|0",
                    "unpack|1",
                    "unpack|8",
                    "unpack|16",
                    "unpack|511",
                    "unpack|256",
                    "unpack|1048576",
                    "rotation|true|false",
                    "rotation|false|true",
                    "rotation|true|true",
                    "rotation|false|false",
                    "position|true|false|false",
                    "position|false|true|false",
                    "position|true|true|true",
                    "position|false|false|false",
                    "direction|true|false|false",
                    "direction|true|true|true",
                    "direction|false|false|false",
                    "union|x|y",
                    "union|x,y|-",
                    "union|x,y|y,z|z",
                    "union|-|-");

    private RelativeMaskOracle() {}

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
                "usage: RelativeMaskOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 按枚举序数排序后渲染；空集输出 `-`。 */
    private static String render(Set<Relative> set) {
        if (set.isEmpty()) {
            return "-";
        }
        StringBuilder out = new StringBuilder();
        for (Relative relative : Relative.values()) {
            if (set.contains(relative)) {
                if (out.length() > 0) {
                    out.append(',');
                }
                out.append(relative.name().toLowerCase(java.util.Locale.ROOT));
            }
        }
        return out.toString();
    }

    /** 解析逗号分隔的名字列表；`-` 表示空集。 */
    private static Set<Relative> parse(String text) {
        Set<Relative> set = EnumSet.noneOf(Relative.class);
        if (text.equals("-")) {
            return set;
        }
        for (String token : text.split(",", -1)) {
            if (!token.isEmpty()) {
                set.add(Relative.valueOf(token.toUpperCase(java.util.Locale.ROOT)));
            }
        }
        return set;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "pack" -> out.addProperty("value", Relative.pack(parse(parts[1])));
            case "unpack" -> out.addProperty("value", render(Relative.unpack(
                    Integer.parseInt(parts[1]))));
            case "rotation" -> out.addProperty("value", render(Relative.rotation(
                    Boolean.parseBoolean(parts[1]), Boolean.parseBoolean(parts[2]))));
            case "position" -> out.addProperty("value", render(Relative.position(
                    Boolean.parseBoolean(parts[1]), Boolean.parseBoolean(parts[2]),
                    Boolean.parseBoolean(parts[3]))));
            case "direction" -> out.addProperty("value", render(Relative.direction(
                    Boolean.parseBoolean(parts[1]), Boolean.parseBoolean(parts[2]),
                    Boolean.parseBoolean(parts[3]))));
            case "union" -> {
                List<Set<Relative>> sets = new ArrayList<>();
                for (int i = 1; i < parts.length; i++) {
                    sets.add(parse(parts[i]));
                }
                @SuppressWarnings("unchecked")
                Set<Relative>[] array = sets.toArray(new Set[0]);
                out.addProperty("value", render(Relative.union(array)));
            }
            default -> throw new IllegalArgumentException(
                    "relative_mask: 无法识别的单元: " + unit);
        }

        return out;
    }
}