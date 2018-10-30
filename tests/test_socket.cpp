#include "tests.h"
#include "zfile.h"
#include "zdatagramsocket.h"
#include "zstreamsocket.h"
#include "zconnection.h"
#include "zmultiplex.h"
//#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include "zerror.h"

namespace LibChaosTest {

static bool run = true;

void stopHandler(ZError::zerror_signal sig){
    run = false;
    TASSERT(false);
}

void udp_client(){
    LOG("=== UDP Socket Test...");
    ZError::registerInterruptHandler(stopHandler);
    ZError::registerSignalHandler(ZError::TERMINATE, stopHandler);

    ZDatagramSocket sock;
    if(!sock.open()){
        ELOG("Socket Open Fail");
        TASSERT(false);
    }
    LOG("Sending...");

    ZAddress addr("127.0.0.1", 8998);

    ZString dat = "Hello World out There! ";
    zu64 count = 0;

    for(zu64 i = 0; run && i < 5000; ++i){
        ZString str = dat + ZString::ItoS(i);
        ZBinary data((const unsigned char *)str.cc(), str.size());
        if(sock.send(addr, data)){
            LOG("to " << addr.debugStr() << " (" << data.size() << "): \"" << data << "\"");
            ZAddress sender;
            ZBinary recvdata;
            if(sock.receive(sender, recvdata)){
                LOG("from " << sender.str() << " (" << recvdata.size() << "): \"" << recvdata << "\"");
                count++;
            } else {
                continue;
            }
        } else {
            LOG("failed to send to " << addr.debugStr());
        }
        ZThread::usleep(500000);
    }

    TLOG("Sent " << count);
    sock.close();
}

void udp_server(){
    LOG("=== UDP Socket Server Test...");
    ZError::registerInterruptHandler(stopHandler);
    ZError::registerSignalHandler(ZError::TERMINATE, stopHandler);

    ZDatagramSocket sock;
    ZAddress bind(8998);
    DLOG(bind.debugStr());
    if(!sock.open()){
        ELOG("Socket Open Fail");
        TASSERT(false);
    }
    if(!sock.bind(bind)){
        ELOG("Socket Bind Fail");
        TASSERT(false);
    }

    //sock.setBlocking(false);

    //int out;
    //int len = sizeof(out);
    //getsockopt(sock.getHandle(), SOL_SOCKET, SO_REUSEADDR, &out, (socklen_t*)&len);
    //LOG(out);

    LOG("Listening...");
    //sock.listen(receivedGram);

    zu64 count = 0;

    while(run){
        ZAddress sender;
        ZBinary data;
        if(sock.receive(sender, data)){
            LOG("from " << sender.str() << " (" << data.size() << "): \"" << data << "\"");
            count++;
            ZAddress addr = sender;
            if(sock.send(addr, data)){
                LOG("to " << addr.debugStr() << " (" << data.size() << "): \"" << data << "\"");
            } else {
                LOG("failed to send to " << addr.str());
            }
        } else {
            LOG("error receiving message: " << ZError::getSystemError());
            continue;
        }
    }

    TLOG("Received " << count);
    sock.close();
}

#define TCP_PORT    8998
#define TCP_SIZE    1024
#define TCP_DAT1    "ping"
#define TCP_DAT2    "pong"

void tcp_client(){
    LOG("=== TCP Socket Test...");
    ZError::registerInterruptHandler(stopHandler);
    ZError::registerSignalHandler(ZError::TERMINATE, stopHandler);

    ZAddress addr("::1", TCP_PORT);
    //ZAddress addr("127.0.0.1", TCP_PORT);

    ZConnection conn;
    LOG("Connecting to " << addr.str());
    if(!conn.connect(addr)){
        ELOG("Socket Connect Fail");
        TASSERT(false);
    }
    LOG("connected " << conn.peer().str());

    ZSocket::socket_error err;

    ZString str = TCP_DAT1;
    ZBinary snddata(str);
    err = conn.write(snddata);
    if(err != ZSocket::OK){
        ELOG(ZSocket::errorStr(err));
        TASSERT(false);
    }
    LOG("write (" << snddata.size() << "): \"" << snddata.printable().asChar() << "\"");

    ZBinary data(TCP_SIZE);
    do {
        err = conn.read(data);
    } while(err == ZSocket::DONE);
    if(err != ZSocket::OK){
        ELOG(ZSocket::errorStr(err));
        TASSERT(false);
    }
    ZString recv = ZString(data.printable().asChar());
    LOG("read (" << data.size() << "): \"" << recv << "\"");

    TASSERT(recv == TCP_DAT2);
}

void tcp_server(){
    LOG("=== TCP Socket Server Test...");
    ZError::registerInterruptHandler(stopHandler);
    ZError::registerSignalHandler(ZError::TERMINATE, stopHandler);

    ZStreamSocket sock;
    ZAddress bind(TCP_PORT);
    LOG(bind.debugStr());
    if(!sock.listen(bind)){
        ELOG("Socket Listen Fail");
        TASSERT(false);
    }

    LOG("Listening...");

    bool ok = false;
    while(run){
        ZPointer<ZConnection> client;
        sock.accept(client);

        LOG("accept " << client->peer().debugStr());

        ZSocket::socket_error err;
        ZBinary data(TCP_SIZE);
        do {
            err = client->read(data);
        } while(err == ZSocket::DONE);
        if(err != ZSocket::OK){
            ELOG(ZSocket::errorStr(err));
            TASSERT(false);
        }
        ZString recv = ZString(data.printable().asChar());
        LOG("read (" << data.size() << "): \"" << recv << "\"");

        if(recv == TCP_DAT1){
            ZString str = TCP_DAT2;
            ZBinary snddata(str);
            err = client->write(snddata);
            if(err != ZSocket::OK){
                ELOG(ZSocket::errorStr(err));
                TASSERT(false);
            }
            LOG("write (" << snddata.size() << "): \"" << ZString(snddata.printable().asChar()) << "\"");
            ok = true;
            break;
        }
    }
    TASSERT(ok);
}

void multiplex_tcp_server(){
    ZPointer<ZStreamSocket> lsocket = new ZStreamSocket;
    ZMultiplex events;
    events.add(lsocket);
    while(events.wait()){
        for(zsize i = 0; i < events.count(); ++i){

        }
    }
}

// //////////////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ZSOCKET_WINAPI

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "8080"  // the port users will be connecting to
#define BACKLOG 10  // how many pending connections queue will hold

void sigchld_handler(int s){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    } else if(sa->sa_family == AF_INET6){
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    } else {
        throw ZException("Invalid sa_family");
    }
}

