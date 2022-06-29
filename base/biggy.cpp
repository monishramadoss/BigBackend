// biggy.cpp : Defines the entry point for the application.
//

#include "biggy.h"


int main()
{
	tensor tmp = arange(0, 8);
	auto data_ptr = reinterpret_cast<float*>(tmp.get_data());
	std::cout << data_ptr[0] << " " << data_ptr[1] << " " << data_ptr[2] << std::endl;
	tensor tmp3 = tmp;
	auto data_ptr_2 = reinterpret_cast<float*>(tmp3.get_data());
	data_ptr[1] = 2.0;
	std::cout << data_ptr_2[0] << " " << data_ptr_2[1] << " " << data_ptr_2[2] << std::endl;


	tmp = tmp.reshape({2, 2, 2});
	tensor tmp2 = arange(0, 8);
	tensor tmp4 = tmp2.reshape({2, 2, 2});
	tensor tmp5;

	auto layer1 = vk::mul();
	auto layer2 = vk::add();

	layer1.kernel(tmp, tmp4, tmp5);
	layer2.kernel(tmp5, tmp, tmp5);


	tmp5 = tmp * tmp4 + tmp;
	std::cout << &tmp5 << std::endl;


	return 0;
}
