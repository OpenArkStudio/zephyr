#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <iostream>
#include <mutex>
#include <atomic>
#include <map>
#include <utility>
#include <zephyr/zephyr.hpp>
#include <zephyr/util/packet.hpp>

int main(int argc, char* argv[])
{
    if (argc != 6)
    {
        fprintf(stderr, "Usage: <host> <port> <client num> <packet size> <pipeline>\n");
        exit(-1);
    }

    std::string_view host = argv[1];
    int port = atoi(argv[2]);
    std::size_t client_num = atoi(argv[3]);
    std::size_t packet_size = atoi(argv[4]);
    std::size_t pipeline = atoi(argv[5]);

    std::string packet = std::string(packet_size, 'a');
    std::unique_ptr<zephyr::udp_client[]> clients = std::make_unique<zephyr::udp_client[]>(client_num);
    for (std::size_t i = 0; i < client_num; ++i)
    {
        auto& client = clients[i];
        client.connect_timeout(std::chrono::seconds(3));
        client.auto_reconnect(true, std::chrono::seconds(10));
        client.local_endpoint(asio::ip::address_v4::from_string(host.data()), 0);
        client
            .bind_connect([&](asio::error_code ec) {
                if (zephyr::get_last_error())
                {
                    printf("connect failure : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
                }
                else
                {
                    for (size_t i = 0; i < pipeline; i++)
                    {
                        client.send(packet);
                    }
                }
            })
            .bind_disconnect([](asio::error_code ec) {
                printf("disconnect : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
            })
            .bind_recv([&](std::string_view sv) { client.send(packet); })
            .bind_handshake([&](asio::error_code ec) {
                if (zephyr::get_last_error())
                {
                    printf("handshake failure : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
                }
                else
                {
                    printf("handshake success: %s %d\n", client.local_address().c_str(), client.local_port());
                }
            });

        client.async_start(host, port, zephyr::use_kcp);
    }

    while (std::getchar() != '\n')
    {
    }
    //std::this_thread::sleep_for(std::chrono::seconds(1));

    //client.stop();

    for (std::size_t i = 0; i < client_num; ++i)
    {
        clients[i].stop();
    }

    return 0;
}