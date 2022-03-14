#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>

#include <sstream>

#ifndef BOOST_LOG_DYN_LINK
#define BOOST_LOG_DYN_LINK
#endif
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <i3ds/configurator.hpp>

#define CMD_LEN 4

int main(int argc, char **argv)
{
    int sockfd;
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage remote_addr;
    char buf[CMD_LEN];

    unsigned int delay;
    std::string port;

    i3ds::Configurator configurator;
    po::options_description desc("Service to send UDP ACK messages to emulate a WISDOM GPR");
    configurator.add_common_options(desc);
    desc.add_options()
    ("delay,d", po::value<unsigned int>(&delay)->default_value(3), "Delay in seconds before ACK is sent")
    ("port,p", po::value<std::string>(&port)->default_value(""), "Port number of Wisdom server.");
    po::variables_map vm = configurator.parse_common_options(desc, argc, argv);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0) {
        BOOST_LOG_TRIVIAL(error) << "getaddrinfo failed with errno: " << errno;
        exit(1);
    }

    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                    servinfo->ai_protocol)) == -1) {
        BOOST_LOG_TRIVIAL(error) << "socket failed with errno: " << errno;
        exit(1);
    }

    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        BOOST_LOG_TRIVIAL(error) << "bind failed with errno: " << errno;
        exit(1);
    }

    freeaddrinfo(servinfo);

    BOOST_LOG_TRIVIAL(info) << "WISDOM ACK service ready";
    int n_bytes;
    socklen_t addr_len = sizeof(remote_addr);
    while (strcmp(buf, "end")) {
        if ((n_bytes = recvfrom(sockfd, buf, CMD_LEN , 0,
                        (struct sockaddr *)&remote_addr, &addr_len)) == -1) {
            BOOST_LOG_TRIVIAL(error) << "recvfrom failed with errno: " << errno;
            exit(1);
        }
        std::stringstream ss;
        ss << "Recevied " << n_bytes << " bytes:";
        for (int i = 0; i < n_bytes; ++i) {
            ss << " " << (int)buf[i];
        }
        BOOST_LOG_TRIVIAL(info) << ss.str();
        sleep(delay);
        BOOST_LOG_TRIVIAL(info) << "Sending ACK: " << (int)buf[0];
        if ((sendto(sockfd, buf, 1, 0, (struct sockaddr *)&remote_addr, addr_len)) == -1) {
            BOOST_LOG_TRIVIAL(error) << "sendto failed with errno: " << errno;
            exit(1);
        }
    }

    close(sockfd);

    return 0;
}
