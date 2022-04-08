// biggy.cpp : Defines the entry point for the application.
//

#include "biggy.h"


int main(){
	tensor tmp = arange(0, 8);
	tmp = tmp.reshape({ 2, 2, 2 });


	tensor tmp2 = arange(0, 8);
	tensor tmp3 = tmp;
	tensor tmp4 = tmp2.reshape({ 2,2,2 });
	tensor tmp5;
	auto t1 = tmp3.get_parent();
	auto t2 = tmp.get_parent();
	auto t3 = t2->get_parent();
	tmp5 = tmp3 * tmp4 + tmp3;
	tmp5.dtype;


	auto st1 = vk_device_storage(32);
	auto st2 = vk_device_storage(32);

	return 0;
}