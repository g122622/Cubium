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
import com.mojang.brigadier.arguments.ArgumentType;
import com.mojang.brigadier.suggestion.SuggestionProvider;
import com.mojang.brigadier.tree.ArgumentCommandNode;
import com.mojang.brigadier.tree.CommandNode;
import com.mojang.brigadier.tree.RootCommandNode;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.Map;
import net.minecraft.commands.CommandBuildContext;
import net.minecraft.commands.synchronization.ArgumentTypeInfo;
import net.minecraft.commands.synchronization.ArgumentTypeInfos;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.commands.CommandSource;
import net.minecraft.commands.CommandSourceStack;
import net.minecraft.commands.Commands;
import net.minecraft.network.FriendlyByteBuf;
import net.minecraft.network.protocol.game.ClientboundCommandsPacket;
import net.minecraft.commands.synchronization.SuggestionProviders;
import net.minecraft.network.chat.Component;
import net.minecraft.resources.Identifier;
import net.minecraft.server.permissions.PermissionSet;
import net.minecraft.world.phys.Vec2;
import net.minecraft.world.phys.Vec3;
import net.minecraft.world.flag.FeatureFlags;
import io.netty.buffer.Unpooled;

/**
 * 服务端命令树的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CommandTreeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>用原版 {@code new Commands(CommandSelection.DEDICATED, context)} 注册全部服务端命令（与
 * 独立服务器启动时同一入口），再按 probes/command_tree.cpp 的格式扁平化：
 *
 * <pre>
 *   命令名 [子节点...]，参数写作 名字:类型名，非一级节点展开成 名字(子1,子2)
 * </pre>
 *
 * <p>参数类型名取原版类名（如 {@code GameModeArgument} → {@code gamemode}），与 Cubium 的
 * {@code ArgumentType::getTypeName()} 同一套词汇。
 */
