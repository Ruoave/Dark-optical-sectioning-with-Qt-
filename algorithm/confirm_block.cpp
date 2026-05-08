#include "paramsBasic.h"
#include <opencv2/opencv.hpp>
#include "port_matlab2opencv.h"
#include "ViewMat.h"

using namespace cv;
using namespace std;

// 计算矩阵最大值
// 对应Matlab的max(max())函数
double matMax(const Mat& mat) {
    double maxVal;
    minMaxLoc(mat, nullptr, &maxVal);
    return maxVal;
}

// 计算矩阵总和
// 对应Matlab的sum(sum())函数
double matSum(const Mat& mat) {
    return sum(mat)[0];
}

// 声明PSF_Generator函数
Mat PSF_Generator(double lambada, double pixelsize, double NA, int w, double factor);

// 块大小确认函数
// 输入：
//   params - 系统参数
//   lp - 低通滤波器
// 输出：
//   block_size - 块大小
int confirm_block(ParamsBasic paramsBasicSet, Mat lp) {
    // 生成点扩散函数（PSF）
    // 对应Matlab代码：PSF = PSF_Generator(params.emwavelength,params.pixelsize,params.NA,params.Nx,params.factor);
    Mat PSF = PSF_Generator(paramsBasicSet.emwavelength, paramsBasicSet.pixelsize, paramsBasicSet.NA, paramsBasicSet.Nx, paramsBasicSet.factor);
    
    // 计算低通滤波后的PSF
    // 对应Matlab代码：PSF_Lo = abs(ifft2(fftshift(fft2(PSF)).*fftshift(lp)));
    
    // 1.fft2(PSF)
    Mat complexPSF;
    dft(PSF, complexPSF, DFT_COMPLEX_OUTPUT);
    // 2.1,fftshift(fft2(PSF))
    Mat complexPSF_shifted;
    MYfftshift_plural(complexPSF, complexPSF_shifted);
    // 2.2,fftshift(lp)
    Mat lp_shifted;
    MYfftshift(lp, lp_shifted);
    // 3.1将单通道实数滤波器转为双通道复数矩阵（虚部同实部）
    Mat lp_complex;
    Mat lp_complex_planes[2] = {lp_shifted, lp_shifted.clone()};
    merge(lp_complex_planes, 2, lp_complex);
    // 3.2用 multiply 相乘（两个双通道复数矩阵，通道数/尺寸完全匹配）
    Mat complexPSF_shifted_Multiply_lp_complex;
    multiply(complexPSF_shifted, lp_complex, complexPSF_shifted_Multiply_lp_complex, 1, CV_64F); 
    // 4.逆傅里叶变换
    Mat ifft2ed;
    dft(complexPSF_shifted_Multiply_lp_complex, ifft2ed, DFT_INVERSE | DFT_REAL_OUTPUT);   
    // 5.取绝对值
    Mat PSF_Lo = abs(ifft2ed);
    
    // 归一化
    // 对应Matlab代码：PSF_Lo = PSF_Lo./max(max(PSF_Lo));
    double maxVal = matMax(PSF_Lo);
    Mat PSF_Lo_scale = PSF_Lo/maxVal;

    // 确定块大小
    int center = static_cast<int>(floor(paramsBasicSet.Nx / 2.0));
    center = center-1 ;//因为Nx是size_x，但Mat行号列号从0开始,所以也不能直接用floor(paramsBasicSet.Nx / 2.0)当中心行/列号
    int count_x = center;
    for (; count_x < paramsBasicSet.Nx; count_x++) {

        if (PSF_Lo_scale.at<double>(count_x, center) < 0.01) {
            break;
        }
    }
    
    int block_size = count_x - center;
    
    return block_size;
}
