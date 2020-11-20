/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ZEPHYR_KCP_STREAM_CP_HPP__
#define __ZEPHYR_KCP_STREAM_CP_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <zephyr/base/selector.hpp>
#include <zephyr/base/iopool.hpp>
#include <zephyr/base/error.hpp>
#include <zephyr/base/listener.hpp>
#include <zephyr/base/session_mgr.hpp>

#include <zephyr/base/detail/object.hpp>
#include <zephyr/base/detail/allocator.hpp>
#include <zephyr/base/detail/util.hpp>
#include <zephyr/base/detail/buffer_wrap.hpp>

#include <zephyr/udp/detail/kcp_util.hpp>

namespace zephyr::detail {
/*
	 * because udp is connectionless, in order to simplify the code logic, KCP shakes
	 * hands only twice (compared with TCP handshakes three times)
	 * 1 : client send syn to server
	 * 2 : server send synack to client
	 */
template<class derived_t, bool isSession>
class kcp_stream_cp
{
    template<class, class, class>
    friend class udp_session_impl_t;
    template<class, class, class>
    friend class udp_client_impl_t;
    template<class, class>
    friend class udp_server_impl_t;

public:
    /**
		 * @constructor
		 */
    kcp_stream_cp(derived_t& d, io_t& io)
        : derive(d)
        , kcp_io_(io)
        , kcp_timer_(io.context())
    {
    }

    /**
		 * @destructor
		 */
    ~kcp_stream_cp()
    {
        if (this->kcp_)
        {
            kcp::ikcp_release(this->kcp_);
            this->kcp_ = nullptr;
        }
    }

protected:
    inline void _kcp_start(std::shared_ptr<derived_t> this_ptr, std::uint32_t conv)
    {
        if (this->kcp_)
        {
            kcp::ikcp_release(this->kcp_);
            this->kcp_ = nullptr;
        }

        this->kcp_ = kcp::ikcp_create(conv, (void*)this);
        // default mtu is 1400, if you need another value.
        // please use ikcp_setmtu(mtu) to set different mtu value.
        // but please DONOT bigger than udp_frame_size.

        this->kcp_->output = &kcp_stream_cp<derived_t, isSession>::_kcp_output;

        kcp::ikcp_nodelay(this->kcp_, 1, 10, 2, 1);
        kcp::ikcp_wndsize(this->kcp_, 128, 512);

        this->_post_kcp_timer(std::move(this_ptr));
    }

    //#ifdef XD_KCP

    inline void _kcp_stop(std::shared_ptr<derived_t> this_ptr)
    {
        detail::ignore::unused(this_ptr);

        error_code ec;
        // if is kcp mode, send FIN handshake before close
        if (this->send_fin_)
        {
            kcp::kcphdr_fin hdr = kcp::make_kcphdr_fin(this->sync_id_, this->conv_id_);
            this->_kcp_send_hdr((const void*)&hdr, sizeof(kcp::kcphdr_fin), ec);
        }

        this->kcp_timer_.cancel();
    }

    //#else
    //
    //    inline void _kcp_stop(std::shared_ptr<derived_t> this_ptr)
    //    {
    //        detail::ignore::unused(this_ptr);
    //
    //        error_code ec;
    //        // if is kcp mode, send FIN handshake before close
    //        if (this->send_fin_)
    //        {
    //            this->_kcp_send_hdr(kcp::make_kcphdr_fin(0), ec);
    //        }
    //
    //        this->kcp_timer_.cancel();
    //    }
    //
    //#endif // XD_KCP

protected:
    //#ifdef XD_KCP

    inline std::size_t _kcp_send_hdr(const void* hdr, std::size_t hdr_len, error_code& ec)
    {
        std::size_t sent_bytes = 0;
        if constexpr (isSession)
        {
            sent_bytes = derive.stream().send_to(asio::buffer(hdr, hdr_len), derive.remote_endpoint_, 0, ec);
        }
        else
        {
            sent_bytes = derive.stream().send(asio::buffer(hdr, hdr_len), 0, ec);
        }

        return sent_bytes;
    }

