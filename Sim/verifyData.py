import json
import matplotlib.pyplot as plt
import numpy as np

with open('data.json') as jsonFile:
    data = json.load(jsonFile)

cnt = data["cnt"]
lambda0 = np.array(data["lambda0"])
lambda1 = np.array(data["lambda1"])
mu0 = np.array(data["mu0"])
mu1 = np.array(data["mu1"])
meanServTime = np.array(data["meanServTime"])
meanTimeInSys = np.array(data["meanTimeInSys"])

for i in range(0, cnt):
    L = lambda0[i] + lambda1[i]
    expected = (1/(mu0[i] - lambda0[i]) + 1/(mu1[i] - L)) * (lambda0[i] / L)
    expected = expected + (1/(mu1[i] - lambda1[i])) * (lambda1[i] / L)
    print("Datapoint %d: %.3f %.3f %.3f %.3f \tSim Result: %.3f \tExpected: %.3f" \
        % (i, lambda0[i], mu0[i], lambda1[i], mu1[i], meanTimeInSys[i], expected))
