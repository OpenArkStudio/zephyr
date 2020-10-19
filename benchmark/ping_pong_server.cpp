#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <iostream>
#include <mutex>
#include <atomic>
#include <zephyr/zephyr.hpp>

std::atomic_llong total_recv_size = ATOMIC_VAR_INIT(0);
std::atomic_llong total_client_num = ATOMIC_VAR_INIT(0);
std::atomic_llong total_packet_num = ATOMIC_VAR_INIT(0);

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: <listen port> <net work thread num> <packet size>\n");
        exit(-1);
    }

    std::string_view host = "0.0.0.0";
    std::string_view port = argv[1];
    std::string_view thread_num = argv[2];
    std::size_t packet_size = atoi(argv[3]);

    zephyr::tcp_server server(16 * 1024, 10 * 10 * 1024, atoi(thread_num.data()));
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
        .bind_connect([](auto& session_ptr) {
            session_ptr->no_delay(true);
            total_client_num++;
            //printf("client enter : %s %u %s %u\n", session_ptr->remote_address().c_str(), session_ptr->remote_port(),
            //    session_ptr->local_address().c_str(), session_ptr->local_port());
        })
        .bind_disconnect([](auto& session_ptr) {
            total_client_num--;
            //printf("client leave : %s %u %s\n", session_ptr->remote_address().c_str(), session_ptr->remote_port(),
            //    zephyr::last_error_msg().c_str());
        })
        .bind_start([&server](asio::error_code ec) {
            if (!ec)
            {
                printf("start tcp server : %s %u\n", server.listen_address().c_str(), server.listen_port());
            }
            else
            {
                printf("start tcp server : %s %u %d %s\n", server.listen_address().c_str(), server.listen_port(),
                    ec.value(), ec.message().c_str());
            }
        })
        .bind_stop([](asio::error_code ec) { printf("stop : %d %s\n", ec.value(), ec.message().c_str()); })
        .bind_recv([packet_size](std::shared_ptr<zephyr::tcp_session>& session_ptr, std::string_view sv) {
            //printf("recv : %u %.*s\n", (unsigned)s.size(), (int)s.size(), s.data());
            //session_ptr->send(s, [](std::size_t bytes_sent) {});

            const char* parse_str = sv.data();
            auto leftLen = sv.length();
            while (leftLen >= packet_size)
            {
                session_ptr->send(std::string_view(sv.data(), packet_size));
                leftLen -= packet_size;
                parse_str += packet_size;
                total_packet_num++;

                total_recv_size += packet_size;
            }
            return sv.length() - leftLen;
        });

    server.start(host, port);

    while (std::getchar() != '\n')
    {
    }

    server.stop();

    return 0;
}
