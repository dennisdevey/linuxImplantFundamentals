# Filesystem Anomaly Finders
This part is straightforward but important. It is orders of magnitude easier for an antivirus system to detect a file that is written to disc than an implant that runs exclusively in memory. This means that if you don't want to be detected in the short term, you run "fileless". With that said, if your goal is long term persistence, at some point there will be a reboot or a power off and you will lose access. This trade off must be something you are aware of. 

One of the most straightforward things that a sysadmin or sandbox can do is look for newly changed or added files after execution of a program. If what you leave behind is your implant in its entirety, you will get found eventually by someone paying attention. Various things can be done like making the implant save itself in an encrypted format, in multiple pieces that run eachother in succession, and any of the standard anti-analysis work.

While you might hear talk that the era of fileless malware is over, everything should run in memory, etc, basically no one is doing it. As always, there are tradeoffs, and slightly higher chance of detectability rarely outweighs the benefit of stone cold persistence.The only true existing fileless persistence out there is from APTs who do firmware rewrites, so let's not worry about implementing that. Let's just do regular file-based persistence for now. 


