# Intro to sniffex
In the previous section we stole someone else's code, got it working, and then learned what stripped and statically linked meant.

We are not 1337 yet, but at least we have an idea of what we are trying to do. unfortunately, we haven't written really any code yet, and our backdoor is easily detectable. 

So now let's go look for some basic code to reinplement the entire cd00r framework from scratch. Our basic functionality we are looking for is a packet sniffer, written in C. Googling for C packet sniffer will allow us to pretty quickly come across this sample code.

<https://www.tcpdump.org/sniffex.c>

Wow this is convenient... also if you look at the code side by side, clearly cd00r author's just totally jacked this guy's demo code and then added on the bare minimum of functionality..... so that is what we are going to do. 

First, lets get sniffex.c to compile for 64 bit Linux. 

1. What if anything did you have to do to get that to work?

Run the sniffer and throw some packets at it using netcat. 

2. What do you see? Experiment to see how it works. 

Copy  the code that you have written into your cloned repo from the last assignment. Your assignment is to submit a link to your commit. If you have the repo as private I won't be able to view it remember, so make sure to add 'deveynull' to the repo so I can check it.


Device: ens33
Number of packets: 10
Filter expression: ip

Packet number 1:
       From: 192.168.184.128
         To: 192.168.184.2
   Protocol: UDP

Packet number 2:
       From: 192.168.184.2
         To: 192.168.184.128
   Protocol: UDP

Packet number 3:
       From: 192.168.184.128
         To: 192.168.184.2
   Protocol: UDP

Packet number 4:
       From: 192.168.184.2
         To: 192.168.184.128
   Protocol: UDP

Packet number 5:
       From: 192.168.184.128
         To: 192.168.184.2
   Protocol: UDP

Packet number 6:
       From: 192.168.184.2
         To: 192.168.184.128
   Protocol: UDP

Packet number 7:
       From: 192.168.184.128
         To: 192.168.184.2
   Protocol: UDP

Packet number 8:
       From: 192.168.184.2
         To: 192.168.184.128
   Protocol: UDP

Packet number 9:
       From: 192.168.184.128
         To: 142.250.73.228
   Protocol: TCP
   Src port: 43364
   Dst port: 443
   Payload (271 bytes):
00000   17 03 03 01 0a 9d e3 47  7c 81 e8 74 45 40 b8 93    .......G|..tE@..
00016   00 38 23 cb a6 98 e7 f1  9b 4e 68 9a d9 3e 6c 85    .8#......Nh..>l.
00032   c6 8b 42 e1 a7 d7 10 e4  7e 15 ee b9 c1 ff c7 b7    ..B.....~.......
00048   bc ee 9d f0 c3 e8 b8 4a  42 50 24 f1 79 7d ce 55    .......JBP$.y}.U
00064   73 c9 6c 47 fc d7 bd 16  92 92 9b 37 91 b0 9a 15    s.lG.......7....
00080   0c c5 1b 43 41 50 dd 82  36 dc a7 33 b7 56 5a bc    ...CAP..6..3.VZ.
00096   d1 de 55 f2 67 b0 56 ce  1a 4a b7 b8 87 f7 45 97    ..U.g.V..J....E.
00112   01 e1 75 00 a9 e4 8b 73  85 eb 62 34 46 4f 19 e5    ..u....s..b4FO..
00128   34 65 5f 2e 1e 5a 7e f0  bd 0f cd bd 08 b1 25 b3    4e_..Z~.......%.
00144   99 77 24 10 00 bb b4 86  39 22 68 d0 d9 48 3d b6    .w$.....9"h..H=.
00160   b9 19 fd e9 8e b0 d9 10  ed 56 25 67 0f 52 9a 15    .........V%g.R..
00176   72 ac 95 ea d7 fe 01 87  ff 05 fb 5b 42 94 92 00    r..........[B...
00192   a8 95 63 19 ec e6 31 c5  99 27 cc c3 16 5d d9 1a    ..c...1..'...]..
00208   3d ca fd 5f bc 23 b0 37  49 25 1f cb 16 8b 8a d3    =.._.#.7I%......
00224   5c 21 e4 f8 1a 09 4c 5a  71 82 94 33 64 5a 1e 80    \!....LZq..3dZ..
00240   76 0b 9c 57 0e 9f c4 56  2d 97 81 e4 80 71 e1 19    v..W...V-....q..
00256   43 4d 0e 48 f1 b5 a0 ce  65 b7 e8 e5 35 55 29       CM.H....e...5U)

