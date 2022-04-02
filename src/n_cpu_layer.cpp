#include "n_cpu_layer.h"
#include <chrono>
#include <stdexcept>

using namespace std::chrono_literals;

binary_op::binary_op() : compute_job() {}

tensor& binary_op::operator()(tensor& x, tensor& w)
{
	push_input(x);
	push_input(w);

	dim_vec x_shape = x.shape();
	dim_vec w_shape = w.shape();
	if (x_shape.empty() || w_shape.empty())
		throw std::runtime_error("Inputs shape are invalid");

	auto y_shape = broadcast_contribution(x_shape, w_shape, differ, similar);
	tensor y (data_filln<float>(y_shape, 0.0), y_shape, x.dtype);
	push_output(y);

	return m_outputs.back();
}

void binary_op::kernel(tensor& x, tensor& w, tensor& y)
{
	if (x.ndim() != w.ndim())
		throw std::runtime_error("inoperable");
	throw std::runtime_error("no kernel developed");
}

void binary_op::run()
{
	if (!parallel_kernels.empty())
	{
		for (auto& pk : parallel_kernels)
		{
			pk.run();
		}
	}
	else
	{
		kernel(m_inputs[0], m_inputs[1], m_outputs[2]);
	}

}

void add::kernel(tensor& x, tensor& w, tensor& y)
{
	auto* _x = reinterpret_cast<const float*>(x.get_data());
	auto* _w = reinterpret_cast<const float*>(w.get_data());
	auto* _y = reinterpret_cast<float*>(y.get_data());

	for (_int i = 0; i < x.data_size(); ++i)
		_y[i] = _x[i] + _w[i];
	
}

void sub::kernel(tensor& x, tensor& w, tensor& y)
{
	auto* _x = reinterpret_cast<const float*>(x.get_data());
	auto* _w = reinterpret_cast<const float*>(w.get_data());
	auto* _y = reinterpret_cast<float*>(y.get_data());

	for (_int i = 0; i < x.data_size(); ++i)
		_y[i] = _x[i] - _w[i];
}

void mul::kernel(tensor& x, tensor& w, tensor& y)
{
	auto* _x = reinterpret_cast<const float*>(x.get_data());
	auto* _w = reinterpret_cast<const float*>(w.get_data());
	auto* _y = reinterpret_cast<float*>(y.get_data());

	for (_int i = 0; i < x.data_size(); ++i)
		_y[i] = _x[i] * _w[i];
}

void true_div::kernel(tensor& x, tensor& w, tensor& y)
{
	auto* _x = reinterpret_cast<const float*>(x.get_data());
	auto* _w = reinterpret_cast<const float*>(w.get_data());
	auto* _y = reinterpret_cast<float*>(y.get_data());

	for (_int i = 0; i < x.data_size(); ++i)
		_y[i] = _x[i] / _w[i];
}



tensor tensor::operator+(tensor& w)
{
	auto kernel = add();
	return kernel(*this, w);
}

tensor tensor::operator-(tensor& w)
{
	auto kernel = sub();
	return kernel(*this, w);
}

tensor tensor::operator*(tensor& w)
{
	auto kernel = mul();
	return kernel(*this, w);
}

tensor tensor::operator/(tensor& w)
{
	auto kernel = true_div();
	return kernel(*this, w);
}

