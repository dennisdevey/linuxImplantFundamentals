# Activation and Connecting to Shells
You manually activated the implant and connected to that bind shell with netcat, but we need to do things automatically, and at scale. 

Using the Python script that generates payloads you wrote earlier: 

1. Modify it to take in the IP address and the knock codes required to activate the implant. 
2. Take the logs that are outputted by the first script and parse them with a second script that identifies existing payloads and conducts the portknock to activate them. 
3. Once you have that working, add some code to your activation script that will connect automatically and hand you the shell. Writing a handler for multiple shells is a bit of a pain in the ass, I did it with a Python libary for Metasploit's Remote Procedure Call API, but that was overkill. I recommend you make some modifications to this library instead: <https://github.com/buckyroberts/Turtle>




Submit your commit, ensure that you have moved your handler into the reverseShellHandler
directory.
