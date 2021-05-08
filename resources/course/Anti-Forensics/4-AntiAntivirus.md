# AntiAntivirus
The cat and mouse game of not getting detected by AV is ongoing. As a general rule, you are probably just better off not running your fancy implant on a box that is guarded by AV because at anytime their signatures can be updated and your implant will be burned. 

Anti-virus can run checks of all files on the system, of all processes in memory, or just look for extra things that seem wrong and investigate from there. They're rootkits, and if you are living in userland, it's a good time trying to work around them.

In order to check for AV, either to run or stop execution, you do it by looking for active processes, installation remnants, and hooks. 

This is Linux though, so there is basically no AV! Let's ignore it for now.

Submit a sentence on why AV is hard to beat for this section until I find something better.