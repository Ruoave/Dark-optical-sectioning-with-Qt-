#include <opencv2/opencv.hpp>
#include "ViewMat.h"

using namespace cv;
using namespace std;

// 窗口求和滤波函数
// 输入：
//   image - 输入图像
//   r - 窗口半径
// 输出：
//   sum_img - 窗口求和结果
// 功能：
//   sum_img(x, y) = sum(sum(image(x-r:x+r, y-r:y+r)));

Mat window_sum_filter(const Mat& image, int r) {

    int h = image.rows;
    int w = image.cols;
    // 使用双精度浮点型进行计算，确保与guided_filter的CV_64F类型一致
    Mat sum_img = Mat::zeros(h, w, CV_64F);

    
    // ***对应 Matlab: im_cum = cumsum(image, 1);
    // **1.计算Y轴方向的累积和
    Mat im_cum = Mat::zeros(h, w, CV_64F);
    for (int j = 0; j < w; j++) {
        im_cum.at<double>(0, j) = image.at<double>(0, j);
        for (int i = 1; i < h; i++) {
            im_cum.at<double>(i, j) = im_cum.at<double>(i-1, j) + image.at<double>(i, j); // 对应 Matlab 的 cumsum(image, 1)
        }
    }
        //ViewMat(im_cum, "im_cum__In__window_sum_filter_y");
    
    // ***对应 Matlab: sum_img(1:r+1, :) = im_cum(1+r:2*r+1, :);
    // **2.处理顶部边界
    for (int i = 0; i <= r; i++) {
        for (int j = 0; j < w; j++) {
            sum_img.at<double>(i, j) = im_cum.at<double>(i + r, j); // 对应 Matlab 的 sum_img(1:r+1, :) = im_cum(1+r:2*r+1, :)
        }
    }
        //ViewMat(sum_img, "sum_img__In__window_sum_filter__2");
    
    // ***对应 Matlab: sum_img(r+2:h-r, :) = im_cum(2*r+2:h, :) - im_cum(1:h-2*r-1, :);
    // **3.处理中间区域
    for (int i = r + 1; i < h - r; i++) {
        for (int j = 0; j < w; j++) {
            sum_img.at<double>(i, j) = im_cum.at<double>(i + r, j) - im_cum.at<double>(i - r - 1, j); // 对应 Matlab 的 sum_img(r+2:h-r, :) = im_cum(2*r+2:h, :) - im_cum(1:h-2*r-1, :)
        }
    }
        //ViewMat(sum_img, "sum_img__In__window_sum_filter__3");
    
    // ***对应 Matlab: sum_img(h-r+1:h, :) = repmat(im_cum(h, :), [r, 1]) - im_cum(h-2*r:h-r-1, :);
    // **4.处理底部边界
    for (int i = h - r; i < h; i++) {
        for (int j = 0; j < w; j++) {
            sum_img.at<double>(i, j) = im_cum.at<double>(h-1, j) - im_cum.at<double>(i - r - 1, j); // 对应 Matlab 的 sum_img(h-r+1:h, :) = repmat(im_cum(h, :), [r, 1]) - im_cum(h-2*r:h-r-1, :)
        }
    }
        //ViewMat(sum_img, "sum_img__In__window_sum_filter__4");
    
    // ***对应 Matlab: im_cum = cumsum(sum_img, 2);
    // **5.计算X轴方向的累积和
    for (int i = 0; i < h; i++) {
        im_cum.at<double>(i, 0) = sum_img.at<double>(i, 0);
        for (int j = 1; j < w; j++) {
            im_cum.at<double>(i, j) = im_cum.at<double>(i, j-1) + sum_img.at<double>(i, j); // 对应 Matlab 的 cumsum(sum_img, 2)
        }
    }
        //ViewMat(im_cum, "im_cum__In__window_sum_filter_y_x");
    
    // ***对应 Matlab: sum_img(:, 1:r+1) = im_cum(:, 1+r:2*r+1);
    // **6.处理左侧边界
    for (int i = 0; i < h; i++) {
        for (int j = 0; j <= r; j++) {
            sum_img.at<double>(i, j) = im_cum.at<double>(i, j + r); // 对应 Matlab 的 sum_img(:, 1:r+1) = im_cum(:, 1+r:2*r+1)
        }
    }
        //ViewMat(sum_img, "sum_img__In__window_sum_filter__6");
    
    // ***对应 Matlab: sum_img(:, r+2:w-r) = im_cum(:, 2*r+2:w) - im_cum(:, 1:w-2*r-1);
    // **7.处理中间区域
    for (int i = 0; i < h; i++) {
        for (int j = r + 1; j < w - r; j++) {
            sum_img.at<double>(i, j) = im_cum.at<double>(i, j + r) - im_cum.at<double>(i, j - r - 1); // 对应 Matlab 的 sum_img(:, r+2:w-r) = im_cum(:, 2*r+2:w) - im_cum(:, 1:w-2*r-1)
        }
    }
        //ViewMat(sum_img, "sum_img__In__window_sum_filter__7");
    
    // ***对应 Matlab: sum_img(:, w-r+1:w) = repmat(im_cum(:, w), [1, r]) - im_cum(:, w-2*r:w-r-1);
    // **8.处理右侧边界
    for (int i = 0; i < h; i++) {
        for (int j = w - r; j < w; j++) {
            sum_img.at<double>(i, j) = im_cum.at<double>(i, w-1) - im_cum.at<double>(i, j - r - 1); // 对应 Matlab 的 sum_img(:, w-r+1:w) = repmat(im_cum(:, w), [1, r]) - im_cum(:, w-2*r:w-r-1)
        }
    }
        //ViewMat(sum_img, "sum_img__In__window_sum_filter__8");
    
    return sum_img;
}
