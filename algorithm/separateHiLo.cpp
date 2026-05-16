#include <vector>
#include <complex>
#include <opencv2/opencv.hpp>
#include "paramsBasic.h"
#include <ViewMat.h>
#include "port_matlab2opencv.h"

using namespace cv;
using namespace std;

// 高低频分离函数
// 输入：
//   image - 输入图像
//   params - 系统参数
//   deg - 降解参数
//   divide - 分割参数
// 输出：所有输出参数都是单通道二维矩阵
//   Hi - 高频分量
//   Lo - 低频分量
//   lp - 低通滤波器
//   EL - 极低频分量

Mat lpgauss(int H, int W, double SIGMA);
Mat hpgauss(int H, int W, double SIGMA);

void separateHiLo(Mat image, ParamsBasic paramsBasicSet, double deg, double divide, Mat& Hi, Mat& Lo, Mat& lp, Mat& EL)
{
    
    // 高低频分离：参数初始化
    int Nx = paramsBasicSet.Nx;
    int Ny = paramsBasicSet.Ny;
    double NA = paramsBasicSet.NA;  //NA: 物镜的数值孔径
    double emwavelength = paramsBasicSet.emwavelength;  //emwavelength: 光波长
    double factor = paramsBasicSet.factor;  //factor: Upscaling factor，上采样因子
    double pixel_size = paramsBasicSet.pixelsize;  //pixelsize: 像素大小

    // 高低频分离：截止频率计算
    double res = 0.5 * emwavelength / NA / factor;     // 分辨率
    double k_m = Ny / (res / pixel_size);    // 物镜截止频率
    int kc = floor(k_m * 0.2);             // 高低通滤波器之间的截止频率
    double sigmaLP = kc * 2 / 2.355;                // 低通滤波器的sigma值

    // 高低频分离：滤波器生成
    lp = lpgauss(Nx, Ny, sigmaLP * 2 * divide);    // 低通滤波器
    Mat hp = hpgauss(Nx, Ny, sigmaLP * 2 * divide);    // 高通滤波器
    Mat elp = lpgauss(Nx, Ny, sigmaLP / deg);        // 极低通滤波器
    Mat ehp = hpgauss(Nx, Ny, sigmaLP / deg);        // 极高通滤波器

    // 确保输入图像是浮点类型
    Mat image_float;
    if (image.type() != CV_64F) {
        image.convertTo(image_float, CV_64F);
    } else {
        image_float = image;
    }
    
    // 高低频分离：频域变换和分量提取

    // 高低频分离的频域变换部分：对应 Matlab: fft_image = fftshift(fft2(image));
    Mat fft_image;  //fft_image 用于存储对image_float进行傅里叶变换的结果
    dft(image_float, fft_image, DFT_COMPLEX_OUTPUT);  // 对应 Matlab 的 fft2(image)
    Mat fft_image_shift;  //fft_image_shift 用于存储对fft_image进行fftshift的结果
    MYfftshift_plural(fft_image, fft_image_shift);  // 对应 Matlab 的 fftshift(fft2(image)) 对fft_image进行fftshift处理 


    // 高低频分离的分量提取部分
    // 对应 Matlab: Hi = real(ifft2(ifftshift(fft_image.*fftshift(hp))));
    // 1.用 MYfftshift(hp) 直接得到一个单通道实数矩阵的滤波器
    Mat hp_MYfftshift;
    MYfftshift(hp, hp_MYfftshift);  // 对应 Matlab 的 fftshift(hp)
    // 2.1将单通道实数滤波器转为双通道复数矩阵（虚部同实部，因为multiply函数要求参与运算的矩阵要么同尺寸+同通道数，要么是矩阵+标量
    Mat hp_MYfftshift_complex;
    Mat complex_planes[2] = {hp_MYfftshift, hp_MYfftshift.clone()};
    merge(complex_planes, 2, hp_MYfftshift_complex); // 合并为CV_64FC2
    // 2.2用 multiply 相乘（两个双通道复数矩阵，通道数/尺寸完全匹配）
    Mat for_MYifftshift;
    multiply(fft_image_shift, hp_MYfftshift_complex, for_MYifftshift, 1, CV_64F); // 无报错
    // 3.再将结果用 MYifftshift_plural 处理
    Mat MYifftshifted;  // MYifftshifted;用于存储对for_MYifftshift进行ifftshift的结果
    MYifftshift_plural(for_MYifftshift, MYifftshifted);  // 对应 Matlab 的 ifftshift
    // 4.再将结果用 dft 进行二维逆离散傅里叶变换处理，标志符DFT_REAL_OUTPUT舍去可能残余的虚数部分误差
    Mat dft_ifft2ed;  // dft_ifft2ed;用于存储对MYifftshifted进行逆傅里叶变换的结果
    dft(MYifftshifted, dft_ifft2ed, DFT_INVERSE | DFT_REAL_OUTPUT | DFT_SCALE);  // 对应 Matlab的ifft2和real; 标志位DFT_INVERSE意为逆傅里叶，DFT_REAL_OUTPUT为去除残余虚部，DFT_SCALE为归一化(因为ifft2自带归一化，为了与之一致)

    // 对应Matlab: Lo = real(ifft2(ifftshift(fft_image.*fftshift(lp))));方法同上
    Mat lp_MYfftshift;
    MYfftshift(lp, lp_MYfftshift);
    Mat lp_MYfftshift_complex;
    Mat lp_complex_planes[2] = {lp_MYfftshift, lp_MYfftshift.clone()};
    merge(lp_complex_planes, 2, lp_MYfftshift_complex);
    Mat for_MYifftshift_lp;
    multiply(fft_image_shift, lp_MYfftshift_complex, for_MYifftshift_lp, 1, CV_64F);
    Mat MYifftshifted_lp;
    MYifftshift_plural(for_MYifftshift_lp, MYifftshifted_lp);
    Mat dft_ifft2ed_lp;
    dft(MYifftshifted_lp, dft_ifft2ed_lp, DFT_INVERSE | DFT_REAL_OUTPUT | DFT_SCALE);

    // 极低频分量分离：对应 Matlab: EL = real(ifft2(fft2(image).*elp));
    // 1.将单通道实数滤波器elp转为双通道复数矩阵
    Mat elp_complex;
    Mat elp_planes[2] = {elp, elp.clone()};
    merge(elp_planes, 2, elp_complex); // 合并为CV_64FC2
    // 2.用 multiply 相乘（两个双通道复数矩阵）
    Mat for_ifft2_el;
    multiply(fft_image, elp_complex, for_ifft2_el, 1, CV_64F);
    // 3.进行逆傅里叶变换并取实部
    Mat dft_ifft2ed_el;
    dft(for_ifft2_el, dft_ifft2ed_el, DFT_INVERSE | DFT_REAL_OUTPUT | DFT_SCALE);
    

    Hi = dft_ifft2ed;  // 将处理结果赋值给输出参数 Hi,本行检查正确
    Lo = dft_ifft2ed_lp;  // 将处理结果赋值给输出参数 Lo,本行检查正确
    EL = dft_ifft2ed_el;  // 将处理结果赋值给输出参数 EL,
    //EH = dft_ifft2ed_eh;  //没必要，matlab项目里写了但是没用上
    
}

