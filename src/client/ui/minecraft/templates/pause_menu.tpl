<screen id="pause" title="Game Paused" modal="true" background-color="#B4000000">
    <text id="title" text="Game Paused" align="center" size="200,30"/>

    <container id="buttonContainer" layout="flex-column" gap="10" size="200,100" align-items="center">
        <button id="resume" text="Back to Game" size="200,20" on:click="onResume"/>
        <button id="options" text="Options..." size="200,20" on:click="onOptions"/>
        <button id="quit" text="Save and Quit to Title" size="200,20" on:click="onSaveAndQuit"/>
    </container>
</screen>
