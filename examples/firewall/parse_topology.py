#!/usr/bin/python


from xml.dom.minidom import parseString
import sys
import re

sys.stderr.write("\n This script translates a plain svg generated with inkscape\n")
sys.stderr.write(" to a omnet obstacles file (supports only rectangles)\n\n")

if (len(sys.argv) < 2):
  sys.stderr.write(" usage: ./parse_topology.py filepath\n\n")
  exit(1)

filepath = sys.argv[1]


try:
   file = open(filepath,'r')
except IOError:
   print "  ERROR: file "+filepath+ " not readable "
   exit(1)

data = file.read()
file.close()
dom = parseString(data)
rectangleTag = "rect"

svgTag = dom.getElementsByTagName("svg")[0]
totalWidth = float(svgTag.getAttribute("width"))
totalHeigth = float(svgTag.getAttribute("height"))



gTag = dom.getElementsByTagName("g")
if (len(gTag) != 0):
  transform = gTag[0].getAttribute("transform")
  trans = re.sub('\)','',re.sub('\(','',re.findall('\(.*?\)',transform)[0]))
  xTranslate = float(trans.partition(',')[0])
  yTranslate = float(trans.partition(',')[2])
else:
  xTranslate = 0
  yTranslate = 0


xmlTag = dom.getElementsByTagName(rectangleTag)
count= int(0)
if len(xmlTag) == 0:
 print "The xml file you gave has no " + rectangleTag + " tags"
 exit 
print "<obstacles>"
for tag in xmlTag:
 x = float(tag.getAttribute("x")) + xTranslate
 y = (float(tag.getAttribute("y")) + yTranslate)
 width = float(tag.getAttribute("width"))
 height = float(tag.getAttribute("height"))
 xmlstring = "<poly id=\"building#"+str(count)
 xmlstring +="\" type=\"building\" color=\"#F00\" shape=\""
 xmlstring += str(int(x))+","+str(int(y))+" "+str(int(x+width))+"," 
 xmlstring += str(int(y))+" "+ str(int(x+width))+","+ str(int(y+height))+" "
 xmlstring += str(int(x))+","+ str(int(y+height)) +"\" />"
 count = count+1
 print xmlstring

print "</obstacles>"

sys.stderr.write("\n conversion done \n\n")