void tcp_server_2(){
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
        ELOG("getaddrinfo: " << gai_strerror(rv));
        TASSERT(false);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            ELOG("server: socket");
            continue;
        }

        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            ELOG("setsockopt");
            exit(1);
        }

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            ELOG("server: bind");
            continue;
        }

        break;
    }

    if(p == NULL){
        ELOG("server: failed to bind");
        TASSERT(false);
    }

    freeaddrinfo(servinfo); // all done with this structure

    if(listen(sockfd, BACKLOG) == -1){
        ELOG("listen");
        TASSERT(false);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        ELOG("sigaction");
        TASSERT(false);
    }

    LOG("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if(new_fd == -1){
            ELOG("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        LOG("server: got connection from " << s);

        ZAddress addr((sockaddr *)&their_addr);
        LOG(addr.debugStr());

        ZBinary bin;
        const long bufsize = 1024;
        unsigned char *buffer[bufsize];
        long len = 0;
        zu64 currpos = 0;

        ArZ warnings;

        enum parsestate {
            beginning = 0,
            requestline = 1,

            stopparsing = 30,
        };

        parsestate state = beginning;
        zu16 breakcounter = 0;

        do {
            len = recv(new_fd, buffer, bufsize, 0);
            if(len < 0){
                ELOG("recv: " << ZError::getSystemError());
                state = stopparsing;
                break;
            } else  if(len == 0){
                state = stopparsing;
                break;
            } else if(len > 0){
                ZBinary newdata(buffer, (zu64)len);
                LOG("Got " << newdata.size());

                bin.concat(newdata);

                // Parse request header
                for(zu64 i = currpos; i < bin.size(); ++i){
                    switch(bin[i]){
                    case '\r':
                        if(state == beginning){
                            if(breakcounter == 0){
                                ++breakcounter;
                            } else {
                                warnings.push(ZString("Bad CR in request line at ") + i + " " + breakcounter);
                            }
                        }
                        break;

                    case '\n':
                        if(state == beginning){
                            if(breakcounter == 1){
                                ArZ requestparts = ZString(bin.printable().asChar()).explode(' ');
                                if(requestparts.size() == 3){
                                    if(requestparts[2] != "HTTP/1.1" && requestparts[2] != "HTTP/1.0"){
                                        warnings.push(ZString("Bad HTTP-Version"));
                                        state = stopparsing;
                                        break;
                                    }
                                    breakcounter = 0;
                                    state = requestline;

                                } else {
                                    warnings.push(ZString("Bad Request-Line format"));
                                    state = stopparsing;
                                    break;
                                }
                            } else {
                                warnings.push(ZString("Bad LF in request line at ") + i + " " + breakcounter);
                            }
                        }
                        break;

                    default:
                        if(state == beginning){
                            if(breakcounter != 0){
                                warnings.push(ZString("Character inside CRLF in request line at ") + i + " " + breakcounter);
                            }
                        }
                        break;
                    }
                }
                if(state == stopparsing){
                    break;
                }

                continue;

                if(currpos == 0){ // first pass

                    for(zu64 i = currpos; i < bin.size(); ++i){
                        switch(bin[i]){
                        case ' ':
                        case '\n':
                        case '\r':

                        default:
                            break;
                        }
                    }

                    zu64 breakpos = bin.findFirst({0x0d, 0x0a, 0x0d, 0x0a});
                    LOG(breakpos);

                    if(breakpos != ZBinary::NONE){
                        ZBinary head = bin.getSub(0, breakpos);
                        head.nullTerm();
                        ZString header = head.printable().asChar();
                        ArZ headers = header.strExplode("\r\n");
                        for(zu64 i = 0; i < headers.size(); ++i){
                            LOG(headers[i]);
                        }
                        break;
                    }
                } else { // continue parsing

                }
            }
        } while(len > 0);

        if(state == stopparsing){
            close(new_fd);
            break;
        }

        ZFile::writeBinary("tcpserver.bin", bin);
        LOG("Done");
        //bin.nullTerm();
        //LOG(bin.size() << " !! " << bin.asChar());
        //LOG(bin.findFirst({'H','T','T','P'}));

        //if(!fork()){ // this is the child process
            //close(sockfd); // child doesn't need the listener
            if(send(new_fd, "Hello, world!", 13, 0) == -1)
                ELOG("send");
            close(new_fd);
            //exit(0);
        //}
        //close(new_fd);  // parent doesn't need this
    }
}

