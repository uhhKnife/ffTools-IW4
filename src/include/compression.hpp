#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include "miniz.h"

namespace compression
{
	inline void* myalloc(void* opaque, size_t items, size_t size)
	{
		(void)opaque;
		return calloc(items, size);
	}

	inline void myfree(void* opaque, void* address)
	{
		(void)opaque;
		free(address);
	}

	inline std::vector<unsigned char> compress_data(const unsigned char* data, size_t data_len)
	{
		mz_stream c_stream;
		memset(&c_stream, 0, sizeof(c_stream));

		c_stream.zalloc = reinterpret_cast<mz_alloc_func>(myalloc);
		c_stream.zfree = reinterpret_cast<mz_free_func>(myfree);
		c_stream.opaque = nullptr;
		c_stream.next_in = data;
		c_stream.avail_in = static_cast<unsigned int>(data_len);

		int err = mz_deflateInit(&c_stream, MZ_DEFAULT_COMPRESSION);
		if (err != MZ_OK)
			return std::vector<unsigned char>();

		std::vector<unsigned char> output;
		const size_t buf_size = 4096;
		unsigned char buf[buf_size];

		for (;;)
		{
			c_stream.next_out = buf;
			c_stream.avail_out = buf_size;

			err = mz_deflate(&c_stream, MZ_FINISH);

			size_t have = buf_size - c_stream.avail_out;
			output.insert(output.end(), buf, buf + have);

			if (err == MZ_STREAM_END)
				break;

			if (err != MZ_OK)
			{
				mz_deflateEnd(&c_stream);
				return std::vector<unsigned char>();
			}
		}

		err = mz_deflateEnd(&c_stream);

		return output;
	}

	inline std::vector<unsigned char> decompress_data(const unsigned char* data, size_t data_len, size_t max_size = 10 * 1024 * 1024)
	{
		mz_stream d_stream;
		memset(&d_stream, 0, sizeof(d_stream));

		d_stream.zalloc = reinterpret_cast<mz_alloc_func>(myalloc);
		d_stream.zfree = reinterpret_cast<mz_free_func>(myfree);
		d_stream.opaque = nullptr;
		d_stream.next_in = data;
		d_stream.avail_in = static_cast<unsigned int>(data_len);

		int err = mz_inflateInit(&d_stream);
		if (err != MZ_OK)
			return std::vector<unsigned char>();

		std::vector<unsigned char> output;
		const size_t buf_size = 4096;
		unsigned char buf[buf_size];

		for (;;)
		{
			d_stream.next_out = buf;
			d_stream.avail_out = buf_size;

			err = mz_inflate(&d_stream, MZ_NO_FLUSH);

			size_t have = buf_size - d_stream.avail_out;
			if (have > 0)
				output.insert(output.end(), buf, buf + have);

			if (err == MZ_STREAM_END)
				break;

			if (err != MZ_OK)
			{
				mz_inflateEnd(&d_stream);
				return std::vector<unsigned char>();
			}

			if (output.size() > max_size)
			{
				mz_inflateEnd(&d_stream);
				return std::vector<unsigned char>();
			}
		}

		mz_inflateEnd(&d_stream);
		return output;
	}
}
