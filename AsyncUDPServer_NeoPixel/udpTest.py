import socket
import time
import random

PIXEL_NUM = 226

pixN = list(range(PIXEL_NUM))

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

server_address = '0.0.0.0'
server_port = 3333

server = (server_address, server_port)
sock.bind(server)
print("Listening on " + server_address + ":" + str(server_port))
data, client_address = sock.recvfrom(1024)
print(data, '***', str(client_address))

# with open('zroz.txt', 'r', encoding='utf-8') as f:
#     for line in f:
#         line = line.strip('\n')
#         lst = line.split(',')
#         if len(lst) < 3:
#             continue
#         print(lst)
#         bytesData = bytes(map(int, lst))
#         sent = sock.sendto(bytesData, client_address)
#         time.sleep(1)
#     res= map(str, res)
#     res = ','.join(res)
#     res += '\n'
#     print(res)
def ledTurnAllColor(red=0, green=0, blue=0):
    pixR = [_ledTestVar(red)]*PIXEL_NUM
    pixG = [_ledTestVar(green)]*PIXEL_NUM
    pixB = [_ledTestVar(blue)]*PIXEL_NUM
    res = [j for i in zip(pixN, pixR, pixG, pixB) for j in i]
    bytesData = bytes(map(int, res))
    sent = sock.sendto(bytesData, client_address)

def _ledTestVar(var):
    assert (type(var) == int), 'Input data expected to be \'int8\' type'
    if var > 255:
        return 255
    if var < 0:
        return 0
    return var

ledTurnAllColor(0, 250, 250)



pixBlk = 0
n = 0

bitR = 0
bitG = 0
bitB = 0

pixR = [bitR]*PIXEL_NUM
pixG = [bitG]*PIXEL_NUM
pixB = [bitB]*PIXEL_NUM


pic = [[0,0,1],
       [0,1,0],
       [0,0,-1]]

rr = gg = 255

bb=0
# strip 0.. 43.. 87.. 131.. 175
# girl 176.. 188.. 201.. 213

while True:
    ledTurnAllColor(rr, gg, bb)
    gg -= 1
    if gg < 0:
        gg=0
    rr -= 1
    if rr < 0:
        rr=0
    time.sleep(1)
        



