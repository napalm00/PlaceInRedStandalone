/*
* The MIT License
* Copyright 2017-2019 naPalm / PapaRadroach
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute,
* sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
* LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
* THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include "stdafx.h"

//#define INTRIN_SCAN 1

// Credits: gir489/NTAuthority/DarthTon/Forza

struct Pattern
{
public:
	struct PatternByte
	{
		PatternByte() : ignore(true)
		{
			//
		}

		PatternByte(std::string byteString, bool ignoreThis = false)
		{
			data = StringToUint8(byteString);
			ignore = ignoreThis;
		}

		bool ignore;
		UINT8 data;

		static UINT8 StringToUint8(std::string str)
		{
			std::istringstream iss(str);

			UINT32 ret;

			if(iss >> std::hex >> ret)
			{
				return (UINT8)ret;
			}

			return 0;
		}
	};

	// Credits to gir489/NTAuthority: https://bitbucket.org/gir489/m0d-s0beit-v-redux/
	static DWORD64 Scan(DWORD64 dwStart, DWORD64 dwLength, std::string s)
	{
		std::vector<PatternByte> p;
		std::istringstream iss(s);
		std::string w;

		while(iss >> w)
		{
			if(w.data()[0] == '?')
			{ // Wildcard
				p.push_back(PatternByte());
			}
			else if(w.length() == 2 && isxdigit(w.data()[0]) && isxdigit(w.data()[1]))
			{ // Hex
				p.push_back(PatternByte(w));
			}
			else
			{
				return NULL; // You dun fucked up
			}
		}

		for(DWORD64 i = 0; i < dwLength; i++)
		{
			UINT8* lpCurrentByte = (UINT8*)(dwStart + i);

			bool found = true;

			for(size_t ps = 0; ps < p.size(); ps++)
			{
				if(p[ps].ignore == false && lpCurrentByte[ps] != p[ps].data)
				{
					found = false;
					break;
				}
			}

			if(found)
			{
				return (DWORD64)lpCurrentByte;
			}
		}

		return NULL;
	}

#ifdef INTRIN_SCAN
	// Credits to DarthTon: https://github.com/DarthTon/findpattern-bench/blob/master/patterns/DarthTon.h
	static DWORD64 ScanFastIntrin(DWORD64 dwStart, DWORD64 dwLength, std::string s)
	{
		std::vector<UINT8> p;
		std::string mask;
		std::istringstream iss(s);
		std::string w;

		while(iss >> w)
		{
			if(w.data()[0] == '?')
			{ // Wildcard
				p.push_back(0x00);
				mask += "?";
			}
			else if(w.length() == 2 && isxdigit(w.data()[0]) && isxdigit(w.data()[1]))
			{ // Hex
				p.push_back(PatternByte::StringToUint8(w));
				mask += "x";
			}
			else
			{
				return NULL; // You dun fucked up
			}
		}

		return DarthTonScan((UINT8*)dwStart, dwLength, p.data(), mask.c_str());
    }
#endif

	// Credits to DarthTon: https://www.unknowncheats.me/forum/1064177-post5.html
	static DWORD64 ScanFastStd(void* dwStart, DWORD64 dwLength, std::string s)
	{
		const UINT8* cstart = (const UINT8*)dwStart;
		const UINT8* cend = cstart + dwLength;

		std::vector<UINT8> p;
		std::istringstream iss(s);
		std::string w;

		while(iss >> w)
		{
			if(w.data()[0] == '?') // Wildcard
			{
				p.push_back(0xCC);
			}
			else if(w.length() == 2 && isxdigit(w.data()[0]) && isxdigit(w.data()[1])) // Hex
			{
				p.push_back(PatternByte::StringToUint8(w));
			}
			else
			{
				return NULL; // You dun fucked up
			}
		}

		const UINT8* res = std::search(cstart, cend, p.begin(), p.end(),
			[](UINT8 val1, UINT8 val2)
		{
			return (val1 == val2 || val2 == 0xCC);
		});

		return (DWORD64)res;
	}

    static DWORD64 Scan(MODULEINFO mi, std::string s)
    {
        return Scan((DWORD64)mi.lpBaseOfDll, (DWORD64)mi.SizeOfImage, s);
    }

	static DWORD64 ScanFast(MODULEINFO mi, std::string s)
	{
#ifdef INTRIN_SCAN
		if(IntrinSupported())
		{
			return ScanFastIntrin((DWORD64)mi.lpBaseOfDll, (DWORD64)mi.SizeOfImage, s);
		}
		else
		{
			return ScanFastStd(mi.lpBaseOfDll, (DWORD64)mi.SizeOfImage, s);
		}
#else
		return ScanFastStd(mi.lpBaseOfDll, (DWORD64)mi.SizeOfImage, s);
#endif
	}

#ifdef INTRIN_SCAN
private:

	struct PartData
	{
		int32_t mask = 0;
		__m128i needle = {0};
	};

	// Credits to DarthTon: https://github.com/DarthTon/findpattern-bench/blob/master/patterns/DarthTon.h
	static const DWORD64 DarthTonScan(const UINT8* data, const DWORD64 size, const UINT8* pattern, const char* mask)
	{
		const UINT8* result = nullptr;
		auto len = strlen(mask);
		auto first = strchr(mask, '?');
		size_t len2 = (first != nullptr) ? (first - mask) : len;
		auto firstlen = min(len2, 16);
		intptr_t num_parts = (len < 16 || len % 16) ? (len / 16 + 1) : (len / 16);
		PartData parts[4];

		for(intptr_t i = 0; i < num_parts; ++i, len -= 16)
		{
			for(size_t j = 0; j < min(len, 16) - 1; ++j)
				if(mask[16 * i + j] == 'x')
					_bittestandset((LONG*)&parts[i].mask, j);

			parts[i].needle = _mm_loadu_si128((const __m128i*)(pattern + i * 16));
		}

#pragma omp parallel for
		for(intptr_t i = 0; i < static_cast<intptr_t>(size) / 32 - 1; ++i)
		{
			auto block = _mm256_loadu_si256((const __m256i*)data + i);
			if(_mm256_testz_si256(block, block))
				continue;

			auto offset = _mm_cmpestri(parts->needle, firstlen, _mm_loadu_si128((const __m128i*)(data + i * 32)), 16, _SIDD_CMP_EQUAL_ORDERED);
			if(offset == 16)
			{
				offset += _mm_cmpestri(parts->needle, firstlen, _mm_loadu_si128((const __m128i*)(data + i * 32 + 16)), 16, _SIDD_CMP_EQUAL_ORDERED);
				if(offset == 32)
					continue;
			}

			for(intptr_t j = 0; j < num_parts; ++j)
			{
				auto hay = _mm_loadu_si128((const __m128i*)(data + (2 * i + j) * 16 + offset));
				auto bitmask = _mm_movemask_epi8(_mm_cmpeq_epi8(hay, parts[j].needle));
				if((bitmask & parts[j].mask) != parts[j].mask)
					goto next;
			}

			result = data + 32 * i + offset;
			break;

		next:;
		}

		return (DWORD64)result;
	}

	// Credits to Forza: https://github.com/learn-more/findpattern-bench/blob/master/patterns/Forza.h
	static bool IntrinSupported()
	{
		int id[4] = {0};
		__cpuid(id, 1);

		bool sse42 = (id[3] & 0x04000000) != 0;
		bool avx = (id[2] & 0x18000000) != 0;

		return (sse42 && avx);
	}
#endif
};