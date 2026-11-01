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
import java.util.UUID;

/**
 * UUID 字符串解析与格式化的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.UuidStringsOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用 JDK {@link UUID#fromString(String)} 与 {@link UUID#toString()}——
 * 这正是原版 Minecraft 使用的实现（源码里到处是 {@code UUID.fromString}）。
 *
 * <p><b>本能力的探针对接 Cubium 的真实实现</b>
 * （{@code util::uuidFromString} / {@code uuidToString} / {@code uuidToStringWithDashes}）。
 *
 * <p>原版解析失败抛 {@link IllegalArgumentException}，本 oracle 输出字面量 {@code null}；
 * Cubium 的 {@code uuidFromString} 失败返回全零数组，探针也输出 {@code null}。
 *
 * <p>JDK 的 {@code UUID.fromString} 有两条容易忽略的规则：
 * <ol>
 *   <li><b>只接受带连字符的 36 位格式</b>，32 位无连字符会抛异常；</li>
 *   <li><b>短段会向前补零</b>（如 {@code ...-fca90e38aaf} 补成 {@code ...-0fca90e38aaf}），
 *       但总长超过 36 位一律抛异常。</li>
 * </ol>
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/uuid_strings.cpp 逐项一致。
 */
public final class UuidStringsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "size",
                    "parse|069a79f4-44e9-4726-a5be-fca90e38aaf5",
                    "parse|00000000-0000-0000-0000-000000000000",
                    "parse|ffffffff-ffff-ffff-ffff-ffffffffffff",
                    "parse|069A79F4-44E9-4726-A5BE-FCA90E38AAF5",
                    "parse|069a79f444e94726a5befca90e38aaf5",
                    "parse|",
                    "parse|dunno",
                    "parse|069a79f4-44e9-4726-a5be-fca90e38aaf",
                    "parse|069a79f444e94726a5befca90e38aaf5-",
                    "parse|-069a79f4-44e9-4726-a5be-fca90e38aaf5",
                    "parse|069a79f4_44e9_4726_a5be_fca90e38aaf5",
                    "toString|069a79f4-44e9-4726-a5be-fca90e38aaf5",
                    "toString|00000000-0000-0000-0000-000000000000",
                    "toStringDashes|069a79f4-44e9-4726-a5be-fca90e38aaf5",
                    "toStringDashes|00000000-0000-0000-0000-000000000000",
                    "roundTrip|069a79f4-44e9-4726-a5be-fca90e38aaf5",
                    "roundTrip|ffffffff-ffff-ffff-ffff-ffffffffffff");

    private UuidStringsOracle() {}

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
        System.err.println("usage: UuidStringsOracle list <units.txt> "
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

    /** 解析 UUID，失败返回 null（对应 JDK 的 IllegalArgumentException）。 */
    private static UUID tryParse(String text) {
        try {
            return UUID.fromString(text);
        } catch (IllegalArgumentException error) {
            return null;
        }
    }

    private static String hexWithoutDashes(UUID uuid) {
        return uuid.toString().replace("-", "");
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1 && parts[0].equals("size")) {
            // 等价于 Java 的 16 字节 UUID 长度。
            out.addProperty("value", "16");
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("uuid_strings: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "parse" -> {
                UUID uuid = tryParse(parts[1]);
                out.addProperty("value", uuid == null ? "null" : uuid.toString());
            }
            case "toString" -> {
                UUID uuid = tryParse(parts[1]);
                out.addProperty("value", uuid == null ? "null" : hexWithoutDashes(uuid));
            }
            case "toStringDashes" -> {
                UUID uuid = tryParse(parts[1]);
                out.addProperty("value", uuid == null ? "null" : uuid.toString());
            }
            case "roundTrip" -> {
                UUID uuid = tryParse(parts[1]);
                if (uuid == null) {
                    out.addProperty("value", "false");
                } else {
                    UUID back = tryParse(uuid.toString());
                    out.addProperty("value", String.valueOf(uuid.equals(back)));
                }
            }
            default -> throw new IllegalArgumentException(
                    "uuid_strings: 无法识别的单元: " + unit);
        }

        return out;
    }
}