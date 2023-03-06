#include <boost/asio.hpp>

#include <conio.h>

#include <cctype>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace asio = boost::asio;

// ANSI ESC sequences
namespace ansi
{

const char *const clearDisplay{"\x1b[J"};

const char *const clearEol{"\x1b[K"};

const char *const boldOn{"\x1b[1m"};

const char *const boldOff{"\x1b[0m"};

const char *const graphicCharSet{"\x1b(0"};

const char *const normalCharSet{"\x1b(B"};

constexpr char graphicBox{'\x60'};

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

    void openSerialPort(const std::string &device);

  private:
    void timerExpired(const boost::system::error_code &ec);
    void readLine();
    void lineReceived(const boost::system::error_code &ec, std::size_t size);

    asio::io_context &m_ctx;
    asio::steady_timer m_timer;
    asio::serial_port m_port;
    asio::streambuf m_serialData;
    bool m_stop{};
    int m_charCount{};
};

Service::Service(asio::io_context &ctx) :
    m_ctx(ctx),
    m_timer(ctx, asio::chrono::seconds(1)),
    m_port(ctx)
{
    post(m_ctx,
         []
         {
             std::cout << (ansi::clearDisplay + ansi::rowCol(10, 3) +
                           "Input:" + ansi::rowCol(12, 3) + "Value:" +
                           ansi::rowCol(14, 30) + "Type Ctrl+C to exit.");
         });
    m_timer.async_wait([this](const boost::system::error_code &ec)
                       { timerExpired(ec); });
}

void Service::stop()
{
    m_timer.cancel();
    m_port.cancel();
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
             std::string output =
                 ansi::rowCol(10, 10 + m_charCount++) + ansi::clearEol;
             if (std::isprint(c))
             {
                 output += c;
             }
             else if (std::iscntrl(c))
             {
                 output += ansi::boldOn;
                 output += static_cast<char>(c | 0x40);
                 output += ansi::boldOff;
             }
             else
             {
                 output += ansi::graphicCharSet;
                 output += ansi::graphicBox;
                 output += ansi::normalCharSet;
             }
             std::cout << output;
         });
}

void Service::openSerialPort(const std::string &device)
{
    m_port.open(device);
    m_port.set_option(asio::serial_port::baud_rate(9600));
    readLine();
}

void Service::readLine()
{
    async_read_until(m_port, m_serialData, "\n",
                     [this](const boost::system::error_code &ec,
                            std::size_t size) { lineReceived(ec, size); });
}

void Service::lineReceived(const boost::system::error_code &ec,
                           std::size_t size)
{
    if (ec)
        return;

    std::istream str(&m_serialData);
    unsigned int value;
    str >> value;
    if (str.good())
    {
        value = (value >> 4) + 1; // map 0-1023 to 1-64
        std::string boxes;
        boxes.assign(value, ansi::graphicBox);
        std::cout << ansi::rowCol(12, 10) + ansi::graphicCharSet + boxes +
                         ansi::normalCharSet + ansi::clearEol;
    }

    readLine();
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
    m_timer.async_wait([this](const boost::system::error_code &ec)
                       { timerExpired(ec); });
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

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2)
            return 1;

        asio::io_context ctx;
        Service svc(ctx);
        svc.openSerialPort(argv[1]);

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
