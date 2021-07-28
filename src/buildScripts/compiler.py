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
parser.add_argument("-o", "--outputName",type=str, metavar='',
                    help="output filename", default="implant")

args = parser.parse_args()

log_exists = filesys.isfile('log.csv')

with open('log.csv', mode='a+') as log_file:
    
    log_writer = csv.writer(log_file, delimiter='\t', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    if (log_exists != 1):
        fieldnamesList = ["datetime", "ipAddress", "outputName"]
        log_writer.writerow(fieldnamesList)

    log_writer.writerow([str(datetime.datetime.now()), str(args.ipAddress), str(args.outputName)])

print(str(args.ipAddress))


# cmdString = ["gcc", "testing.c", "-o", args.outputName]

# if args.debug:
#     cmdString.insert(1, "-D DEBUG")

# if args.ipAddress != "unknown":
#     cmdString.insert(1, "-D IPADDR=\"" + (args.ipAddress)+ "\"")


# subprocess.run(cmdString)

# subprocess.run("./implant")




