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

#ifndef TORRENT_HTTP_SEED_CONNECTION_HPP_INCLUDED
#define TORRENT_HTTP_SEED_CONNECTION_HPP_INCLUDED

#include <ctime>
#include <algorithm>
#include <deque>
#include <string>

#include "libtorrent/debug.hpp"

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif

#include <boost/smart_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/array.hpp>
#include <boost/optional.hpp>
#include <boost/cstdint.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "libtorrent/config.hpp"
#include "libtorrent/peer_connection.hpp"
#include "libtorrent/disk_buffer_holder.hpp"
#include "libtorrent/torrent.hpp"
#include "libtorrent/piece_block_progress.hpp"
#include "libtorrent/http_parser.hpp"

namespace libtorrent
{
	class torrent;
	struct peer_request;

	namespace detail
	{
		struct session_impl;
	}

	class TORRENT_EXPORT http_seed_connection
		: public peer_connection
	{
	friend class invariant_access;
	public:

		// this is the constructor where the we are the active part.
		// The peer_conenction should handshake and verify that the
		// other end has the correct id
		http_seed_connection(
			aux::session_impl& ses
			, boost::weak_ptr<torrent> t
			, boost::shared_ptr<socket_type> s
			, tcp::endpoint const& remote
			, std::string const& url
			, policy::peer* peerinfo);
		void start();

		~http_seed_connection();

		virtual int type() const { return peer_connection::http_seed_connection; }

		// called from the main loop when this connection has any
		// work to do.
		void on_sent(error_code const& error
			, std::size_t bytes_transferred);
		void on_receive(error_code const& error
			, std::size_t bytes_transferred);
			
		std::string const& url() const { return m_url; }
		
		virtual void get_specific_peer_info(peer_info& p) const;
		virtual bool in_handshake() const;

		// the following functions appends messages
		// to the send buffer
		void write_choke() {}
		void write_unchoke() {}
		void write_interested() {}
		void write_not_interested() {}
		void write_request(peer_request const& r);
		void write_cancel(peer_request const& r) {}
		void write_have(int index) {}
		void write_piece(peer_request const& r, disk_buffer_holder& buffer) { TORRENT_ASSERT(false); }
		void write_keepalive() {}
		void on_connected();
		void write_reject_request(peer_request const&) {}
		void write_allow_fast(int) {}
		void write_suggest(int piece) {}

#ifdef TORRENT_DEBUG
		void check_invariant() const;
#endif

	private:

		// returns the block currently being
		// downloaded. And the progress of that
		// block. If the peer isn't downloading
		// a piece for the moment, the boost::optional
		// will be invalid.
		boost::optional<piece_block_progress> downloading_piece_progress() const;

		// this has one entry per bittorrent request
		std::deque<peer_request> m_requests;

		std::string m_server_string;
		http_parser m_parser;
		std::string m_auth;
		std::string m_host;
		int m_port;
		std::string m_path;
		std::string m_url;
			
		// the first request will contain a little bit more data
		// than subsequent ones, things that aren't critical are left
		// out to save bandwidth.
		bool m_first_request;

		// the number of bytes left to receive of the response we're
		// currently parsing
		int m_response_left;
		
		// the number of bytes in the start of the receive buffer
		// that's http header
		int m_body_start;
	};
}

#endif // TORRENT_WEB_PEER_CONNECTION_HPP_INCLUDED

