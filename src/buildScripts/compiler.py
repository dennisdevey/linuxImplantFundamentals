import datetime
import argparse
import binascii
import os

import subprocess

parser = argparse.ArgumentParser("python compiler.py", usage='%(prog)s [-o fileName] [-p listener] [-intfc eth0] [-act SECRET_PORTS] [-key 200,300,400] [-atkSc] [-a x64] [-p linux] [-ip 192.160.1.100] [-revip 192.168.2.132] [-revport 1337] [-strip]')

parser.add_argument("-d", "--debug", action="store_true",
                    help="compile with debugging")
parser.add_argument("-ip", "--ipAddress",type=str,
                    help="target address", metavar='', default="unknown")
parser.add_argument("-o", "--outputName",type=str, metavar='',
                    help="output filename", default="implant")

args = parser.parse_args()

# with open('log.csv', mode='a+') as log_file:
    
#     log_writer = csv.writer(log_file, delimiter='\t', quotechar='"', quoting=csv.QUOTE_MINIMAL)
#     if (file_exists != 1):
#         fieldnamesList = ["datetime", "ipAddress", "domain", "architecture", "platform", "os", "versionNumber", "payload", "activate", "interface", "key", "size", "dateDelay", "timeDelay", "trigger", "persistence", "bang", "downloadURL", "loadShellcode", "reverseShell", "reverseIP", "reversePort", "Notes", "debug", "outputName", "strip", "static"]
#         log_writer.writerow(fieldnamesList)

#     log_writer.writerow([str(datetime.datetime.now()), str(args.ipAddress), str(args.domain), str(args.architecture), str(args.platform), str(args.os), str(args.versionNumber), str(args.payload), str(args.activate), str(args.interface), str(args.key), str(args.size), str(args.dateDelay), str(args.timeDelay), str(args.trigger), str(args.persistence), str(args.bang), str(args.downloadURL), str(args.loadShellcode), str(args.reverseShell), str(args.reverseIP), str(args.reversePort), str(args.notes), str(args.debug), str(args.outputName), str(args.strip), str(args.static)])

# print(str(args.ipAddress))

cmdString = ["gcc", "testing.c", "-o", args.outputName]

if args.debug:
    cmdString.insert(1, "-D DEBUG")

if args.ipAddress != "unknown":
    cmdString.insert(1, "-D IPADDR=\"" + (args.ipAddress)+ "\"")


subprocess.run(cmdString)

subprocess.run("./implant")




