#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <iostream>
#include <mutex>
#include <atomic>
#include <zephyr/zephyr.hpp>
#include <zephyr/util/packet.hpp>

std::atomic_llong total_send_size = ATOMIC_VAR_INIT(0);
std::atomic_llong total_recv_size = ATOMIC_VAR_INIT(0);

std::atomic_llong send_packet_num = ATOMIC_VAR_INIT(0);
std::atomic_llong recv_packet_num = ATOMIC_VAR_INIT(0);

std::vector<std::shared_ptr<zephyr::tcp_session>> clients;

static void add_client(std::shared_ptr<zephyr::tcp_session>& session_ptr)
{
    clients.push_back(session_ptr);
}

static void remove_client(std::shared_ptr<zephyr::tcp_session>& session_ptr)
{
    for (auto iter = clients.begin(); iter != clients.end(); iter++)
    {
        if (*iter == session_ptr)
        {
            clients.erase(iter);
            break;
        }
    }
}

static size_t get_client_num()
{
    return clients.size();
}

static void broadcast_packet(std::string_view packet)
{
    auto packet_len = packet.length();
    recv_packet_num++;
    total_recv_size += packet_len;

    for (const auto& session : clients)
    {
        if (session == nullptr)
        {
            continue;
        }

        session->send(packet);
    }

    send_packet_num += clients.size();
    total_send_size += (clients.size() * packet_len);
}

int main(int argc, char* argv[])
{
    using namespace zephyr::detail;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: <listen port> <net work thread num>\n");
        exit(-1);
    }

    std::string_view host = "0.0.0.0";
    std::string_view port = argv[1];
    std::string_view thread_num = argv[2];

    auto now = std::chrono::steady_clock::now();

    zephyr::tcp_server server(1024 * 1024, 10 * 1024 * 1024, atoi(thread_num.data()));
    server.start_timer(1, std::chrono::seconds(1), [now]() mutable {
        auto diff = std::chrono::steady_clock::now() - now;
        auto msDiff = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
        std::cout << "cost:" << msDiff << " ms, client_num:" << get_client_num() << ", recv "
                  << (total_recv_size / 1024) * 1000 / msDiff << " K/s "
                  << "recv_num: " << recv_packet_num * 1000 / msDiff << ", send "
                  << (total_send_size / 1024) / 1024 * 1000 / msDiff << " M/s "
                  << " send_num: " << send_packet_num * 1000 / msDiff << std::endl;
        total_recv_size = 0;
        total_send_size = 0;
        send_packet_num = 0;
        recv_packet_num = 0;
        now = std::chrono::steady_clock::now();
    });

    server
        .bind_connect([](auto& session_ptr) {
            session_ptr->no_delay(true);
            session_ptr->sndbuf_size(32 * 1024);
            session_ptr->rcvbuf_size(32 * 1024);
            add_client(session_ptr);
        })
        .bind_disconnect([](auto& session_ptr) { remove_client(session_ptr); })
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
        .bind_recv([](std::shared_ptr<zephyr::tcp_session>& session_ptr, std::string_view s) {
            const char* parse_str = s.data();
            size_t total_proc_len = 0;
            size_t leftLen = s.length();

            while (true)
            {
                bool flag = false;
                auto HEAD_LEN = sizeof(uint32_t) + sizeof(uint16_t);
                if (leftLen >= HEAD_LEN)
                {
                    BasePacketReader rp(parse_str, leftLen);
                    auto packet_len = rp.readUINT32();
                    if (leftLen >= packet_len && packet_len >= HEAD_LEN)
                    {
                        broadcast_packet(std::string_view(parse_str, packet_len));

                        total_proc_len += packet_len;
                        parse_str += packet_len;
                        leftLen -= packet_len;
                        flag = true;
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

    server.start(host, port);

    while (std::getchar() != '\n')
    {
    }

    server.stop();

    return 0;
}
