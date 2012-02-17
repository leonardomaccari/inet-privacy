#!/usr/bin/python

import sys
import random 
from Tkinter import *

class Shape:
  """ A shape in the playground"""
  xmin = 0
  xmax = 0
  ymin = 0
  ymax = 0
  margin = 0
  def __init__(self, x1,x2,y1,y2):
    self.xmin = x1
    self.xmax = x2
    self.ymin = y1
    self.ymax = y2
  def reshape(self, size, X, Y, mr):
    random.seed()
    side = size # we want squares, or else intersection is harder to compute. 
    self.margin = mr
    self.xmin = random.randint(mr,(X-side-mr))
    self.ymin = random.randint(mr,(Y-side-mr))
    self.xmax = self.xmin+side
    self.ymax = self.ymin+side
  def printShape(self):
    print "%d,%d - %d,%d" % (self.xmin,self.ymin,self.xmax, self.ymax)
  def isPointInShape(self, x,y):
    if ((self.xmin - self.margin) <= x and (self.xmax + self.margin) >=x and (self.ymin - self.margin) <= y and (self.ymax + self.margin) >= y):
      return True
    else:
      return False
  def isShapeInShape(self, shape):
    if (self.isPointInShape(shape.xmin, shape.ymin)):
      return True
    if (self.isPointInShape(shape.xmax, shape.ymin)):
      return True
    if (self.isPointInShape(shape.xmax, shape.ymax)):
      return True
    if (self.isPointInShape(shape.xmin, shape.ymax)):
      return True
    else:
      return False    
    
if (len(sys.argv) < 4):
  print "usage: ./create_obstacles.sh numobstacles obstaclesmaxside margin areasideX areasideY"
  exit(1)

obstacleNumber = int(sys.argv[1])
size = int(sys.argv[2])
border = int(sys.argv[3])
X = int(sys.argv[4])
Y = int(sys.argv[5])

i = obstacleNumber

side=random.randint(size/2, size-1)

obstacleList = []
count = 0
while ( i > 0 ):
  count = count +1 
  if (count > 1000):
    sys.stderr.write("\nERROR: you chose some hard parameters to have a topology, looping too much\n\n")
    # consider that we're trying to make a set of non-overlapping squares, with
    # sidelenght between size/2 and size. It may be that for the chosen
    # parameters such a configuration is not possible to achieve. probably you should
    # decrease sidelenght or the number of squares  
    exit(1)
  p = Shape(0,0,0,0)
  p.reshape(side,X,Y,border)
#  print "new",
#  p.printShape()
  if len(obstacleList) == 0:
      obstacleList.append(p)
      i = i-1
  else:
    for x in obstacleList:
      added=True
#      print "-list-"
#      x.printShape()
      if x.isShapeInShape(p):
        added=False
#        p.printShape()
#        print "regenerating\n" 
        break
    if (added):
#      print "added",
#      p.printShape()
      obstacleList.append(p)
      i = i-1


master = Tk()
www=X
hhh=Y
w = Canvas(master, width=www, height=hhh)
w.pack()

count = 0
print "<obstacles>"
for x in obstacleList:
 xmlstring = "<poly id=\"building#"+str(count)
 xmlstring +="\" type=\"building\" color=\"#F00\" shape=\""
 xmlstring += str(x.xmin)+","+ str(x.ymin)+" "+ str(x.xmax)+"," 
 xmlstring += str(x.ymin)+" "+ str(x.xmax)+","+ str(x.ymax)+" "
 xmlstring += str(x.xmin)+","+ str(x.ymax) +"\" />"
 #\" type=\"building\" color=\"#F00\" shape=\"%d,%d %d,%d %d,%d %d,%d\" />" % {int(count), x.xmin, x.ymin, x.xmax, x.ymin, x.xmax, x.ymax, x.xmin, x.ymax}
 print xmlstring
 w.create_rectangle(x.xmin, x.ymin, x.xmax, x.ymax, fill="blue")
 count = count + 1
print "</obstacles>"
mainloop()

