import os, sys
lib_path = os.path.abspath('../tmnet')
sys.path.append(lib_path)
import tmnet

import json

from PyQt4 import QtGui
from PyQt4 import QtCore

import optparse

#config
screenWidth = 1024

# plot widget
class PlotWidget(QtGui.QWidget):
    """Network Plot Widget"""
    #sentPkts = range(100)
    #dropped = range(100)
    
    def __init__(self, *args):
            QtGui.QWidget.__init__(self, *args)

            #self.setMinimumSize(600,300)
            self.bytes = range(100)
            for i in range(100):
                    #self.sentPkts[i] = 0
                    #self.dropped[i] = 0
                    self.bytes[i] = 0
    
    #def updateStats(self, sentBytes=0, sentPkts=0, dropped=0, overlimits=0, requeues=0, rateb=0, ratep=0, backlogb=0, backlogp=0, requeues2=0):
    def updateStats(self, bytes=0):
            #i = 99
            #while i > 0:
            for i in range(99):
                    #self.sentPkts[i] = self.sentPkts[i+1]
                    #self.dropped[i] = self.dropped[i+1]
                    self.bytes[i] = self.bytes[i+1]
                    #i = i - 1 
            #self.sentPkts[99] = sentPkts
            #self.dropped[99] = dropped
            self.bytes[99] = bytes
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
                    #sentDiff = float(self.sentPkts[i+1]-self.sentPkts[i])
                    #droppedDiff = float(self.dropped[i+1]-self.dropped[i])
                    #if (sentDiff+droppedDiff) != 0:
                    #        ratio = droppedDiff / (sentDiff + droppedDiff)
                    #else:
                    #        ratio = 0
                    #ppath.lineTo((i+1)*width/100, height-1 - ratio*height)
                    ratio = 0.0 + self.bytes[i+1]
                    #print(ratio)
                    ratio = ratio/100
                    #print(ratio)
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
                    
class VerticalLabel(QtGui.QLabel):
    def __init__(self, parent=None):
        QtGui.QLabel.__init__(self,parent)

    def paintEvent(self, event):
        p = QtGui.QPainter(self)
        p.translate(self.width()-3,self.height())
        p.rotate(-90)
        p.drawText(0,0,self.text())


class MainWindowContainer(QtGui.QWidget):
    """Main Window Container Class, starts the Main Window"""

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

class MainWindow(QtGui.QLabel):
    """Contains everything"""
    def __init__(self, parent=None):
        
        # class attributes
        self.plot1 = None
        self.plot2 = None
        self.plot3 = None
        
        QtGui.QWidget.__init__(self, parent)
        
        # name of the window
        self.setWindowTitle('Emulated Networks')
        
        # set background image and resize main window to the image size
        background = QtGui.QPixmap("demo-bild.png").scaledToWidth(screenWidth, QtCore.Qt.SmoothTransformation)
        self.setPixmap(background)
        self.resize(background.size())
       
        # font for y labels
        yfont = QtGui.QFont(self)
        yfont.setPointSize(7)
        
        # plot for network 1
        plot1 = PlotWidget(self)
        plot1.resize(self.width()/7, self.height()/7)
        #lossPlot.resize(400, 200)
        plot1.move(1.5*self.width()/20, self.height()/2-plot1.height()/2+10)
        self.plot1 = plot1
        
        yLabel1 = VerticalLabel(self)
        yLabel1.setText("[1MByte/s]")
        #yLabel1.resize(yLabel.height(),yLabel.width()+100)
        yLabel1.setFont(yfont)
        yLabel1.resize(14,200)
        yLabel1.move(plot1.x()-15,plot1.y()/1.75)
        
        # plot for network 2
        plot2 = PlotWidget(self)
        plot2.resize(self.width()/7, self.height()/7)
        #lossPlot.resize(400, 200)
        plot2.move(6.7*self.width()/20, self.height()/2-plot2.height()/2+10)
        self.plot2 = plot2
        
        yLabel2 = VerticalLabel(self)
        yLabel2.setText("[1MByte/s]")
        #yLabel2.resize(yLabel.height(),yLabel.width()+100)
        yLabel2.setFont(yfont)
        yLabel2.resize(14,200)
        yLabel2.move(plot2.x()-15,plot2.y()/1.75)
        
        # plot for network 3
        plot3 = PlotWidget(self)
        plot3.resize(self.width()/7, self.height()/7)
        #lossPlot.resize(400, 200)
        plot3.move(12.1*self.width()/20, self.height()/2-plot3.height()/2+10)
        self.plot3 = plot3
        
        yLabel3 = VerticalLabel(self)
        yLabel3.setText("[100KByte/s]")
        yLabel3.setFont(yfont)
        yLabel3.resize(14,200)
        yLabel3.move(plot3.x()-15,plot3.y()/1.7)

        # timer for plot updates
        self.timer = QtCore.QTimer(self)
        self.connect(self.timer, QtCore.SIGNAL("timeout()"), self.handleTimer)
        self.timer.start(1000)
        
    
    def getDataRate(self, uri):
        """Helper function to retrieve DataRate from NENA"""    

        # retrieve traffic information from NENA
        handle = tmnet.get(uri, "")
        nenaData = ""
#        while not handle.isEndOfStream():
#            nenaData = nenaData + handle.read(1024)
        nenaData = handle.read(2048)
        handle.close()
        
        # nenaData is a JSON object, parse it
        traffic = json.loads(nenaData)
        
        dataRate = (traffic["txRate"] + traffic["rxRate"] + 0.0)
        print(dataRate)
        return dataRate
    
    def handleTimer(self):
        """Timer for plot updates"""
        
        # update network plots
        self.plot1.updateStats(self.getDataRate("nena://localhost/netadapt/web/0")/100000)
        self.plot1.update()
        self.plot2.updateStats(self.getDataRate("nena://localhost/netadapt/cdn/0")/100000)
        self.plot2.update()
        self.plot3.updateStats(self.getDataRate("nena://localhost/netadapt/video/0")/10000)
        self.plot3.update()

# main
if __name__ == "__main__":
    # parse command line parameters
    parser = optparse.OptionParser()
    parser.add_option("--socket", dest="socket", default="/tmp/nena_socket_router", help="Socket for connection to local NENA daemon")
    (options, args) = parser.parse_args()
    
    nenaSocket = options.socket
    
    print("Starting demoNetworkGui. Socket: %s"%(nenaSocket))
    
    # Initialize TMNET/NENA API
    tmnet.init()
    tmnet.set_plugin_option("tmnet::nena", "ipcsocket", nenaSocket)
    
    # start gui
    app = QtGui.QApplication(sys.argv)
    widget = MainWindowContainer()
    widget.show()
    app.exec_()