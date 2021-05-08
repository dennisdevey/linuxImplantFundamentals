# Detection at the Macro Level
Long story short, the more targeted you are, the less boxes you will be on, and the less likely you will be picked up by an automated antivirus system or found by some bored sysadmin, run in a sandbox, earn a high detection score, and get RE'd by a Kaspersky employee. Once you are RE'd, there will be an initial blogpost and signatures will be distributed to all Kaspersky antivirus engines, which will run those signatures and look for your implant on all of their protected computers. More of your implants will be found, and then  one of them will be uploaded to VirusTotal by some random company's security personnel. From there, every antivirus provider in the world will have your signatures RE'd in a week and all of your accesses on protected computers will be burned and your C2 domains identifed. Then everyone who got a detection will go and do incident response and will find you on the computers that weren't running antivirus. All of your C2 will be burned, and the threat intel folk will write all about you and FireEye analysts will critique the way you implemented your encryption algorithms. Eventually, Brian Krebs will post a picture of you from middle school, a receipt from your favorite ice cream stand, and will send you a message on Facebook asking if you want to comment on the story. 

Your goal is to be, in generally this order: 

1. Targeted. 
2. Randomized so that signatures and heuristics aren't useful for burning all of your accesses... immediately.
3. Not run malicious code on an automated sandbox so detection scores are low and you don't get RE'd by a human being
4. Not have static C2

Follow these rules and you might stick around for a bit.

