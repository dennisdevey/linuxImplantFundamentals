# ptrace libcdlopen Injection
Notably for the last ptrace injection, you just kind of ganked the process when you took over RIP and wandered off into the night. Here we are going to do an advanced type of inject where we return control of execution back into the original process while our code has daemonized off into the darkness to do its thing. 

This is very complicated, but I'll get good tutorials built out eventually. For now, read this: <https://blog.xpnsec.com/linux-process-injection-aka-injecting-into-sshd-for-fun/> 


Try to understand what is going on here and get it working. <https://github.com/gaffe23/linux-inject>.
Save a commented copy of the code in your commentedCode directory.

Then implement their dlopen inject to your existing tool, alongside the shitty RIP inject. This is expected to take a long time, but will add a ton of functionality to your tool.

Submit your commit.