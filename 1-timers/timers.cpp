#include <boost/asio.hpp>

#include <cctype>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <conio.h>

namespace asio = boost::asio;

// ANSI ESC sequences
namespace ansi
{

const char *const clearDisplay{"\x1b[J"};

const char *const clearEol{"\x1b[K"};

const char *const reverseVideo{"\x1b[7m"};

const char *const normalVideo{"\x1b[0m"};

const char *const graphicCharSet{"\x1b(0"};

const char *const normalCharSet{"\x1b(B"};

constexpr char graphicDiamond{'\x60'};

std::string rowCol(int row, int col)
{
    return "\x1b[" + std::to_string(row) + ';' + std::to_string(col) + 'H';
}

} // namespace ansi

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
    post(m_ctx,
         [] {
             std::cout << ansi::clearDisplay + ansi::rowCol(10, 3) + "Input: ";
         });
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
             std::string output = ansi::rowCol(10, 10 + m_charCount++) + ansi::clearEol;
             if (std::isprint(c))
             {
                 output += c;
             }
             else if (std::iscntrl(c))
             {
                 output += ansi::reverseVideo;
                 output += static_cast<char>(c | 0x40);
                 output += ansi::normalVideo;
             }
             else
             {
                 output += ansi::graphicCharSet;
                 output += ansi::graphicDiamond;
                 output += ansi::normalCharSet;
             }
             std::cout << output;
         });
}

void Service::timerExpired(const boost::system::error_code &ec)
{
    if (ec || m_stop)
    {
        return;
    }

    std::time_t now = std::time(nullptr);
    std::cout << ansi::rowCol(1, 40) + ansi::clearEol + std::ctime(&now);
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
        std::cout << ansi::rowCol(24, 1);
        std::cerr << bang.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cout << ansi::rowCol(24, 1);
        std::cerr << "Unknown error\n";
        return 1;
    }

    std::cout << ansi::rowCol(24, 1);
    return 0;
}
