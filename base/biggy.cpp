// biggy.cpp : Defines the entry point for the application.
//

#include "biggy.h"


int main(){
	tensor tmp = arange(0, 8);
	tensor tmp2 = arange(0, 8);
	tmp.reshape({ 2,2,2 });
	tmp2.reshape({ 2, 4 });
	auto kernel = naive::matmul(1.0, 1.0);
	tensor tmp3 = kernel(tmp, tmp2);
	auto relu_kernel = naive::relu(false);
	tensor tmp4 = relu_kernel(tmp3);

	auto* d = tmp4.get_data();
	

	return 0;
}