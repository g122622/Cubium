<screen id="loom" title="Loom" modal="true">
    <!-- 半透明背景遮罩 -->
    <container id="overlay" pos="0,0" size="100%,100%" background-color="#B4000000"/>

    <!-- 主面板 176x166 -->
    <container id="panel" pos="0,0" size="176,166" background-color="#FFC6C6C6">
        <!-- 标题 -->
        <text id="title" text="Loom" pos="8,6" size="80,12" align="left" color="#FF404040"/>

        <!-- 旗帜槽 -->
        <slot id="bannerSlot" pos="31,22" size="18,18" on:click="onSlotClick"/>

        <!-- 染料槽 -->
        <slot id="dyeSlot" pos="31,53" size="18,18" on:click="onSlotClick"/>

        <!-- 图案物品槽 -->
        <slot id="patternSlot" pos="7,53" size="18,18" on:click="onSlotClick"/>

        <!-- 图案选择区域（可滚动） -->
        <scrollable id="patternList" pos="14,14" size="50,72" on:scroll="onPatternScroll">
        </scrollable>

        <!-- 输出槽 -->
        <slot id="resultSlot" pos="145,33" size="18,18" on:click="onSlotClick"/>

        <!-- 玩家背包 (3x9) -->
        <grid id="playerInventory" cols="9" rows="3" pos="7,84" size="162,54">
            <slot id="invSlot" on:click="onSlotClick"/>
        </grid>

        <!-- 快捷栏 (1x9) -->
        <grid id="hotbar" cols="9" rows="1" pos="7,142" size="162,18">
            <slot id="hotbarSlot" on:click="onSlotClick"/>
        </grid>
    </container>
</screen>
