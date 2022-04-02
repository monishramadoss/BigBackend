#pragma once
#include <vector>
#include <numeric>
#include <functional>
#include <algorithm>

#include "device.h"
#include "tensor.h"
// TODO construct compute flow graph
// TODO construct data flow graph from compute flow
// TODO update compute flow graph with updated data flow with sync and async pipelines


class compute_job
{

private:
	void split_kernel();

protected:
	std::vector<compute_job> parallel_kernels;
	device dev = get_avalible_device();
	int state; // -1 = error 0 = running 1 = transit 2 = complete
	
public:
	compute_job();
	compute_job(compute_job&);
	compute_job(const compute_job&);
	compute_job(compute_job&&) noexcept;

	size_t local_task_id;

	std::vector<tensor> m_inputs;
	std::vector<tensor> m_outputs;

	dim_vec similar;
	dim_vec differ;

	void push_input(tensor& input);
	void push_output(tensor& output);

	virtual void run() { state = 2; }
	virtual ~compute_job();
};



#ifdef VULKAN


static std::vector<uint32_t> compileSource(const std::string& source)
{
	if (system(std::string("glslangValidator --stdin -S comp -V -o tmp_kp_shader.comp.spv << END\n" + source + "\nEND").c_str()))
		throw std::runtime_error("Error running glslangValidator command");
	std::ifstream fileStream("./tmp_kp_shader.comp.spv", std::ios::binary);
	std::vector<unsigned char> buffer;
	buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(fileStream), {});
	return { reinterpret_cast<uint32_t*>(buffer.data()), reinterpret_cast<uint32_t*>(buffer.data() + buffer.size()) };
}

#endif