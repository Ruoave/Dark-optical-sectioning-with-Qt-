#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// 暗通道去雾函数
// 输入：
//   image - 输入图像
//   omega - 雾度保留因子
//   win_size - 窗口大小
//   lambda - 正则化参数
// 输出：
//   radiance - 去雾后的图像

// dehaze implementation
