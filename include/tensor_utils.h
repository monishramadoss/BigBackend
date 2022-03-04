#ifndef  T_UTILS
#define T_UTILS


#include <vector>
#include <numeric>
#include <algorithm>
#include <string>
#include <iostream>

static void rewrite_stride_size(const dim_vec& shape, dim_vec& stride, dim_vec& size) {
	stride.resize(shape.size());
	size.resize(shape.size() + 1);

	stride[0] = 1;
	size[shape.size()] = 1;

	for (_int i = 1; i < shape.size(); ++i)
		stride[i] = shape[i - 1] * stride[i - 1];

	for (_int i = shape.size() - 1;;) {
		size[i] = shape[i] * size[i + 1];
		if (i == 0)
			break;
		--i;
	}
	//size[shape.size()] = shape[shape.size() - 1] * size[shape.size() - 1];
	std::reverse(stride.begin(), stride.end());
}

static _int count(const dim_vec& shape, _int start_axis = 0, int64_t end_axis = -1)
{
	if (end_axis == -1) end_axis = static_cast<int>(shape.size());
	if (start_axis == end_axis) return 0;
	
	return  std::accumulate<std::vector<_int>::const_iterator, _int>
	(shape.begin() + start_axis, shape.begin() + end_axis, 1,  std::multiplies<_int>());
}


static void axis_idx(int64_t axis, _int idx, _int t_size, const dim_vec& shape, const dim_vec& stride, offset& offsets)
{
	if (shape[axis] < idx)
		std::cerr << "UNABLE TO SPLIT ACCROSS IDX" << std::endl;

	if (axis == -1)
		axis = shape.size() - 1;

	const _int upper_split_size = axis == 0 ? 1 : count(shape, 0, axis);
	const _int lower_split_size = count(shape, axis, -1) * t_size;

	_int split_stride = stride[axis];
	const _int off = split_stride * idx * t_size;

	std::vector<_int> dst_shape = shape;
	dst_shape[axis] = 1;

	for (_int i = 0; i < upper_split_size; ++i)
		offsets.emplace_back(std::make_pair((off + i * lower_split_size), split_stride));

}

static void multi_idx(std::vector<std::string>& idx, const dim_vec& shape, const dim_vec& stride, offset& offsets)
{
	dim_vec unk_axis;
	dim_vec kn_axis;

	
}

//
//
//struct Object
//{
//    struct Proxy_View
//    {
//        Object* mObj;
//        int mI;
//        Proxy_View(Object* obj, int i) : mObj(obj), mI(i) { }
//
//        int operator[](int j)
//        {
//            return mI * j;
//        }
//    };
//
//    Proxy_View operator[](int i)
//    {
//        return Proxy_View(this, i);
//    }
//};
//
//#include <iostream>
//#include <vector>
//#include <array>
//#include <iterator>
//
//template<class T>
//struct slice {
//    T* data = 0;
//    std::_int const* stride = 0;
//    slice operator[](std::_int I) const {
//        return { data + I  * stride, stride + 1 };
//    }
//    operator T& ()const {
//        return *data;
//    }
//    T& operator=(T in) {
//        *data = std::move(in);
//    	return *data;
//    }
//};
//
//
//template<class T>
//struct buffer {
//    std::vector<T> data;
//    std::vector<std::_int> strides;
//
//    buffer(std::vector<std::_int> sizes, std::vector<T> d) :
//        data(std::move(d)),  strides(sizes)
//    {
//        std::_int scale = 1;
//        for (std::_int i = 0; i < sizes.size(); ++i) {
//            auto next = scale * strides[sizes.size() - 1 - i];
//            strides[sizes.size() - 1 - i] = scale;
//            scale = next;
//        }
//    }
//    slice<T> get() { return { data.data(), strides.data() }; }
//};
//
//void test_indexing()
//{
//    buffer<int> b({ 2,5,3 }, std::vector<int>(2 * 5 * 3));
//    
//    for (int i = 0; i < 2; i++)
//    {
//        for (int j = 0; j < 5; j++)
//        {
//            for (int k = 0; k < 3; k++)
//            {
//                b.get()[i][j][k] = 3 * (5 * (i)+j) + k + 1;
//            }
//        }
//    }
//    std::copy(b.data.data(), b.data.data() + 2 * 5 * 3, std::ostream_iterator<int>(std::cout, " "));
//    std::cout << std::endl;
//}
//


#endif // !1
