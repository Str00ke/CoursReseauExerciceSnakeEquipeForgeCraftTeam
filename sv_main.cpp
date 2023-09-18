#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "sh_constants.hpp"
#include <SFML/System/Clock.hpp> 
#include <cassert> 
#include <iostream> 
#include <string> 
#include <thread> 
#include <vector> 
#include <winsock2.h> 
#include <ws2tcpip.h>        

int
server(SOCKET sock);
void
tick();

int
main()
{  
  WSADATA data;
  WSAStartup(
    MAKEWORD(2, 2),
    &data);        
  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {  
    std::cerr << "failed to open socket (" << WSAGetLastError() << ")\n";
    return EXIT_FAILURE;
  }

  BOOL option = 1;
  if (setsockopt(sock,
                 IPPROTO_TCP,
                 TCP_NODELAY,
                 reinterpret_cast<const char*>(&option),
                 sizeof(option)) == SOCKET_ERROR) {
    std::cerr << "failed to disable Naggle's algorithm (" << WSAGetLastError()
              << ")\n";
    return EXIT_FAILURE;
  }

  int r = server(sock);  
  closesocket(sock); 
  WSACleanup();

  return r; 
}

int
server(SOCKET sock)
{   
  sockaddr_in bindAddr;
  bindAddr.sin_addr.s_addr = INADDR_ANY;
  bindAddr.sin_port =
    htons(port); 
  bindAddr.sin_family = AF_INET; 
  if (bind(sock, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) ==
      SOCKET_ERROR) {
    std::cerr << "failed to bind socket (" << WSAGetLastError() << ")\n";
    return EXIT_FAILURE;
  }   
  if (listen(sock, 10) == SOCKET_ERROR) {
    std::cerr << "failed to put socket into listen mode (" << WSAGetLastError()
              << ")\n";
    return EXIT_FAILURE;
  }  
  unsigned int nextClientId = 1;

  struct Client
  {
    SOCKET socket;
    unsigned int id;
    std::vector<std::uint8_t> pendingData;
  };

  std::vector<Client> clients; 
  sf::Clock clock; 
  sf::Time tickInterval = sf::seconds(tickDelay);
  sf::Time nextTick = clock.getElapsedTime() + tickInterval; 
  for (;;) {          
    std::vector<WSAPOLLFD> pollDescriptors;
    {    
      auto& serverDescriptor = pollDescriptors.emplace_back();
      serverDescriptor.fd = sock;
      serverDescriptor.events = POLLRDNORM;
      serverDescriptor.revents = 0;
    } 
    for (Client& client : clients) {
      auto& clientDescriptor = pollDescriptors.emplace_back();
      clientDescriptor.fd = client.socket;
      clientDescriptor.events = POLLRDNORM;
      clientDescriptor.revents = 0;
    }    
    int activeSockets =
      WSAPoll(pollDescriptors.data(), pollDescriptors.size(), 1);
    if (activeSockets == SOCKET_ERROR) {
      std::cerr << "failed to poll sockets (" << WSAGetLastError() << ")\n";
      return EXIT_FAILURE;
    }      

    if (activeSockets > 0) {  
      for (WSAPOLLFD& descriptor : pollDescriptors) { 
        if (descriptor.revents == 0)
          continue;      
        if (descriptor.fd == sock) {  
          sockaddr_in clientAddr;
          int clientAddrSize = sizeof(clientAddr);

          SOCKET newClient = accept(
            sock, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
          if (newClient == INVALID_SOCKET) {
            std::cerr << "failed to accept new client (" << WSAGetLastError()
                      << ")\n";
            return EXIT_FAILURE;
          } 
          auto& client = clients.emplace_back();
          client.id = nextClientId++;
          client.socket = newClient;  
          char strAddr[INET_ADDRSTRLEN];
          inet_ntop(clientAddr.sin_family,
                    &clientAddr.sin_addr,
                    strAddr,
                    INET_ADDRSTRLEN);

          std::cout << "client #" << client.id << " connected from " << strAddr
                    << std::endl;  

		  if (clients.size() > 0)
		  {
			  std::string spawnMsg = " ";
			  std::vector<std::uint8_t> spawnBuffer(
				  sizeof(std::uint16_t) + sizeof(std::uint8_t) + spawnMsg.size());

			  std::uint16_t size = htons(sizeof(std::uint8_t) + spawnMsg.size());
			  std::memcpy(&spawnBuffer[0], &size, sizeof(std::uint16_t));

			  std::uint8_t opcode = 2;
			  std::memcpy(&spawnBuffer[sizeof(std::uint16_t)],
				  &opcode,
				  sizeof(std::uint8_t));
			  std::memcpy(
				  &spawnBuffer[sizeof(std::uint16_t) + sizeof(std::uint8_t)],
				  spawnMsg.data(),
				  spawnMsg.size());

			  for (Client& c : clients) {
                  if (c.id != client.id)
                  {
					  if (send(c.socket,
						  (char*)spawnBuffer.data(),
						  spawnBuffer.size(),
						  0) == SOCKET_ERROR) {
						  std::cerr << "failed to send message to client #" << c.id
							  << ": (" << WSAGetLastError() << ")\n";
					  }
                  }
                  else
                  {
                      for (size_t i = 0; i < clients.size(); ++i)
                      {
						  if (send(c.socket,
							  (char*)spawnBuffer.data(),
							  spawnBuffer.size(),
							  0) == SOCKET_ERROR) {
							  std::cerr << "failed to send message to client #" << c.id
								  << ": (" << WSAGetLastError() << ")\n";
						  }
                      }
                  }
			  }
		  }
        } else {  
          auto clientIt =
            std::find_if(clients.begin(), clients.end(), [&](const Client& c) {
              return c.socket == descriptor.fd;
            });
          assert(clientIt != clients.end());

          Client& client = *clientIt; 
          char buffer[1024];
          int byteRead = recv(client.socket, buffer, sizeof(buffer), 0);
          if (byteRead == SOCKET_ERROR || byteRead == 0) {  
            if (byteRead == SOCKET_ERROR)
              std::cerr << "failed to read from client #" << client.id << " ("
                        << WSAGetLastError() << "), disconnecting..."
                        << std::endl;
            else
              std::cout << "client #" << client.id << " disconnected"
                        << std::endl;    
            closesocket(client.socket);
            clients.erase(clientIt);
          } else {  

            std::size_t oldSize = client.pendingData.size();
            client.pendingData.resize(oldSize + byteRead);
            std::memcpy(&client.pendingData[oldSize], buffer, byteRead);

            while (client.pendingData.size() >= sizeof(std::uint16_t)) {  
              std::uint16_t messageSize;
              std::memcpy(
                &messageSize, &client.pendingData[0], sizeof(messageSize));

              messageSize = ntohs(messageSize);

              if (client.pendingData.size() - sizeof(messageSize) < messageSize)
                break; 
              std::string message(messageSize, ' ');
              std::memcpy(&message[0],
                          &client.pendingData[sizeof(messageSize)],
                          messageSize); 
              std::size_t handledSize = sizeof(messageSize) + messageSize;
              client.pendingData.erase(client.pendingData.begin(),
                                       client.pendingData.begin() +
                                         handledSize);   
              message =
                "Client #" + std::to_string(client.id) + " - " + message; 
              std::vector<std::uint8_t> sendBuffer(
                sizeof(std::uint16_t) + sizeof(std::uint8_t) + message.size()); 
              std::uint16_t size = htons(sizeof(std::uint8_t) + message.size());
              std::memcpy(&sendBuffer[0], &size, sizeof(std::uint16_t));

              std::uint8_t opcode = 1;
              std::memcpy(&sendBuffer[sizeof(std::uint16_t)],
                          &opcode,
                          sizeof(std::uint8_t)); 
              std::memcpy(
                &sendBuffer[sizeof(std::uint16_t) + sizeof(std::uint8_t)],
                message.data(),
                message.size());

              std::cout << message << std::endl;
              for (Client& c : clients) {
                if (send(c.socket,
                         (char*)sendBuffer.data(),
                         sendBuffer.size(),
                         0) == SOCKET_ERROR) {
                  std::cerr << "failed to send message to client #" << c.id
                            << ": (" << WSAGetLastError() << ")\n";   
                }
              }
            }
          }
        }
      }
    }

    sf::Time now = clock.getElapsedTime();  
    if (now >= nextTick) { 
      tick(); 
      nextTick += tickInterval;
    }
  }

  return EXIT_SUCCESS;
}

void
tick()
{
}