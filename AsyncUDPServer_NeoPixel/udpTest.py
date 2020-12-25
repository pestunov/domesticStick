import socket
import time
import random

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

pixBlk = 0
forward = True

while True:
    
    res = []
    for ii in range(226):
        res.append(ii)
        if ((ii > 176) and (ii < 188)) or ((ii > 201) and (ii < 213)):
            res.append(0) # red
            res.append(0) # green
            res.append(0) # blue
        else:
            res.append(pixBlk) # red
            res.append(pixBlk) # green
            res.append(pixBlk) # blue
            
        
#     res= map(str, res)
#     res = ','.join(res)
#     res += '\n'
#     print(res)
    bytesData = bytes(map(int, res))
    sent = sock.sendto(bytesData, client_address)
    print(pixBlk)
    pixBlk += 1
    if (pixBlk > 255):
        pixBlk = 0
    time.sleep(1)
        



