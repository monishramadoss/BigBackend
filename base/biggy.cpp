// biggy.cpp : Defines the entry point for the application.
//

#include "biggy.h"


int main(){
	tensor tmp = arange(0, 8);
	tensor tmp2 = arange(0, 8);
	tmp.reshape({ 2,2,2 });

	const float* d = nullptr;
	byte_* data = nullptr;

	tensor t1 = tmp[0][0];
	data = t1.get_data();
	d = reinterpret_cast<const float*>(data);
	std::cout << "[0][0]" << " " << d[0] << ", " << d[1] << std::endl;
	tensor t2 = tmp[0][1];
	data = t2.get_data();
	d = reinterpret_cast<const float*>(data);
	std::cout << "[0][1]" << " " << d[0] << ", " << d[1] << std::endl;
	tensor t3 = tmp[1][0];
	data = t3.get_data();
	d = reinterpret_cast<const float*>(data);
	std::cout << "[1][0]" << " " << d[0] << ", " << d[1] << std::endl;
	tensor t4 = tmp[1][1];
	data = t4.get_data();
	d = reinterpret_cast<const float*>(data);
	std::cout << "[1][1]" << " " << d[0] << ", " << d[1] << std::endl;

	return 0;
}