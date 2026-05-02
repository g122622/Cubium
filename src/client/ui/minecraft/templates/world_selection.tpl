<screen id="worldSelection" title="Select World">
    <text id="title" text="Select World" pos="200,20" size="200,30" align="center"/>

    <list id="worldList" pos="20,60" size="360,280" bind:items="worlds"/>

    <text id="emptyText" text="No worlds found. Click 'Create New World' to start."
          pos="20,150" size="360,40" bind:visible="worlds.empty" align="center"/>

    <container id="buttonBar" pos="50,350" size="300,30">
        <button id="btn_play" text="Play" pos="0,0" size="80,20" on:click="onPlay" bind:active="worlds.selected"/>
        <button id="btn_create" text="Create New World" pos="90,0" size="120,20" on:click="onCreateWorld"/>
        <button id="btn_delete" text="Delete" pos="220,0" size="80,20" on:click="onDelete" bind:active="worlds.selected"/>
    </container>

    <button id="btn_back" text="Back" pos="200,390" size="80,20" on:click="onBack"/>
</screen>
