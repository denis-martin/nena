#!/usr/bin/python

import sys
import re
import os
import subprocess as sp

import socket
import struct

from PyQt4 import QtGui
from PyQt4 import QtCore

port = "50779"
device = "eth1"

netletFile = "/tmp/nodearch-available-netlets"
netletAId = "netlet://edu.kit.tm/itm/simpleArch/SimpleVideoTransportNetlet\n"
netletBId = "netlet://edu.kit.tm/itm/simpleArch/SimpleVideoTransportWithFecNetlet\n"

lossFile = "/tmp/nodearch-pkt-loss"


# udp networking settings for current loss probability
#dests_loss = ("127.0.0.1","127.0.0.1") # set recipients...
#dests_loss = ("127.0.0.1", "141.3.71.90") # set recipients...
dests_loss = ("10.10.10.2",) # set recipients...
dest_port_loss = 2323 # ...and destination port
msg_type_loss = 1 # data type for loss messages

socket_fd_loss = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# TODO: close? ;)


def sendUdpMsg(socketfd, destinations, port, type, value):
        msg = struct.pack("!ii", type, value) # pack type and value as integers in network byte order into a the msg string
        
        for dest in destinations:
                socketfd.sendto(msg,(dest,port))
                
def sendLossMsg(loss):
        sendUdpMsg(socket_fd_loss, dests_loss, dest_port_loss, msg_type_loss, loss)

# plot widget
class PlotWidget(QtGui.QWidget):

        sentPkts = range(100)
        dropped = range(100)
        
        def __init__(self, *args):
                QtGui.QWidget.__init__(self, *args)

                #self.setMinimumSize(600,300)
                
                for i in range(100):
                        self.sentPkts[i] = 0
                        self.dropped[i] = 0
        
        def updateStats(self, sentBytes=0, sentPkts=0, dropped=0, overlimits=0, requeues=0, rateb=0, ratep=0, backlogb=0, backlogp=0, requeues2=0):
                #i = 99
                #while i > 0:
                for i in range(99):
                        self.sentPkts[i] = self.sentPkts[i+1]
                        self.dropped[i] = self.dropped[i+1]
                        #i = i - 1 
                self.sentPkts[99] = sentPkts
                self.dropped[99] = dropped

                #print sentPkts, dropped
                        
                        

        def paintEvent(self, event):
                #print "plot"
                p = QtGui.QPainter(self)
                
                ppath = QtGui.QPainterPath()

                height = self.size().height()
                width = self.size().width()


                ppath.moveTo(width/100,height)

                mean = 0.0

                for i in range(99):
                        sentDiff = float(self.sentPkts[i+1]-self.sentPkts[i])
                        droppedDiff = float(self.dropped[i+1]-self.dropped[i])
                        if (sentDiff+droppedDiff) != 0:
                                ratio = droppedDiff / (sentDiff + droppedDiff)
                        else:
                                ratio = 0
                        #ppath.lineTo((i+1)*width/100, height-1 - ratio*height)
                        ppath.lineTo((i+1)*width/100, height-1 - ratio*height*2)
                        mean = mean + ratio

                mean = mean / 100
                #print mean

                ppath.lineTo(99*width/100,height)
                ppath.lineTo(width/100,height)
                p.drawPath(ppath)
                p.fillPath(ppath, QtGui.QColor(150,150,150))

                p.setPen(QtGui.QColor(255,0,0))
                #p.drawLine(0, height-mean*height-1, width-1, height-mean*height-1)
                p.drawLine(0, height-2*mean*height-1, width-1, height-2*mean*height-1)
                p.setPen(QtGui.QColor(0,0,0))

                # achsen und beschriftung
                p.drawLine(0, height-1, width, height-1)
                p.drawLine(0, 0, width, 0)
                p.drawLine(0, 0, 0, height-1)
                p.drawLine(width-1, 0, width-1, height-1)
                
                for i in range(5):
                        #p.drawLine(5, i*height/10, 0, i*height/10)
                        #p.drawLine(width -5, i*height/10, width, i*height/10)
                        p.drawLine(5, i*height/5-1, 0, i*height/5-1)
                        p.drawLine(width -5, i*height/5-1, width, i*height/5-1)
                for i in range(50):
                        p.drawLine(2*i*width/100, 5, 2*i*width/100, 0)
                        p.drawLine(2*i*width/100, height-5, 2*i*width/100, height)