public final class CommandTreeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().serializeNulls().create();

    private CommandTreeOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            Files.writeString(Path.of(args[1]), "tree\n", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]), "");
            return;
        }
        if (args.length == 4 && args[0].equals("run")) {
            // 第四个参数是逗号分隔的命令名：这些命令不在命令树里注册，后面的命令因此
            // 看不到它们带来的副作用（例如 /item 的槽位参数）。
            run(Path.of(args[1]), Path.of(args[2]), args[3]);
            return;
        }
        System.err.println("usage: CommandTreeOracle list <units.txt> | run <input.jsonl> <output.jsonl> [skip,commands]");
        System.exit(2);
    }

    /** 满权限、无实体、无世界的命令源：只用来判定 {@code requires}，不会被真正执行。 */
    private static final CommandSourceStack FULL_PERMISSION_SOURCE = new CommandSourceStack(
            CommandSource.NULL,
            Vec3.ZERO,
            Vec2.ZERO,
            null,
            PermissionSet.ALL_PERMISSIONS,
            "",
            Component.empty(),
            null,
            null);

    private static void run(Path input, Path output, String skipSpec) throws IOException {
        DataPackRegistries registries = DataPackRegistries.load();
        CommandBuildContext context = CommandBuildContext.simple(registries.lookup, FeatureFlags.DEFAULT_FLAGS);
        CommandNode<CommandSourceStack> root =
                new Commands(Commands.CommandSelection.DEDICATED, context).getDispatcher().getRoot();

        Set<String> skip = new HashSet<>();
        for (String name : skipSpec.split(",")) {
            if (!name.isBlank()) {
                skip.add(name.trim());
                root.getChildren().removeIf(node -> node.getName().equals(name.trim()));
            }
        }

        canonicaliseChildren(root, Collections.newSetFromMap(new IdentityHashMap<>()));

        List<String> lines = new ArrayList<>();
        for (CommandNode<CommandSourceStack> child : root.getChildren()) {
            lines.add(describeTree(child));
        }
        lines.sort(String::compareTo);

        JsonObject arguments = describeArgumentWire(root);
        JsonObject components = describeComponents(root);

        JsonObject result = new JsonObject();
        result.addProperty("unit", "tree");
        result.add("tree", GSON.toJsonTree(lines));
        result.add("arguments", arguments);
        result.add("components", components);
        // 整棵树按原版 ClientboundCommandsPacket 编码后的字节数。
        // 逐节点的 typeId/payload 只能发现「某个参数写错了」，发现不了节点顺序、节点计数、
        // 重定向下标等结构级偏差；总字节数可以对这类偏差做一次总量校验。
        JsonObject wire = new JsonObject();
        FriendlyByteBuf encoded = encodeTree(root);
        wire.addProperty("byteLength", encoded.readableBytes());
        wire.addProperty("payload", hex(encoded));
        ClientboundCommandsPacket.STREAM_CODEC.decode(encoded);
        encoded.release();
        result.add("wire", wire);
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (!line.isBlank()) {
                text.append(GSON.toJson(result)).append('\n');
            }
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /**
     * 把整棵命令树按原版 clientbound/minecraft:commands 的格式编码，返回承载它的缓冲。
     *
     * <p>用官方 {@code ClientboundCommandsPacket} 的构造函数 + STREAM_CODEC，与真实下发给
     * 客户端的字节完全同源，不自己拼字节。{@code NodeInspector} 的三个回调与专用服务器一致：
     * 建议提供器取 {@code SuggestionProviders.getName}（未注册的 provider 会回退成 ask_server），
     * 可执行性取 {@code CommandNode.canUse} 之外的 {@code getCommand() != null}，
     * 受限性取 {@code requires} 判定——这里默认 false，因为原版专用服务器的 inspector
     * 对非受限命令同样返回 false。
     */
    @SuppressWarnings({"unchecked", "rawtypes"})
    private static FriendlyByteBuf encodeTree(CommandNode<CommandSourceStack> root) {
        ClientboundCommandsPacket.NodeInspector<CommandSourceStack> inspector =
                new ClientboundCommandsPacket.NodeInspector<>() {
                    @Override
                    public Identifier suggestionId(ArgumentCommandNode<CommandSourceStack, ?> node) {
                        return node.getCustomSuggestions() == null
                                ? null
                                : SuggestionProviders.getName(node.getCustomSuggestions());
                    }

                    @Override
                    public boolean isExecutable(CommandNode<CommandSourceStack> node) {
                        return node.getCommand() != null;
                    }

                    @Override
                    public boolean isRestricted(CommandNode<CommandSourceStack> node) {
                        return !node.getRequirement().test(FULL_PERMISSION_SOURCE);
                    }
                };
        FriendlyByteBuf buffer = new FriendlyByteBuf(Unpooled.buffer());
        ClientboundCommandsPacket.STREAM_CODEC.encode(buffer, new ClientboundCommandsPacket((RootCommandNode) root, (ClientboundCommandsPacket.NodeInspector) inspector));
        return buffer;
    }

    /**
     * 每个参数节点的网络编码，键与 probes/command_tree.cpp 一致（从根到节点的路径，叶子参数带
     * {@code :类型名} 后缀）。
     *
     * <p>子节点一律按名字排序遍历，两侧用同一套遍历规则，因此同一段路径的键可直接对齐。值为
     * {@code {typeId, payload, suggestions}}：typeId 是 COMMAND_ARGUMENT_TYPE 注册表里的数值 id，
     * payload 是序列化后的十六进制字节（不含名字），suggestions 是建议提供器 Identifier。
     */
    private static JsonObject describeArgumentWire(CommandNode<CommandSourceStack> root) {
        JsonObject result = new JsonObject();
        walkArguments(root, "", result);
        return result;
    }

    /**
     * 递归遍历命令树，把每个参数节点的网络编码记录到 {@code out}，键为从根到该节点的路径
     * （如 {@code " team add <team>:word"}），与 probes/command_tree.cpp 完全一致。
     */
    private static void walkArguments(
            CommandNode<CommandSourceStack> node, String path, JsonObject out) {
        List<CommandNode<CommandSourceStack>> children = new ArrayList<>(node.getChildren());
        // 子节点按名字排序后再遍历：Brigadier 的 children 是插入序（LinkedHashMap），
        // Cubium 侧按名字排好序遍历，统一顺序才能让同一段路径的键对齐。
        children.sort(Comparator.comparing(CommandNode::getName));
        for (CommandNode<CommandSourceStack> child : children) {
            String childPath = path + " " + child.getName();
            if (child instanceof ArgumentCommandNode<CommandSourceStack, ?> argument) {
                // 带子节点的参数（filtered 的 filter、loot 的 loot_table）在路径里不带类型后缀:
                // 与 probes/command_tree.cpp 的规则一致——它们的子参数续在父路径之后。
                if (child.getChildren().isEmpty()) {
                    childPath += ":" + typeName(argument.getType());
                }
                out.add(childPath, describeArgument(argument));
            }
            walkArguments(child, childPath, out);
        }
    }

    /**
     * 每个节点的网络属性，键为从根到该节点的路径（参数节点不带 {@code :类型名} 后缀），
     * 与 probes/command_tree.cpp 的 {@code components} 一致。
     *
     * <p>逐节点的 argument payload 只能比出「参数类型编码」，比不出节点语义：同一个通用类型
     * （string、entity）在不同命令里承担不同角色时，可执行性、重定向方式、建议提供器都可能不同。
     * 这里把 ClientboundCommandsPacket 里除名字与参数类型之外的字段全部摊平出来比对。
     */
    private static JsonObject describeComponents(CommandNode<CommandSourceStack> root) {
        JsonObject result = new JsonObject();
        Map<CommandNode<CommandSourceStack>, String> paths = new IdentityHashMap<>();
        collectPaths(root, "", paths);
        walkComponents(root, "", paths, result);
        return result;
    }

    private static void canonicaliseChildren(CommandNode<CommandSourceStack> node, Set<CommandNode<CommandSourceStack>> visited) {
        if (!visited.add(node)) {
            return;
        }
        List<CommandNode<CommandSourceStack>> children = new ArrayList<>(node.getChildren());
        children.sort(Comparator.<CommandNode<CommandSourceStack>, Boolean>comparing(child -> child instanceof ArgumentCommandNode)
                .thenComparing(CommandNode::getName));
        node.getChildren().clear();
        for (CommandNode<CommandSourceStack> child : children) {
            node.addChild(child);
            canonicaliseChildren(child, visited);
        }
        if (node.getRedirect() != null) {
            canonicaliseChildren(node.getRedirect(), visited);
        }
    }

    private static void collectPaths(CommandNode<CommandSourceStack> node, String path,
            Map<CommandNode<CommandSourceStack>, String> paths) {
        if (paths.putIfAbsent(node, path) != null) {
            return;
        }
        for (CommandNode<CommandSourceStack> child : node.getChildren()) {
            collectPaths(child, path + " " + child.getName(), paths);
        }
    }

    private static void walkComponents(
            CommandNode<CommandSourceStack> node, String path,
            Map<CommandNode<CommandSourceStack>, String> paths, JsonObject out) {
        List<CommandNode<CommandSourceStack>> children = new ArrayList<>(node.getChildren());
        children.sort(Comparator.comparing(CommandNode::getName));
        for (CommandNode<CommandSourceStack> child : children) {
            String childPath = path + " " + child.getName();
            JsonObject component = new JsonObject();
            component.addProperty("type", typeKind(child));
            component.addProperty("executable", child.getCommand() != null);
            // 与 encodeTree 里的 NodeInspector.isRestricted 同一口径：以满权限的控制台源判定 requires。
            component.addProperty("restricted", !child.getRequirement().test(FULL_PERMISSION_SOURCE));
            component.addProperty("redirectModifier", child.getRedirectModifier() == null ? "none" : child.isFork() ? "fork" : "single");
            component.add("redirect", child.getRedirect() == null ? com.google.gson.JsonNull.INSTANCE : GSON.toJsonTree(paths.get(child.getRedirect())));
            component.addProperty("suggestions", suggestionId(child));
            out.add(childPath, component);
            walkComponents(child, childPath, paths, out);
        }
    }

    /** flags 的 bits 0-1：root / literal / argument，与 Cubium 的 NodeType 同名。 */
    private static String typeKind(CommandNode<CommandSourceStack> node) {
        if (node instanceof RootCommandNode) {
            return "root";
        }
        if (node instanceof ArgumentCommandNode) {
            return "argument";
        }
        return "literal";
    }

    /** 建议提供器 Identifier；未注册的 provider 与 ClientboundCommandsPacket 一样回退成 ask_server。 */
    private static String suggestionId(CommandNode<CommandSourceStack> node) {
        if (!(node instanceof ArgumentCommandNode<CommandSourceStack, ?> argument)) {
            return "";
        }
        SuggestionProvider<CommandSourceStack> provider = argument.getCustomSuggestions();
        return provider == null ? "" : SuggestionProviders.getName(provider).toString();
    }

    /** 单个参数节点的网络编码：{typeId, payload, suggestions}。 */
    private static JsonObject describeArgument(ArgumentCommandNode<CommandSourceStack, ?> argument) {
        JsonObject entry = new JsonObject();
        ArgumentType<?> type = argument.getType();
        ArgumentTypeInfo<?, ?> info = ArgumentTypeInfos.byClass(type);
        entry.addProperty("typeId", networkId(info));

        // 原版序列化：数值 id 之后的 properties 部分，单独取出来比字节。
        FriendlyByteBuf buffer = new FriendlyByteBuf(Unpooled.buffer());
        serializeInfo(info, type, buffer);
        entry.addProperty("payload", hex(buffer));
        entry.addProperty("suggestions", suggestionId(argument));
        buffer.release();
        return entry;
    }


    /**
     * 把参数类型编码成网络上的 properties 载荷（数值 id 之后的部分）。
     *
     * <p>原版先由 {@code ArgumentTypeInfo} 生成该参数类型的 Template，再把 Template 写进缓冲，
     * 客户端按同一张表反序列化。这里的泛型参数在编译期无法对齐，故走反射调用同一对方法，
     * 语义与原版一致；无法往返的类型留空载荷，由 typeId 差异暴露。
     */
    @SuppressWarnings("unchecked")
    private static void serializeInfo(
            ArgumentTypeInfo<?, ?> info, ArgumentType<?> type, FriendlyByteBuf buffer) {
        ArgumentTypeInfo<ArgumentType<?>, ArgumentTypeInfo.Template<ArgumentType<?>>> typed =
                (ArgumentTypeInfo<ArgumentType<?>, ArgumentTypeInfo.Template<ArgumentType<?>>>) info;
        ArgumentTypeInfo.Template<ArgumentType<?>> template = typed.unpack(type);
        typed.serializeToNetwork(template, buffer);
    }

    /**
     * ArgumentTypeInfo → COMMAND_ARGUMENT_TYPE 注册表里的数值 id。
     *
     * <p>原版就用 {@code BuiltInRegistries.COMMAND_ARGUMENT_TYPE.getId(info)} 写这个编号
     * （ClientboundCommandsPacket:226），直接沿用同一接口，避免自己按迭代顺序推算出错。
     */
    private static int networkId(ArgumentTypeInfo<?, ?> info) {
        return BuiltInRegistries.COMMAND_ARGUMENT_TYPE.getId(info);
    }

    private static String hex(FriendlyByteBuf buffer) {
        StringBuilder text = new StringBuilder();
        // 只取已写入的部分（readerIndex..writerIndex），不碰未用的容量区。
        int start = buffer.readerIndex();
        int end = buffer.writerIndex();
        for (int i = start; i < end; i++) {
            text.append(String.format("%02x", buffer.getByte(i)));
        }
        return text.toString();
    }

    private static String describeTree(CommandNode<CommandSourceStack> root) {
        List<String> parts = new ArrayList<>();
        for (CommandNode<CommandSourceStack> child : root.getChildren()) {
            StringBuilder text = new StringBuilder(signature(child));
            List<String> grandchildren = childSignatures(child);
            if (!grandchildren.isEmpty()) {
                text.append('(').append(String.join(",", grandchildren)).append(')');
            }
            parts.add(text.toString());
        }
        parts.sort(String::compareTo);
        StringBuilder line = new StringBuilder(root.getName());
        for (String part : parts) {
            line.append(' ').append(part);
        }
        return line.toString();
    }

    private static List<String> childSignatures(CommandNode<CommandSourceStack> node) {
        List<String> result = new ArrayList<>();
        for (CommandNode<CommandSourceStack> child : node.getChildren()) {
            result.add(signature(child));
        }
        result.sort(String::compareTo);
        return result;
    }

    /**
     * 字面量输出名字；参数输出 `名字:类型名`。
     *
     * <p>类型名取原版 {@code COMMAND_ARGUMENT_TYPE} 注册表里的 Identifier 路径
     * （{@code minecraft:entity}、{@code brigadier:integer}……），这是原版协议真正使用的
     * 词汇；Cubium 侧的 {@code ArgumentType::getTypeName()} 也按同一套命名。
     */
    private static String signature(CommandNode<CommandSourceStack> node) {
        if (node instanceof ArgumentCommandNode<CommandSourceStack, ?> argument) {
            return node.getName() + ":" + typeName(argument.getType());
        }
        return node.getName();
    }

    /**
     * 参数类型 → 注册表里的类型名（如 {@code entity}、{@code integer}）。
     *
     * <p>先用 {@code ArgumentTypeInfos.byClass} 按实现类精确定位参数类型信息，再反查
     * 注册表键；注册表里 brigadier 的四种基础类型是 {@code brigadier:integer} 等，
     * 取路径后与 Cubium 的类型名同形（integer/float/double/bool/string）。
     */
    private static String typeName(ArgumentType<?> type) {
        ArgumentTypeInfo<?, ?> info = ArgumentTypeInfos.byClass(type);
        for (var entry : BuiltInRegistries.COMMAND_ARGUMENT_TYPE.entrySet()) {
            if (entry.getValue() == info) {
                return entry.getKey().identifier().getPath();
            }
        }
        return "unknown";
    }
}
