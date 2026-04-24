#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// 引导滤波函数（O(1)时间实现）
// 输入：
//   I - 引导图像（应是灰度/单通道图像）
//   p - 滤波输入图像（应是灰度/单通道图像）
//   r - 局部窗口半径
//   eps - 正则化参数
// 输出：
//   q - 滤波后的结果

// guidedfilter implementation
