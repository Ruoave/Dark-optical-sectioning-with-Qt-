#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>
#include "ViewMat.h"

using namespace cv;
using namespace std;

// 大气光计算函数
// 输入：
//   image - 输入图像
//   dark_channel - 暗通道图像
// 输出：
//   atmosphere - 大气光值（标量）

// Copyright (c) 2014 Stephen Tierney

double get_atmosphere(Mat image, Mat dark_channel)
{
        //checked ViewMat(image, "image_In__get_atmosphere");
        //checked ViewMat(dark_channel, "dark_channel_In__get_atmosphere");
    
    // 参数初始化
    // 对应Matlab代码：[m, n, ~] = size(image);
    int m = image.rows;
    int n = image.cols;
    int n_pixels = m * n;
    
    // 计算搜索像素数量（总像素的1%）
    // 对应Matlab代码：n_search_pixels = floor(n_pixels * 0.01);
    int n_search_pixels = static_cast<int>(floor(n_pixels * 0.01));
    
    // 确保至少有一个像素
    if (n_search_pixels < 1) {
        n_search_pixels = 1;
    }
    
    // 将暗通道和图像转换为向量（按列优先，与Matlab一致）
    // 对应Matlab代码：dark_vec = reshape(dark_channel, n_pixels, 1);
    // 对应Matlab代码：image_vec = reshape(image, n_pixels,1);
    Mat dark_vec = Mat::zeros(n_pixels, 1, CV_64F);
    Mat image_vec = Mat::zeros(n_pixels, 1, CV_64F);
    
    // 按列优先顺序填充向量（Matlab风格）
    for (int col = 0; col < n; col++) {
        for (int row = 0; row < m; row++) {
            int idx = col * m + row;  // 列优先索引计算
            dark_vec.at<double>(idx, 0) = dark_channel.at<double>(row, col);
            image_vec.at<double>(idx, 0) = image.at<double>(row, col);
        }
    }
    
        //ViewMat(dark_vec, "dark_vec");
        //ViewMat(image_vec, "image_vec");
    // 创建索引数组
    vector<int> indices(n_pixels);
    for (int i = 0; i < n_pixels; i++) {
        indices[i] = i;
    }
    
    // 对暗通道值进行降序排序，获取索引
    // 对应Matlab代码：[~, indices] = sort(dark_vec, 'descend');
    std::sort(indices.begin(), indices.end(), [&dark_vec](int a, int b) {
        return dark_vec.at<double>(a, 0) > dark_vec.at<double>(b, 0);
    });//这里用的是std::sort，indices结果与matlab有差异

    
    // 计算大气光值：取暗通道值最大的1%像素的平均值
    // 对应Matlab代码：accumulator = 0;
    // 对应Matlab代码：for k = 1 : n_search_pixels
    // 对应Matlab代码：    accumulator = accumulator + image_vec(indices(k),:);
    // 对应Matlab代码：end
    // 对应Matlab代码：atmosphere = accumulator / n_search_pixels;
    double accumulator = 0.0;
    
    for (int k = 0; k < n_search_pixels; k++) {  // 注意：C++索引从0开始
        int idx = indices[k];
        accumulator += image_vec.at<double>(idx, 0);
    }
    
    double atmosphere_value_result = accumulator / n_search_pixels;
        cout << "atmosphere: " << atmosphere_value_result << endl;

    return atmosphere_value_result;
}
