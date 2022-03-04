#pragma once
#include "compute.h"
#include "types.h"
#include "tensor.h"

namespace naive
{
	class matmul final : public compute_job
	{
	public:
		matmul(float alpha=1.0, float beta=1.0);
		static void kernel(tensor&, tensor&, tensor&);
		static void kernel(tensor&, tensor&, tensor&, tensor&);
		void run();
	private:
		float alpha;
		float beta;
		int split_dim = 0;
	};

}
