#pragma once
#include <fstream>

#include <string>
#include <vector>

#include "device.h"
#include "types.h"

static _int io_{0};

class base_storage
{
public:
	base_storage(_int);
	base_storage(const byte_*, _int);
	base_storage(const base_storage&);

	[[nodiscard]] byte_* get_data() const
	{
		return data.get();
	}
	void set_data(const byte_*, _int, _int, _int) const;
	void offload();
	void onload();
	~base_storage();
protected:
	std::unique_ptr<byte_> data;
	_int byte_size;
	_int device_offset;
	device& dev = get_avalible_device();
	std::string file_name;
	std::fstream  afile;
};


class batch_storage: public base_storage
{
public:
	batch_storage(byte_*, _int, _int);
	batch_storage(_int, _int);
	[[nodiscard]] byte_* get_data(_int idx) const { return b_data[idx].get_data(); }
	void set_data(const byte_*, _int, _int, _int, _int) const;
	static void set_data(base_storage&, _int);

protected:
	std::vector<base_storage> b_data;
	_int batch_size;
};