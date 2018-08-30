#include <QDebug>
#include "socket.h"
//#include "string.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <errno.h>
#include <fcntl.h>


Socket::Socket() :
    m_sock ( -1 )
{
    _addr_len = sizeof(_their_addr);
    memset ( &m_addr,
             0,
             sizeof ( m_addr ) );

}

Socket::~Socket() {
    if ( is_valid() )
        ::close ( m_sock );
}

bool Socket::create() {
    m_sock = socket ( AF_INET,
                      SOCK_DGRAM,
                      0 );
    // m_sock = socket ( AF_INET,
    //                   SOCK_STREAM,
    //                   0 );

    if ( ! is_valid() )
        return false;


    // TIME_WAIT - argh
    int on = 1;
    if ( setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
        return false;


    return true;

}



bool Socket::bind ( const int port ) {

    if ( ! is_valid() )
        {
            return false;
        }



    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = INADDR_ANY;
    m_addr.sin_port = htons ( port );

    int bind_return = ::bind ( m_sock,
                               ( struct sockaddr * ) &m_addr,
                               sizeof ( m_addr ) );


    if ( bind_return == -1 )
        {
            return false;
        }

    return true;
}


bool Socket::listen() const {
    if ( ! is_valid() )
        {
            return false;
        }

    int listen_return = ::listen ( m_sock, MAXCONNECTIONS );


    if ( listen_return == -1 )
        {
            return false;
        }

    return true;
}


bool Socket::accept ( Socket& new_socket ) const {
    int addr_length = sizeof ( m_addr );
    new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, ( socklen_t * ) &addr_length );

    if ( new_socket.m_sock <= 0 )
        return false;
    else
        return true;
}


bool Socket::send ( const std::string s ) const {
    int status = ::send ( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL );
    if ( status == -1 )
        {
            return false;
        }
    else
        {
            return true;
        }
}

//size_t sendto(int sockfd, const void *buf, size_t len, int flags,
//               const struct sockaddr *dest_addr, socklen_t addrlen);
bool Socket::send_to ( const std::string s, const char* addr, const int port ) const {
    struct sockaddr_in dest; 
    memset(&dest, 0, sizeof(dest));
    
    dest.sin_family = AF_INET;                       /* set the type of connection to TCP/IP */
    //dest.sin_addr.s_addr = inet_addr("192.168.1.5"); /* set our address to any interface */
    dest.sin_addr.s_addr = inet_addr(addr); /* set our address to any interface */
    dest.sin_port = htons(port);                    /* set the server port number */    

    int status = ::sendto ( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL, (struct sockaddr *)&dest, sizeof(dest) );
    if ( status == -1 ) {
        return false;
    } else {
        return true;
    }
}


int Socket::recv ( std::string& s ) const {
    char buf [ MAXRECV + 1 ];

    s = "";

    memset ( buf, 0, MAXRECV + 1 );

    int status = ::recv ( m_sock, buf, MAXRECV, 0 );

    if ( status == -1 )
        {
            std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
            return 0;
        }
    else if ( status == 0 )
        {
            return 0;
        }
    else
        {
            s = buf;
            return status;
        }
}

int Socket::recvFrom(uint8_t *buf, int packet_size) {
    int pac_len = 0;
    //uint8_t buf[1470];

    pac_len = ::recvfrom(m_sock, buf, packet_size, 0, (struct sockaddr *)&_their_addr, &_addr_len);
    if (pac_len == -1) {
        //::perror("recvfrom");
    }

    //qDebug() << "Packet #: " << buf[2] << " " << buf[3];
    //qDebug() << "Packet #: " << ((uint16_t *)buf)[1];

    return pac_len;
}


bool Socket::connect ( const std::string host, const int port ) {
    if ( ! is_valid() ) return false;

    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons ( port );

    int status = inet_pton ( AF_INET, host.c_str(), &m_addr.sin_addr );

    if ( errno == EAFNOSUPPORT ) return false;

    status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

    if ( status == 0 )
        return true;
    else
        return false;
}

void Socket::set_non_blocking ( const bool b ) {

    int opts;

    opts = fcntl ( m_sock,
                   F_GETFL );

    if ( opts < 0 )
        {
            return;
        }

    if ( b )
        opts = ( opts | O_NONBLOCK );
    else
        opts = ( opts & ~O_NONBLOCK );

    fcntl ( m_sock,
            F_SETFL,opts );

}
