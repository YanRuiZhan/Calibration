//定义卷积运算

//头文件定义 Matrix = std::vector<std::vector<int>>
# include "includes/kernels.hpp"
# include <cstddef>
# include <iostream>
# include <vector>

Matrix padding(Matrix& image, int kernel_size);   // 声明
int calculate_conv(Matrix& image,Matrix& filter,int i,int j);

Matrix conv(Matrix& image,Matrix& filter){
/* 实现基本卷积操作：*/
    int m = image.size();
    int n = image[0].size();
    int z = filter.size();
    auto padding_img = padding(image,z);

    //初始化卷积输出图像
    Matrix conv_img(m,std::vector<int>(n,0));
    //卷积运算
    for(int i = 0;i<m;++i){
        for(int j = 0;j<n;++j){
            //这里的i、j指的是输出图像上第i行j列的像素
            conv_img[i][j] = calculate_conv(padding_img,filter,i,j);
        }
    }

    return conv_img;
}

Matrix padding(Matrix& image,int kernel_size){
/* 填充原图片 这里采用复制边界填充(replicate)，即把最外圈像素向外复制到pad圈 */
    //新图片尺寸和原来保持一致，由此计算pad
    int pad = (kernel_size-1)/2;
    int h = image.size();
    int w = image[0].size();
    int m = h+2*pad;
    int n = w+2*pad;

    Matrix padding_img(m, std::vector<int>(n, 0));

    for(int i=0;i<m;i++){
        for(int j=0;j<n;j++){
            // 把超出原图范围的坐标赋值为最近的边界像素
            int si = i-pad;
            int sj = j-pad;
            if(si<0) si=0;
            if(si>h-1) si=h-1;
            if(sj<0) sj=0;
            if(sj>w-1) sj=w-1;
            padding_img[i][j] = image[si][sj];
        }
    }
    return padding_img;
}

int calculate_conv(Matrix& image,Matrix& filter,int i,int j){
/*由图像、滤波器模板、具体位置计算单个像素滤波结果*/
    int z = filter.size();
    int sum = 0;

    //计算单步卷积结果
    for(int p = 0;p<z;++p){
        for(int q = 0;q<z;++q){
            sum += filter[p][q]*image[i+p][j+q];
        }
    }
    return sum;
}
