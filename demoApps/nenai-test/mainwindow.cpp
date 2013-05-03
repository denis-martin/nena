#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <string>
#include <QDebug>

using std::string;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	connect (&mt, SIGNAL(testFinished(QString)), this, SLOT(testFinished(QString)), Qt::QueuedConnection);
	connect (&st, SIGNAL(testFinished(QString)), this, SLOT(testFinished(QString)), Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
	size_t testSize = 0;
	size_t packetSize = 0;
	string socketPath = "boost_sock";
	bool extTest = false;

	testSize = ui->spinBox_testSize->value ();
	packetSize = ui->spinBox_packetSize->value ();
	socketPath = ui->lineEdit_SocketPath->text ().toStdString();
	extTest = ui->radioButton_extTest->isChecked ();

	qDebug () << "Read Options, starting threads.";

	if (ui->checkBox_SharedMemory->isChecked())
	{
		mt.wait();

		qDebug () << "Waited for MemNenai.";

		mt.setPacketSize(packetSize);
		mt.setTestSize(testSize);

		mt.start();

		qDebug () << "Started MemNenai Thread.";
	}

	if (ui->checkBox_Socket->isChecked())
	{
		st.wait();

		qDebug () << "Waited for SocketNenai.";

		st.setPacketSize(packetSize);
		st.setTestSize(testSize);
		st.setSocketPath(socketPath);

		st.start();

		qDebug () << "Started SocketNenai Thread.";
	}

}

void MainWindow::on_checkBox_Socket_toggled(bool checked)
{
    if (checked)
        ui->lineEdit_SocketPath->setEnabled(true);
    else
        ui->lineEdit_SocketPath->setEnabled(false);
}

void MainWindow::testFinished (QString str)
{
	ui->log->textCursor().insertText(str);
}
