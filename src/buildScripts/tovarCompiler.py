import datetime
import argparse
import csv
import subprocess

def parseCommands():
    parser = argparse.ArgumentParser(usage="python3 tovarCompiler.py [options] FILE", description="Script to compile and run FILE,\
        and log the commands/results to a file. For now it uses the default cd00r,\
        args. It opens a reverse shell on port 8008 and key opening ports are 200,80,22,53,3 in order.\
        Also, if running cd00r and most packet listeners, will need to run 'sudo python3  <etc>'")
    parser.add_argument("-c", "--compiler", help="compiler used", default="gcc")
    parser.add_argument("-f", "--flags", help="compiler flags", default=['-lpcap', '-o'], nargs='+') #nargs + requires at least 1 arg. stores a list. Also ends with -o so should be at end of command
    parser.add_argument("-i", "--interface", help="network interface used", default="lo")
    parser.add_argument("-d", "--debug", help="enable debug", action='store_true')
    parser.add_argument("-v", "--valgrind", help="enable valgrind", action='store_true')
    parser.add_argument("-s", "--strip", action="store_true",help="strip the binary")
    parser.add_argument("FILE", help="file to be compiled and ran")
    return parser.parse_args()

def interpretCommands(args):
    # just gcc and g++ support for now
    if(args.compiler != 'gcc' and args.compiler != 'g++'):
        exit('Invalid compiler specified. Exiting.')
    command = [args.compiler, args.FILE]
    if args.debug:
        print('DEBUG STYLE') #DEBUG
        command += '-g'
    # TODO: add interface choice. command += [args.interface]

    # add the flags and the 
    # TODO: allow implant source to be in a different directory and still have the executable output to the current directory
    command += [ fl for fl in args.flags] + [args.out]
    print('InterpretCommands: ', command) #DEBUG
    return command

def logCmd(msg):
# took me about 2 hours to figure out how to only print the header for the first line.
# was working when done at the end but when made into funciton does not work
# TODO: fix this
    try:
        log = open('logCompiler.csv', mode='x')
        log_writer = csv.writer(log, delimiter='\t', quotechar='"', quoting=csv.QUOTE_MINIMAL)
        log_writer.writerow(['|DateTime|', '|Command|', '|Results|'])
        log_writer.writerow([str(datetime.datetime.now()), sys.argv, msg])
        log.close()

    except:
        log = open('logCompiler.csv', mode='a+')
        log_writer = csv.writer(log, delimiter='\t', quotechar='"', quoting=csv.QUOTE_MINIMAL)   
        # log commands
        log_writer.writerow([str(datetime.datetime.now()), sys.argv, msg])
        log.close()

# TODO: valgrind, strip, keep debug in mind

args = parseCommands()
# name of the executable. eg: sniffex.c FILE returns sniffex. Done so that the command is <rest of command> -o sniffex.o
args.out =  args.FILE[:(args.FILE).rindex('.')] + '.o'


# ------------- TESTS ------------

## Compiling
listCompile = interpretCommands(args)
cmdCompile = subprocess.run(listCompile)
if cmdCompile.returncode != 0:
    logCmd('ERROR: Compile: Didn\'t exit with 0.')
    exit('ERROR: Compile: Didn\'t exit with 0\n')
if cmdCompile.stderr:
    logCmd(cmdCompile.stderr)
    exit(cmdCompile.stderr)


## Binary Stripping
if args.strip:
    print('STRIP') #DEBUG
    cmdStrip = subprocess.run(['strip', '--strip-all', args.out ])
    if cmdStrip.returncode != 0:
        logCmd('ERROR: Binary Stripping: Didn\'t exit with 0.')
        exit('ERROR: Binary Stripping: Didn\'t exit with 0\n')
    if cmdStrip.stderr:
        logCmd(cmdStrip.stderr)
        exit(cmdStrip.stderr)


### Reminder: To check for running cd00r
###     sudo lsof | grep SOCK_RAW | grep -P '(?<!\d)\d{4}(?!\d)' -o
###     sudo pkill -f 'cd00r.o'


## Valgrind
cmdRun = None
if args.valgrind:
    print("VALGRIND") # DEBUG
    cmdRun = subprocess.run(['valgrind', '-q', '--log-file=logValgrind.log', '--error-limit=yes', './'+args.out])
    if cmdRun.returncode !=0:
        logCmd('ERROR: Valgrind: Didn\'t exit with 0. Uses child process\'s return code. Likely problem with input executable.')
        exit('ERROR: Valgrind: Didn\'t exit with 0. Uses child process\'s return code. Likely problem with input executable.\n')
    if cmdRun.stderr:
        logCmd(cmdRun.stderr)
        exit(cmdRun.stderr)
## No Valgrind
else:
    print("RUNNING") # DEBUG
    cmdRun = subprocess.run(['./'+args.out])
    if cmdRun.returncode !=0:
        exit('ERROR: Running: Didn\'t exit with 0.\n')
    if cmdRun.stdout:
        logCmd(cmdRun.stdout)
        exit(cmdRun.stdout)
    if cmdRun.stderr:
        logCmd(cmdRun.stderr)
        exit(cmdRun.stderr)


## Kill It
if args.debug:
    cmdKill = subprocess.run(["sudo", 'pkill', "-f", args.out])
    if cmdKill.returncode !=0:
        logCmd('ERROR: Killing: Didn\'t exit with 0.')
        exit('ERROR: Killing: Didn\'t exit with 0.\n')
    if cmdKill.stdout:
        logCmd(cmdKill.stdout)
        exit(cmdKill.stdout)
    if cmdKill.stderr:
        logCmd(cmdKill.stderr)
        exit(cmdKill.stderr)
    # TODO: remove executable .o
else:
    print("\nRemember to sudo pkill -f <file.o> when done!!\n")

exit('SUCCESSFUL')

"""
BONUS (If you activate the implant using "nc -z", does it do something? have it create a file and check if the file has been created)
kill the process
"""
## Listening on Port
### Should be listening on all ports. Cd00r style for now.
