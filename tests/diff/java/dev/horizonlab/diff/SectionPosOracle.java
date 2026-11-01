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
import net.minecraft.core.SectionPos;

/**
 * {@code SectionPos} 坐标换算与 {@code long} 编码的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.SectionPosOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link SectionPos#blockToSectionCoord(int)} /
 * {@link SectionPos#blockToSectionCoord(double)} / {@link SectionPos#sectionToBlockCoord(int)} /
 * {@link SectionPos#sectionToBlockCoord(int, int)} / {@link SectionPos#asLong(int, int, int)} /
 * {@link SectionPos#x(long)} / {@link SectionPos#y(long)} / {@link SectionPos#z(long)}。
 *
 * <p>单元表必须与 probes/section_pos.cpp 逐项一致。
 */
public final class SectionPosOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "blockToSection|0",
                    "blockToSection|15",
                    "blockToSection|16",
                    "blockToSection|31",
                    "blockToSection|32",
                    "blockToSection|-1",
                    "blockToSection|-15",
                    "blockToSection|-16",
                    "blockToSection|-17",
                    "blockToSection|-32",
                    "blockToSection|-33",
                    "blockToSection|2147483647",
                    "blockToSection|-2147483648",
                    "blockToSectionD|0.0",
                    "blockToSectionD|15.9",
                    "blockToSectionD|-0.1",
                    "blockToSectionD|-1.0",
                    "blockToSectionD|-16.5",
                    "blockToSectionD|16.0",
                    "sectionToBlock|0",
                    "sectionToBlock|1",
                    "sectionToBlock|-1",
                    "sectionToBlock|-2",
                    "sectionToBlock|100",
                    "sectionToBlockOff|0|0",
                    "sectionToBlockOff|0|15",
                    "sectionToBlockOff|-1|8",
                    "sectionToBlockOff|2|100",
                    "asLong|0|0|0",
                    "asLong|1|2|3",
                    "asLong|-1|-1|-1",
                    "asLong|-5|10|-20",
                    "asLong|100|200|-300",
                    "decode|0",
                    "decode|4398046511104",
                    "roundtrip|1|2|3",
                    "roundtrip|-1|-1|-1",
                    "roundtrip|-5|10|-20",
                    "roundtrip|2097151|524287|2097151",
                    "roundtrip|-2097152|-524288|-2097152");

    private SectionPosOracle() {}

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
                "usage: SectionPosOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static String triple(int x, int y, int z) {
        return x + "," + y + "," + z;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("section_pos: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "blockToSection" -> out.addProperty(
                    "value", SectionPos.blockToSectionCoord(Integer.parseInt(parts[1])));
            case "blockToSectionD" -> out.addProperty(
                    "value", SectionPos.blockToSectionCoord(Double.parseDouble(parts[1])));
            case "sectionToBlock" -> out.addProperty(
                    "value", SectionPos.sectionToBlockCoord(Integer.parseInt(parts[1])));
            case "sectionToBlockOff" -> out.addProperty(
                    "value",
                    SectionPos.sectionToBlockCoord(
                            Integer.parseInt(parts[1]), Integer.parseInt(parts[2])));
            case "asLong" -> {
                long packed = SectionPos.asLong(
                        Integer.parseInt(parts[1]),
                        Integer.parseInt(parts[2]),
                        Integer.parseInt(parts[3]));
                // 长整型按字符串输出：JS 安全整数上限是 2^53。
                out.addProperty("value", Long.toString(packed));
            }
            case "decode" -> {
                long packed = Long.parseLong(parts[1]);
                out.addProperty("value",
                        triple(SectionPos.x(packed), SectionPos.y(packed), SectionPos.z(packed)));
            }
            case "roundtrip" -> {
                long packed = SectionPos.asLong(
                        Integer.parseInt(parts[1]),
                        Integer.parseInt(parts[2]),
                        Integer.parseInt(parts[3]));
                out.addProperty("value",
                        triple(SectionPos.x(packed), SectionPos.y(packed), SectionPos.z(packed)));
            }
            default -> throw new IllegalArgumentException("section_pos: 无法识别的单元: " + unit);
        }

        return out;
    }
}