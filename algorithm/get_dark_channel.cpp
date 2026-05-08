#include <opencv2/opencv.hpp>
#include "ViewMat.h"

using namespace cv;
using namespace std;

// 3D暗通道估计函数
// 输入：
//   image - 不是原始图像，而是dehaze_fast2在输入，即Lo经过或不经过处理的Mat，Lo为separateHiLo的输出Lo - 低频分量
//   win_size - 窗口大小
// 输出：
//   dark_channel - 暗通道图像

Mat get_dark_channel(Mat image, int win_size)
{
    // 检查输入是否为空
    // 对应Matlab代码：if isempty(image)
    if (image.empty()) {
        cout <<"[[[for test]]]:"<< "get_dark_channel.cpp:16 - image is empty" << endl;
        return Mat();
    }
    
    // 获取图像尺寸
    // 对应Matlab代码：[m, n, ~] = size(image);
    int m = image.rows;
    int n = image.cols;
    
    // 计算padding大小
    // 对应Matlab代码：pad_size = floor(win_size/2);
    int pad_size = static_cast<int>(floor(win_size / 2.0));
    
    // 对图像进行padding，使用最大值作为填充值（对应Matlab的Inf）
    // 对应Matlab代码：padded_image = padarray(image, [pad_size pad_size], Inf);
    Mat padded_image;
    copyMakeBorder(image, padded_image, pad_size, pad_size, pad_size, pad_size, 
                   BORDER_CONSTANT, Scalar::all(1e10));  // 使用大数值模拟Inf
    
    // 方法：使用形态学腐蚀操作
    // 1.计算每个像素在所有通道中的最小值
    // 对应Matlab代码：dark_channel_temp(k) = min(patch(:));
    Mat min_channels;
    if (padded_image.channels() == 1) {
        // 单通道图像，直接使用
        min_channels = padded_image.clone();
    } else {
        // 多通道图像，计算每个像素在所有通道中的最小值
        min_channels = Mat::zeros(padded_image.size(), CV_64F);
        vector<Mat> channels;
        split(padded_image, channels);
        
        // 1.1初始化为第一个通道
        channels[0].convertTo(min_channels, CV_64F);
        
        // 1.2计算所有通道的最小值
        for (size_t c = 1; c < channels.size(); c++) {
            Mat channel_double;
            channels[c].convertTo(channel_double, CV_64F);
            min(min_channels, channel_double, min_channels);  // 对应Matlab的min操作
        }
    }
    
    // 2.创建结构元素（对应Matlab代码中的窗口操作）
    // 对应Matlab代码：patch = padded_image(j : j + (win_size-1), i : i + (win_size-1), :);
    Mat kernel = getStructuringElement(MORPH_RECT, Size(win_size, win_size));
    
    // 3.使用腐蚀操作计算最小值（相当于窗口最小值滤波）
    // 对应Matlab代码：dark_channel_temp(k) = min(patch(:));
    Mat dark_channel_padded;
    erode(min_channels, dark_channel_padded, kernel);
    
    // 4.提取中心区域（去除padding）
    // 对应Matlab代码：dark_channel(floor((k+m-1)/m),mod(k+m-1,m)+1) = dark_channel_temp(k);
    Rect roi(pad_size, pad_size, n, m);
    Mat dark_channel = dark_channel_padded(roi).clone();
    
    return dark_channel;
}
