#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// K-means聚类函数
// 输入：
//   I - 输入图像
//   k - 聚类数量
// 输出：
//   C - 聚类中心
//   label - 每个像素的聚类标签
//   J - 目标函数值

// kmeans implementation
