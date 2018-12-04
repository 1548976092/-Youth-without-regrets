#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include <QtGui/QMainWindow>
//#include <QFileDialog>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <values.h>             // DBL_MAX, maybe
#include <limits.h>             // DBL_MAX, maybe
#include <stdarg.h>
#include <sys/stat.h>           // struct stat, stat()
#include <unistd.h>
#include <fcntl.h>              // O_CREAT
#include <inttypes.h>

#include "rcs.hh"               // etime()
#include "emc.hh"               // EMC NML
#include "emc_nml.hh"
#include "emcglb.h"             // EMC_NMLFILE, TRAJ_MAX_VELOCITY, TOOL_TABLE_FILE
#include "emccfg.h"             // DEFAULT_TRAJ_MAX_VELOCITY
#include "inifile.hh"           // INIFILE
#include "rcs_print.hh"
#include "nml_oi.hh"
#include "timer.hh"

#include <iostream>

//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

#define debug 1
extern int sendEstop();
extern int sendEstopReset();
extern int sendMachineOn();
extern int sendMachineOff();
extern int sendManual();
extern int sendAuto();
extern int sendMdi();                                     //wait
extern int sendJogCont(int axis, double speed);
extern int sendHome(int axis);


void MainWindow::pzx_testopencv()
{
    try{
//        Mat srcImage = imread("pictureopencv.png");rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
//        cvtColor( srcImage, srcImage, CV_BGR2RGB );rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
//        QImage img = QImage( (const unsigned char*)(srcImage.data), srcImage.cols, srcImage.rows, QImage::Format_RGB888 );rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
//        ui->label->setPixmap( QPixmap::fromImage(srcImage) );rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
//        imshow( "Image_Show", srcImage );

        cv::Mat srcImage=cv::imread("/home/dahua/myx86cnc/pictureopencv.png");  rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
        cv::imshow( "Image_Show1", srcImage );              rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
        cv::cvtColor( srcImage, srcImage, CV_BGR2RGB );rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
        cv::imshow( "Image_Show2", srcImage );              rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);

//        Mat src= imread("/home/dahua/myx86cnc/pictureopencv.png");
//        QImage img = QImage( (const unsigned char*)(src.data), src.cols, src.rows, QImage::Format_RGB888 );
//        ui->label->setPixmap( QPixmap::fromImage(img) );
//        rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);

//            Mat src= imread("/home/dahua/myx86cnc/pictureopencv.png");
            QImage img = QImage( (const unsigned char*)(srcImage.data), srcImage.cols, srcImage.rows, QImage::Format_RGB888 );
            ui->label->setPixmap( QPixmap::fromImage(img) );
            rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);


    }catch(cv::Exception & e){
        rcs_print_error("%s(%d)-<%s>Exception:%s \n: ",__FILE__, __LINE__, __FUNCTION__,e.what());
    }

}


void MainWindow::open_pzx_control()
{


//   imshow("srcIMage",srcImage);
#if debug

    sendEstopReset();
    sendMachineOn();
    sendManual();
    rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
#endif
}
void MainWindow::close_pzx_control()
{
#if debug

    sendEstop();
    sendMachineOff();
//    sendManual();
    rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
#endif
}
void MainWindow::pzx_sendEstop()
{
    sendEstop();
    rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
}
void MainWindow::pzx_sendEstopReset()
{
    sendEstopReset();
    rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
}
void MainWindow::pzx_sendMachineOn()
{
    sendMachineOn();
    rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
}
void MainWindow::pzx_sendMachineOff()
{
    sendMachineOff();
    rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
}
void MainWindow::pzx_sendManual()
{
    sendManual();
    rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
}
void MainWindow::pzx_sendAuto()
{
    sendAuto();
    rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
}
void MainWindow::pzx_sendMdi()
{
    sendMdi();
    rcs_print_error("%s(%d)-<%s>\n: ",__FILE__, __LINE__, __FUNCTION__);
}


void MainWindow::open_pzx_axis_control()
{
    int    axis =  0  ;
    double speed=  0;

    QPushButton* sender = (QPushButton*)(QObject::sender());
{
    if(sender == ui->xadd)
    {
        axis =    0;
        speed=  300;
    }
    else if(sender == ui->xless)
    {
        axis =     0;
        speed=  -300;
    }
    else if(sender == ui->yadd)
    {
        axis =    1;
        speed=  300;
    }
    else if(sender == ui->yless)
    {
        axis =    1;
        speed= -300;
    }
    else if(sender == ui->zadd)
    {
        axis =    2;
        speed=  300;
    }
    else if(sender == ui->zless)
    {
        axis =    2;
        speed= -300;
    }
    else if(sender == ui->Aadd)
    {
        axis =    3;
        speed=  300;
    }
    else if(sender == ui->Aless)
    {
        axis =     3;
        speed=  -300;
    }
    else if(sender == ui->Badd)
    {
        axis =    4;
        speed=  300;
    }
    else if(sender == ui->Bless)
    {
        axis =    4;
        speed= -300;
    }
    else if(sender == ui->Cadd)
    {
        axis =    5;
        speed=  300;
    }
    else if(sender == ui->Cless)
    {
        axis =    5;
        speed= -300;
    }
    else if(sender == ui->Dadd)
    {
        axis =    6;
        speed=  300;
    }
    else if(sender == ui->Dless)
    {
        axis =     6;
        speed=  -300;
    }
    else if(sender == ui->Eadd)
    {
        axis =    7;
        speed=  300;
    }
    else if(sender == ui->Eless)
    {
        axis =    7;
        speed= -300;
    }
    else if(sender == ui->Fadd)
    {
        axis =    8;
        speed=  300;
    }
    else if(sender == ui->Fless)
    {
        axis =    8;
        speed= -300;
    }
}
#if debug

    sendJogCont(axis,speed);
    rcs_print_error("%s(%d)-<%s> axis:%d,speed:%f:\n",__FILE__, __LINE__, __FUNCTION__,axis,speed);
#endif
}

