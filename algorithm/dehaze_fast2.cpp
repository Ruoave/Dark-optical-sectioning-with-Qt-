#include <opencv2/opencv.hpp>
#include "ViewMat.h"
#include <iostream>

using namespace cv;
using namespace std;

// 暗通道去雾函数
// 输入：
//   image - 输入图像，为Lo，Lo为separateHiLo的输出Lo-低频分量
//   omega - 雾度保留因子
//   win_size - 窗口大小
//   EL - 极低频分量
//   dep - 深度参数 // mainwindow.cpp中定义，dep 取自 dep_matrix 数组，background=1 (严重背景) 时 dep_matrix={3,3,2}，background=0 (中等背景) 时 dep_matrix={3}
//   thres - 阈值
// 输出：
//   radiance - 去雾后的图像

// Copyright (c) 2014 Stephen Tierney

Mat get_dark_channel(Mat image, int win_size);
double get_atmosphere(Mat image, Mat dark_channel);
Mat get_transmission_estimate(Mat atmosphere, Mat image, double omega, int win_size);
Mat guided_filter(Mat I, Mat p, int r, double eps);
Mat get_radiance(Mat atmosphere, Mat image, Mat transmission);

Mat dehaze_fast2(Mat image, double omega, int win_size, Mat EL, double dep, int thres)
{
    int Nx = image.rows;
    int Ny = image.cols;

    if (omega == 0) {//改行不对劲，matlab为if ~exist('omega', 'var')
        omega = 0.95;
    }

    if (win_size == 0) {//改行不对劲，matlab为if ~exist('win_size', 'var')
        win_size = 15;
    }

    int r = 15; // 引导滤波窗口大小
    double res = 0.001; // 引导滤波正则化参数

    // 创建掩码
    Mat Mask = Mat::zeros(Nx, Ny, CV_64F);
    for (int i = 0; i < Nx; i++) {
        for (int j = 0; j < Ny; j++) {
            if (image.at<double>(i, j) < thres) {
                Mask.at<double>(i, j) = 1;
            }
        }
    }

    //*** 暗通道估计和大气光计算（低频分量Lo掩码区域）
    Mat image_masked = image.mul(Mask);
    Mat dark_channel = get_dark_channel(image_masked, win_size);   
 //////////////////////到这里都是完全一致的，但排序索引一致难搞(暗通道重复元素多)，故而atmosphere值不一致//////////////////   
    double min_atmosphere = get_atmosphere(image_masked, dark_channel);
    //此处测试相差0.0016，0.004%，c++60.5845，matlab60.5861

    //*** 暗通道估计和大气光计算（整个低频分量Lo）
    dark_channel = get_dark_channel(image, win_size);
    double max_atmosphere = get_atmosphere(image, dark_channel);
    //此处测试相差0.0004，c++148.583，matlab148.5834

        //ViewMat(image, "image_IsLo__In__dehaze_fast2__For_get_atmosphere");

    //*** 处理极低频分量并计算大气光，对应matlab代码: EL = EL - min(min(EL));
    double minEL_0, maxEL_0;
    minMaxLoc(EL, &minEL_0, &maxEL_0);
    EL = EL - minEL_0;
    
    //*** 获取大气光值（标量）
    // 计算rep_atmosphere_process

    double minEL_1, maxEL_1;
    minMaxLoc(EL, &minEL_1, &maxEL_1);
    Mat rep_atmosphere_process = EL / maxEL_1 * (max_atmosphere - min_atmosphere) + min_atmosphere;
    rep_atmosphere_process = dep * rep_atmosphere_process;
        //有差别，但应该是因为atmosphere值不一致，不是该部分代码问题

    //ViewMat(image, "image_IsLo__In__dehaze_fast2__For_Transmission_Estimate");
    //*** 透射率初始估计
    Mat trans_est = get_transmission_estimate(rep_atmosphere_process, image, omega, win_size);
        //ViewMat(trans_est, "trans_est__In__dehaze_fast2");
        //基本一致        trans_est min: 0.632729, max: 1
        //trans_est min: 0.606203, max: 1.00518
    
    //*** 透射率优化
    //** 引导滤波
    Mat x = guided_filter(image, trans_est, r, res); // r为引导滤波窗口大小（15，第40行），res为引导滤波正则化参数（0.001，第41行）
        //有差别，小数点第四位不同   经检验guided_filter结果不对
    //** 透射率优化结果
    Mat transmission = x.reshape(0, image.rows);    
    //*** 辐照度恢复
    Mat radiance = get_radiance(rep_atmosphere_process, image, transmission);




    return radiance;
}
