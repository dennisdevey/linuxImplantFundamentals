# Dynamic Compilation
Dynamic compilation is the default for a variety of reasons. 

We've already discussed dynamic linking as being fairly difficult to pull off because of the likelihood that the target environment is different. However, there are a variety of ways we can increase the chance that our stuff hits. 

The most important thing we can do is to keep to C standard libraries and minimize our linking complexity. The less complexity, the better. 

Another way we can do this is by using old versions of libraries. By using a 15 year old (ancient) version of a library, the chances of the target environment being compatible significantly increase.

Though it is only sometimes possible, occasionally we can have intelligence on a system, or have a  "validator" implant that will run a system profiler on a target that can get us the architecture, OS, and libraries available. If the validator passes and this is an appropriate target, then we can compile on our system for that target and be very confident it will execute appropriately. 

A real APT would have a ton of VMs with different OSs and architectures to guarantee success... we're operating on a budget here, so we just have to hope best practices get us where we need to go.


Submit answers to the following:

1. How does dynamic compilation effect the portability of code? How about the size/detectability? 
2. What are the pros and cons of dynamic, especially as an APT?
3. Discuss how you would implement a validator/system profiler and what commands you would run on a target box to determine all the information you would need to compile an implant for it.