# Activation and Connecting to Shells

## Bind Shells

You manually activated the implant and connected to that bind shell with netcat, but we need to do things automatically, and at scale. 

You can write your own shell multihandler, or you can use the existing Meterpreter shell handler. I highly recommend doing that. 

Write documentation for the operators on how to install and setup the Meterpreter multi/handler, and how to configure RHOST and LPORT based on the build logs to connect to bind shells. 

Hints: [1](https://www.whitelist1.com/2016/06/metasploit-windows-7-bind-tcp-shell.html), [2](https://www.puckiestyle.nl/msfvenom/)

## Reverse Shells

Similar to bind shells, those reverse shells are going to be calling back to you on execution! 

Write documentation for the operators on how to install and setup the Meterpreter multi/handler, and how to configure LHOST and LPORT based on the build logs to catch reverse shells. 

Hint: [1](https://www.offensive-security.com/metasploit-unleashed/binary-payloads/)


In a more perfect world we could use the Metasploit RPC library to do this, but we aren't going to make it that easy for the operators. I did it and it was a bit of work.
