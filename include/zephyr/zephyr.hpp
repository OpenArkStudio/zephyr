/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 * 
 * author   : zhllxt
 * email    : 37792738@qq.com
 *
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ZEPHYR_HPP__
#define __ZEPHYR_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(_MSC_VER) && !defined(NOMINMAX)
#  define ZEPHYR_NOMINMAX
#  define NOMINMAX
#endif

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:4191) // asio inner : from FARPROC to cancel_io_ex_t is unsafe
#  pragma warning(disable:4996) // warning STL4009: std::allocator<void> is deprecated in C++17, _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#endif

#include <zephyr/version.hpp>
#include <zephyr/config.hpp>

#include <zephyr/base/timer.hpp>
#include <zephyr/tcp/tcp_client.hpp>
#include <zephyr/tcp/tcp_server.hpp>
#include <zephyr/udp/udp_client.hpp>
#include <zephyr/udp/udp_server.hpp>
#include <zephyr/udp/udp_cast.hpp>

#ifdef ZEPHYR_ENABLE_RPC
    #include <zephyr/rpc/rpc_client.hpp>
    #include <zephyr/rpc/rpc_server.hpp>
#endif

#include <zephyr/icmp/ping.hpp>
#include <zephyr/scp/scp.hpp>

#if defined(ZEPHYR_USE_SSL)
	#include <zephyr/tcp/tcps_client.hpp>
	#include <zephyr/tcp/tcps_server.hpp>
#endif

#ifndef ASIO_STANDALONE
	#include <zephyr/http/http_client.hpp>
	#include <zephyr/http/http_server.hpp>
	#include <zephyr/http/ws_client.hpp>
	#include <zephyr/http/ws_server.hpp>
	#include <zephyr/http/httpws_server.hpp>
	#if defined(ZEPHYR_USE_SSL)
		#include <zephyr/http/https_client.hpp>
		#include <zephyr/http/https_server.hpp>
		#include <zephyr/http/wss_client.hpp>
		#include <zephyr/http/wss_server.hpp>
		#include <zephyr/http/httpwss_server.hpp>
	#endif
#endif // ASIO_STANDALONE

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

#if defined(_MSC_VER) && defined(NOMINMAX) && defined(ZEPHYR_NOMINMAX)
#  undef NOMINMAX
#  undef ZEPHYR_NOMINMAX
#endif

#endif // !__ZEPHYR_HPP__
