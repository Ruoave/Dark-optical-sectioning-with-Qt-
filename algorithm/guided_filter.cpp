#include <opencv2/opencv.hpp>
#include "ViewMat.h"

using namespace cv;
using namespace std;

// 窗口求和滤波函数声明
Mat window_sum_filter(const Mat& image, int r);

// 引导滤波函数（用于透射率优化）
// 输入：
//   guide - 引导图像 =image=Lo(图片低频分量)
//   target - 目标图像
//   radius - 滤波窗口半径
//   eps - 正则化参数
// 输出：
//   q - 滤波后的结果

// Copyright (c) 2014 Stephen Tierney

Mat guided_filter(Mat guide, Mat target, int radius, double eps)
{

    int h = guide.rows;
    int w = guide.cols;
    
    // ***对应 Matlab: avg_denom = window_sum_filter(ones(h, w), radius);
    // **1.计算窗口总和的分母
    Mat ones_mat = Mat::ones(h, w, CV_64F);
    Mat avg_denom = window_sum_filter(ones_mat, radius);
        //ViewMat(avg_denom,"avg_denom__In__guided_filter") ;
    
    // ***对应 Matlab: mean_g = window_sum_filter(guide, radius) ./ avg_denom;
    // ***对应 Matlab: mean_t = window_sum_filter(target, radius) ./ avg_denom;
    // **2.计算引导图像和目标图像的均值
    Mat mean_g = window_sum_filter(guide, radius) / avg_denom; 
    Mat mean_t = window_sum_filter(target, radius) / avg_denom; 
        //ViewMat(mean_g,"mean_g__In__guided_filter") ;
        //ViewMat(mean_t,"mean_t__In__guided_filter") ;

    
    // ***对应 Matlab: corr_gg = window_sum_filter(guide .* guide, radius) ./ avg_denom;
    // ***对应 Matlab: corr_gt = window_sum_filter(guide .* target, radius) ./ avg_denom;
    // **3.计算引导图像的自相关和引导图像与目标图像的互相关
    Mat corr_gg = window_sum_filter(guide.mul(guide), radius) / avg_denom; 
    Mat corr_gt = window_sum_filter(guide.mul(target), radius) / avg_denom; 
    
    // ***对应 Matlab: var_g = corr_gg - mean_g .* mean_g;
    // ***对应 Matlab: cov_gt = corr_gt - mean_g .* mean_t;
    // **4.计算引导图像的方差和引导图像与目标图像的协方差
    Mat var_g = corr_gg - mean_g.mul(mean_g); 
    Mat cov_gt = corr_gt - mean_g.mul(mean_t); 
    
    // ***对应 Matlab: a = cov_gt ./ (var_g + eps);
    // ***对应 Matlab: b = mean_t - a .* mean_g;
    // **5.计算线性系数 a 和 b
    Mat a = cov_gt / (var_g + eps); // 对应 Matlab 的 a = cov_gt ./ (var_g + eps)
    Mat b = mean_t - a.mul(mean_g); // 对应 Matlab 的 b = mean_t - a .* mean_g
    
    // ***对应 Matlab: mean_a = window_sum_filter(a, radius) ./ avg_denom;
    // ***对应 Matlab: mean_b = window_sum_filter(b, radius) ./ avg_denom;
    // **6.计算系数的均值
    Mat mean_a = window_sum_filter(a, radius) / avg_denom; // 对应 Matlab 的 mean_a = window_sum_filter(a, radius) ./ avg_denom
    Mat mean_b = window_sum_filter(b, radius) / avg_denom; // 对应 Matlab 的 mean_b = window_sum_filter(b, radius) ./ avg_denom
    
    // ***对应 Matlab: q = mean_a .* guide + mean_b;
    // **7.计算最终的滤波结果
    Mat mean_a_mul_guide;
    multiply(mean_a, guide, mean_a_mul_guide, 1.0, CV_64F);
    Mat q = mean_a_mul_guide + mean_b; 
        //ViewMat(q,"q__In__guided_filter") ;
    
    return q;
}
