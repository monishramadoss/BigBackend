#pragma once
#include <vector>
#include <map>
#include "device.h"

// TODO construct compute flow graph
// TODO construct data flow graph from compute flow
// TODO update compute flow graph with updated data flow with sync and async pipelines

static int global_task_id{ 0 };
static std::vector<std::vector<int>>  compute_adj_list = {};

class compute_job
{
protected:
	std::vector<compute_job> parallel_kernels;
	int local_task_id;
	device dev = get_avalible_device();
	int state; // -1 = error 0 = running 1 = transit 2 = complete
public:
	virtual ~compute_job() = default;
	compute_job();

	//compute_job(compute_job&) = default;
	//compute_job(const compute_job&) = default;
	compute_job(compute_job&&) = default;

	virtual void run() { state = 2; }
};



#ifdef VULKAN

static std::vector<uint32_t> compileSource(
	const std::string& source)
{
	if (system(std::string("glslangValidator --stdin -S comp -V -o tmp_kp_shader.comp.spv << END\n" + source + "\nEND").c_str()))
		throw std::runtime_error("Error running glslangValidator command");
	std::ifstream fileStream("./tmp_kp_shader.comp.spv", std::ios::binary);
	std::vector<unsigned char> buffer;
	buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
	return { reinterpret_cast<uint32_t*>(buffer.data()), reinterpret_cast<uint32_t*>(buffer.data() + buffer.size()) };
}

#endif