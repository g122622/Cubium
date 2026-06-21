<screen id="confirm" title="Confirm" modal="true" background-color="#C0000000">
    <text id="confirmTitle" bind:text="confirm.title" pos="200,60" size="200,30" align="center"/>
    <text id="confirmMessage" bind:text="confirm.message" pos="100,100" size="400,40" align="center" wrap="true"/>

    <container id="buttonContainer" pos="100,160" size="400,30">
        <button id="btn_confirm" bind:text="confirm.yesText" pos="0,0" size="120,20" on:click="onConfirm"/>
        <button id="btn_cancel" bind:text="confirm.noText" pos="280,0" size="120,20" on:click="onCancel"/>
    </container>
</screen>
