/*

Copyright (c) 2008, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include "libtorrent/lazy_entry.hpp"

namespace libtorrent
{
	int fail_bdecode() { return -1; }

	// fills in 'val' with what the string between start and the
	// first occurance of the delimiter is interpreted as an int.
	// return the pointer to the delimiter, or 0 if there is a
	// parse error. val should be initialized to zero
	char* parse_int(char* start, char* end, char delimiter, boost::int64_t& val)
	{
		while (start < end && *start != delimiter)
		{
			if (!std::isdigit(*start)) { fail_bdecode(); return 0; }
			val *= 10;
			val += *start - '0';
			++start;
		}
		return start;
	}

	char* find_char(char* start, char* end, char delimiter)
	{
		while (start < end && *start != delimiter) ++start;
		return start;
	}

	// return 0 = success
	int lazy_bdecode(char* start, char* end, lazy_entry& ret, int depth_limit)
	{
		ret.clear();
		if (start == end) return 0;

		std::vector<lazy_entry*> stack;

		stack.push_back(&ret);
		while (start < end)
		{
			if (stack.empty()) break; // done!

			lazy_entry* top = stack.back();

			if (stack.size() > depth_limit) return fail_bdecode();
			if (start == end) return fail_bdecode();
			char t = *start;
			*start = 0; // null terminate any previous string
			++start;
			if (start == end && t != 'e') return fail_bdecode();

			switch (top->type())
			{
				case lazy_entry::dict_t:
				{
					if (t == 'e')
					{
						stack.pop_back();
						continue;
					}
					boost::int64_t len = t - '0';
					start = parse_int(start, end, ':', len);
					if (start == 0 || start + len + 3 > end || *start != ':') return fail_bdecode();
					++start;
					lazy_entry* ent = top->dict_append(start);
					start += len;
					stack.push_back(ent);
					t = *start;
					*start = 0; // null terminate any previous string
					++start;
					break;
				}
				case lazy_entry::list_t:
				{
					if (t == 'e')
					{
						stack.pop_back();
						continue;
					}
					lazy_entry* ent = top->list_append();
					stack.push_back(ent);
					break;
				}
				default: break;
			}

			top = stack.back();
			switch (t)
			{
				case 'd':
					top->construct_dict();
					continue;
				case 'l':
					top->construct_list();
					continue;
				case 'i':
					top->construct_int(start);
					start = find_char(start, end, 'e');
					if (start == end) return fail_bdecode();
					TORRENT_ASSERT(*start == 'e');
					*start++ = 0;
					stack.pop_back();
					continue;
				default:
				{
					if (!std::isdigit(t)) return fail_bdecode();

					boost::int64_t len = t - '0';
					start = parse_int(start, end, ':', len);
					if (start == 0 || start + len + 1 > end || *start != ':') return fail_bdecode();
					++start;
					top->construct_string(start);
					stack.pop_back();
					start += len;
					continue;
				}
			}
			return 0;
		}
		return 0;
	}

	boost::int64_t lazy_entry::int_value() const
	{
		TORRENT_ASSERT(m_type == int_t);
		boost::int64_t val = 0;
		bool negative = false;
		if (*m_data.start == '-') negative = true;
		parse_int(negative?m_data.start+1:m_data.start, m_data.start + 100, 0, val);
		if (negative) val = -val;
		return val;
	}

	lazy_entry* lazy_entry::dict_append(char* name)
	{
		TORRENT_ASSERT(m_type == dict_t);
		TORRENT_ASSERT(m_size <= m_capacity);
		if (m_capacity == 0)
		{
			int capacity = 10;
			m_data.dict = new (std::nothrow) std::pair<char*, lazy_entry>[capacity];
			if (m_data.dict == 0) return 0;
			m_capacity = capacity;
		}
		else if (m_size == m_capacity)
		{
			int capacity = m_capacity * 2;
			std::pair<char*, lazy_entry>* tmp = new (std::nothrow) std::pair<char*, lazy_entry>[capacity];
			if (tmp == 0) return 0;
			std::memcpy(tmp, m_data.dict, sizeof(std::pair<char*, lazy_entry>) * m_size);
			delete[] m_data.dict;
			m_data.dict = tmp;
			m_capacity = capacity;
		}

		TORRENT_ASSERT(m_size < m_capacity);
		std::pair<char*, lazy_entry>& ret = m_data.dict[m_size++];
		ret.first = name;
		return &ret.second;
	}

	lazy_entry* lazy_entry::dict_find(char const* name)
	{
		TORRENT_ASSERT(m_type == dict_t);
		for (int i = 0; i < m_size; ++i)
		{
			if (strcmp(name, m_data.dict[i].first) == 0)
				return &m_data.dict[i].second;
		}
		return 0;
	}

	lazy_entry* lazy_entry::list_append()
	{
		TORRENT_ASSERT(m_type == list_t);
		TORRENT_ASSERT(m_size <= m_capacity);
		if (m_capacity == 0)
		{
			int capacity = 10;
			m_data.list = new (std::nothrow) lazy_entry[capacity];
			if (m_data.list == 0) return 0;
			m_capacity = capacity;
		}
		else if (m_size == m_capacity)
		{
			int capacity = m_capacity * 2;
			lazy_entry* tmp = new (std::nothrow) lazy_entry[capacity];
			if (tmp == 0) return 0;
			std::memcpy(tmp, m_data.list, sizeof(lazy_entry) * m_size);
			delete[] m_data.list;
			m_data.list = tmp;
			m_capacity = capacity;
		}

		TORRENT_ASSERT(m_size < m_capacity);
		return m_data.list + (m_size++);
	}

	void lazy_entry::clear()
	{
		switch (m_type)
		{
			case list_t: delete[] m_data.list; break;
			case dict_t: delete[] m_data.dict; break;
			default: break;
		}
		m_size = 0;
		m_capacity = 0;
		m_type = none_t;
	}
};

