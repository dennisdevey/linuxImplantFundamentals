#!/usr/bin/python3
import argparse, subprocess
def scriptor():
    parser = argparse.ArgumentParser(description='Process some command line inputs')
    parser.add_argument('--name', type=str)
    args = parser.parse_args()
    print('Hello,', args.name)

def main():
   scriptor()
   subprocess.call(['gcc','-o','sniffex','sniffex.c','-lpcap'])
   subprocess.call(['./sniffex.c'])

   return

if __name__ == '__main__':
   main()
