#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// 拉普拉斯矩阵计算函数
// 输入：
//   image - 输入图像
// 输出：
//   L - 拉普拉斯矩阵

// 辅助函数：计算矩阵的逆
Mat matrix_inverse(Mat mat) {
    Mat inv_mat;
    invert(mat, inv_mat, DECOMP_SVD);
    return inv_mat;
}

Mat get_laplacian(Mat image)
{
    // 检查输入是否为空
    if (image.empty()) {
        cout << "[[[for test]]]:" << "get_laplacian.cpp:22 - image is empty" << endl;
        return Mat();
    }
    
    // 参数初始化
    int m = image.rows;
    int n = image.cols;
    int c = image.channels();
    int img_size = m * n;
    int win_rad = 1;          // 窗口半径
    double epsilon = 0.0000001;  // 正则化参数
    
    int max_num_neigh = (win_rad * 2 + 1) * (win_rad * 2 + 1);  // 最大邻居数
    
    // 创建索引矩阵
    vector<int> ind_mat(m * n);
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            ind_mat[i * n + j] = i * n + j;
        }
    }
    
    vector<int> indices(img_size);
    for (int i = 0; i < img_size; i++) {
        indices[i] = i;
    }
    int num_ind = indices.size();
    
    // 预分配内存
    int max_num_vertex = max_num_neigh * num_ind;
    vector<int> row_inds(max_num_vertex);
    vector<int> col_inds(max_num_vertex);
    vector<double> vals(max_num_vertex);
    
    int len = 0;
    
    // 计算拉普拉斯矩阵
    for (int k = 0; k < num_ind; k++) {
        int ind = indices[k];
        
        // 将线性索引转换为二维坐标
        int i = ind / n;
        int j = ind % n;
        
        // 计算窗口边界
        int m_min = max(0, i - win_rad);
        int m_max = min(m - 1, i + win_rad);
        int n_min = max(0, j - win_rad);
        int n_max = min(n - 1, j + win_rad);
        
        // 获取窗口内的索引
        vector<int> win_inds;
        for (int x = m_min; x <= m_max; x++) {
            for (int y = n_min; y <= n_max; y++) {
                win_inds.push_back(x * n + y);
            }
        }
        int num_neigh = win_inds.size();
        
        // 获取窗口内的图像数据
        vector<vector<double>> win_data(num_neigh, vector<double>(c));
        int idx = 0;
        for (int x = m_min; x <= m_max; x++) {
            for (int y = n_min; y <= n_max; y++) {
                for (int ch = 0; ch < c; ch++) {
                    win_data[idx][ch] = image.at<Vec3d>(x, y)[ch];
                }
                idx++;
            }
        }
        
        // 计算窗口均值
        vector<double> win_mean(c, 0.0);
        for (int ch = 0; ch < c; ch++) {
            double sum = 0;
            for (int p = 0; p < num_neigh; p++) {
                sum += win_data[p][ch];
            }
            win_mean[ch] = sum / num_neigh;
        }
        
        // 构建窗口图像矩阵
        Mat win_image_mat(num_neigh, c, CV_64F);
        for (int p = 0; p < num_neigh; p++) {
            for (int ch = 0; ch < c; ch++) {
                win_image_mat.at<double>(p, ch) = win_data[p][ch];
            }
        }
        
        // 计算窗口协方差矩阵的逆
        Mat win_cov = (win_image_mat.t() * win_image_mat) / num_neigh;
        Mat mean_mat(1, c, CV_64F);
        for (int ch = 0; ch < c; ch++) {
            mean_mat.at<double>(0, ch) = win_mean[ch];
        }
        win_cov -= mean_mat.t() * mean_mat;
        Mat identity = Mat::eye(c, c, CV_64F);
        win_cov += (epsilon / num_neigh) * identity;
        Mat win_var = matrix_inverse(win_cov);
        
        // 去均值
        Mat win_image_centered = win_image_mat.clone();
        for (int p = 0; p < num_neigh; p++) {
            for (int ch = 0; ch < c; ch++) {
                win_image_centered.at<double>(p, ch) -= win_mean[ch];
            }
        }
        
        // 计算权重
        Mat win_vals = Mat::ones(num_neigh, num_neigh, CV_64F);
        Mat temp = win_image_centered * win_var * win_image_centered.t();
        win_vals += temp;
        win_vals /= num_neigh;
        
        int sub_len = num_neigh * num_neigh;
        
        // 填充索引和值
        for (int p = 0; p < num_neigh; p++) {
            for (int q = 0; q < num_neigh; q++) {
                row_inds[len] = win_inds[p];
                col_inds[len] = win_inds[q];
                vals[len] = win_vals.at<double>(p, q);
                len++;
            }
        }
    }
    
    // 构建邻接矩阵（使用稠密矩阵）
    Mat A = Mat::zeros(img_size, img_size, CV_64F);
    for (int i = 0; i < len; i++) {
        int row = row_inds[i];
        int col = col_inds[i];
        double val = vals[i];
        A.at<double>(row, col) = val;
    }
    
    // 构建度矩阵
    Mat D = Mat::zeros(img_size, img_size, CV_64F);
    for (int i = 0; i < img_size; i++) {
        double sum = 0;
        for (int j = 0; j < img_size; j++) {
            sum += A.at<double>(i, j);
        }
        D.at<double>(i, i) = sum;
    }
    
    // 拉普拉斯矩阵
    Mat L = D - A;
    
    return L;
}
