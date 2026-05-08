#include <opencv2/opencv.hpp>
#include "port_matlab2opencv.h"
#include <cmath>
#include <ViewMat.h>

using namespace cv;
using namespace std;

// 点扩散函数（PSF）生成函数
// 输入：
//   lambada - 波长
//   pixelsize - 像素大小
//   NA - 数值孔径
//   w - 图像宽度
//   factor - 缩放因子
// 输出：
//   PSF - 点扩散函数
Mat PSF_Generator(double lambada, double pixelsize, double NA, int w, double factor) {
    // 生成网格。对应Matlab代码：[X,Y]=meshgrid(linspace(0,w-1,w),linspace(0,w-1,w));
    vector<int> a = MYlinspace(0, w-1, w);
    vector<int> b = MYlinspace(0, w-1, w);
    Mat X, Y;
    MYmeshgrid_2(a, b, X, Y);
    
    // 计算缩放因子
    double scale = 2 * CV_PI * NA / lambada * pixelsize;
    scale = scale * factor;
    
    // 计算径向距离
    // 对应Matlab代码：R=sqrt(min(X,abs(X-w)).^2+min(Y,abs(Y-w)).^2);
    Mat X_w = X - w;
    Mat Y_w = Y - w;
    
    Mat X_abs = cv::abs(X_w);
    Mat Y_abs = cv::abs(Y_w);
    
    Mat X_min, Y_min;
    cv::min(X, X_abs, X_min);
    cv::min(Y, Y_abs, Y_min);
    
    Mat X_min_sq, Y_min_sq;
    cv::pow(X_min, 2.0, X_min_sq);
    cv::pow(Y_min, 2.0, Y_min_sq);
    
    Mat R = X_min_sq + Y_min_sq;
    Mat R_sqrt;
    cv::sqrt(R, R_sqrt);
    R = R_sqrt;
    
    // 计算点扩散函数（使用贝塞尔函数），对应Matlab代码：PSF=abs(2*besselj(1,scale*R+eps,1)./(scale*R+eps)).^2; 原理应该是光学理想透镜的艾里斑光强公式
    // 步骤1：计算 scale * R + eps，对应Matlab：scale*R+eps
    Mat scaled_R;
    multiply(R, Scalar(scale), scaled_R);
    double eps = 1e-10;
    add(scaled_R, Scalar(eps), scaled_R);
    // 步骤2：计算 2*j1(x)./x 并平方，对应Matlab：abs(2*besselj(1,scale*R+eps,1)./(scale*R+eps)).^2
    Mat PSF(w, w, CV_64F);
    
    for (int i = 0; i < w; ++i) {
        for (int j = 0; j < w; ++j) {
            double x = scaled_R.at<double>(i, j);
            double val;
            
            // 数值稳定性处理：当x很小时，2*j1(x)/x → 1，对应Matlab中避免除以零的处理
            if (std::abs(x) < 1e-10) {
                val = 1.0;
            } else {
                // 使用C标准库j1函数计算第一类一阶贝塞尔函数，对应Matlab的besselj(1, x)
                val = 2.0 * j1(x) / x;
            }
            
            PSF.at<double>(i, j) = val * val;
        }
    }
    
    // 归一化
    // 对应Matlab代码：PSF = PSF/(sum(sum(PSF)));
    double sum_psf = sum(PSF)[0];
    PSF = PSF / sum_psf;
    
    // 移位到中心
    // 对应Matlab代码：PSF=fftshift(PSF);
    Mat PSF_shifted;
    MYfftshift(PSF, PSF_shifted);
    
    return PSF_shifted;
}
