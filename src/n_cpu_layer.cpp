#include "n_cpu_layer.h"


namespace naive
{
	matmul::matmul(float alpha, float beta) : compute_job(), alpha(alpha), beta(beta) {}

	void matmul::kernel(tensor& x, tensor& w, tensor& y)
	{
		if (x.shape().size() == 3) // TODO batched matmul
			return;

		if (x.shape()[1] != w.shape()[0]) // TODO custom errors
			throw std::bad_alloc();

		y = tensor(nullptr, {x.shape()[0], w.shape()[1]}, x.dtype);
	}

	void matmul::kernel(tensor& x, tensor& w, tensor& z, tensor& y)
	{
		
		if (x.shape().size() == 3) // TODO batched matmul
			return;

		if (x.shape()[1] != w.shape()[0]) // TODO custom errors
			throw std::bad_alloc();

		y = tensor(nullptr, { x.shape()[0], w.shape()[1] }, x.dtype);
	}


	void matmul::run()
	{
		
	}
}
