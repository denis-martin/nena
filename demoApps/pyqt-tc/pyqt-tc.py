import sys
import os
import re

import subprocess as sp

from PyQt4 import QtGui
from PyQt4 import uic
from PyQt4 import QtCore

#class graphicsView(QtGui.QGraphicsView):
#	def paintEvent(self, qpaintevent):
#		p = QtGui.QPainter(self)
#		p.drawLine(0,1,1,1)
#		print "blub"


# plot widget
class PlotWidget(QtGui.QWidget):

	sentPkts = range(100)
	dropped = range(100)
	
	def __init__(self, *args):
		QtGui.QWidget.__init__(self, *args)

		#self.color = QtGui.QColor(110,110,110)
		#self.setAutoFillBackground(True)
		#self.setGeometry(300, 300, 280, 170)
		#self.setMinimumSize(self.parent().size())
		self.setMinimumSize(800,400)
		
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
		#p.drawLine(0,1,1,1)

		height = self.size().height()
		width = self.size().width()
		

		#for i in range(98):
			#diff = self.sentPkts[i+1]-self.sentPkts[i]
			#print diff
			#print width
			#p.drawPoint(i * (width /100), height-diff - 1)
			
			#diffnext = self.sentPkts[i+2]-self.sentPkts[i+1]
			#p.drawLine(i* (width/100), height-diff-1, (i+1)*(width/100),height-diffnext-1)
		#	sentDiff = float(self.sentPkts[i+1]-self.sentPkts[i])
		#	sentDiffNext = float(self.sentPkts[i+2]-self.sentPkts[i+1])
		#	droppedDiff = float(self.dropped[i+1]-self.dropped[i])
		#	droppedDiffNext = float(self.dropped[i+2]-self.dropped[i+1])
		#	if (sentDiff+droppedDiff) != 0:
		#		ratio = droppedDiff / (sentDiff + droppedDiff)
		#	else:
		#		ratio = 0
		#	if (sentDiffNext+droppedDiffNext) != 0:
		#		ratioNext = droppedDiffNext / (sentDiffNext + droppedDiffNext)
		#	else:
		#		ratioNext = 0

			#print sentDiff, droppedDiff, ratio
		#	p.drawLine(i* (width/100), height-1 - ratio*height,(i+1)*(width/100), height-1 - ratioNext*height)
			
			

			#ppath.moveTo(0,height)
			#ppath.lineTo(width,0)
			#ppath.lineTo(width,height/2)
			#ppath.lineTo(0,height)
			#p.drawPath(ppath)
			#p.fillPath(ppath, QtGui.QColor(0,0,0))	

		ppath.moveTo(width/100,height)

		mean = 0.0

		for i in range(99):
			sentDiff = float(self.sentPkts[i+1]-self.sentPkts[i])
			droppedDiff = float(self.dropped[i+1]-self.dropped[i])
			if (sentDiff+droppedDiff) != 0:
				ratio = droppedDiff / (sentDiff + droppedDiff)
			else:
				ratio = 0
			ppath.lineTo((i+1)*width/100, height-1 - ratio*height)
			mean = mean + ratio

		mean = mean / 100
		#print mean

		ppath.lineTo(99*width/100,height)
		ppath.lineTo(width/100,height)
		p.drawPath(ppath)
		p.fillPath(ppath, QtGui.QColor(150,150,150))

		p.setPen(QtGui.QColor(255,0,0))
		p.drawLine(0, height-mean*height-1, width-1, height-mean*height-1)
		p.setPen(QtGui.QColor(0,0,0))

		# achsen und beschriftung
		p.drawLine(0, height-1, width, height-1)
		p.drawLine(0, 0, width, 0)
		p.drawLine(0, 0, 0, height-1)
		p.drawLine(width-1, 0, width-1, height-1)
		
		for i in range(10):
			p.drawLine(5, i*height/10, 0, i*height/10)
			p.drawLine(width -5, i*height/10, width, i*height/10)
		for i in range(100):
			p.drawLine(i*width/100, 5, i*width/100, 0)
			p.drawLine(i*width/100, height-5, i*width/100, height)


