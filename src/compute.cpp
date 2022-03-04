#include "compute.h"

compute_job::compute_job() : local_task_id(global_task_id++), state(-1)
{
	compute_adj_list.emplace_back();
}
