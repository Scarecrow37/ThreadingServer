#include <iostream>
#include <process.h>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")

unsigned WINAPI WorkerThread(void* Arg)
{
    fd_set* List = (fd_set*)Arg;
    fd_set CopyList;
    TIMEVAL Timeout;
    Timeout.tv_sec = 0;
    Timeout.tv_usec = 100;
    while (true)
    {
        CopyList = *List;
        int EventCount = select(0, &CopyList, nullptr, nullptr, &Timeout);
        if (EventCount <= 0) continue;
        for (unsigned int i = 0; i < List->fd_count; ++i)
        {
            if (!FD_ISSET(List->fd_array[i], &CopyList)) continue;
            char Buffer[1024] = {};
            const int ReceiveByte = recv(List->fd_array[i], Buffer, sizeof(Buffer), 0);
            if (ReceiveByte <= 0)
            {
                // Abnormal
                closesocket(List->fd_array[i]);
                FD_CLR(List->fd_array[i], &List);
                continue;
            }
            std::cout << i << " : " << Buffer << std::endl;
            for (unsigned int j = 0; j < List->fd_count; ++j)
            {
                if (i == j) continue;
                send(List->fd_array[j], Buffer, ReceiveByte, 0);
            }
        }
    }


    return 0;
}

int main(int argc, char* argv[])
{
    WSAData WsaData;
    WSAStartup(MAKEWORD(2, 2), &WsaData);

    const SOCKET ListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN ListenAddress = {};
    ListenAddress.sin_family = AF_INET;
    ListenAddress.sin_addr.s_addr = INADDR_ANY;
    ListenAddress.sin_port = htons(22222);

    bind(ListenSocket, (SOCKADDR*)&ListenAddress, sizeof(ListenAddress));

    listen(ListenSocket, 5);
    fd_set ClientSocketList;
    FD_ZERO(&ClientSocketList);

    HANDLE ThreadHandle = (HANDLE)_beginthreadex(0, 0, WorkerThread, &ClientSocketList, 0, 0);

    while (true)
    {
        SOCKADDR_IN ClientAddress = {};
        int ClientAddressSize = sizeof(ClientAddress);
        SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddress, &ClientAddressSize);
        FD_SET(ClientSocket, &ClientSocketList);
    }

    closesocket(ListenSocket);
    WSACleanup();


    return 0;
}