void MainWindow::pzx_home()
{
    int i  =0 ;
    for(i = 0;i++;i < 9)
    {
        sendHome(i);
    }

}
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle( "pzx_control");

 //   Mat srcImage = imread("pictureopencv.png");
   // imshow("srcIMage",srcImage);

    //第3个参数表示查找文件时从哪个目录开始，如果为"."的话，表示从该工程目录开始查找，最后那个参数的过滤器的名字之间
        //要用空格，否则识别不出来
//        QString img_name = QFileDialog::getOpenFileName( this, tr("Open Image"), ".",
//                                                         tr("Image Files(*.png *.jpg *.jpeg *.bmp)"));
        //toAscii()返回8位描述的string,为QByteArray,data()表示返回QByteArray的指针，QByteArray为字节指针

//     Mat src;
//        cvtColor( src, src, CV_BGR2RGB );
//        QImage img = QImage( (const unsigned char*)(src.data), src.cols, src.rows, QImage::Format_RGB888 );
    //    namedWindow( "Image_Show", WINDOW_AUTOSIZE );
    //    imshow( "Image_Show", src );
//        ui->label->setPixmap( QPixmap::fromImage(img) );
     //   ui->label->resize( ui->label->pixmap()->size() );
//ui->graphicsView-

    //open button
{
    QObject::connect(ui->on,SIGNAL(clicked(bool)),this,SLOT(open_pzx_control()));
    QObject::connect(ui->off,SIGNAL(clicked(bool)),this,SLOT(close_pzx_control()));

    QObject::connect(ui->EstopOn,SIGNAL(clicked(bool)),this,SLOT(pzx_sendEstop()));
    QObject::connect(ui->EstopOff,SIGNAL(clicked(bool)),this,SLOT(pzx_sendEstopReset()));
    QObject::connect(ui->MachineOn,SIGNAL(clicked(bool)),this,SLOT(pzx_sendMachineOn()));
    QObject::connect(ui->MachineOff,SIGNAL(clicked(bool)),this,SLOT(pzx_sendMachineOff()));
    QObject::connect(ui->manual,SIGNAL(clicked(bool)),this,SLOT(pzx_sendManual()));
    QObject::connect(ui->automode,SIGNAL(clicked(bool)),this,SLOT(pzx_sendAuto()));
    QObject::connect(ui->mdi,SIGNAL(clicked(bool)),this,SLOT(pzx_sendMdi()));
}
    //axis
    {
        QObject::connect(ui->xadd,SIGNAL(clicked(bool)),this,SLOT( open_pzx_axis_control()));
        QObject::connect(ui->xless,SIGNAL(clicked(bool)),this,SLOT(open_pzx_axis_control()));
        QObject::connect(ui->yadd,SIGNAL(clicked(bool)),this,SLOT( open_pzx_axis_control()));
        QObject::connect(ui->yless,SIGNAL(clicked(bool)),this,SLOT(open_pzx_axis_control()));
        QObject::connect(ui->zadd,SIGNAL(clicked(bool)),this,SLOT( open_pzx_axis_control()));
        QObject::connect(ui->zless,SIGNAL(clicked(bool)),this,SLOT(open_pzx_axis_control()));

        QObject::connect(ui->Aadd,SIGNAL(clicked(bool)),this,SLOT( open_pzx_axis_control()));
        QObject::connect(ui->Aless,SIGNAL(clicked(bool)),this,SLOT(open_pzx_axis_control()));
        QObject::connect(ui->Badd,SIGNAL(clicked(bool)),this,SLOT( open_pzx_axis_control()));
        QObject::connect(ui->Bless,SIGNAL(clicked(bool)),this,SLOT(open_pzx_axis_control()));
        QObject::connect(ui->Cadd,SIGNAL(clicked(bool)),this,SLOT( open_pzx_axis_control()));
        QObject::connect(ui->Cless,SIGNAL(clicked(bool)),this,SLOT(open_pzx_axis_control()));

        QObject::connect(ui->Dadd,SIGNAL(clicked(bool)),this,SLOT( open_pzx_axis_control()));
        QObject::connect(ui->Dless,SIGNAL(clicked(bool)),this,SLOT(open_pzx_axis_control()));
        QObject::connect(ui->Eadd,SIGNAL(clicked(bool)),this,SLOT( open_pzx_axis_control()));
        QObject::connect(ui->Eless,SIGNAL(clicked(bool)),this,SLOT(open_pzx_axis_control()));
        QObject::connect(ui->Fadd,SIGNAL(clicked(bool)),this,SLOT( open_pzx_axis_control()));
        QObject::connect(ui->Fless,SIGNAL(clicked(bool)),this,SLOT(open_pzx_axis_control()));
    }

    QObject::connect(ui->home,SIGNAL(clicked(bool)),this,SLOT(pzx_home()));

    //opencv
    {
        QObject::connect(ui->testopencv,SIGNAL(clicked(bool)),this,SLOT(pzx_testopencv()));

    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