void tcp_server_3(){
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buffer[256];
    int nbytes;

    //char remoteIP[INET6_ADDRSTRLEN];

    int yes = 1;        // for setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

//    // get us a socket and bind it
//    memset(&hints, 0, sizeof hints);
//    hints.ai_family = AF_UNSPEC;
//    hints.ai_socktype = SOCK_STREAM;
//    hints.ai_flags = AI_PASSIVE;
//    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
//        ELOG("Selectserver Error: " << gai_strerror(rv));
//        return 1;
//    }

//    for(p = ai; p != NULL; p = p->ai_next) {
//        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
//        if (listener < 0) {
//            continue;
//        }

//        // lose the pesky "address already in use" error message
//        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

//        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
//            close(listener);
//            continue;
//        }

//        break;
//    }

//    // if we got here, it means we didn't get bound
//    if(p == NULL){
//        ELOG("selectserver: failed to bind");
//        return 2;
//    }

//    freeaddrinfo(ai); // all done with this

//    // listen
//    if(listen(listener, 10) == -1){
//        ELOG("listen");
//        return 3;
//    }

    ZStreamSocket listensock;
    if(!listensock.listen(ZAddress(8080))){
        ELOG("Listen fail");
        TASSERT(false);
    }

    listener = listensock.getSocket();

    LOG("Listening");

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    while(true){
        read_fds = master; // copy it
        if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1){
            ELOG("Select Error");
            TASSERT(false);
        }

        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; ++i){
            if(FD_ISSET(i, &read_fds)){
                // Found a socket with available data

                if(i == listener){
                    // Data on listening socket, so accept new connection

                    addrlen = sizeof(remoteaddr);
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

                    if(newfd == -1){
                        ELOG("Accept Error");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        ZAddress addr(&remoteaddr, addrlen);
                        LOG("New connection " << addr.debugStr() << " on " << newfd);
                    }

                } else {
                    // Data data from existing socket

                    if((nbytes = recv(i, buffer, sizeof(buffer), 0)) <= 0){
                        // got error or connection closed by client
                        if(nbytes == 0){
                            // connection closed
                            LOG("Remote socket " << i << " hung up");
                        } else {
                            ELOG("Recv Error");
                        }

                        close(i);
                        FD_CLR(i, &master); // remove from master set

                    } else {
                        LOG("Read " << nbytes);
                        // we got some data from a client


                    }

                }
            }
        }
    }
}

#else

void tcp_server_2(){

}

void tcp_server_3(){

}

#endif

ZArray<Test> socket_tests(){
    return {
        { "udp-client",             udp_client,             false, {} },
        { "udp-server",             udp_server,             false, {} },
        { "tcp-client",             tcp_client,             false, {} },
        { "tcp-server",             tcp_server,             false, {} },
        { "multiplex-tcp-server",   multiplex_tcp_server,   false, {} },
        { "tcp-server-2",           tcp_server_2,           false, {} },
        { "tcp-server-3",           tcp_server_3,           false, {} },
    };
}

}