# the class has to have the same base class as your UI
class NetworkProperties(QtGui.QTabWidget):

	prop_string = ""
	rate_string = ""
	del_string = ""
	device = "eth0"
	timer = None

	plotWidget = None

	loss = 0

	# ssh hacks
	sshHost1 = "10.10.10.1"
	sshHost2 = "10.10.10.2"
	sshUser = "b@"
	sshLossFile = "/tmp/nodearch-pkt-loss"

	##################
	### STATISTICS ###
	##################

	def handleTimer(self):
		
		# run tc command to get statistics
		p = sp.Popen("/sbin/tc -s qdisc show dev %s" %self.device, shell=True, stdout=sp.PIPE)
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
		#self.graphicsView.paintEvent()
		#self.graphicsView.repaint()
		#self.graphicsView.paintEvent(QtGui.QPaintEvent())
		self.plotWidget.update()
	
	def __init__(self, *args):
		QtGui.QMainWindow.__init__(self, *args)
		# just pass self
		uic.loadUi("pyqt-tc.ui", self)

		# draw plot widget
		self.plotWidget	= PlotWidget(self.Statistics)	

		#self.timer = QTimer(self)
		self.timer = QtCore.QTimer(self)
		self.connect(self.timer,
                     QtCore.SIGNAL("timeout()"),
                     self.handleTimer)
		self.timer.start(1000)


	##############
	### DEVICE ###
	##############
	
	
	def updateBridge(self):
		#bridgeString = "brctl addbr {0}\n".format(self.device)
		bridgeString = "brctl addbr %s\n" %self.device

		for index in xrange(self.bridgeListWidget.count()):
    			item = self.bridgeListWidget.item(index)
			#bridgeString += "brctl addif {0} {1}\n".format(self.device,item.text())
			bridgeString += "brctl addif %s %s\n" %(self.device,item.text())
		
		self.deviceDisplayTextEdit.clear()	
		self.deviceDisplayTextEdit.setText(bridgeString)


	# check boxes
	@QtCore.pyqtSignature("bool")
	def on_deviceCheckBox_toggled(self, checked):
		self.bridgeCheckBox.setChecked(not checked)
		
		if checked == True:
			self.deviceLineEdit.setEnabled(True)

			self.device = self.deviceLineEdit.text()
			self.updateProperties()
		else:
			self.deviceLineEdit.setEnabled(False)
	
	@QtCore.pyqtSignature("bool")
	def on_bridgeCheckBox_toggled(self, checked):
		self.deviceCheckBox.setChecked(not checked)
		
		if checked == True:
			self.bridgeLineEdit.setEnabled(True)
			self.bridge_addLineEdit.setEnabled(True)
			self.bridgeListWidget.setEnabled(True)
			self.bridge_addButton.setEnabled(True)
			self.bridge_remButton.setEnabled(True)

			self.device = self.bridgeLineEdit.text()
			self.updateProperties()
		else:
			self.bridgeLineEdit.setEnabled(False)
			self.bridge_addLineEdit.setEnabled(False)
			self.bridgeListWidget.setEnabled(False)
			self.bridge_addButton.setEnabled(False)
			self.bridge_remButton.setEnabled(False)

	# add devices to bridge
	@QtCore.pyqtSignature("")
	def on_bridge_addButton_clicked(self):
		item = self.bridge_addLineEdit.text()
		if item != "":
			self.bridgeListWidget.addItem(item)
		self.bridge_addLineEdit.clear()
		self.updateBridge()

	# remove devices from bridge
	@QtCore.pyqtSignature("")
	def on_bridge_remButton_clicked(self):
		item = self.bridgeListWidget.takeItem(self.bridgeListWidget.currentRow())
		item = None
		#print self.bridgeListWidget.currentItem()
		#for item in self.bridgeListWidget.selectedItems():
		#	self.bridgeListWidget.removeItemWidget(item)
		#for item in self.bridgeListWidget.selectedItems():
		#	print item.text()


	# set device to "device" if deviceLineEdit is changed
	@QtCore.pyqtSignature("QString")
	def on_deviceLineEdit_textChanged(self, t):
		#self.device = "{0}".format(t)
		self.device = "%s" %t

	# set device to bridge if bridgeLineEdit is changed
	@QtCore.pyqtSignature("QString")
	def on_bridgeLineEdit_textChanged(self, t):
		#self.device = "{0}".format(t)
		self.device = "%s" %t



	#################################
	### UPDATE NETWORK PROPERTIES ###
	#################################

	def updateProperties(self):
		#print "Update!"

		#device = 'eth0'
		device = self.device

		# delay, delay distribution, loss (packets), corruption (bits),
		# packet re-ordering, and data rate of the emulated device
		delay = self.delaySlider.value()
		delay_var = self.delay_varSlider.value()
		delay_dist = "normal"
		loss = self.lossSlider.value()
		loss_corr = self.loss_corrLineEdit.text().toInt()[0]
		corruption = 0
		
		# reordering modes
		if self.reorderCheckBox.isChecked() == True: # mode 1
			reorder = self.reorderSlider.value()
			reorder_corr = self.reorder_corrLineEdit.text().toInt()[0]
			reorder_gap = 0
		else: # mode 2 (gap)
			reorder = 0
			reorder_corr = 0
			reorder_gap = self.reorder_gapLineEdit.text().toInt()[0]

		duplicate = self.duplicateSlider.value()
		rate = self.rateLineEdit.text().toInt()[0]
		buffer = self.bufferLineEdit.text().toInt()[0]
		limit = self.limitLineEdit.text().toInt()[0]

		# set properties string, if device is present or abort
		if device != "":
			#prop_string = "tc qdisc add dev {0} root handle 1: netem ".format(device)
			#prop_string = "tc qdisc add dev %s root handle 1: netem " %device
			prop_string = "tc qdisc add dev %s parent 1:3 handle 30: netem " %device # filtered traffic
		else:
			return
	
		# set delay
		if delay > 0:
			#prop_string += "delay {0}ms ".format(delay)
			prop_string += "delay %sms " %delay
			# set delay variation
			if delay_var > 0:
				#prop_string += "{0}ms ".format(delay_var)
				prop_string += "%sms " %delay_var
			
				# set delay distribution
				if delay_dist != "normal":
					#prop_string += "distribution {0} ".format(delay_var)
					prop_string += "distribution %s " %delay_var
				else:
					prop_string += "distribution normal "
		else:
			# set the delay to 0ms
			prop_string += "delay 0ms "

		# set message reordering, mode 1
		if reorder > 0:
			#prop_string += "reorder {0}% ".format(reorder)
			prop_string += "reorder %s%% " %reorder
			if reorder_corr > 0:
				#prop_string += "{0}% ".format(reorder_corr)
				prop_string += "%s%% " %reorder_corr
		# set message reodering, mode 2 (gap)
		if reorder_gap > 0:
			#prop_string += "reorder gap {0} ".format(reorder_gap)
			prop_string += "reorder gap %s " %reorder_gap

		# set message loss
		if loss > 0:
			#prop_string += "loss {0}% ".format(loss)
			prop_string += "loss %s%% " %loss
			
			self.loss = loss # store loss for later usage
			
			if loss_corr > 0:
				#prop_string += "{0}% ".format(loss_corr)
				prop_string += "%s%% " %loss_corr
		else:
			self.loss = 0

		# set message duplication
		if duplicate > 0:
			#prop_string += "duplicate {0}% ".format(duplicate)
			prop_string += "duplicate %s%% " %duplicate

		#self.prop_string = prop_string
		port = "50779"
		self.prop_string = "tc qdisc add dev %s root handle 1:0 prio\n" %device + prop_string + "\ntc filter add dev %s protocol ip parent 1:0 prio 1 u32 match ip dport %s 0xffff flowid 1:3" %(device, port) # filtered traffic, TODO: extra variables for the extra strings?
		print_string = prop_string

		# set data rate string
		if rate != 0 and buffer != 0 and limit != 0:
			#rate_string = "tc qdisc add dev {0} parent 1:1 handle 10: tbf rate {1}kbit buffer {2} limit {3}".format(device, rate, buffer, limit)
			#rate_string = "tc qdisc add dev %s parent 1:1 handle 10: tbf rate %skbit buffer %s limit %s" %(device, rate, buffer, limit)
			rate_string = "tc qdisc add dev %s parent 30:1 handle 300: tbf rate %skbit buffer %s limit %s" %(device, rate, buffer, limit) # filtered traffic
			print_string += "\n\n" + rate_string
			self.rate_string = rate_string

	
		self.displayTextEdit.clear()	
		self.displayTextEdit.setText(print_string)

		
		#self.del_string = "tc qdisc del dev %s parent 1:1\ntc qdisc del dev %s root " %(device,device)
		self.del_string = "tc qdisc del dev %s parent 300:1\ntc qdisc del dev %s parent 30:1\ntc qdisc del dev %s root" %(device,device,device)


	###############
	### BUTTONS ###
	###############

	#this line makes sure that we are only looking for a string
	#if we want a bool as well we can add 	@QtCore.pyqtSignature("bool")
	# set button clicked

	# set network properties
	@QtCore.pyqtSignature("")
	def on_setButton_clicked(self):
		print self.del_string
		print self.prop_string
		print self.rate_string

		os.system(self.del_string)
		os.system(self.prop_string)
		os.system(self.rate_string)

		# ssh string hack
		sshcmd = "echo %s > %s" %(self.loss,self.sshLossFile)
		sshstring1 = "ssh %s%s \"%s\"" %(self.sshUser,self.sshHost1,sshcmd)
		sshstring2 = "ssh %s%s \"%s\"" %(self.sshUser,self.sshHost2,sshcmd)
		print sshstring1,sshstring2

		os.system(sshstring1)
		os.system(sshstring2)

	# clear button clicked, delete properties
	@QtCore.pyqtSignature("")
	def on_clearButton_clicked(self):
		print self.del_string

		self.rate_string = ""
		self.prop_string = ""
		self.displayTextEdit.clear()
		os.system(self.del_string)
		self.del_string = ""

		# ssh string hack
		self.loss = 0
		sshhost1 = self.sshHost1
		sshhost2 = self.sshHost2
		sshcmd = "echo %s > %s" %(self.loss,self.sshLossFile)
		sshstring1 = "ssh %s%s \"%s\"" %(self.sshUser,self.sshHost1,sshcmd)
		sshstring2 = "ssh %s%s \"%s\"" %(self.sshUser,self.sshHost2,sshcmd)
		print sshstring1,sshstring2

		os.system(sshstring1)
		os.system(sshstring2)

	#############
	### DELAY ###
	#############

	# delay slider changed
	@QtCore.pyqtSignature("int")
	def on_delaySlider_valueChanged(self, i):
		#print "delay slider moved", i
		#self.delayLineEdit.setText("{0}".format(i))
		self.delayLineEdit.setText("%i" %i)
		self.updateProperties()

	# delay line edit changed
	@QtCore.pyqtSignature("QString")
	def on_delayLineEdit_textChanged(self, t):
		value = t.toInt()
		if value[1] == True:
			self.delaySlider.setValue(value[0])
			self.updateProperties()
	
	# delay var slider changed
	@QtCore.pyqtSignature("int")
	def on_delay_varSlider_valueChanged(self, i):
		#self.delay_varLineEdit.setText("{0}".format(i))
		self.delay_varLineEdit.setText("%i" %i)
		self.updateProperties()

	# delay var line edit changed
	@QtCore.pyqtSignature("QString")
	def on_delay_varLineEdit_textChanged(self, t):
		value = t.toInt()
		if value[1] == True:
			self.delay_varSlider.setValue(value[0])
			self.updateProperties()

	############
	### LOSS ###
	############	

	# loss slider changed
	@QtCore.pyqtSignature("int")
	def on_lossSlider_valueChanged(self, i):
		#print "delay slider moved", i
		#self.lossLineEdit.setText("{0}".format(i))
		self.lossLineEdit.setText("%i" %i)
		self.updateProperties()

	# loss line edit changed
	@QtCore.pyqtSignature("QString")
	def on_lossLineEdit_textChanged(self, t):
		value = t.toInt()
		if value[1] == True:
			self.lossSlider.setValue(value[0])
			self.updateProperties()

	# loss correlation line edit changed
	@QtCore.pyqtSignature("QString")
	def on_loss_corrLineEdit_textChanged(self, t):
		value = t.toInt()
		if value[1] == True:
			self.updateProperties()

	###############
	### REORDER ###	
	###############


	# check boxes
	@QtCore.pyqtSignature("bool")
	def on_reorderCheckBox_toggled(self, checked):
		self.reorder_gapCheckBox.setChecked(not checked)
		
		if checked == True:
			self.reorderSlider.setEnabled(True)
			self.reorderLineEdit.setEnabled(True)
			self.reorder_corrLineEdit.setEnabled(True)
		else:
			self.reorderSlider.setEnabled(False)
			self.reorderLineEdit.setEnabled(False)
			self.reorder_corrLineEdit.setEnabled(False)
	
		self.updateProperties()
	
	@QtCore.pyqtSignature("bool")
	def on_reorder_gapCheckBox_toggled(self, checked):
		self.reorderCheckBox.setChecked(not checked)
		
		if checked == True:
			self.reorder_gapLineEdit.setEnabled(True)
		else:
			self.reorder_gapLineEdit.setEnabled(False)

		self.updateProperties()

	# reorder slider changed
	@QtCore.pyqtSignature("int")
	def on_reorderSlider_valueChanged(self, i):
		#self.reorderLineEdit.setText("{0}".format(i))
		self.reorderLineEdit.setText("%i" %i)
		self.updateProperties()

	# reorder line edit changed
	@QtCore.pyqtSignature("QString")
	def on_reorderLineEdit_textChanged(self, t):
		value = t.toInt()
		if value[1] == True:
			self.reorderSlider.setValue(value[0])
			self.updateProperties()

	# reorder correlation line edit changed
	@QtCore.pyqtSignature("QString")
	def on_reorder_corrLineEdit_textChanged(self, t):
		value = t.toInt()
		if value[1] == True:
			self.updateProperties()


	# reorder gap changed
	@QtCore.pyqtSignature("QString")
	def on_reorder_gapLineEdit_textChanged(self, t):
		#print "gap changed", t
		self.updateProperties()
	

	#################
	### DUPLICATE ###
	#################	

	# duplicate slider changed
	@QtCore.pyqtSignature("int")
	def on_duplicateSlider_valueChanged(self, i):
		#self.duplicateLineEdit.setText("{0}".format(i))
		self.duplicateLineEdit.setText("%i" %i)
		self.updateProperties()

	# duplicate line edit changed
	@QtCore.pyqtSignature("QString")
	def on_duplicateLineEdit_textChanged(self, t):
		value = t.toInt()
		if value[1] == True:
			self.duplicateSlider.setValue(value[0])
			self.updateProperties()

	
	#################
	### DATA RATE ###
 	#################
	
	# data rate line edit changed
	@QtCore.pyqtSignature("QString")
	def on_rateLineEdit_textChanged(self, t):
		self.updateProperties()
 
	# buffer line edit changed
	@QtCore.pyqtSignature("QString")
	def on_bufferLineEdit_textChanged(self, t):
		self.updateProperties()
 
	# limit line edit changed
	@QtCore.pyqtSignature("QString")
	def on_limitLineEdit_textChanged(self, t):
		self.updateProperties()


	
 
app = QtGui.QApplication(sys.argv)
widget = NetworkProperties()
widget.show()
app.exec_()
