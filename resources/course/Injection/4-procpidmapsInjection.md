# procpidmaps Injection
This stuff is crazy complicated so we won't worry about it for this course. Sorry to disappoint you. 

Basically, it is possible to ROP around inside of /proc/pid/mem and read and write around until you have the gadgets required to inject. But that is too much work, so just read  these. 

https://github.com/ouadev/proc_maps_parser

https://attack.mitre.org/techniques/T1055/009/

http://hick.org/code/skape/papers/needle.txt

https://blog.gdssecurity.com/labs/2017/9/5/linux-based-inter-process-code-injection-without-ptrace2.html

For this assignment, describe what makes proc/pid/maps and mem usable for this type of inject and when you would use it. 