#include "InetAddress.h"


InetAddress::InetAddress(uint16_t port,std::string ip)
{
    bzero(&addr_,sizeof(addr_));
    addr_.sin_family = AF_INET;//ipv4
    addr_.sin_port = htons(port);//主机字节序转网络字节序
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());//点分十进制转网络字节序
    //点分十进制是人类可读的，网络字节序是计算机可读的
}

std::string InetAddress::toIp() const
{
    char buf[32];
    inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));//网络字节序转点分十进制
    return buf;
}

std::string InetAddress::toIpPort() const
{
    char buf[32];
    inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    snprintf(buf+end,sizeof(buf)-end,":%u",port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

// #include <iostream>
// int main(){
//     InetAddress addr(8080);
//     std::cout << addr.toIp() << std::endl;
//     std::cout << addr.toIpPort() << std::endl;
//     std::cout << addr.toPort() << std::endl;
//     return 0;
// }
