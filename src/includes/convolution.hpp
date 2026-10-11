#ifndef CONVOLUTION_HPP
#define CONVOLUTION_HPP

# include <vector>
# include "kernels.hpp"

Matrix padding(Matrix& image, int kernel_size);
int calculate_conv(Matrix& image,Matrix& filter,int i,int j);
Matrix conv(Matrix& image,Matrix& filter);

#endif
