

segLength = 40

def setup():
    size(600, 600)
    strokeWeight(10)
    stroke(255, 160)
    global x, y
    x = width * 0.5
    y = height * 0.9

def draw():
    background(0)

    force = (sin(millis() / 1000.0) + 1.0) * 0.5 * 0.4
    
    angle = 0
    with pushMatrix():
        segment(x, y, - PI / 2)
        for i in range(0, 9):
            rigidity = 1.0 - ((i * i) * 0.006 + 0.1)
            angle = force * (1.0 - rigidity)
            segment(segLength, 0, angle)
            
    angle = -PI / 2
    np = [x - 0, y]
    for i in range(0, 10):
        
        np = absSegment(np[0], np[1], angle, segLength)
        rigidity = 1.0 - ((i * i) * 0.006 + 0.1)
        angle += force * (1.0 - rigidity)

    angle = -PI / 2
    np = [x + 0, y]
    for i in range(0, 100):
        np = absSegment(np[0], np[1], angle, segLength / 10.0)
        ii = i / 10.0
        rigidity = 1.0 - ((ii * ii) * 0.006 + 0.1)
        angle += force * (1.0 - rigidity) * 0.1
    #    print("x: " + str(ii) + " a: " + str(angle))
    
    #angle = -PI / 2
    np = [x + 80.0, y]
    for i in range(0, 1000):
        
        ii = i / 100.0
        angle = 0.002 * force * (ii * ii * ii) +0.1 * force * ii  -PI / 2
        np = absSegment(np[0], np[1], angle, segLength / 100.0)
        
        #rigidity = 1.0 - ((ii * ii) * 0.006 + 0.1)
        #angle += force * (1.0 - rigidity) * 0.01
        

def segment(x, y, a):
    translate(x, y)
    rotate(a)
    line(0, 0, segLength, 0)

def absSegment(x, y, a, l):
    nx = cos(a) * l + x
    ny = sin(a) * l + y
    line(x, y, nx, ny)
    return [nx, ny]