class MainWindowContainer(QtGui.QWidget):

    mainWindow = None
    isFullscreen = False

    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)
        self.mainWindow = MainWindow(self)
        self.isFullScreen = False

    def resizeEvent(self,event):
        self.mainWindow.move(self.width()/2 - self.mainWindow.width()/2, self.height()/2 - self.mainWindow.height()/2)

    def keyPressEvent(self,event):
        if event.key() == QtCore.Qt.Key_F:
                event.accept()
                if self.isFullScreen == False:
                        self.showFullScreen()
                        self.isFullScreen = True
                else:
                        self.showNormal()
                        self.isFullScreen = False
        else:
                event.ignore()

class VerticalLabel(QtGui.QLabel):
    def __init__(self, parent=None):
        QtGui.QLabel.__init__(self,parent)

    def paintEvent(self, event):
        p = QtGui.QPainter(self)
        #p.save()
        p.translate(self.width()-3,self.height())
        p.rotate(-90)
        #p.translate(0,-self.heigth())
        #p.drawText("blub")
        #p.drawText(self.width()/2,self.height()/2,self.text())
        p.drawText(0,0,self.text())
        #p.restore()

class MainWindow(QtGui.QLabel):

    plotWidget = None
    lossSlider = None
    lossLabel = None

    netletA = None
    netletB = None

    # current loss probability (don't change
    currentLoss = 0


    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)
        
        # name of the window
        self.setWindowTitle('Emulated Networks')
        
        # set background image and resize main window to the image size
        background = QtGui.QPixmap("demo-gui-netem.png")
        self.setPixmap(background)
        self.resize(background.size())


        # netlet A is displayed as a label
        netletA = QtGui.QLabel(self)
        netletA_pixmap = QtGui.QPixmap("demo-gui-netlet-a.png")
        netletA.setPixmap(netletA_pixmap)
        netletA.resize(netletA_pixmap.size())
        
        # netlet B is displayed as a label
        netletB = QtGui.QLabel(self)
        netletB_pixmap = QtGui.QPixmap("demo-gui-netlet-b.png")
        netletB.setPixmap(netletB_pixmap)
        netletB.resize(netletB_pixmap.size())

        # move netlets to their final position
        netletA.move(self.width()/2 - netletA.width(), self.height() - netletA.height() - 75) # fixed offset from bottom
        netletB.move(self.width()/2 + netletB.width(), self.height() - netletB.height() - 75) # fixed offset from bottom

        # hide netlets
        netletA.hide()
        netletB.hide()

        # save netlets in class attributes
        self.netletA = netletA
        self.netletB = netletB

        
        # packet loss plot
        lossPlot = PlotWidget(self)
        lossPlot.resize(self.width()/3, self.height()/3)
        #lossPlot.resize(400, 200)
        lossPlot.move(self.width()/2 - lossPlot.width()/2, 75)
        self.plotWidget = lossPlot

        xLabel = QtGui.QLabel(self)
        xLabel.setText("Time in seconds ->")
        yLabel = VerticalLabel(self)
        yLabel.setText("Percent of Packets dropped ->")
        #yLabel.resize(yLabel.height(),yLabel.width()+100)
        yLabel.resize(14,200)
        xLabel.move(lossPlot.x(),lossPlot.y()+lossPlot.height())
        yLabel.move(lossPlot.x()-15,lossPlot.y()+50)
        #yLabel.move(200,100)

        # packet loss slider
        lossSlider = QtGui.QSlider(self)
        lossSlider.setMinimum(0)
        #lossSlider.setMaximum(99)
        lossSlider.setMaximum(30)
        lossSlider.resize(20,lossPlot.height())
        #lossSlider.setFixedSize(lossSlider.width()+30,lossPlot.height())
        #lossSlider.setFixedSize(100,lossPlot.height())
        #lossSlider.move(self.width()/2 + lossPlot.width()/2 - 15, 75)
        lossSlider.move(self.width()/2 + lossPlot.width()/2 + 25, 75)
        lossSlider.setTracking(False)
        
        pal = lossSlider.palette()
        pal.setColor(QtGui.QPalette.Window, QtCore.Qt.lightGray)
        lossSlider.setPalette(pal)
        lossSlider.setAutoFillBackground(True)

        self.lossSlider = lossSlider

        lossLabel = QtGui.QLabel(self)
        
        font = QtGui.QFont()
        font.setPointSize(32)
        #font.setBold(True)
        
        lossLabel.setFont(font)
        lossLabel.setText("0")
        lossLabel.setAlignment(QtCore.Qt.AlignRight)
        
        lossLabel.move(self.width()/2 + lossPlot.width()/2 + 20, lossPlot.height()/2 + 50)
        lossLabel.resize(lossLabel.width(),lossLabel.height()+12)
        percent = QtGui.QLabel(self)
        percent.setFont(font)
        percent.setText("%")
        percent.move(lossLabel.x()+lossLabel.width(), lossLabel.y())
        pktloss = QtGui.QLabel(self)
        pktlossFont = QtGui.QFont()
        pktlossFont.setPointSize(16)
        pktloss.setFont(pktlossFont)
        pktloss.setText("Packet Loss:")
        pktloss.move(percent.x()-pktloss.width()/2,percent.y()-percent.height()+pktloss.height()/3)
        
        self.lossLabel = lossLabel

        self.connect(self.lossSlider, QtCore.SIGNAL("sliderMoved(int)"), self.lossLabel, QtCore.SLOT("setNum(int)"))
        self.connect(self.lossSlider, QtCore.SIGNAL("valueChanged(int)"), self.updateLoss)

        # timer
        self.timer = QtCore.QTimer(self)
        self.connect(self.timer, QtCore.SIGNAL("timeout()"), self.handleTimer)
        self.timer.start(1000)

        
        # load pkt loss from pkt loss file, or initialize pkt loss file if it does not exist
        if os.path.exists(lossFile):
                f = open(lossFile, "r")
                #data = f.readline().toInt() # read first line containing loss probability
                #if data[1]: # if conversion to int was successful...
                #       self.updateLoss(data[0]) #... update the loss rate
                loss = int(f.readline())
                self.updateLoss(loss)
                self.lossSlider.setValue(loss)
                f.close()
                
        else:
                f = open(lossFile, "w")
                f.write("0\n")
                f.close()
                


    def handleTimer(self):

        # run tc command to get statistics
        p = sp.Popen("/sbin/tc -s qdisc show dev %s" %device, shell=True, stdout=sp.PIPE)
        stats = p.communicate()

        # parse statistics using regex  
        result = re.search(r"Sent (\d+) bytes (\d+) pkt", stats[0])
        sentBytes = int(result.group(1))
        sentPkts = int(result.group(2))
                
        result = re.search(r"dropped (\d+), overlimits (\d+) requeues (\d+)", stats[0])
        dropped = int(result.group(1))
        overlimits = int(result.group(2))
        requeues = int(result.group(3))
        
        result = re.search(r"rate (\d+)bit (\d+)pps backlog (\d+)b (\d+)p requeues (\d+)", stats[0])
        rateb = int(result.group(1))
        ratep = int(result.group(2))
        backlogb = int(result.group(3))
        backlogp = int(result.group(4))
        requeues2 = int(result.group(5))
                
        self.plotWidget.updateStats(sentBytes, sentPkts, dropped, overlimits, requeues, rateb, ratep, backlogb, backlogp, requeues2)
        self.plotWidget.update()
        
        
        # parse netlet file and display netlets accordingly
        if os.path.exists(netletFile):
                showA = False
                showB = False
                
                # are the netlet IDs in the file?
                f = open(netletFile)
                for line in f:
                        if line == netletAId:
                                showA = True
                        if line == netletBId:
                                showB = True

                # if yes, show them in the GUI; if no, hide them
                if showA:
                        self.netletA.show()
                else:
                        self.netletA.hide()
                if showB:
                        self.netletB.show()
                else:
                        self.netletB.hide()

        else:
                #print "No netlet file, skipping..."
                self.netletA.hide()
                self.netletB.hide()
        
        # send current loss probability to other nodes
        sendLossMsg(self.currentLoss)

    def updateLoss(self, loss):

        # store current loss probability in global currentLoss variable
        self.currentLoss = loss

        # if label doesn't display the right value, set it
        if self.lossLabel.text().toInt()[0] != loss:
                self.lossLabel.setNum(loss)

        # string that is executed to activate/update TC/NetEm settings
        tcString = "tc qdisc del dev %s parent 300:1\ntc qdisc del dev %s parent 30:1\ntc qdisc del dev %s root\n" %(device,device,device)
        tcString += "tc qdisc add dev %s root handle 1:0 prio\n" %device
        tcString += "tc qdisc add dev %s parent 1:3 handle 30: netem loss %s%%\n" %(device,loss)
        tcString += "tc filter add dev %s protocol ip parent 1:0 prio 1 u32 match ip dport %s 0xffff flowid 1:3" %(device, port)
        print tcString
        os.system(tcString)
        
        # write loss probability to the loss file
        #f = open(lossFile, "w")
        #f.write(str(loss)+"\n")
        #f.close()
        
        

app = QtGui.QApplication(sys.argv)
widget = MainWindowContainer()
widget.show()
app.exec_()

