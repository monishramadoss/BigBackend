#pragma once
#include "compute.h"
#include "init.h"
#include "types.h"
#include "tensor.h"

namespace naive
{
	class matmul final : public compute_job
	{
	public:
		matmul(float alpha=1.0, float beta=1.0);

		tensor batch(tensor& x, tensor& w, tensor& y);
		tensor operator()(tensor& x, tensor& w);
		void kernel(tensor& x, tensor& w, tensor& y);
		tensor operator()(tensor& x, tensor& w, tensor& z);

		void run() override;

	private:
		float alpha;
		float beta;
	};

	class relu : public compute_job
	{
	public:
		relu(bool in_place=false);
		tensor operator()(tensor&);
		void kernel(tensor&, tensor&);
		void run() override;
	private:
		bool in_place;
	};

}
