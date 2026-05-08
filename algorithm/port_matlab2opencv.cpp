#include "port_matlab2opencv.h"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// MYlinspace函数：生成线性间隔的向量
// 输入：
//   start - 起始值
//   end - 结束值
//   num - 生成元素个数
// 输出：
//   返回包含num个元素的vector<int>，元素在start到end之间均匀分布
// 功能：
//   类似于MATLAB的linspace函数，生成线性间隔的行向量
//   例如：MYlinspace(0, 5, 6) 返回 [0, 1, 2, 3, 4, 5]
vector<int> MYlinspace(int start, int end, size_t num)
{
    vector<int> result;
    if (num == 0) {
        return result;
    }
    if (num == 1) {
        result.push_back(start);
        return result;
    }
    
    result.reserve(num);
    double step = static_cast<double>(end - start) / static_cast<double>(num - 1);
    
    for (size_t i = 0; i < num; i++) {
        result.push_back(static_cast<int>(round(start + step * i)));
    }
    
    return result;
}


// MYmeshgrid函数重载版本：使用vector<int>作为输入
// 输入：
//   a - 整数向量，表示x方向的坐标（会被强制转换为行向量）
//   b - 整数向量，表示y方向的坐标（会被强制转换为列向量）
// 输出：
//   x - x坐标矩阵，大小为len(b) x len(a)
//   y - y坐标矩阵，大小为len(b) x len(a)
// 功能：
//   类似于MATLAB的meshgrid函数，包含强制标准化逻辑：

void MYmeshgrid_2(const std::vector<int>& a, const std::vector<int>& b, Mat& x, Mat& y)
{
    // 空向量判断
    if (a.empty()) {
        cout << "Error: MYmeshgrid_2 input vector 'a' is empty!" << endl;
        return;
    }
    if (b.empty()) {
        cout << "Error: MYmeshgrid_2 input vector 'b' is empty!" << endl;
        return;
    }
    
    // 强制标准化：将a转换为行向量（1 x na）,   相当于MATLAB的：xrow = full(a(:)).'
    // 
    int na = static_cast<int>(a.size());
    Mat xrow(1, na, CV_64F);
    for (int j = 0; j < na; j++) {
        xrow.at<double>(0, j) = static_cast<double>(a[j]);
    }
    
    // 强制标准化：将b转换为列向量（nb x 1）,    相当于MATLAB的：ycol = full(b(:))
    int nb = static_cast<int>(b.size());
    Mat ycol(nb, 1, CV_64F);
    for (int i = 0; i < nb; i++) {
        ycol.at<double>(i, 0) = static_cast<double>(b[i]);
    }
    
    // 使用repeat函数批量复制生成网格矩阵
    // x矩阵：将xrow（1 x na）垂直重复nb次，得到nb x na矩阵
    x = repeat(xrow, nb, 1);
    // y矩阵：将ycol（nb x 1）水平重复na次，得到nb x na矩阵
    y = repeat(ycol, 1, na);
}

// MYmeshgrid函数：生成以中心为原点的网格坐标（保留原版本用于兼容性，原版本只用于separateHiLo.cpp中） 
// 输入：
//   size - 网格大小 Size(W, H)
// 输出：
//   x - x坐标矩阵
//   y - y坐标矩阵
// 功能：
//   生成网格坐标，类似于MATLAB的meshgrid函数
//   x范围为-floor(W/2)到floor((W-1)/2)
//   y范围为-floor(H/2)到floor((H-1)/2)
void MYmeshgrid(Size size, Mat& x, Mat& y)
{
    int W = size.width;
    int H = size.height;
    
    x.create(H, W, CV_64F);
    y.create(H, W, CV_64F);
    
    int x_start = -floor(W / 2.0);
    int y_start = -floor(H / 2.0);
    
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            x.at<double>(i, j) = x_start + j;
            y.at<double>(i, j) = y_start + i;
        }
    }
}