// 创建傅里叶空间图像的2D高斯低通滤波器
// 输入：
//   H - 图像高度
//   W - 图像宽度
//   SIGMA - 高斯标准差
// 输出：
//   out - 低通滤波器
Mat lpgauss(int H, int W, double SIGMA)
{
    double kcx = SIGMA;
    double kcy = (static_cast<double>(H) / static_cast<double>(W)) * SIGMA;

    Mat x, y;
    MYmeshgrid(Size(W, H), x, y);//为滤波器生成对应图片大小的矩阵网格

    Mat x2, y2;
    pow(x, 2, x2);  // 把矩阵x中每一个元素分别平方，结果存到x2
    pow(y, 2, y2);  // 把矩阵y中每一个元素分别平方，结果存到y2
    
    double kcx2 = kcx * kcx;
    double kcy2 = kcy * kcy;
    Mat temp = -(x2 / kcx2 + y2 / kcy2);
    Mat out;
    exp(temp, out);

    MYifftshift(out, out);    // Shift the filter

    return out;
}

// 创建傅里叶空间图像的2D高斯高通滤波器
// 输入：
//   H - 图像高度
//   W - 图像宽度
//   SIGMA - 高斯标准差
// 输出：
//   out - 高通滤波器
Mat hpgauss(int H, int W, double SIGMA)
{
    Mat out = 1.0 - lpgauss(H, W, SIGMA);
    return out;
}




