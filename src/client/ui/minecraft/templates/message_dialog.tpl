<screen id="message" title="Message" modal="true" background-color="#C0000000">
    <text id="messageTitle" bind:text="message.title" pos="200,60" size="200,30" align="center"/>
    <text id="messageBody" bind:text="message.message" pos="100,100" size="400,40" align="center" wrap="true"/>

    <container id="buttonContainer" pos="190,160" size="220,30">
        <button id="btn_ok" bind:text="message.okText" pos="0,0" size="120,20" on:click="onOk"/>
    </container>
</screen>
