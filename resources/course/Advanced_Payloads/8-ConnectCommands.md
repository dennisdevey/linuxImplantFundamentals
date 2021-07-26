Modify your existing bind and reverse shell code so that on connect, the client and server are able to exchange arbitrary commands and data. Place the checks for these commands in a loop. 

"SLEEP": The implant will sleep N seconds. If a reverse shell, the client will update it's internal default sleep state to N seconds and next time that it disconnects and attempts to reconnect, it will operate on that new sleep schedule.
"UNINSTALL": The implant will uninstall itself using the previously developed Uninstall functon:
"SHELL": The implant will dup2 STDIO and execute /bin/sh like a standard reverse/bind shell. Upon exit from shell, the implant will drop back into the command loop. 
"PROFILER": The implant will run the profiler function and send back as much data as collected on the target
"EXIT": The implant will kill itself, but will not uninstall

