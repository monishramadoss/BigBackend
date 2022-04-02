// biggy.cpp : Defines the entry point for the application.
//

#include "biggy.h"

void shape_contribution(dim_vec& x, dim_vec& y, dim_vec& intersection, dim_vec& difference)
{
	for (auto it1 = x.begin(), it2 = y.begin(); it1 != x.end() && it2 != y.end(); ++it2) {
		while (it1 != x.end() && *it1 < *it2)
		{
			difference.push_back(it1 - x.begin());
			++it1;
		}
		if (it1 != x.end() && *it1 == *it2)
			intersection.push_back(it2 - y.begin());
		
	}


}

int main(){
	tensor tmp = arange(0, 8);
	tmp = tmp.reshape({ 2, 2, 2 });


	tensor tmp2 = arange(0, 8);
	tensor tmp3 = tmp;
	tensor tmp4 = tmp2.reshape({ 2,2,2 });
	auto t1 = tmp3.get_parent();
	auto t2 = tmp.get_parent();
	auto t3 = t2->get_parent();
	auto tmp5 = tmp3 * tmp4 + tmp3;

	auto storage = vk_device_storage(4096);
	auto buffer = storage.get_buffer();

	return 0;
}