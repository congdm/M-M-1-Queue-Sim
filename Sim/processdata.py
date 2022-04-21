import json
import matplotlib.pyplot as plt
import numpy as np

with open('output.json') as jsonFile:
    data = json.load(jsonFile)

dataCnt = data["dataCnt"]
servTimeData = np.array(data["servTimeData"])
queueTimeData = np.array(data["queueTimeData"])
droppedPktFlag = data["droppedPktData"]

plt.plot(servTimeData + queueTimeData)
plt.ylabel('Total time in system')
plt.xlabel('Packet')
plt.show()