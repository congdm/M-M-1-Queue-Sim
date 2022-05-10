import json
import matplotlib.pyplot as plt
import numpy as np
import sys
import os

def expected(lambd, mu, nNode, i):
    sumLambd = [0.0] * nNode
    sumLambd[0] = lambd[0][i]
    for j in range(1, nNode):
        sumLambd[j] = sumLambd[j-1] + lambd[j][i]
    res = 0.0
    time = 0.0
    for j in range(0, nNode):
        time = 1 / (mu[j][i] - sumLambd[j])
        res = res + (lambd[j][i] / sumLambd[nNode-1]) * time
    return res

filename = sys.argv[1]
with open(filename) as jsonFile:
    data = json.load(jsonFile)

nData = data['nData']
nNode = data['nNode']
lambd = []
mu = []

for i in range(0, nNode):
    lambd.append(np.array(data['lambda' + str(i)]))
    mu.append(np.array(data['mu' + str(i)]))
meanServTime = np.array(data['meanServTime'])
meanTimeInSys = np.array(data['meanTimeInSys'])

with open(filename + '.txt', 'w') as outFile:
    for i in range(0, nData):
        print('Datapoint %d:' % (i), end=' ', file=outFile)
        for j in range(0, nNode):
            print('%.4f' % lambd[j][i], end=' ', file=outFile)
            print('%.4f' % mu[j][i], end=' ', file=outFile)
        print('\tSim Result: %.4f' % meanTimeInSys[i], end=' ', file=outFile)
        print('\tExpected: %.4f' % expected(lambd, mu, nNode, i), file=outFile)
