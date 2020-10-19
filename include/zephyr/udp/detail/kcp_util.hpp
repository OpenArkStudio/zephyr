/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 *
 * KCP MTU , UDP PACKET MAX LENGTH 576
 */

#ifndef __ZEPHYR_KCP_UTIL_HPP__
#define __ZEPHYR_KCP_UTIL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <memory>
#include <future>
#include <utility>
#include <string_view>
#include <chrono>

#include <zephyr/base/selector.hpp>
#include <zephyr/base/error.hpp>
#include <zephyr/base/detail/condition_wrap.hpp>

#include <zephyr/util/ikcp.h>

namespace zephyr::detail::kcp {
// __attribute__((aligned(1)))
#if defined(__GNUC__) || defined(__GNUG__)
#define __KCPHDR_ONEBYTE_ALIGN__ __attribute__((packed))
#elif defined(_MSC_VER)
#define __KCPHDR_ONEBYTE_ALIGN__
#pragma pack(push, 1)
#endif
////	struct psdhdr
////	{
////		unsigned long saddr;
////		unsigned long daddr;
////		char mbz;
////		char ptcl;
////		unsigned short tcpl;
////	}__KCPHDR_ONEBYTE_ALIGN__;
////
////	struct tcphdr
////	{
////		std::uint16_t th_sport;        /* source port */
////		std::uint16_t th_dport;        /* destination port */
////		std::uint32_t th_seq;          /* sequence number */
////		std::uint32_t th_ack;          /* acknowledgement number */
////#if IWORDS_BIG_ENDIAN
////		std::uint16_t th_off : 4;      /* data offset */
////		std::uint16_t th_x2 : 6;       /* (unused) */
////#else
////		std::uint16_t th_x2 : 6;       /* (unused) */
////		std::uint16_t th_off : 4;      /* data offset */
////#endif
////		std::uint16_t thf_urg : 1;     /* flags : Urgent Pointer Field Significant */
////		std::uint16_t thf_ack : 1;     /* flags : Acknowledgement field significant */
////		std::uint16_t thf_psh : 1;     /* flags : Push Function */
////		std::uint16_t thf_rst : 1;     /* flags : Reset the connection */
////		std::uint16_t thf_syn : 1;     /* flags : Synchronize sequence numbers */
////		std::uint16_t thf_fin : 1;     /* flags : No more data from sender */
////		std::uint16_t th_win;          /* window */
////		std::uint16_t th_sum;          /* checksum */
////		std::uint16_t th_urp;          /* urgent pointer */
////		std::uint32_t th_option : 24;  /* option */
////		std::uint32_t th_padding : 8;  /* padding */
////	}__KCPHDR_ONEBYTE_ALIGN__;
//
//	struct kcphdr
//	{
//		std::uint32_t th_seq;
//		std::uint32_t th_ack;
//		std::uint16_t thf_urg : 1;
//		std::uint16_t thf_ack : 1;
//		std::uint16_t thf_psh : 1;
//		std::uint16_t thf_rst : 1;
//		std::uint16_t thf_syn : 1;
//		std::uint16_t thf_fin : 1;
//		std::uint16_t th_padding : 10;
//		std::uint16_t th_sum;
//	}__KCPHDR_ONEBYTE_ALIGN__;
//#if defined(__GNUC__) || defined(__GNUG__)
//	#undef __KCPHDR_ONEBYTE_ALIGN__
//#elif defined(_MSC_VER)
//	#pragma pack(pop)
//	#undef __KCPHDR_ONEBYTE_ALIGN__
//#endif
//
//	template<typename = void>
//	unsigned short checksum(unsigned short * addr, int count)
//	{
//		long sum = 0;
//		while (count > 1)
//		{
//			sum += *(unsigned short*)addr++;
//			count -= 2;
//		}
//		if (count > 0)
//		{
//			std::uint8_t left_over[2] = { 0 };
//			left_over[0] = static_cast<std::uint8_t>(*addr);
//			sum += *(unsigned short*)left_over;
//		}
//		while (sum >> 16)
//			sum = (sum & 0xffff) + (sum >> 16);
//		return static_cast<unsigned short>(~sum);
//	}
//
//	template<typename = void>
//	inline bool is_kcphdr_syn(std::string_view s)
//	{
//		if (s.size() != sizeof(kcphdr))
//			return false;
//
//		kcphdr * hdr = (kcphdr*)(s.data());
//		if (!(!hdr->thf_urg && !hdr->thf_ack && !hdr->thf_psh && !hdr->thf_rst && hdr->thf_syn && !hdr->thf_fin))
//			return false;
//
//		return (hdr->th_sum == checksum(reinterpret_cast<unsigned short *>(hdr),
//			static_cast<int>(sizeof(kcphdr) - sizeof(kcphdr::th_sum))));
//	}
//
//	template<typename = void>
//	inline bool is_kcphdr_synack(std::string_view s, std::uint32_t seq)
//	{
//		if (s.size() != sizeof(kcphdr))
//			return false;
//
//		kcphdr * hdr = (kcphdr*)(s.data());
//		if (!(!hdr->thf_urg && hdr->thf_ack && !hdr->thf_psh && !hdr->thf_rst && hdr->thf_syn && !hdr->thf_fin))
//			return false;
//
//		if (hdr->th_ack != seq + 1)
//			return false;
//
//		return (hdr->th_sum == checksum(reinterpret_cast<unsigned short *>(hdr),
//			static_cast<int>(sizeof(kcphdr) - sizeof(kcphdr::th_sum))));
//	}
//
//	template<typename = void>
//	inline bool is_kcphdr_ack(std::string_view s, std::uint32_t seq)
//	{
//		if (s.size() != sizeof(kcphdr))
//			return false;
//
//		kcphdr * hdr = (kcphdr*)(s.data());
//		if (!(!hdr->thf_urg && hdr->thf_ack && !hdr->thf_psh && !hdr->thf_rst && !hdr->thf_syn && !hdr->thf_fin))
//			return false;
//
//		if (hdr->th_ack != seq + 1)
//			return false;
//
//		return (hdr->th_sum == checksum(reinterpret_cast<unsigned short *>(hdr),
//			static_cast<int>(sizeof(kcphdr) - sizeof(kcphdr::th_sum))));
//	}
//
//	template<typename = void>
//	inline bool is_kcphdr_fin(std::string_view s)
//	{
//		if (s.size() != sizeof(kcphdr))
//			return false;
//
//		auto * hdr = (kcphdr*)(s.data());
//		if (!(!hdr->thf_urg && !hdr->thf_ack && !hdr->thf_psh && !hdr->thf_rst && !hdr->thf_syn && hdr->thf_fin))
//			return false;
//
//		return (hdr->th_sum == checksum(reinterpret_cast<unsigned short *>(hdr),
//			static_cast<int>(sizeof(kcphdr) - sizeof(kcphdr::th_sum))));
//	}
//
//	template<typename = void>
//	inline kcphdr make_kcphdr_syn(std::uint32_t seq)
//	{
//        kcphdr hdr;
//        memset(&hdr, 0x0, sizeof(kcphdr));
//		hdr.th_seq = seq;
//		hdr.thf_syn = 1;
//		hdr.th_sum = checksum(reinterpret_cast<unsigned short *>(&hdr),
//			static_cast<int>(sizeof(kcphdr) - sizeof(kcphdr::th_sum)));
//
//		return hdr;
//	}
//
//	template<typename = void>
//	inline kcphdr make_kcphdr_synack(std::uint32_t seq, std::uint32_t ack)
//	{
//        kcphdr hdr;
//        memset(&hdr, 0x0, sizeof(kcphdr));
//		hdr.th_seq = seq;
//		hdr.th_ack = ack + 1;
//		hdr.thf_ack = 1;
//		hdr.thf_syn = 1;
//		hdr.th_sum = checksum(reinterpret_cast<unsigned short *>(&hdr),
//			static_cast<int>(sizeof(kcphdr) - sizeof(kcphdr::th_sum)));
//
//		return hdr;
//	}
//
//	template<typename = void>
//	inline kcphdr make_kcphdr_ack(std::uint32_t ack)
//	{
//        kcphdr hdr;
//        memset(&hdr, 0x0, sizeof(kcphdr));
//		hdr.th_ack = ack + 1;
//		hdr.thf_ack = 1;
//		hdr.th_sum = checksum(reinterpret_cast<unsigned short *>(&hdr),
//			static_cast<int>(sizeof(kcphdr) - sizeof(kcphdr::th_sum)));
//
//		return hdr;
//	}
//
//	template<typename = void>
//	inline kcphdr make_kcphdr_fin(std::uint32_t seq)
//	{
//		kcphdr hdr;
//		memset(&hdr, 0x0, sizeof(kcphdr));
//		hdr.th_seq = seq;
//		hdr.thf_fin = 1;
//		hdr.th_sum = checksum(reinterpret_cast<unsigned short *>(&hdr),
//			static_cast<int>(sizeof(kcphdr) - sizeof(kcphdr::th_sum)));
//
//		return hdr;
//	}

enum class kcp_flag_type : uint8_t
{
    NONE = 0,
    SYN, // SYN
    ACK, // ACK->SYN
    FIN, // FIN disconnect
    RST, // RST handshaking is not finished or active closure
};

struct kcphdr_syn
{
    std::uint8_t flag_type = static_cast<uint8_t>(kcp_flag_type::SYN); // syn
    std::uint32_t sync_id;                                                        // sync_id
} __KCPHDR_ONEBYTE_ALIGN__;

struct kcphdr_ack
{
    std::uint8_t flag_type = static_cast<uint8_t>(kcp_flag_type::ACK); // ack
    std::uint32_t sync_id;                                                        // sync_id
    std::uint32_t conv_id;                                                        // conv_id
} __KCPHDR_ONEBYTE_ALIGN__;

struct kcphdr_fin
{
    std::uint8_t flag_type = static_cast<uint8_t>(kcp_flag_type::FIN); // fin
    std::uint32_t sync_id;                                                        // sync_id
    std::uint32_t conv_id;                                                        // conv_id
} __KCPHDR_ONEBYTE_ALIGN__;

struct kcphdr_rst
{
    std::uint8_t flag_type = static_cast<uint8_t>(kcp_flag_type::FIN);
} __KCPHDR_ONEBYTE_ALIGN__;

#if defined(__GNUC__) || defined(__GNUG__)
#undef __KCPHDR_ONEBYTE_ALIGN__
#elif defined(_MSC_VER)
#pragma pack(pop)
#undef __KCPHDR_ONEBYTE_ALIGN__
#endif

template<typename = void>
inline bool is_kcphdr_syn(std::string_view s)
{
    if (s.size() != sizeof(kcphdr_syn))
    {
        return false;
    }

    kcphdr_syn* hdr = (kcphdr_syn*)(s.data());
    return hdr->flag_type == static_cast<std::uint8_t>(kcp_flag_type::SYN);
}

template<typename = void>
inline bool is_kcphdr_syn(std::string_view s, std::uint32_t sync_id)
{
    if (s.size() != sizeof(kcphdr_ack))
    {
        return false;
    }

    kcphdr_ack* hdr = (kcphdr_ack*)(s.data());
    return (hdr->flag_type == static_cast<std::uint8_t>(kcp_flag_type::ACK)) && (hdr->sync_id == sync_id);
}

template<typename = void>
inline bool is_kcphdr_fin(std::string_view s, std::uint32_t sync_id, std::uint32_t conv_id)
{
    if (s.size() != sizeof(kcphdr_fin))
    {
        return false;
    }

    kcphdr_fin* hdr = (kcphdr_fin*)(s.data());
    return (hdr->flag_type == static_cast<std::uint8_t>(kcp_flag_type::FIN)) && (hdr->sync_id == sync_id) &&
           (hdr->conv_id == conv_id);
}

template<typename = void>
inline bool is_kcphdr_rst(std::string_view s)
{
    if (s.size() != sizeof(kcphdr_rst))
    {
        return false;
    }

    kcphdr_rst* hdr = (kcphdr_rst*)(s.data());
    return (hdr->flag_type == static_cast<std::uint8_t>(kcp_flag_type::RST));
}

inline bool is_kcphdr_msg(std::string_view s)
{
    static size_t constexpr kcp_header_len = 24;
    return s.length() >= kcp_header_len;
}

template<typename = void>
inline kcphdr_syn make_kcphdr_syn(std::uint32_t sync_id)
{
    kcphdr_syn hdr;
    hdr.sync_id = sync_id;

    return hdr;
}

template<typename = void>
inline kcphdr_ack make_kcphdr_synack(std::uint32_t sync_id, std::uint32_t conv_id)
{
    kcphdr_ack hdr;
    hdr.sync_id = sync_id;
    hdr.conv_id = conv_id;

    return hdr;
}

template<typename = void>
inline kcphdr_fin make_kcphdr_fin(std::uint32_t sync_id, std::uint32_t conv_id)
{
    kcphdr_fin hdr;
    hdr.sync_id = sync_id;
    hdr.conv_id = conv_id;

    return hdr;
}

template<typename = void>
inline kcphdr_rst make_kcphdr_rst()
{
    kcphdr_rst hdr;
    return hdr;
}

struct kcp_deleter
{
    inline void operator()(ikcpcb* p) const
    {
        kcp::ikcp_release(p);
    };
};

} // namespace zephyr::detail::kcp

#endif // !__ZEPHYR_KCP_UTIL_HPP__
