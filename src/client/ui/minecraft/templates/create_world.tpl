<screen id="createWorld" title="Create New World">
    <text id="title" text="Create New World" pos="200,20" size="200,30" align="center"/>

    <container id="formContainer" pos="100,70" size="280,200">
        <text id="nameLabel" text="World Name" pos="0,0" size="80,20"/>
        <textfield id="nameField" pos="90,0" size="180,20" placeholder="My World" max-length="50"/>

        <text id="seedLabel" text="Seed (optional)" pos="0,30" size="80,20"/>
        <textfield id="seedField" pos="90,30" size="180,20" placeholder="Leave empty for random" max-length="64"/>

        <text id="gameModeLabel" text="Game Mode" pos="0,60" size="80,20"/>
        <button id="gameModeBtn" text="Survival" pos="90,60" size="100,20" on:click="onCycleGameMode"/>

        <text id="worldTypeLabel" text="World Type" pos="0,90" size="80,20"/>
        <button id="worldTypeBtn" text="Default" pos="90,90" size="100,20" on:click="onCycleWorldType"/>
    </container>

    <container id="buttonBar" pos="150,300" size="200,30">
        <button id="btn_create" text="Create World" pos="0,0" size="90,20" on:click="onCreate"/>
        <button id="btn_cancel" text="Cancel" pos="110,0" size="80,20" on:click="onCancel"/>
    </container>
</screen>
