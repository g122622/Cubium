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
import net.minecraft.util.StringRepresentable;
import net.minecraft.world.level.block.entity.trialspawner.TrialSpawnerState;
import net.minecraft.world.level.block.entity.vault.VaultState;
import net.minecraft.world.level.block.state.properties.ComparatorMode;
import net.minecraft.world.level.block.state.properties.CreakingHeartState;
import net.minecraft.world.level.block.state.properties.NoteBlockInstrument;
import net.minecraft.world.level.block.state.properties.SideChainPart;
import net.minecraft.world.level.block.state.properties.StructureMode;
import net.minecraft.world.level.block.state.properties.TestBlockMode;
import net.minecraft.world.level.block.state.properties.WallSide;

/**
 * 第三组方块状态枚举字符串映射的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockStateEnumStrings3Oracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用各枚举的 {@code getSerializedName()}，反向用 {@code StringRepresentable.fromEnum}
 * 的单参数 {@code byName}（未知输入返回 {@code null}）。
 *
 * <p><b>本能力的探针对接 Cubium 的真实实现</b>
 * （{@code EnumProperty<T>::Traits::toString} / {@code fromName}）。
 *
 * <p>注意两点：
 * <ul>
 *   <li>{@code NoteBlockInstrument} 有 <b>23</b> 项，其中 4 项含下划线
 *       （{@code iron_xylophone} / {@code cow_bell} / {@code wither_skeleton} /
 *       {@code custom_head}），但 {@code BASEDRUM} 的序列化名是 {@code basedrum}
 *       <b>不带下划线</b>；</li>
 *   <li>{@code SideChainPart} 的原版枚举序是 {@code UNCONNECTED}/{@code RIGHT}/
 *       {@code CENTER}/{@code LEFT}——{@code RIGHT} 在 {@code CENTER} <b>之前</b>。</li>
 * </ul>
 *
 * <p>注意 {@code OxidationLevel} 在 1.21.11 中<b>不是</b>独立的
 * {@code StringRepresentable} 枚举（只有 {@code CopperGolemOxidationLevel} record），
 * 因此本能力不覆盖它。
 *
 * <p>本能力<b>需要 bootstrap</b>：{@link NoteBlockInstrument} 的静态初始化引用
 * {@code SoundEvents}（注册表条目），必须先调用 {@code Bootstrap.bootStrap()}。
 *
 * <p>单元表必须与 probes/block_state_enum_strings_3.cpp 逐项一致。
 */
