#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <iostream>
#include <mutex>
#include <atomic>
#include <map>
#include <utility>
#include <zephyr/zephyr.hpp>

//std::string random_string(std::size_t length)
//{
//    auto random_str = []() -> char {
//        const char charset[] = "0123456789"
//                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//                               "abcdefghijklmnopqrstuvwxyz";
//        const size_t max_index = (sizeof(charset) - 1);
//        return charset[rand() % max_index];
//    };
//    std::string str(length, 0);
//    std::generate_n(str.begin(), length, random_str);
//    return str;
//}

int main(int argc, char* argv[])
{
    if (argc != 6)
    {
        fprintf(stderr, "Usage: <host> <port> <client num> <packet size> <pipeline>\n");
        exit(-1);
    }

    std::string_view host = argv[1];
    std::string_view port = argv[2];
    std::size_t client_num = atoi(argv[3]);
    std::size_t packet_size = atoi(argv[4]);
    std::size_t pipeline = atoi(argv[5]);

    std::string packet = std::string(packet_size, 'a'); //random_string(packet_size);

    std::map<std::size_t, std::shared_ptr<zephyr::tcp_client>> clients;
    for (std::size_t i = 0; i < client_num; ++i)
    {
        auto client = std::make_shared<zephyr::tcp_client>(16 * 1024, 10 * 10 * 1024);
        clients[i] = client;
    }

    //std::unique_ptr<zephyr::tcp_client[]> clients = std::make_unique<zephyr::tcp_client[]>(client_num);

    for (auto& [index, client_ptr] : clients)
    {
        auto& client = *client_ptr;
        client.auto_reconnect(true, std::chrono::seconds(10));
        client.no_delay(true);
        //client.sndbuf_size(32 * 1024);
        client
            .bind_connect([&](asio::error_code ec) {
                if (zephyr::get_last_error())
                {
                    printf("connect failure : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
                }
                else
                {
                    //printf("connect success : %s %u\n", client.local_address().c_str(), client.local_port());
                    for (size_t i = 0; i < pipeline; i++)
                    {
                        client.send(packet);
                    }
                }
            })
            .bind_disconnect([&](asio::error_code ec) {
                printf("disconnect : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
            })
            .bind_recv([&](std::string_view sv) {
                const char* parse_str = sv.data();
                auto leftLen = sv.length();
                while (leftLen >= packet_size)
                {
                    client.send(std::string_view(sv.data(), packet_size));
                    leftLen -= packet_size;
                    parse_str += packet_size;
                }
                return sv.length() - leftLen;

                //printf("recv: %zu\n", sv.length());
            });

        client.start(host, port);
    }

    while (std::getchar() != '\n')
    {
    }

    for (auto& [index, client_ptr] : clients)
    {
        client_ptr->stop();
    }

    return 0;
}