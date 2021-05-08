# Mutex
Learn about mutexes and add one to your implant so that you have the ability to set a flag at compile time that ensures you are only able to have one process per implant running at a time. This is less important because we aren't worming, but can be good to avoid eating memory if the persistence mechanism we are using activates itself too frequently. 

This option should be default OFF. Ensure that the mutex is unique to each generated implant and will not allow a reverse engineer to identify other implants with different mutexes.

Read this from SANS to get an idea of what you need to do, and what to watch out for: <https://www.sans.org/blog/looking-at-mutex-objects-for-malware-discovery-indicators-of-compromise/>

Submit your  commit.