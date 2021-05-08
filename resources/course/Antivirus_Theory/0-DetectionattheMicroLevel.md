# Detection at the Micro Level
How do we get detected? This is a complicated question. 

On Linux, there aren't many boxes with active antivirus (or active users for that matter). Most Linux boxen out there (I'll call them Category 1) are happy to sit there and chew CPU cycles doing whatever they are doing, hosting webpages, flying planes, cooking your hotpockets. Then there are servers (Category 2) that are used by Sysadmins every couple of weeks or days. Then there are the odd people who use them as their everyday workstation (Cat 3). Each of these carries a different level of risk of detection. 

When you choose what to put on a box, and where, it is largely based off of a threat modeling perspective. 

There's plenty to say about this, but the general answer is for Cat 1, don't break anything and noone will ever check. Cat 2, this is the highest chance of being caught, but also the highest reward. Cat 3, there is basically no reward unless they are an individual of interest. There's a reason Linux malware isn't a thing, anyone who uses a free operating system because they don't have the money for Windows doesn't have anything worth taking. 

But I digress. Don't break anything, don't eat too many CPU cycles, don't cause weird popups, don't save files into the four directories that users actually use and you won't get caught, for a while. On a long enough time frame, someone will find you, especially if you don't time box your implant.