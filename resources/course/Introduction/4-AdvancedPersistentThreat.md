# Advanced Persistent Threat
Despite what the naysayers are naysaying, we can become an APT, but it's time to set some ground rules. Hopper's Roppers hates gatekeeping, but we need to hit some pretty high bars to get a name like Hopping Goat or Ropping Frog. 

Sorry I don't make the rules, FireEye does.  

# Advanced 

In order to be deemed 'advanced', a strong knowledge of programming is required. We are going to have to develop our own toolset to even come close to hitting the minimum requirements to get a named group for ourselves. Otherwise the good threat intelligence folks won't be able to tell our malware apart from other threat actors, which would really hurt our brand. 

To become officially not a script kiddie in the eyes of the HackForums crew, we must develop our own implants that are totally FUD (that's what high schoolers call 0/56 in VirusTotal) in C or C++. That's what they told me back in middle school and it still holds true today. Some things just are timeless.

A working knowledge of C is assumed. If you don't feel comfortable with C, you're not ready to be an APT yet. 

Come back when you can write C. I recommend <https://github.com/h0mbre/Learning-C/>

# Persistent 
Our second requirement to become among those ranks is to be 'persistent'. To take it a bit further than establishing persistence, persistent means the attacker maintains a longer dwell time on the network to achieve a specific objective, rather than an opportunistic attack once initial access has been made. 

***Note: this means a high dwell time on the network, not that we are very persistent in our attempts to break in.***

Persistence to survive reboots is fairly straightforward, but to be APT-level persistent (which often involves no persistence and memory only implants) we want dwell time, which means our implants have to be quiet and our C2 needs to be durable.
 
"Low and slow" and long haul C2 is good and all, but you know what is better? Not having to ever, ever, ever worry about your domains getting sinkholed and losing your implants. In the past, the primary way malware authors tried to do long term retention was with a domain generation algorithm that allowed implants to know which new domains to connect back to and when. I want my implants to last for 10 years, and I don't want some pesky internet researchers reversing my DGA, sinkholing my botnet, and writing a blog post about it.

*(To any threat intel weenies reading this,  I'd be sooo mad if you called us something like the Hopping Roppers)*

# Threat 

Our third requirement to become a threat is to accomplish some sort of strategic goal. We're not trying to loot a LAN here like some hungry North Koreans burning firewood to heat their bunker as they connect into a South Korean McDonald's wifi hotspot, we are trying to create strategic effects in cyberspace, like we read about in JP 3-12. (We read JP 3-12 right?)

So what kind of strategic effect? I think an interesting one here is the idea of "holding at risk". I go into this in some detail in my poorly formatted rant on cyber policy (<https://github.com/deveyNull/TenetsOfCyber>) but generally, if you are trying to coerce behavior, you need to be able to convince the adversary that you have the ability to hold them at risk. Otherwise, If you are just trying to have an arbitrary effect at any scheduled point in time, you need to have the access to do so. That will require 0day or an existing implant. 0days cost a lot, and the longer the implant sits, the better chance of discovery, the sinkholing of the c2 domains, and the reverse engineering and inevitable blog post. 

***Q: So how do we get around this???***

A: By creating an implant that does not have any hardcoded C2 domain, but instead, waits for an activation signal that tells it which domain to connect back to for a payload. This means we never have to worry about the head getting cut off the snake of our long term implants. To do this, I choose portknocking as an initial activation method, though there are others available. This method is underdocumented.
 
Alright, that's enough acronyms for now. Let's start doing things.