Packet number 10:
       From: 192.168.184.128
         To: 142.250.73.228
   Protocol: TCP
   Src port: 43364
   Dst port: 443
   Payload (1091 bytes):
00000   17 03 03 04 3e 56 0c 0b  86 13 5f 33 d2 23 26 50    ....>V...._3.#&P
00016   d9 6c 49 2a 94 e3 7f 48  e8 01 10 a1 28 76 3f d0    .lI*...H....(v?.
00032   4e 43 f3 6a ab 02 57 7d  c8 16 98 18 61 3b 9c 4b    NC.j..W}....a;.K
00048   d7 7e 28 96 cf c6 8d a1  c3 8a a7 a8 a0 58 b8 0e    .~(..........X..
00064   3d 58 3d ae c4 94 b6 05  e5 7e 4d bd 5d 6e af e7    =X=......~M.]n..
00080   ed 18 69 2a 15 b9 9b 2c  ed 02 81 93 9d 73 33 79    ..i*...,.....s3y
00096   0b 38 f7 e8 28 97 36 19  67 0f d7 c7 df 5a c3 ea    .8..(.6.g....Z..
00112   75 c6 a8 b1 73 d6 68 89  0f 1f 13 b6 fc 6b 60 85    u...s.h......k`.
00128   f7 64 28 4d 95 17 39 e7  56 00 b8 ab 3b 78 bf 31    .d(M..9.V...;x.1
00144   d7 78 46 e0 e3 84 8c 78  79 90 cb 05 93 7b 6b 3b    .xF....xy....{k;
00160   eb 0e ff 7d a9 7f ae c0  25 e3 f0 59 c7 b2 08 df    ...}....%..Y....
00176   a0 c1 bb 23 4c 84 98 db  b5 a2 81 d9 6a ca 22 f9    ...#L.......j.".
00192   ac 6e d9 17 b6 f3 fe 83  53 9e e2 87 e4 32 fa 56    .n......S....2.V
00208   a6 e3 fe 04 cb e8 c6 a3  9c 0e b7 12 c8 6f 89 6f    .............o.o
00224   7f 23 67 a6 64 3c 85 90  dc 0c 42 07 c7 c0 ee e1    .#g.d<....B.....
00240   da 52 07 50 74 22 55 5a  97 76 d6 4c a4 1e f1 75    .R.Pt"UZ.v.L...u
00256   d1 9a ab 96 c0 6e c8 73  47 11 26 b1 7e 54 6c 16    .....n.sG.&.~Tl.
00272   04 49 4b 9e 33 46 e8 7b  25 ec ac b1 61 86 85 13    .IK.3F.{%...a...
00288   3d 47 b0 aa 35 b5 61 bc  f2 ed 7a 94 3b 6f 2d 52    =G..5.a...z.;o-R
00304   29 2a 06 c2 75 5a 79 3e  8f c0 9b ef 7f 13 e7 2f    )*..uZy>......./
00320   af 36 f9 ab 27 06 b7 8e  43 50 40 ca 00 71 44 c1    .6..'...CP@..qD.
00336   4c bc 99 f1 65 1e 2b a1  4a 39 63 44 8d 7e 0d 8f    L...e.+.J9cD.~..
00352   0a 35 69 6c 04 41 2e 24  35 e7 9e 72 a2 a7 3f 55    .5il.A.$5..r..?U
00368   a1 01 48 1b f6 1b 2a 0c  51 40 32 02 18 4a bf f2    ..H...*.Q@2..J..
00384   1f 90 e7 0c c9 73 47 c6  e0 b2 b6 7c 06 27 25 c3    .....sG....|.'%.
00400   79 bd 4a 18 8d 9b 1e 8e  d5 b9 46 6e 41 f2 81 c6    y.J.......FnA...
00416   87 07 25 14 28 4a 13 1a  47 43 f0 78 6b 02 39 2c    ..%.(J..GC.xk.9,
00432   0e 4c d5 d0 c2 33 7d 30  20 ec b6 14 cc 97 16 64    .L...3}0 ......d
00448   84 5a c9 37 a2 38 6e 1f  2f 3c 91 e2 8f be a8 71    .Z.7.8n./<.....q
00464   1b 49 ac cf 65 91 29 68  59 3e e1 09 76 cb 51 5f    .I..e.)hY>..v.Q_
00480   67 3b 4a 32 45 b7 7b 0c  61 3d c0 29 ce 3a 0c 9a    g;J2E.{.a=.).:..
00496   8c 86 52 0f f6 8a dc dd  5b cc 8a 95 cb 9d bf bf    ..R.....[.......
00512   8b 35 9b 5a 77 d2 40 69  78 ea 43 5a e3 d7 bc 1b    .5.Zw.@ix.CZ....
00528   27 2b 15 98 bb c3 a1 38  fb b3 45 3e a9 de b6 55    '+.....8..E>...U
00544   14 75 5a be 67 27 8c 16  c5 3d 01 22 59 14 1d 9a    .uZ.g'...=."Y...
00560   f9 a6 e9 bb be c9 1e 6e  2d 6c b9 b6 a7 d8 bd ba    .......n-l......
00576   b0 8a 66 31 dd cb fd ab  37 66 b8 98 e2 16 6a 72    ..f1....7f....jr
00592   c9 f0 9e 15 84 ab 3e 28  5a f6 0f a2 44 61 4e a6    ......>(Z...DaN.
00608   c5 36 cb 5e 69 26 eb ac  1f 84 72 e3 20 54 89 30    .6.^i&....r. T.0
00624   4e 32 93 e3 cc 25 3b 52  c9 ed 59 f8 f6 8b 9f 32    N2...%;R..Y....2
00640   34 9c 7b 57 00 8a 9c 9e  e8 d6 a4 dc a4 1c 34 fc    4.{W..........4.
00656   d9 21 4a 58 28 c3 96 87  b1 ad e9 8f 1d f5 fe d8    .!JX(...........
00672   81 95 bf 9e d1 a7 ae 1d  0f 45 25 ff 39 2f 89 01    .........E%.9/..
00688   88 21 a0 3e 2c 35 02 e1  62 ca 52 64 88 dd 5f 4c    .!.>,5..b.Rd.._L
00704   ea a3 fa 87 32 67 b1 90  ba 79 85 bf c8 f1 60 0f    ....2g...y....`.
00720   bb 42 53 28 72 60 2c 12  29 96 fe f7 d4 62 46 9f    .BS(r`,.)....bF.
00736   f9 54 1c 24 df f4 2c 61  16 24 6e e7 70 f4 89 19    .T.$..,a.$n.p...
00752   e0 60 ea 6f 49 d7 7a 9d  dd 97 a8 ab 6d 7b a5 a4    .`.oI.z.....m{..
00768   9f b9 a9 2b ba a8 0c d5  ec ed 28 77 8d 02 40 67    ...+......(w..@g
00784   50 55 00 ad 44 6c ea f4  f0 73 16 00 7b 88 3f 0b    PU..Dl...s..{.?.
00800   f0 9c 9e 3e 1d 62 73 20  ba 43 59 1e b1 50 bd 94    ...>.bs .CY..P..
00816   53 9e f1 27 5f ca 48 4d  82 66 fd 34 a9 2f f8 1f    S..'_.HM.f.4./..
00832   21 77 61 d0 a8 2d e3 6a  83 73 75 81 33 40 64 3a    !wa..-.j.su.3@d:
00848   b8 7a ed dd 53 40 ea f4  15 a2 2a 5e 75 5e 81 aa    .z..S@....*^u^..
00864   1b 0d 36 4f 70 4a 1a fe  e1 54 e6 e2 65 b0 f7 8d    ..6OpJ...T..e...
