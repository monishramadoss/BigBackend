#pragma once
#include <chrono>
#include <stdexcept>


#include "compute.h"
#include "init.h"
#include "types.h"
#include "tensor.h"


class binary_op : public compute_job // operate on n dim
{
public:
	binary_op();
	tensor operator()(tensor&, tensor&);
	virtual void kernel(tensor&, tensor&, tensor&);
	void run() override;
	std::string m_type = "naive_cpu_empty";
};

class add : public binary_op
{
public:
	add() : binary_op() { m_type = "naive_cpu_add"; }
	static void kernel(tensor&, tensor&, tensor&);
};


class sub : public binary_op
{
public:
	sub() : binary_op() { m_type = "naive_cpu_sub"; }
	static void kernel(tensor&, tensor&, tensor&);
};


class mul : public binary_op
{
public:
	mul() : binary_op() { m_type = "naive_cpu_mul"; }
	static void kernel(tensor&, tensor&, tensor&);
};


class true_div : public binary_op
{
public:
	true_div() : binary_op() { m_type = "naive_cpu_div"; }
	static void kernel(tensor&, tensor&, tensor&);
};

using namespace std::chrono_literals;

inline binary_op::binary_op() : compute_job() {  }

inline tensor binary_op::operator()(tensor& x, tensor& w)
{
	set_input(x);
	set_input(w);
	
	dim_vec x_shape = x.shape();
	dim_vec w_shape = w.shape();
	if (x_shape.empty() || w_shape.empty())
		throw std::runtime_error("Inputs shape are invalid");

	auto y_shape = broadcast_contribution(x_shape, w_shape, differ, similar);
	tensor y(data_filln<float>(y_shape, 0.0), y_shape, x.dtype);
	set_output(y);

	return *get_output(0);
}

inline void binary_op::kernel(tensor& x, tensor& w, tensor& y)
{
	if (x.ndim() != w.ndim())
		throw std::runtime_error("inoperable");
	throw std::runtime_error("no kernel developed");
}

inline void binary_op::run()
{
	//if (!parallel_kernels.empty())
	//{
	//	for (auto& pk : parallel_kernels)
	//	{
	//		pk.run();
	//	}
	//}
	//else
	{
		kernel(*get_input(0), *get_input(1), *get_output(0));
	}

}

inline void add::kernel(tensor& x, tensor& w, tensor& y)
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

inline void mul::kernel(tensor& x, tensor& w, tensor& y)
{
	auto* _x = reinterpret_cast<const float*>(x.get_data());
	auto* _w = reinterpret_cast<const float*>(w.get_data());
	auto* _y = reinterpret_cast<float*>(y.get_data());

	for (_int i = 0; i < x.data_size(); ++i)
		_y[i] = _x[i] * _w[i];
}

inline void true_div::kernel(tensor& x, tensor& w, tensor& y)
{
	auto* _x = reinterpret_cast<const float*>(x.get_data());
	auto* _w = reinterpret_cast<const float*>(w.get_data());
	auto* _y = reinterpret_cast<float*>(y.get_data());

	for (_int i = 0; i < x.data_size(); ++i)
		_y[i] = _x[i] / _w[i];
}



inline tensor tensor::operator+(tensor& t)
{
	return add()(*this, t);
}

inline tensor tensor::operator-(tensor& t)
{
	return sub()(*this, t);
}

inline tensor tensor::operator*(tensor& t)
{
	return mul()(*this, t);
}

inline tensor tensor::operator/(tensor& t)
{
	return true_div()(*this, t);
}
