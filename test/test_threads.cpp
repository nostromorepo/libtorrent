/*

Copyright (c) 2010, Arvid Norberg
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

#include <boost/bind.hpp>
#include <list>
#include "libtorrent/thread.hpp"
#include "test.hpp"

using namespace libtorrent;

void fun(condition* s, mutex* m, int i)
{
	fprintf(stderr, "thread %d waiting\n", i);
	mutex::scoped_lock l(*m);
	s->wait(l);
	fprintf(stderr, "thread %d done\n", i);
}

int test_main()
{
	condition cond;
	mutex m;
	std::list<thread*> threads;
	for (int i = 0; i < 20; ++i)
	{
		threads.push_back(new thread(boost::bind(&fun, &cond, &m, i)));
	}

	// make sure all threads are waiting on the condition
	sleep(10);

	mutex::scoped_lock l(m);
	cond.signal_all(l);
	l.unlock();

	for (std::list<thread*>::iterator i = threads.begin(); i != threads.end(); ++i)
	{
		(*i)->join();
		delete *i;
	}

	return 0;
}

