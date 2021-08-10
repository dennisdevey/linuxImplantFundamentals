import subprocess
import argparse

parser = argparse.ArgumentParser("python valgrinder.py", usage='%(prog)s [-fn fileName] [-lc leakcheck] [-v verbose]')

parser.add_argument("-fn", "--filename", type=str, metavar='',
        help="specify filename to run thru valgrind", default="implant")
parser.add_argument("-lc", "--leakcheck", action="store_false",
        help="when -lc option is given, turn leakchecking off", default=True)
parser.add_argument("-v", "--verbose", action="store_true",
        help="turn verbosity on", default=False)

args = parser.parse_args()

isLeakCheckOn = "yes"
if args.leakcheck == False:
    isLeakCheckOn = "no"

cmdList = ["valgrind", "--leak-check=" + isLeakCheckOn, "./" + args.filename]

if args.verbose:
    cmdList.insert(2, "-v")

subprocess.run(cmdList)
