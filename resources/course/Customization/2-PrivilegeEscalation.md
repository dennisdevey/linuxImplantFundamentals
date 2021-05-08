# Privilege Escalation
Remember that your implant needs to execute as root in order to sniff traffic for the portknocks? If your backdoor is initially executed by a non-root user and you don't have any built in methods of privilege escalation the implant becomes useless.

I won't tell you what to do here, but there are plenty of resources out there, here are some ideas: 

* <https://attack.mitre.org/tactics/TA0004/>
* [payloadsallthethings/linuxprivsec](https://github.com/swisskyrepo/PayloadsAllTheThings/blob/master/Methodology%20and%20Resources/Linux%20-%20Privilege%20Escalation.md)

Submit your code that allows your implant to escalate privileges.