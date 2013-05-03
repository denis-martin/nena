#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <testthreads.h>
#include <QMutex>
#include <QString>

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

	SharedMemoryTestThread mt;
	SocketTestThread st;

	QMutex logMutex;

private slots:
    void on_checkBox_Socket_toggled(bool checked);
    void on_pushButton_clicked();

	void testFinished (QString str);
};

#endif // MAINWINDOW_H
