#include <boost/asio.hpp>

#include <cctype>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <conio.h>

namespace asio = boost::asio;

namespace
{

class Service
{
  public:
    explicit Service(asio::io_context &ctx);

    void stop();

    void input(char c);

  private:
    void timerExpired(const boost::system::error_code &ec);

    asio::io_context &m_ctx;
    asio::steady_timer m_timer;
    bool m_stop{};
    int m_charCount{};
};

Service::Service(asio::io_context &ctx) :
    m_ctx(ctx),
    m_timer(ctx, asio::chrono::seconds(1))
{
    post(m_ctx, []{ std::cout << "\x1b[J\x1b[10;3HInput: "; });
    m_timer.async_wait([this](const boost::system::error_code &ec) { timerExpired(ec); });
}

void Service::stop()
{
    m_timer.cancel();
    m_stop = true;
}

void Service::input(char c)
{
    post(m_ctx,
         [c, this]
         {
             if (m_charCount > 60)
             {
                 m_charCount = 0;
             }
             std::cout << "\x1b[10;" << std::to_string(10 + m_charCount++) + "H\x1b[K";
             if (std::isprint(c))
             {
                 std::cout << c;
             }
             else if (std::iscntrl(c))
             {
                 std::cout << "\x1b[1m" << static_cast<char>(c | 0x40) << "\x1b[0m";
             }
             else
             {
                 std::cout << "\x1b(0\x60\x1b(B";
             }
         });
}

void Service::timerExpired(const boost::system::error_code &ec)
{
    if (ec || m_stop)
    {
        return;
    }

    std::time_t now = std::time(nullptr);
    std::cout << "\x1b[1;40H\x1b[K" << std::ctime(&now);
    m_timer.expires_after(asio::chrono::seconds(1));
    m_timer.async_wait([this](const boost::system::error_code &ec) { timerExpired(ec); });
}

void getConsoleInput(Service &svc)
{
    static const int CTRL_C = 3;

    while (true)
    {
        int c = _getch();
        if (c == CTRL_C)
        {
            break;
        }

        svc.input(static_cast<char>(c));
    }
}

} // namespace

int main()
{
    try
    {
        asio::io_context ctx;
        Service svc(ctx);

        std::thread thread(
            [&svc]
            {
                getConsoleInput(svc);
                svc.stop();
            });

        ctx.run();
        thread.join();
    }
    catch (const std::exception &bang)
    {
        std::cout << "\x1b[24;1H";
        std::cerr << bang.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cout << "\x1b[24;1H";
        std::cerr << "Unknown error\n";
        return 1;
    }

    std::cout << "\x1b[24;1H";
    return 0;
}