public final class BlockStateEnumStrings3Oracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "count|NoteBlockInstrument",
                    "count|ComparatorMode",
                    "count|StructureMode",
                    "count|TestBlockMode",
                    "count|TrialSpawnerState",
                    "count|VaultState",
                    "count|CreakingHeartState",
                    "count|SideChainPart",
                    "count|WallHeight",
                    "NoteBlockInstrument|toString|HARP",
                    "NoteBlockInstrument|toString|BASEDRUM",
                    "NoteBlockInstrument|toString|SNARE",
                    "NoteBlockInstrument|toString|HAT",
                    "NoteBlockInstrument|toString|BASS",
                    "NoteBlockInstrument|toString|FLUTE",
                    "NoteBlockInstrument|toString|BELL",
                    "NoteBlockInstrument|toString|GUITAR",
                    "NoteBlockInstrument|toString|CHIME",
                    "NoteBlockInstrument|toString|XYLOPHONE",
                    "NoteBlockInstrument|toString|IRON_XYLOPHONE",
                    "NoteBlockInstrument|toString|COW_BELL",
                    "NoteBlockInstrument|toString|DIDGERIDOO",
                    "NoteBlockInstrument|toString|BIT",
                    "NoteBlockInstrument|toString|BANJO",
                    "NoteBlockInstrument|toString|PLING",
                    "NoteBlockInstrument|toString|ZOMBIE",
                    "NoteBlockInstrument|toString|SKELETON",
                    "NoteBlockInstrument|toString|CREEPER",
                    "NoteBlockInstrument|toString|DRAGON",
                    "NoteBlockInstrument|toString|WITHER_SKELETON",
                    "NoteBlockInstrument|toString|PIGLIN",
                    "NoteBlockInstrument|toString|CUSTOM_HEAD",
                    "NoteBlockInstrument|fromName|harp",
                    "NoteBlockInstrument|fromName|basedrum",
                    "NoteBlockInstrument|fromName|base_drum",
                    "NoteBlockInstrument|fromName|iron_xylophone",
                    "NoteBlockInstrument|fromName|custom_head",
                    "ComparatorMode|toString|COMPARE",
                    "ComparatorMode|toString|SUBTRACT",
                    "ComparatorMode|fromName|compare",
                    "ComparatorMode|fromName|subtract",
                    "StructureMode|toString|SAVE",
                    "StructureMode|toString|LOAD",
                    "StructureMode|toString|CORNER",
                    "StructureMode|toString|DATA",
                    "StructureMode|fromName|corner",
                    "TestBlockMode|toString|START",
                    "TestBlockMode|toString|LOG",
                    "TestBlockMode|toString|FAIL",
                    "TestBlockMode|toString|ACCEPT",
                    "TestBlockMode|fromName|accept",
                    "TrialSpawnerState|toString|INACTIVE",
                    "TrialSpawnerState|toString|WAITING_FOR_PLAYERS",
                    "TrialSpawnerState|toString|ACTIVE",
                    "TrialSpawnerState|toString|WAITING_FOR_REWARD_EJECTION",
                    "TrialSpawnerState|toString|EJECTING_REWARD",
                    "TrialSpawnerState|toString|COOLDOWN",
                    "TrialSpawnerState|fromName|waiting_for_players",
                    "TrialSpawnerState|fromName|waiting_for_reward_ejection",
                    "VaultState|toString|INACTIVE",
                    "VaultState|toString|ACTIVE",
                    "VaultState|toString|UNLOCKING",
                    "VaultState|toString|EJECTING",
                    "VaultState|fromName|unlocking",
                    "CreakingHeartState|toString|UPROOTED",
                    "CreakingHeartState|toString|DORMANT",
                    "CreakingHeartState|toString|AWAKE",
                    "CreakingHeartState|fromName|uprooted",
                    "SideChainPart|toString|UNCONNECTED",
                    "SideChainPart|toString|RIGHT",
                    "SideChainPart|toString|CENTER",
                    "SideChainPart|toString|LEFT",
                    "SideChainPart|fromName|center",
                    "WallHeight|toString|NONE",
                    "WallHeight|toString|LOW",
                    "WallHeight|toString|TALL",
                    "WallHeight|fromName|low",
                    "NoteBlockInstrument|fromName|dunno",
                    "SideChainPart|fromName|dunno");

    private BlockStateEnumStrings3Oracle() {}

    public static void main(String[] args) throws IOException {
        // NoteBlockInstrument 的静态初始化引用 SoundEvents（注册表），必须先 bootstrap。
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
        System.err.println("usage: BlockStateEnumStrings3Oracle list <units.txt> "
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

    /** 通用反向查找：按序列化名精确匹配，未命中返回 null。 */
    private static <E extends Enum<E> & StringRepresentable> E byName(E[] values, String name) {
        for (E value : values) {
            if (value.getSerializedName().equals(name)) {
                return value;
            }
        }
        return null;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 2 && parts[0].equals("count")) {
            out.addProperty("value", String.valueOf(countOf(parts[1])));
            return out;
        }
        if (parts.length != 3) {
            throw new IllegalArgumentException(
                    "block_state_enum_strings_3: 单元段数不符: " + unit);
        }

        String enumName = parts[0];
        String op = parts[1];
        String arg = parts[2];

        if (op.equals("toString")) {
            out.addProperty("value", serializedNameOf(enumName, arg));
            return out;
        }
        if (!op.equals("fromName")) {
            throw new IllegalArgumentException(
                    "block_state_enum_strings_3: 未知操作: " + op);
        }

        String found;
        switch (enumName) {
            case "NoteBlockInstrument" -> {
                NoteBlockInstrument v = byName(NoteBlockInstrument.values(), arg);
                found = v == null ? null : v.name();
            }
            case "ComparatorMode" -> {
                ComparatorMode v = byName(ComparatorMode.values(), arg);
                found = v == null ? null : v.name();
            }
            case "StructureMode" -> {
                StructureMode v = byName(StructureMode.values(), arg);
                found = v == null ? null : v.name();
            }
            case "TestBlockMode" -> {
                TestBlockMode v = byName(TestBlockMode.values(), arg);
                found = v == null ? null : v.name();
            }
            case "TrialSpawnerState" -> {
                TrialSpawnerState v = byName(TrialSpawnerState.values(), arg);
                found = v == null ? null : v.name();
            }
            case "VaultState" -> {
                VaultState v = byName(VaultState.values(), arg);
                found = v == null ? null : v.name();
            }
            case "CreakingHeartState" -> {
                CreakingHeartState v = byName(CreakingHeartState.values(), arg);
                found = v == null ? null : v.name();
            }
            case "SideChainPart" -> {
                SideChainPart v = byName(SideChainPart.values(), arg);
                found = v == null ? null : v.name();
            }
            case "WallHeight" -> {
                // Cubium 叫 WallHeight，原版叫 WallSide。
                WallSide v = byName(WallSide.values(), arg);
                found = v == null ? null : v.name();
            }
            default -> throw new IllegalArgumentException(
                    "block_state_enum_strings_3: 未知枚举: " + enumName);
        }
        out.addProperty("value", found == null ? "null" : found);
        return out;
    }

    private static int countOf(String enumName) {
        return switch (enumName) {
            case "NoteBlockInstrument" -> NoteBlockInstrument.values().length;
            case "ComparatorMode" -> ComparatorMode.values().length;
            case "StructureMode" -> StructureMode.values().length;
            case "TestBlockMode" -> TestBlockMode.values().length;
            case "TrialSpawnerState" -> TrialSpawnerState.values().length;
            case "VaultState" -> VaultState.values().length;
            case "CreakingHeartState" -> CreakingHeartState.values().length;
            case "SideChainPart" -> SideChainPart.values().length;
            case "WallHeight" -> WallSide.values().length;
            default -> throw new IllegalArgumentException(
                    "block_state_enum_strings_3: 未知枚举: " + enumName);
        };
    }

    private static String serializedNameOf(String enumName, String valueName) {
        String upper = valueName.toUpperCase(Locale.ROOT);
        return switch (enumName) {
            case "NoteBlockInstrument" -> NoteBlockInstrument.valueOf(upper).getSerializedName();
            case "ComparatorMode" -> ComparatorMode.valueOf(upper).getSerializedName();
            case "StructureMode" -> StructureMode.valueOf(upper).getSerializedName();
            case "TestBlockMode" -> TestBlockMode.valueOf(upper).getSerializedName();
            case "TrialSpawnerState" -> TrialSpawnerState.valueOf(upper).getSerializedName();
            case "VaultState" -> VaultState.valueOf(upper).getSerializedName();
            case "CreakingHeartState" -> CreakingHeartState.valueOf(upper).getSerializedName();
            case "SideChainPart" -> SideChainPart.valueOf(upper).getSerializedName();
            case "WallHeight" -> WallSide.valueOf(upper).getSerializedName();
            default -> throw new IllegalArgumentException(
                    "block_state_enum_strings_3: 未知枚举: " + enumName);
        };
    }
}