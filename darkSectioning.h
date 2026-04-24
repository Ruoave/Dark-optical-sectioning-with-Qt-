#ifndef DARKSECTIONING_H
#define DARKSECTIONING_H

#include <QMainWindow>
#include "ui_mainwindow.h"
#include "params.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <vector>

class DarkSectioning
{
public:
    explicit DarkSectioning(Ui::MainWindow *ui);
    ~DarkSectioning();
    
    void process();
    
private:
    Ui::MainWindow *ui;
    
    // 辅助函数
    void getImageDimensions(const cv::Mat &image, int &Nx, int &Ny, int &Nc);
};

#endif // DARKSECTIONING_H
