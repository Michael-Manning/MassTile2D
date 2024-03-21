import matplotlib.pyplot as plt
import math

def absSegment(x, y, a, l):
   nx = math.cos(a) * l + x
   ny = math.sin(a) * l + y
   #line(x, y, nx, ny)
   return [nx, ny]


force = 0.2

segLength = 40

x = 0
y = 0
      
anglePlot10 = list()
anglePlot100 = list()
anglePlot1000 = list()

positionPlot10 = list()
positionPlot100 = list()
positionPlot1000 = list()

angle = -math.pi / 2
np = [x, y]
for i in range(0, 10):
   anglePlot10.append([i, angle])
   positionPlot10.append([i, np[0], -np[1]])
   rigidity = 1.0 - ((i * i) * 0.006 + 0.1)
   angle += force * (1.0 - rigidity)
   np = absSegment(np[0], np[1], angle, segLength)

angle = -math.pi / 2
np = [x, y]
for i in range(0, 100):
   anglePlot100.append([i / 10.0, angle])
   positionPlot100.append([i / 10.0, np[0], -np[1]])
   ii = i / 10.0
   rigidity = 1.0 - ((ii * ii) * 0.006 + 0.1)
   angle += force * (1.0 - rigidity) * 0.1
   np = absSegment(np[0], np[1], angle, segLength / 10.0)

np = [x, y]
for i in range(0, 1000):
   
   ii = i / 100.0

   # angle accumulation function
   angle = 0.002 * force * ii**3 + 0.1 * force * ii

   s = segLength
   f = force
   
   # flip x and y to treat angle as starting from zero (flip 90 degrees)
   # because aproximated functions work closer to 0

   # sin summation
   # np[0] += math.sin(angle) * (s / 100.0)

   #sin aproximation summation
   # np[0] += angle * (s / 100.0)

   # sin aproximation accumulation function
   np[0] = 0.0005*f*s*ii**4 + 0.05*f*s*ii**2

   # cos summation
   # np[1] += math.cos(angle) * (s / 100.0)
   
   # cos aproximation summation 
   # np[1] += (1 - angle**2 / 2) * (s / 100.0)

   #cos aproximation accumulation function
   np[1] = -2.85714285714286e-7*f**2*s*ii**7 - 4.0e-5*f**2*s*ii**5 - 0.00166666666666667*f**2*s*ii**3 + 1.0*s*ii
   
   anglePlot1000.append([i / 100.0, angle])
   positionPlot1000.append([i / 100.0, np[0], np[1]])

fig, ax = plt.subplots()

## angles

# xVals = [angle[0] for angle in anglePlot10]
# yVals = [angle[1] for angle in anglePlot10]
# ax.plot(xVals, yVals, marker='o', color = "red")

# xVals = [angle[0] for angle in anglePlot100]
# yVals = [angle[1] for angle in anglePlot100]
# ax.plot(xVals, yVals, color = "black")

# xVals = [angle[0] for angle in anglePlot1000]
# yVals = [angle[1] for angle in anglePlot1000]
# ax.plot(xVals, yVals, color = "blue")

## x positions

# xVals = [angle[0] for angle in positionPlot10]
# yVals = [angle[1] for angle in positionPlot10]
# ax.plot(xVals, yVals, marker='o', color = "red")

# xVals = [angle[0] for angle in positionPlot100]
# yVals = [angle[1] for angle in positionPlot100]
# ax.plot(xVals, yVals, color = "black")

# xVals = [angle[0] for angle in positionPlot1000]
# yVals = [angle[1] for angle in positionPlot1000]
# ax.plot(xVals, yVals, color = "blue")

## y positions
xVals = [angle[0] for angle in positionPlot10]
yVals = [angle[2] for angle in positionPlot10]
ax.plot(xVals, yVals, marker='o', color = "red")

xVals = [angle[0] for angle in positionPlot100]
yVals = [angle[2] for angle in positionPlot100]
ax.plot(xVals, yVals, color = "black")

xVals = [angle[0] for angle in positionPlot1000]
yVals = [angle[2] for angle in positionPlot1000]
ax.plot(xVals, yVals, color = "blue")

plt.show()