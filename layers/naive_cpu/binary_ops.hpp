#pragma once

#include "compute.h"
#include "compute_manager.h"
#include "init.h"
#include "types.h"
#include "tensor.h"


#include <chrono>
#include <stdexcept>
#include <utility>




//CLASS CODE

class binary_op : public compute_job // operate on n dim
{
public:
	binary_op(const std::string& compute_type = "naive_cpu_empty");
	tensor operator()(tensor&, tensor&);
	virtual void kernel(block*, block*, block*);
	void run() override;
};

class add : public binary_op
{
public:
	add() : binary_op("naive_cpu_add") {}
	void kernel(block* x, block* w, block* y);
};


class sub : public binary_op
{
public:
	sub() : binary_op("naive_cpu_sub") {}
	void kernel(block* x, block* w, block* y);
};


class mul : public binary_op
{
public:
	mul() : binary_op("naive_cpu_mul") {}
	void kernel(block* x, block* w, block* y);
};


class true_div : public binary_op
{
public:
	true_div() : binary_op("naive_cpu_div") {}
	void kernel(block* x, block* w, block* y);
};


// IMPLEMENTATION CODE 

using namespace std::chrono_literals;

inline binary_op::binary_op(const std::string& compute_type) : compute_job(compute_type)
{
}

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

	return y;
}

inline void binary_op::kernel(block* x, block* w, block* y)
{
	throw std::runtime_error("no kernel developed");
}

inline void binary_op::run()
{
	kernel(global_compute_manager.get_input(this, 0), global_compute_manager.get_input(this, 1), global_compute_manager.get_output(this, 0));
}

inline void add::kernel(block* x, block* w, block* y)
{
	auto* _x = reinterpret_cast<const float*>(x->ptr + x->offset);
	auto* _w = reinterpret_cast<const float*>(w->ptr + w->offset);
	auto* _y = reinterpret_cast<float*>(y->ptr + y->offset);

	for (_int i = 0; i < x->size / sizeof(float); ++i)
		_y[i] = _x[i] + _w[i];
}

inline void sub::kernel(block* x, block* w, block* y)
{
	auto* _x = reinterpret_cast<const float*>(x->ptr + x->offset);
	auto* _w = reinterpret_cast<const float*>(w->ptr + w->offset);
	auto* _y = reinterpret_cast<float*>(y->ptr + y->offset);

	for (_int i = 0; i < x->size / sizeof(float); ++i)
		_y[i] = _x[i] - _w[i];
}

inline void mul::kernel(block* x, block* w, block* y)
{
	auto* _x = reinterpret_cast<const float*>(x->ptr + x->offset);
	auto* _w = reinterpret_cast<const float*>(w->ptr + w->offset);
	auto* _y = reinterpret_cast<float*>(y->ptr + y->offset);

	for (_int i = 0; i < x->size / sizeof(float); ++i)
		_y[i] = _x[i] * _w[i];
}

inline void true_div::kernel(block* x, block* w, block* y)
{
	auto* _x = reinterpret_cast<const float*>(x->ptr + x->offset);
	auto* _w = reinterpret_cast<const float*>(w->ptr + w->offset);
	auto* _y = reinterpret_cast<float*>(y->ptr + y->offset);

	for (_int i = 0; i < x->size / sizeof(float); ++i)
		_y[i] = _x[i] / _w[i];
}


inline tensor tensor::operator+(tensor& t)
{
	auto layer = add();
	std::cout << &layer << std::endl;
	return layer(*this, t);
}

inline tensor tensor::operator-(tensor& t)
{
	auto layer = sub();
	std::cout << &layer << std::endl;
	return layer(*this, t);
}

inline tensor tensor::operator*(tensor& t)
{
	auto layer = mul();
	std::cout << &layer << std::endl;
	return layer(*this, t);
}

inline tensor tensor::operator/(tensor& t)
{
	auto layer = true_div();
	std::cout << &layer << std::endl;
	return layer(*this, t);
}
