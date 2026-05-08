#ifndef PORT_MATLAB2OPENCV_H
#define PORT_MATLAB2OPENCV_H

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;

// MYlinspace函数：生成线性间隔的向量
// 输入：
//   start - 起始值
//   end - 结束值
//   num - 生成元素个数
// 输出：
//   返回包含num个元素的std::vector<int>，元素在start到end之间均匀分布
// 功能：
//   类似于MATLAB的linspace函数，生成线性间隔的行向量
//   例如：MYlinspace(0, 5, 6) 返回 [0, 1, 2, 3, 4, 5]
std::vector<int> MYlinspace(int start, int end, size_t num);

// MYmeshgrid函数重载版本：使用vector<int>作为输入
// 输入：
//   a - 整数向量，表示x方向的坐标
//   b - 整数向量，表示y方向的坐标
// 输出：
//   x - x坐标矩阵，大小为len(b) x len(a)
//   y - y坐标矩阵，大小为len(b) x len(a)
void MYmeshgrid_2(const std::vector<int>& a, const std::vector<int>& b, Mat& x, Mat& y);

// MYmeshgrid函数：生成以中心为原点的网格坐标
// 输入：
//   size - 网格大小 Size(W, H)
// 输出：
//   x - x坐标矩阵
//   y - y坐标矩阵
// 功能：
//   生成网格坐标，类似于MATLAB的meshgrid函数
//   x范围为-floor(W/2)到floor((W-1)/2)
//   y范围为-floor(H/2)到floor((H-1)/2)
void MYmeshgrid(Size size, Mat& x, Mat& y);

// MYifftshift函数：将FFT的零频率分量移到频谱中心
// 输入：
//   src - 输入矩阵
// 输出：
//   dst - 输出矩阵
// 功能：
//   对输入矩阵进行象限交换，对应MATLAB的ifftshift函数
void MYifftshift(Mat& src, Mat& dst);

// MYfftshift函数：将零频率分量从中心移开
// 输入：
//   src - 输入矩阵
// 输出：
//   dst - 输出矩阵
// 功能：
//   对输入矩阵进行象限交换，对应MATLAB的fftshift函数
void MYfftshift(Mat& src, Mat& dst);

// MYfftshift_plural函数：直接处理双通道复数矩阵的fftshift
// 输入：
//   src - 输入双通道复数矩阵
// 输出：
//   dst - 输出双通道复数矩阵
// 功能：
//   对输入的双通道复数矩阵进行fftshift操作
//   自动分离实部和虚部，分别进行fftshift，然后合并
void MYfftshift_plural(Mat& src, Mat& dst);

// MYifftshift_plural函数：直接处理双通道复数矩阵的ifftshift
// 输入：
//   src - 输入双通道复数矩阵
// 输出：
//   dst - 输出双通道复数矩阵
// 功能：
//   对输入的双通道复数矩阵进行ifftshift操作
//   自动分离实部和虚部，分别进行ifftshift，然后合并
void MYifftshift_plural(Mat& src, Mat& dst);

#endif // PORT_MATLAB2OPENCV_H
