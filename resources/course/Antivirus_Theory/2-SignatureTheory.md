# Signature Theory
While you've worked with an antivirus before,  there is a ton of complexity hidden beneath the hood that you might not be aware of. 

***NOTE: As an aside, the vast majority of Linux boxes do not have any antivirus on them at all, which is fairly convenient for us as once the implant is on the box, it probably won't ever get scanned. You want to reduce signatures for the initial access, but honestly once you have access, antivirus is not much of a concern on Linux.***

There are a variety of ways that antivirus works but the vast majority of detections come from what are called signatures. These signatures are simple enough; a reverse engineer (or now sometimes, a script) gets a malware sample, works through it and identifies unique characteristics like a common string or series of instructions, writes a signature for it, and then that signature is distributed to detect on all other platforms the antivirus is on. What does that mean for us? We must avoid any reuse of any strings or things that can be used to identify our implant at scale. If a signature can be written for our implant and pushed, we can lose all the implants that we have.

Basically everyone has now added some level of polymorphism to their implants so that a single static signature cannot be applied universally. 

The next level to detect a polymorphic implant is still a signature detection. When "dumb" polymorphism has been implemented (usually garbage randomized characters to replace strings or extra space), the next level of detection works the same as before but wildcards (*) out the randomization in order for signatures to be more generally applicable. 

To make signatures generally not applicable to your implant, you need to randomize nearly every aspect of your implant, from the obvious things like strings down to adding junk functions that change the "shape" of the binary, modified versions of the same function such as different compression or encryption algorithms. This can get very complicated very quickly, but if you follow this guide appropriately you will not have a problem with signature based scanners.

