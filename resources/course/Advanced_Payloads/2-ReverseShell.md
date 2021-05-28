# Reverse Shell
Very similar to the bind shell you did before, implement a reverse shell in C and modify your implant generation, logging, and activation scripts so that you can automatically catch the reverse shells from your implant. 

You can write your own revshell multihandler, or you can modify https://github.com/buckyroberts/Turtle to be controlled via the existing Terminal tool. The only changes you need to make is if a certain shell is not being actively controlled, check the next command "database" and send that data, along with implementing logging. You might also want to add the ability to drop a shell temporarily and come back to it, but who am I to tell you what to do.


Submit your commit.
