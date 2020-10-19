#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <iostream>
#include <mutex>
#include <atomic>
#include <zephyr/zephyr.hpp>
#include <zephyr/util/packet.hpp>

std::atomic_llong total_recv_size = ATOMIC_VAR_INIT(0);
std::atomic_llong total_client_num = ATOMIC_VAR_INIT(0);
std::atomic_llong total_packet_num = ATOMIC_VAR_INIT(0);

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: <listen port> <packet size>\n");
        exit(-1);
    }

    std::string_view host = "0.0.0.0";
    std::string_view port = argv[1];
    std::size_t packet_size = atoi(argv[2]);

    ////////////////////////////////////////////////////////////
    std::string packet = std::string(packet_size, 'a');
    zephyr::udp_server server;
    server.start_timer(1, std::chrono::seconds(1), []() {
        if (total_recv_size / 1024 == 0)
        {
            std::cout << "total recv: " << total_recv_size << " b/s, of client num:" << total_client_num;
        }
        else if ((total_recv_size / 1024) / 1024 == 0)
        {
            std::cout << "total recv: " << total_recv_size / 1024 << " K/s, of client num:" << total_client_num;
        }
        else
        {
            std::cout << "total recv: " << (total_recv_size / 1024) / 1024
                      << " M/s, of client num:" << total_client_num;
        }

        std::cout << ", packet num:" << total_packet_num << std::endl;
        total_packet_num = 0;
        total_recv_size = 0;
    });
    server
        .bind_recv([&](std::shared_ptr<zephyr::udp_session>& session_ptr, std::string_view sv) {
            total_recv_size += sv.length();
            session_ptr->send(packet);
            total_packet_num++;
        })
        .bind_connect([](auto& session_ptr) { total_client_num++; })
        .bind_disconnect([](auto& session_ptr) { total_client_num--; })
        .bind_handshake([&](auto& session_ptr, asio::error_code ec) {
            if (ec)
            {
                printf("client handshake failed: %d %s\n", ec.value(), ec.message().c_str());
                std::cout << "shake failed, " << session_ptr->remote_address() << ":" << session_ptr->remote_port()
                          << std::endl;
            }
            else
            {
                printf("handshake success: %s %d\n", session_ptr->remote_address().c_str(), session_ptr->remote_port());
            }
        })
        .bind_start([&server](asio::error_code ec) {
            if (zephyr::get_last_error())
            {
                printf("start udp server kcp failure : %d %s\n", zephyr::last_error_val(),
                    zephyr::last_error_msg().c_str());
            }
            else
            {
                printf("start udp server kcp success : %s %u\n", server.listen_address().c_str(), server.listen_port());
            }
        })
        .bind_stop([&](asio::error_code ec) {
            printf("stop : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
        })
        .bind_init([&]() {
            //// Join the multicast group. you can set this option in the on_init(_fire_init) function.
            //server.acceptor().set_option(
            //	// for ipv6, the host must be a ipv6 address like 0::0
            //	asio::ip::multicast::join_group(asio::ip::make_address("ff31::8000:1234")));
            //	// for ipv4, the host must be a ipv4 address like 0.0.0.0
            //	//asio::ip::multicast::join_group(asio::ip::make_address("239.255.0.1")));
        });
    server.start(host, port, zephyr::use_kcp);

    while (std::getchar() != '\n')
    {
    }

    server.stop();

    return 0;
}