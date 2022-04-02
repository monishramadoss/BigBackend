#pragma once
#include "compute.h"
#include "init.h"
#include "types.h"
#include "tensor.h"


class binary_op : public compute_job // operate on n dim
{
public:
	binary_op();
	tensor& operator()(tensor&, tensor&);
	virtual void kernel(tensor&, tensor&, tensor&);
	void run() override;
};

class add : public binary_op
{
public:
	add() : binary_op() {}
	static void kernel(tensor&, tensor&, tensor&);
};


class sub : public binary_op
{
public:
	sub() : binary_op() {}
	static void kernel(tensor&, tensor&, tensor&);
};


class mul : public binary_op
{
public:
	mul() : binary_op() {}
	static void kernel(tensor&, tensor&, tensor&);
};


class true_div : public binary_op
{
public:
	true_div() : binary_op() {}
	static void kernel(tensor&, tensor&, tensor&);
};
