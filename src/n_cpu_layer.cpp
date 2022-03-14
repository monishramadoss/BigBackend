#include "n_cpu_layer.h"
#include <chrono>

using namespace std::chrono_literals;

namespace naive
{
	matmul::matmul(float alpha, float beta) : compute_job(), alpha(alpha), beta(beta) {}

	void matmul::kernel(tensor& x, tensor& w, tensor& y)
	{
		std::this_thread::sleep_for(10ms);
	}


	tensor matmul::batch(tensor& x, tensor& w, tensor& y)
	{
		for(auto i = 0; i < x.shape(0); ++ i)
		{
			matmul sub_kernel(alpha, beta);
			sub_kernel.setup({ x[i], w, y[i] });
			parallel_kernels.push_back(sub_kernel);
		}
		return y;
	}

	tensor matmul::operator()(tensor& x, tensor& w)
	{
		std::vector<_int> output_shape;
		if (x.ndim() > 3)
		{
			x.reshape({ static_cast<int>(x.shape(0)),  -1, static_cast<int>(x.shape(-1)) });
			output_shape = { x.shape(0), x.shape(1), w.shape(1) };
		}

		if (x.ndim() == 3)
			output_shape = { x.shape(0), x.shape(1), w.shape(1) };
		else
			output_shape = { x.shape(0), w.shape(1) };

		tensor y(data_filln<float>(output_shape, 0), output_shape, x.dtype);
		setup({ x, w, y });

		if (x.ndim() == 3)
			return batch(x, w, y);

		
		return io_lst.back();
	}

	tensor matmul::operator()(tensor& x, tensor& w, tensor& z)
	{
		tensor y = operator()(x, w);
		return io_lst.back();
	}


	void matmul::run()
	{
		if(!parallel_kernels.empty())
		{
			for(size_t i = 0; i < parallel_kernels.size(); ++i)
			{
				parallel_kernels[i].run();
			}
			return;

		}
		else 
		{

			kernel(io_lst[0], io_lst[1], io_lst[2]);
		}
		
	}

	relu::relu(bool in_place) : compute_job(), in_place(in_place)
	{
		
	}

	tensor relu::operator()(tensor& x)
	{
		if (!in_place)
		{
			tensor y(data_filln<float>(x.shape(), 0), x.shape(), x.dtype);
			setup({ x, y });
		} else
		{
			setup({ x, x });
		}

		return io_lst.back();
	}

	void relu::kernel(tensor& x, tensor& y)
	{
		std::this_thread::sleep_for(1ms);
	}

	void relu::run()
	{
		if(!parallel_kernels.empty())
		{
			for(auto& pk: parallel_kernels)
			{
				pk.run();
			}
		} else
		{
			kernel(io_lst[0], io_lst[1]);
		}

	}
}