    //#else
    //
    //    inline std::size_t _kcp_send_hdr(kcp::kcphdr hdr, error_code& ec)
    //    {
    //        std::size_t sent_bytes = 0;
    //        if constexpr (isSession)
    //        {
    //            sent_bytes = derive.stream().send_to(
    //                asio::buffer((const void*)&hdr, sizeof(kcp::kcphdr)), derive.remote_endpoint_, 0, ec);
    //        }
    //        else
    //        {
    //            sent_bytes = derive.stream().send(asio::buffer((const void*)&hdr, sizeof(kcp::kcphdr)), 0, ec);
    //        }
    //
    //        return sent_bytes;
    //    }
    //
    //#endif // XD_KCP

    template<class Data, class Callback>
    inline bool _kcp_send(Data& data, Callback&& callback)
    {
        auto buffer = asio::buffer(data);

        int ret = kcp::ikcp_send(this->kcp_, (const char*)buffer.data(), (int)buffer.size());
        set_last_error(ret);
        if (ret == 0)
        {
            kcp::ikcp_flush(this->kcp_);
        }
        callback(get_last_error(), ret < 0 ? 0 : buffer.size());

#if defined(ZEPHYR_SEND_CORE_ASYNC)
        derive.next_event();
#endif

        return (ret == 0);
    }

    inline void _post_kcp_timer(std::shared_ptr<derived_t> this_ptr)
    {
        std::uint32_t clock1 = static_cast<std::uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count());
        std::uint32_t clock2 = kcp::ikcp_check(this->kcp_, clock1);

        this->kcp_timer_.expires_after(std::chrono::milliseconds(clock2 - clock1));
        this->kcp_timer_.async_wait(asio::bind_executor(this->kcp_io_.strand(),
            make_allocator(this->tallocator_, [this, self_ptr = std::move(this_ptr)](const error_code& ec) {
                this->_handle_kcp_timer(ec, std::move(self_ptr));
            })));
    }

