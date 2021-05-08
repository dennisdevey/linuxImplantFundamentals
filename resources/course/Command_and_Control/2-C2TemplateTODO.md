# C2 Template TODO
We already have Metasploit up as a listening post, which kinda works for now. But we want to have our own,  because that is just a lot more fun.

Of course, there are probably 100+ custom C2 frameworks out there, but building our own is more fun. <https://www.thec2matrix.com/matrix> If you're feeling lazy, just fork someone else's and use it, which is totally valid, or make your own. 

This resource is pretty great to describe what you would do to make your own. <https://shogunlab.gitbook.io/building-c2-implants-in-cpp-a-primer/chapter-1-designing-a-c2-infrastructure>

For this section, I am providing the basic template for a C program that checks a URL for data, parses it as a command, executes it, then sends the results back to the URL.

Additionally, I provide a simple python script that uses a web server to serve arbitrary data based off of the contents of a text file. When an implant connects to the webserver, a directory is created for it that contains its configuration and log files. By modifying the config file you can change what command is run by the implant. When the implant response comes through, a second log file is created for the stdout of the command. 

