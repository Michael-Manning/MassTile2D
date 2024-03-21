import matplotlib.pyplot as plt
import math

positionPlot1000 = list()

pt = [0, 0]

s = 0.4
for i in range(0, 1000):
   x = i / 1000.0

   angle = x

   pt[0] = 1.0 - angle**2 / 2
   pt[1] = math.cos(angle) 
   
   positionPlot1000.append([x, pt[0], pt[1]])


fig, ax = plt.subplots()

xVals = [angle[0] for angle in positionPlot1000]
yVals = [angle[2] for angle in positionPlot1000]
ax.plot(xVals, yVals, color = "black")

xVals = [angle[0] for angle in positionPlot1000]
yVals = [angle[1] for angle in positionPlot1000]
ax.plot(xVals, yVals, color = "blue")

plt.show()