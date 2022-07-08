// biggy.cpp : Defines the entry point for the application.
//

#include "biggy.h"


int main()
{
	tensor tmp = arange(0, 8);
	tensor tmp3 = tmp;
	tmp = tmp.reshape({2, 2, 2});
	tensor tmp2 = arange(0, 8);
	tensor tmp4 = tmp2.reshape({2, 2, 2});
	tensor tmp5;

	/*
	auto layer1 = vk::mul();
	auto layer2 = vk::add();

	layer1.kernel(tmp, tmp4, tmp5);
	layer2.kernel(tmp5, tmp, tmp5);
	/**/


	tmp5 = tmp * tmp4 + tmp;
	

	global_compute_manager.run_kernels();
	std::cout << &tmp5 << std::endl;
	return 0;
}
