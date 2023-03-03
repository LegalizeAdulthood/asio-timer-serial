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
    explicit Service(asio::io_context &ctx) : m_ctx(ctx), m_timer(ctx, asio::chrono::seconds(1))
    {
        post(m_ctx, []{ std::cout << "\x1b[J"; });
        m_timer.async_wait([this](const boost::system::error_code &ec) { timerExpired(ec); });
    }

    void stop()
    {
        m_timer.cancel();
        m_stop = true;
    }

    void input(char c);

    void timerExpired(const boost::system::error_code &ec)
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

  private:
    asio::io_context &m_ctx;
    asio::steady_timer m_timer;
    bool m_stop{};
};

void Service::input(char c)
{
    post(m_ctx,
         [c]
         {
             std::cout << "\x1b[10;1H\x1b[K";
             if (std::isprint(c))
             {
                 std::cout << c;
             }
             else
             {
                 std::cout << "Non-printable";
             }
         });
}

void getConsoleInput(Service &svc)
{
    static const int CTRL_C = 3;

    while (true)
    {
        int c = _getch();
        if (c == CTRL_C)
            break;

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
        std::cerr << bang.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown error\n";
        return 1;
    }

    return 0;
}
