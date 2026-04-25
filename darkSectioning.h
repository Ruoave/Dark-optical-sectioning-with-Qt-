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
    
    // 公共数据成员：供MainWindow访问处理前后的图像数据
    std::vector<cv::Mat> imageStack;              // 处理前图像栈（OpenCV Mat数据）
    std::vector<cv::Mat> final_images;            // 处理后图像栈（OpenCV Mat数据）
    
private:
    Ui::MainWindow *ui;
    
    // 辅助函数
    void getImageDimensions(const cv::Mat &image, int &Nx, int &Ny, int &Nc);
};

#endif // DARKSECTIONING_H
