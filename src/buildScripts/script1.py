#!/usr/bin/python3
import argparse, subprocess, logging
def scriptor():
    parser = argparse.ArgumentParser(description='Process some command line inputs')
    parser.add_argument('--name', type=str)
    args = parser.parse_args()
    print('Hello,', args.name)

def main():
   scriptor()
   subprocess.call(['gcc','-o','sniffex','sniffex.c','-lpcap'])
   logging.basicConfig(format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
   logging.warning('is what time the commands gcc -o sniffex sniffex.c -lpcap and sudo ./sniffex were run succesfully.')
   subprocess.call(['sudo','./sniffex','lo'])
   return

if __name__ == '__main__':
   main()
