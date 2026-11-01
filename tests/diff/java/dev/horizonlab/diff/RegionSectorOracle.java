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
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.world.level.ChunkPos;

/**
 * 区域文件区块索引的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RegionSectorOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>区块坐标换算直接调用原版 {@link ChunkPos#getRegionX()} / {@link ChunkPos#getRegionZ()} /
 * {@link ChunkPos#getRegionLocalX()} / {@link ChunkPos#getRegionLocalZ()}。
 *
 * <p>槽位索引与扇区拆解在 {@code RegionFile} 里是 private static 方法，通过反射调用，
 * 不重写公式。
 *
 * <p>单元表必须与 probes/region_sector.cpp 逐项一致。
 */
public final class RegionSectorOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "region|0|0",
                    "region|1|1",
                    "region|31|31",
                    "region|32|32",
                    "region|63|63",
                    "region|64|64",
                    "region|-1|-1",
                    "region|-32|-32",
                    "region|-33|-33",
                    "region|-64|-64",
                    "region|100|0",
                    "region|0|100",
                    "region|-100|100",
                    "region|1000000|-1000000",
                    "region|31|0",
                    "region|0|31",
                    "sector|0",
                    "sector|514",
                    "sector|51400",
                    "sector|25600",
                    "sector|-1",
                    "sector|131072",
                    "pack|2|1",
                    "pack|0|0",
                    "pack|100|3",
                    "pack|1|256",
                    "pack|1|255");

    private RegionSectorOracle() {}

    public static void main(String[] args) throws IOException {
        // `ChunkPos` 的构造会触发注册表访问，必须先加载数据包注册表。
        DataPackRegistries.load();
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
                "usage: RegionSectorOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 反射取得 `RegionFile` 的私有静态方法。 */
    private static Method regionFileMethod(String name, Class<?>... params) {
        try {
            Class<?> clazz = Class.forName("net.minecraft.world.level.chunk.storage.RegionFile");
            Method method = clazz.getDeclaredMethod(name, params);
            method.setAccessible(true);
            return method;
        } catch (ReflectiveOperationException ex) {
            throw new IllegalStateException("无法取得 RegionFile." + name, ex);
        }
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|");
        if (parts.length < 2) {
            throw new IllegalArgumentException("region_sector: 单元至少两段: " + unit);
        }

        String mode = parts[0];
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        try {
            if (mode.equals("region") && parts.length == 3) {
                ChunkPos pos =
                        new ChunkPos(Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
                int localX = pos.getRegionLocalX();
                int localZ = pos.getRegionLocalZ();

                Method getOffsetIndex = regionFileMethod("getOffsetIndex", ChunkPos.class);
                int offsetIndex = (int) getOffsetIndex.invoke(null, pos);

                result.addProperty("regionX", pos.getRegionX());
                result.addProperty("regionZ", pos.getRegionZ());
                result.addProperty("localX", localX);
                result.addProperty("localZ", localZ);
                result.addProperty("offsetIndex", offsetIndex);
                return result;
            }

            if (mode.equals("sector") && parts.length == 2) {
                int offset = Integer.parseInt(parts[1]);
                Method getSectorNumber = regionFileMethod("getSectorNumber", int.class);
                Method getNumSectors = regionFileMethod("getNumSectors", int.class);
                result.addProperty("sector", (int) getSectorNumber.invoke(null, offset));
                result.addProperty("count", (int) getNumSectors.invoke(null, offset));
                return result;
            }

            if (mode.equals("pack") && parts.length == 3) {
                int sector = Integer.parseInt(parts[1]);
                int count = Integer.parseInt(parts[2]);
                // 原版没有打包函数（只在写入时内联构造），这里按位布局复现。
                result.addProperty("offset", (sector << 8) | (count & 0xFF));
                return result;
            }
        } catch (ReflectiveOperationException ex) {
            throw new IllegalStateException("region_sector: 反射调用失败 " + unit, ex);
        }

        throw new IllegalArgumentException("region_sector: 无法识别的单元: " + unit);
    }
}