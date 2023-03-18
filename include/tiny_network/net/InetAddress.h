#ifndef INET_ADDRESS_H
#define INET_ADDRESS_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <string.h>

class InetAddress
{
public:
    explicit InetAddress(uint16_t port=0,std::string ip ="127.0.0.1");
    explicit InetAddress(const struct sockaddr_in& addr):addr_(addr){}

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const struct sockaddr_in* getSockAddr() const { return &addr_; }
    void setSockAddr(const struct sockaddr_in& addr) { addr_ = addr; }


private:
    struct sockaddr_in addr_;//ipv4地址
};


#endif