    inline void _handle_kcp_timer(const error_code& ec, std::shared_ptr<derived_t> this_ptr)
    {
        if (ec == asio::error::operation_aborted)
            return;

        std::uint32_t clock = static_cast<std::uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count());
        kcp::ikcp_update(this->kcp_, clock);
        if (derive.is_started())
            this->_post_kcp_timer(std::move(this_ptr));
    }

    template<class buffer_t>
    inline void _kcp_recv(std::shared_ptr<derived_t>& this_ptr, std::string_view s, buffer_t& buffer)
    {
        int len = kcp::ikcp_input(this->kcp_, (const char*)s.data(), (long)s.size());
        buffer.consume(buffer.size());
        if (len != 0)
        {
            set_last_error(asio::error::no_data);
            derive._do_disconnect(asio::error::no_data);
            return;
        }
        for (;;)
        {
            len = kcp::ikcp_recv(this->kcp_, (char*)buffer.prepare(buffer.pre_size()).data(), (int)buffer.pre_size());
            if /**/ (len >= 0)
            {
                buffer.commit(len);
                derive._fire_recv(this_ptr,
                    std::string_view(static_cast<std::string_view::const_pointer>(buffer.data().data()), len));
                buffer.consume(len);
            }
            else if (len == -3)
            {
                buffer.pre_size(buffer.pre_size() * 2);
            }
            else
                break;
        }
        kcp::ikcp_flush(this->kcp_);
    }

    //#ifdef XD_KCP

    template<typename MatchCondition>
    inline void _post_handshake(std::shared_ptr<derived_t> self_ptr, condition_wrap<MatchCondition> condition)
    {
        try
        {
            error_code ec;
            if constexpr (isSession)
            {
                // step 3 : server received syn from client (the first_ is the syn)

                // step 4 : server send synack to client
                auto* hdr = (kcp::kcphdr_syn*)(derive.first_.data());
                const auto& key = derive.hash_key();
                auto conv = fnv1a_hash<std::uint32_t>((const unsigned char* const) & key, std::uint32_t(sizeof(key)));
                // set server-side conv_id
                this->conv_id_ = conv;
                this->sync_id_ = hdr->sync_id;
                // make syn-ack to client with server_conv_id and client_conv_id
                kcp::kcphdr_ack synack = kcp::make_kcphdr_synack(this->sync_id_, this->conv_id_);
                this->_kcp_send_hdr((void*)&synack, sizeof(kcp::kcphdr_ack), ec);
                asio::detail::throw_error(ec);

                this->_kcp_start(self_ptr, this->conv_id_);
                this->_handle_handshake(ec, std::move(self_ptr), condition);
            }
            else
            {
                // step 1 : client send syn to server
                this->sync_id_ = static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                                                                .count());
                kcp::kcphdr_syn syn = kcp::make_kcphdr_syn(this->sync_id_);
                this->_kcp_send_hdr((const void*)&syn, sizeof(kcp::kcphdr_syn), ec);
                asio::detail::throw_error(ec);

                std::shared_ptr<asio::steady_timer> timer =
                    mktimer(derive.io(), std::chrono::milliseconds(500), [this, syn](error_code ec) {
                        if (ec == asio::error::operation_aborted)
                        {
                            return false;
                        }
                        this->_kcp_send_hdr((const void*)&syn, sizeof(kcp::kcphdr_syn), ec);
                        if (ec)
                        {
                            set_last_error(ec);
                            derive._do_disconnect(ec);
                            return false;
                        }
                        return true;
                    });

                // step 2 : client wait for recv synack util connect timeout or received some data
                derive.socket().async_receive(derive.buffer().prepare(derive.buffer().pre_size()),
                    asio::bind_executor(derive.io().strand(),
                        make_allocator(derive.rallocator(),
                            [this, this_ptr = std::move(self_ptr), condition, timer = std::move(timer)](
                                const error_code& ec, std::size_t bytes_recvd) mutable {
                                try
                                {
                                    timer->cancel();
                                }
                                catch (system_error&)
                                {
                                }
                                catch (std::exception&)
                                {
                                }

                                if (ec)
                                {
                                    this->_handle_handshake(ec, std::move(this_ptr), condition);
                                    return;
                                }

                                derive.buffer().commit(bytes_recvd);

                                std::string_view s = std::string_view(
                                    static_cast<std::string_view::const_pointer>(derive.buffer().data().data()),
                                    bytes_recvd);
                                // Check whether the data is the correct handshake information
                                if (kcp::is_kcphdr_syn(s, this->sync_id_))
                                {
                                    std::uint32_t conv = ((kcp::kcphdr_ack*)(s.data()))->conv_id;
                                    this->conv_id_ = conv;
                                    this->_kcp_start(this_ptr, conv);
                                    this->_handle_handshake(ec, std::move(this_ptr), condition);
                                }
                                else
                                {
                                    this->_handle_handshake(
                                        asio::error::no_protocol_option, std::move(this_ptr), condition);
                                }

                                derive.buffer().consume(bytes_recvd);
                            })));
            }
        }
        catch (system_error& e)
        {
            set_last_error(e);
            derive._do_disconnect(e.code());
        }
    }

    //#else
    //
    //    template<typename MatchCondition>
    //    inline void _post_handshake(std::shared_ptr<derived_t> self_ptr, condition_wrap<MatchCondition> condition)
    //    {
    //        try
    //        {
    //            error_code ec;
    //            if constexpr (isSession)
    //            {
    //                // step 3 : server recvd syn from client (the first_ is the syn)
    //
    //                // step 4 : server send synack to client
    //                kcp::kcphdr* hdr = (kcp::kcphdr*)(derive.first_.data());
    //                const auto& key = derive.hash_key();
    //                std::uint32_t conv =
    //                    fnv1a_hash<std::uint32_t>((const unsigned char* const) & key, std::uint32_t(sizeof(key)));
    //                this->seq_ = conv;
    //                kcp::kcphdr synack = kcp::make_kcphdr_synack(this->seq_, hdr->th_seq);
    //                this->_kcp_send_hdr(synack, ec);
    //                asio::detail::throw_error(ec);
    //
    //                this->_kcp_start(self_ptr, this->seq_);
    //                this->_handle_handshake(ec, std::move(self_ptr), condition);
    //            }
    //            else
    //            {
    //                // step 1 : client send syn to server
    //                this->seq_ = static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
    //                    std::chrono::system_clock::now().time_since_epoch())
    //                                                            .count());
    //                kcp::kcphdr syn = kcp::make_kcphdr_syn(this->seq_);
    //                this->_kcp_send_hdr(syn, ec);
    //                asio::detail::throw_error(ec);
    //
    //                std::shared_ptr<asio::steady_timer> timer =
    //                    mktimer(derive.io(), std::chrono::milliseconds(500), [this, syn](error_code ec) {
    //                        if (ec == asio::error::operation_aborted)
    //                            return false;
    //                        this->_kcp_send_hdr(syn, ec);
    //                        if (ec)
    //                        {
    //                            set_last_error(ec);
    //                            derive._do_disconnect(ec);
    //                            return false;
    //                        }
    //                        return true;
    //                    });
    //
    //                // step 2 : client wait for recv synack util connect timeout or recvd some data
    //                derive.socket().async_receive(derive.buffer().prepare(derive.buffer().pre_size()),
    //                    asio::bind_executor(derive.io().strand(),
    //                        make_allocator(derive.rallocator(),
    //                            [this, this_ptr = std::move(self_ptr), condition, timer = std::move(timer)](
    //                                const error_code& ec, std::size_t bytes_recvd) mutable {
    //                                try
    //                                {
    //                                    timer->cancel();
    //                                }
    //                                catch (system_error&)
    //                                {
    //                                }
    //                                catch (std::exception&)
    //                                {
    //                                }
    //
    //                                if (ec)
    //                                {
    //                                    this->_handle_handshake(ec, std::move(this_ptr), condition);
    //                                    return;
    //                                }
    //
    //                                derive.buffer().commit(bytes_recvd);
    //
    //                                std::string_view s = std::string_view(
    //                                    static_cast<std::string_view::const_pointer>(derive.buffer().data().data()),
    //                                    bytes_recvd);
    //
    //                                // Check whether the data is the correct handshake information
    //                                if (kcp::is_kcphdr_synack(s, this->seq_))
    //                                {
    //                                    std::uint32_t conv = ((kcp::kcphdr*)(s.data()))->th_seq;
    //
    //                                    this->_kcp_start(this_ptr, conv);
    //                                    this->_handle_handshake(ec, std::move(this_ptr), condition);
    //                                }
    //                                else
    //                                {
    //                                    this->_handle_handshake(
    //                                        asio::error::no_protocol_option, std::move(this_ptr), condition);
    //                                }
    //
    //                                derive.buffer().consume(bytes_recvd);
    //                            })));
    //            }
    //        }
    //        catch (system_error& e)
    //        {
    //            set_last_error(e);
    //            derive._do_disconnect(e.code());
    //        }
    //    }
    //
    //#endif // XD_KCP

    template<typename MatchCondition>
    inline void _handle_handshake(
        const error_code& ec, std::shared_ptr<derived_t> this_ptr, condition_wrap<MatchCondition> condition)
    {
        set_last_error(ec);

        try
        {
            if constexpr (isSession)
            {
                derive._fire_handshake(this_ptr, ec);

                asio::detail::throw_error(ec);

                derive._done_connect(ec, std::move(this_ptr), std::move(condition));
            }
            else
            {
                derive._fire_handshake(this_ptr, ec);

                derive._done_connect(ec, std::move(this_ptr), std::move(condition));
            }
        }
        catch (system_error& e)
        {
            set_last_error(e);
            derive._do_disconnect(e.code());
        }
    }

    static int _kcp_output(const char* buf, int len, kcp::ikcpcb* kcp, void* user)
    {
        std::ignore = kcp;

        kcp_stream_cp* zhis = ((kcp_stream_cp*)user);

        derived_t& derive = zhis->derive;

        error_code ec;
        if constexpr (isSession)
        {
            derive.stream().send_to(asio::buffer(buf, len), derive.remote_endpoint_, 0, ec);
        }
        else
        {
            derive.stream().send(asio::buffer(buf, len), 0, ec);
        }

        return 0;
    }

protected:
    derived_t& derive;

    io_t& kcp_io_;

    kcp::ikcpcb* kcp_ = nullptr;

    // for kcp handshaking
    std::uint32_t sync_id_ = 0; // sync_id
    std::uint32_t conv_id_ = 0; // conv_id

    bool send_fin_ = true;

    handler_memory<> tallocator_;

    asio::steady_timer kcp_timer_;
}; // namespace zephyr::detail
} // namespace zephyr::detail

#endif // !__ZEPHYR_KCP_STREAM_CP_HPP__
