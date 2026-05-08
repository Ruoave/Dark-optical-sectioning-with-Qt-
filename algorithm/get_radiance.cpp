#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// 辐照度恢复函数
// 输入：
//   rep_atmosphere - 重复的大气光值
//   image - 输入图像
//   transmission - 透射率
// 输出：
//   radiance - 去雾后的辐照度

// Copyright (c) 2014 Stephen Tierney

Mat get_radiance(Mat rep_atmosphere, Mat image, Mat transmission)
{
    // 检查输入是否为空
    if (image.empty() || transmission.empty() || rep_atmosphere.empty()) {
        cout <<"[[[for test]]]:"<< "get_radiance.cpp:21 - input is empty" << endl;
        return Mat();
    }
    
    // 确保输入图像和透射率尺寸一致
    if (image.size() != transmission.size()) {
        cout <<"[[[for test]]]:"<< "get_radiance.cpp:26 - size mismatch" << endl;
        return Mat();
    }
    
    // 参数初始化
    // 对应Matlab代码：[m, n, ~] = size(image);
    int m = image.rows;
    int n = image.cols;
    
    // 限制最小透射率为0.1，避免分母过小
    // 对应Matlab代码：max_transmission = max(transmission, 0.1);
    Mat max_transmission;
    max(transmission, 0.1, max_transmission);
    
    // 计算去雾后的辐照度
    // 对应Matlab代码：radiance = ((image - rep_atmosphere) ./ max_transmission) + rep_atmosphere;
    Mat radiance;
    subtract(image, rep_atmosphere, radiance);  // image - rep_atmosphere
    divide(radiance, max_transmission, radiance);  // (image - rep_atmosphere) ./ max_transmission
    add(radiance, rep_atmosphere, radiance);  // ((image - rep_atmosphere) ./ max_transmission) + rep_atmosphere
    
    // 确保结果在有效范围内
    threshold(radiance, radiance, 255.0, 255.0, THRESH_TRUNC);  // 截断最大值为255
    threshold(radiance, radiance, 0.0, 0.0, THRESH_TOZERO);  // 截断最小值为0
    
    return radiance;
}