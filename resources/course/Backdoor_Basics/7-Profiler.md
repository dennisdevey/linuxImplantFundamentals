# Profiler:

Write a system profiler that uses terminal commands to create a C struct of the following form:


* Architecture: uname -a
* Kernel Name:
* Kernel Release:
* Kernel Version:
* Machine Hardware: 
* glibc: ldd --version


* IPv4Addr: ipaddr
* IPV6Addr:
* IPInterfaceName:
* EthAddr:
* GatewayRouterAddr:


* name: id
* UID: 
* GID: 
* Keyboard: localectl status


# Execution Guardrail by Profiler: 

Add the ability to specify an arbitrary number of execution guardrails from items in profiler buffer with AND/OR/NOT logic. 
As demo, implement function that only returns True if a host computer has an IP address in a certain range, with a specific a specific architecture, not a Cyrillic keyboard, and the kernel build is earlier than a specific date, with all information provided at compile time.  

Submit your code when you have this complete. 
