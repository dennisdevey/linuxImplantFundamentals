# Sandbox Theory and Behavioral Detection
Sandbox execution is the primary method of getting the information required for heuristic and other advanced "next-gen" detections. 

The short answer to this is that you don't want your implant executing malicious code if placed in a sandbox. There are a variety of ways to do this, with many varied approaches. Some of the more interesting are detecting virtualization, looking for process monitoring, checking file system size, seeing if the system is used by a real person. Read this for more information: <https://attack.mitre.org/techniques/T1497/> If these checks fail, the implant will run a benign series of actions, and the malicious behavior that we don't want analyzed will not occur. However, these checks themselves can have signatures/heuristics written for them, as any process that hides its true behavior in a VM is likely malicious. The cat and mouse game for that continues all the way down as previously described.

A secondary and surprisingly effective technique is to have your implant sleep or repeat an action repeatedly to cause a few hour delay in execution of malicious functions. Due to general cost of cloud services, few companies run multi-day sandbox executions, which means you can wait them out. 

A third way is to have activation methods which wait for a specific thing to occur like a portknock, domain to resolve, or date to occur which will cause a sleep long enough to sleep past any sandbox. However, many sandboxes will automatically resolve any requests in an attempt to catch C2, so domain resolution as an activation method is a bit unreliable.

There is much more on this topic to be written, but the general answer is that you don't want your implant executing in a sandbox or you will have a bad time. 

Finally, the correct way to do sandbox avoidance is to only execute malicious behavior on the desired network. There are very few reasons you should ever be writing an implant that will work on more than one network. Any responsible author should have safeguards built in to ensure that the implant can only be executed on a specific network within a set series of dates and times. Whether you are an APT or a Red Team, if you are a professional you should have this capability and use it basically at all times. We will implement this in our program as one of our first items. 

