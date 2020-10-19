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

std::atomic_llong total_recv_packet_num = ATOMIC_VAR_INIT(0);
std::atomic_llong total_recv_size = ATOMIC_VAR_INIT(0);

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
    using namespace zephyr::detail;

    if (argc != 5)
    {
        fprintf(stderr, "Usage: <host> <port> <client num> <packet size>\n");
        exit(-1);
    }

    std::string_view host = argv[1];
    std::string_view port = argv[2];
    std::size_t client_num = atoi(argv[3]);
    std::size_t packet_size = atoi(argv[4]);

    //std::string packet = std::string(packet_size, 'a'); //random_string(packet_size);

    auto now = std::chrono::steady_clock::now();
    zephyr::timer log_timer;
    log_timer.start_timer(1, std::chrono::seconds(1), [now]() mutable {
        auto diff = std::chrono::steady_clock::now() - now;
        auto msDiff = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
        if (total_recv_size / 1024 == 0)
        {
            std::cout << "total recv : " << total_recv_size << " bytes/s"
                      << ", num " << total_recv_packet_num << std::endl;
        }
        else if ((total_recv_size / 1024) / 1024 == 0)
        {
            std::cout << "total recv : " << total_recv_size / 1024 << " K/s"
                      << ", num " << total_recv_packet_num << std::endl;
        }
        else
        {
            std::cout << "total recv : " << (total_recv_size / 1024) / 1024 << " M/s"
                      << ", num " << total_recv_packet_num << std::endl;
        }

        now = std::chrono::steady_clock::now();
        total_recv_size = 0;
        total_recv_packet_num = 0;
    });

    std::map<std::size_t, std::shared_ptr<zephyr::tcp_client>> clients;
    for (std::size_t i = 0; i < client_num; ++i)
    {
        auto client = std::make_shared<zephyr::tcp_client>(1024 * 1024, 5 * 1024 * 1024);
        client->no_delay(true);
        client->sndbuf_size(32 * 1024);
        client->rcvbuf_size(32 * 1024);
        clients[i] = client;
    }

    for (auto& [index, client_ptr] : clients)
    {
        auto& client = *client_ptr;
        client.auto_reconnect(true, std::chrono::seconds(10));

        client
            .bind_connect([&](asio::error_code ec) {
                if (zephyr::get_last_error())
                {
                    printf("connect failure : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
                }
                else
                {
                    auto HEAD_LEN = sizeof(uint32_t) + sizeof(uint16_t);

                    std::shared_ptr<BigPacket> sp = std::make_shared<BigPacket>(1);
                    sp->writeUINT32(HEAD_LEN + sizeof(int64_t) + packet_size);
                    sp->writeUINT16(1);
                    sp->writeINT64((int64_t)(&client));
                    sp->writeBinary(std::string(packet_size, '_'));

                    for (int i = 0; i < 1; ++i)
                    {
                        client.send(sp->getData(), sp->getPos());
                    }
                }
            })
            .bind_disconnect([&](asio::error_code ec) {
                printf("disconnect : %d %s\n", zephyr::last_error_val(), zephyr::last_error_msg().c_str());
            })
            .bind_recv([&](std::string_view sv) {
                const char* parse_str = sv.data();
                int total_proc_len = 0;
                size_t left_len = sv.length();

                while (true)
                {
                    bool flag = false;
                    auto HEAD_LEN = sizeof(uint32_t) + sizeof(uint16_t);
                    if (left_len >= HEAD_LEN)
                    {
                        BasePacketReader rp(parse_str, left_len);
                        auto packet_len = rp.readUINT32();
                        if (left_len >= packet_len && packet_len >= HEAD_LEN)
                        {
                            total_recv_size += packet_len;
                            total_recv_packet_num++;

                            BasePacketReader rp(parse_str, packet_len);
                            rp.readUINT32();
                            rp.readUINT16();
                            int64_t addr = rp.readINT64();

                            if (addr == (int64_t)(&client))
                            {
                                client.send(parse_str, packet_len);
                            }

                            total_proc_len += packet_len;
                            parse_str += packet_len;
                            left_len -= packet_len;
                            flag = true;
                            rp.skipAll();
                        }
                        rp.skipAll();
                    }

                    if (!flag)
                    {
                        break;
                    }
                }

                return total_proc_len;
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