// MYifftshift函数：将FFT的零频率分量移到频谱中心
// 输入：
//   src - 输入矩阵
// 输出：
//   dst - 输出矩阵
// 功能：
//   对输入矩阵进行象限交换，对应MATLAB的ifftshift函数
//   对于偶数尺寸，交换四个象限
//   对于奇数尺寸，调整偏移量以确保中心正确
void MYifftshift(Mat& src, Mat& dst)
{
    int rows = src.rows;
    int cols = src.cols;
    
    dst.create(rows, cols, src.type());
    
    int cx = cols / 2;
    int cy = rows / 2;
    
    if (cols % 2 == 0 && rows % 2 == 0) {
        Rect q0(Rect(0, 0, cx, cy));
        Rect q1(Rect(cx, 0, cx, cy));
        Rect q2(Rect(0, cy, cx, cy));
        Rect q3(Rect(cx, cy, cx, cy));
        
        Mat temp;
        src(q0).copyTo(temp);
        src(q3).copyTo(dst(q0));
        temp.copyTo(dst(q3));
        
        src(q1).copyTo(temp);
        src(q2).copyTo(dst(q1));
        temp.copyTo(dst(q2));
    } else {
        int cx_odd = (cols + 1) / 2;
        int cy_odd = (rows + 1) / 2;
        
        Rect q0(Rect(0, 0, cx, cy));
        Rect q1(Rect(cx, 0, cx_odd, cy));
        Rect q2(Rect(0, cy, cx, cy_odd));
        Rect q3(Rect(cx, cy, cx_odd, cy_odd));
        
        Mat temp;
        src(q0).copyTo(temp);
        src(q3).copyTo(dst(q0));
        temp.copyTo(dst(q3));
        
        src(q1).copyTo(temp);
        src(q2).copyTo(dst(q1));
        temp.copyTo(dst(q2));
    }
}

// MYfftshift函数：将零频率分量从中心移开
// 输入：
//   src - 输入矩阵
// 输出：
//   dst - 输出矩阵
// 功能：
//   对输入矩阵进行象限交换，对应MATLAB的fftshift函数
//   对于偶数尺寸，交换四个象限
//   对于奇数尺寸，调整偏移量以确保正确移位
void MYfftshift(Mat& src, Mat& dst)
{
    int rows = src.rows;
    int cols = src.cols;
    
    dst.create(rows, cols, src.type());
    
    int cx = cols / 2;
    int cy = rows / 2;
    
    if (cols % 2 == 0 && rows % 2 == 0) {
        Rect q0(Rect(0, 0, cx, cy));
        Rect q1(Rect(cx, 0, cx, cy));
        Rect q2(Rect(0, cy, cx, cy));
        Rect q3(Rect(cx, cy, cx, cy));
        
        Mat temp;
        src(q0).copyTo(temp);
        src(q3).copyTo(dst(q0));
        temp.copyTo(dst(q3));
        
        src(q1).copyTo(temp);
        src(q2).copyTo(dst(q1));
        temp.copyTo(dst(q2));
    } else {
        int cx_odd = (cols + 1) / 2;
        int cy_odd = (rows + 1) / 2;
        
        Rect q0(Rect(0, 0, cx_odd, cy_odd));
        Rect q1(Rect(cx_odd, 0, cx, cy_odd));
        Rect q2(Rect(0, cy_odd, cx_odd, cy));
        Rect q3(Rect(cx_odd, cy_odd, cx, cy));
        
        Mat temp;
        src(q0).copyTo(temp);
        src(q3).copyTo(dst(q0));
        temp.copyTo(dst(q3));
        
        src(q1).copyTo(temp);
        src(q2).copyTo(dst(q1));
        temp.copyTo(dst(q2));
    }
}

// MYfftshift_plural函数：直接处理双通道复数矩阵的fftshift
// 输入：
//   src - 输入双通道复数矩阵
// 输出：
//   dst - 输出双通道复数矩阵
// 功能：
//   对输入的双通道复数矩阵进行fftshift操作
//   自动分离实部和虚部，分别进行fftshift，然后合并
void MYfftshift_plural(Mat& src, Mat& dst)
{
    // 分离实部和虚部
    Mat planes[2];
    split(src, planes);
    
    // 对实部和虚部分别进行fftshift
    Mat shifted_planes[2];
    MYfftshift(planes[0], shifted_planes[0]);
    MYfftshift(planes[1], shifted_planes[1]);
    
    // 合并实部和虚部
    merge(shifted_planes, 2, dst);
}

// MYifftshift_plural函数：直接处理双通道复数矩阵的ifftshift
// 输入：
//   src - 输入双通道复数矩阵
// 输出：
//   dst - 输出双通道复数矩阵
// 功能：
//   对输入的双通道复数矩阵进行ifftshift操作
//   自动分离实部和虚部，分别进行ifftshift，然后合并
void MYifftshift_plural(Mat& src, Mat& dst)
{
    // 分离实部和虚部
    Mat planes[2];
    split(src, planes);
    
    // 对实部和虚部分别进行ifftshift
    Mat shifted_planes[2];
    MYifftshift(planes[0], shifted_planes[0]);
    MYifftshift(planes[1], shifted_planes[1]);
    
    // 合并实部和虚部
    merge(shifted_planes, 2, dst);
}
