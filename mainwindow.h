#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
private slots://信号槽
    void open_pzx_control();
    void close_pzx_control();
    void open_pzx_axis_control();

    void pzx_sendEstop();
    void pzx_sendEstopReset();
    void pzx_sendMachineOn();
    void pzx_sendMachineOff();
    void pzx_sendManual();
    void pzx_sendAuto();
    void pzx_sendMdi();

    void pzx_home();

    void pzx_testopencv();
    void pzx_testvideo();
};

#endif // MAINWINDOW_H
