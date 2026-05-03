#ifndef DARKSECTIONING_H
#define DARKSECTIONING_H

#include <QMainWindow>
#include "ui_mainwindow.h"
#include "paramsBasic.h"
#include "orangebar.h"
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
    
    // 设置OrangeBar指针（供进度更新时直接调用setProgress）
    // 参数：bar - OrangeBar控件指针
    // 调用时机：MainWindow构造函数中，darkSectioning创建后
    void setOrangeBar(OrangeBar *bar);
    
    // 基本算法参数，供 MainWindow 从 GreenWidget 控件读取后设置
    ParamsBasic paramsBasicSet;
    
    // 公共数据成员：供MainWindow访问处理前后的图像数据
    std::vector<cv::Mat> imageStack;              // 处理前图像栈（OpenCV Mat数据）
    std::vector<cv::Mat> final_images;            // 处理后图像栈（OpenCV Mat数据）
    
private:
    Ui::MainWindow *ui;
    OrangeBar *m_orangeBar;                       // OrangeBar指针，用于直接更新进度条
    double progressValue_calcu;                   // 浮点累加器（精确进度值）
    double progressValue_step;                    // 主循环每帧步进值 = 87.0 / (maxtime * Nc * Nz)
    
    // 辅助函数
    void getImageDimensions(const cv::Mat &image, int &Nx, int &Ny, int &Nc);
};

#endif // DARKSECTIONING_H
