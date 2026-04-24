#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>
#include "ViewMat.h"

using namespace cv;
using namespace std;

// 透射率初始估计函数
// 输入：
//   rep_atmosphere - 重复的大气光值
//   image - 输入图像,为Lo(图像低频分量)
//   omega - 雾度保留因子
//   win_size - 窗口大小
// 输出：
//   trans_est - 初始透射率估计

// Copyright (c) 2014 Stephen Tierney


Mat get_dark_channel(Mat image, int win_size);

Mat get_transmission_estimate(Mat rep_atmosphere, Mat image, double omega, int win_size)
{
    // 获取图像尺寸
    // ***对应 Matlab: [m, n, ~] = size(image);
    int m = image.rows;
    int n = image.cols;
    
    // 计算归一化图像
    // ***对应 Matlab: normalized_image = image ./ rep_atmosphere;
    Mat normalized_image = image / rep_atmosphere;
    
    // 计算归一化图像的暗通道
    // ***对应 Matlab: normalized_dark_channel = get_dark_channel(normalized_image, win_size);
    Mat normalized_dark_channel = get_dark_channel(normalized_image, win_size);
    
    // 计算初始透射率
    // ***对应 Matlab: trans_est = 1 - omega * normalized_dark_channel;
    Mat trans_est = 1.0 - omega * normalized_dark_channel;
    
    return trans_est;
}
