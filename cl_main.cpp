#include "cl_grid.hpp"
#include "cl_resources.hpp"
#include "cl_snake.hpp"
#include "sh_constants.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

const int windowWidth = cellSize * gridWidth;
const int windowHeight = cellSize * gridHeight;

void
game();
void
tick(Grid& grid, Snake& snake);
int
client(SOCKET sock);

SOCKET sock;

int
main()
{
  WSADATA data;
  WSAStartup(MAKEWORD(2, 2), &data);
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    std::cerr << "failed to open socket (" << WSAGetLastError() << ")\n";
    return EXIT_FAILURE;
  }

  {
    sockaddr_in clientIp;
    clientIp.sin_family = AF_INET;
    clientIp.sin_port = htons(port);
    for (;;) {
      std::cout << "please enter the server address: " << std::flush;

      std::string serverIP;
      std::cin >> serverIP;
      if (serverIP.empty())
        continue;
      if (inet_pton(
            clientIp.sin_family, serverIP.c_str(), &clientIp.sin_addr) == 1)
        break;

      std::cout << "invalid IP!" << std::endl;
    }
    if (connect(sock,
                reinterpret_cast<sockaddr*>(&clientIp),
                sizeof(clientIp)) == SOCKET_ERROR) {
      std::cout << "failed to connect to server (" << WSAGetLastError() << ")";
      return EXIT_FAILURE;
    }
    std::cout << "connected to server" << std::endl;
    std::string nickname;
    do {
      std::cout << "enter your nickname: " << std::flush;
      std::cin >> nickname;
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    } while (nickname.empty());
    std::vector<std::uint8_t> buffer(sizeof(std::uint16_t) +
                                     sizeof(std::uint8_t) + nickname.size());
    std::uint16_t size = htons(sizeof(std::uint8_t) + nickname.size());
    std::memcpy(&buffer[0], &size, sizeof(std::uint16_t));
    buffer[sizeof(std::uint16_t)] = 1;
    std::memcpy(&buffer[sizeof(std::uint16_t) + sizeof(std::uint8_t)],
                nickname.data(),
                nickname.size());
    if (send(sock, (char*)buffer.data(), buffer.size(), 0) == SOCKET_ERROR) {
      std::cerr << "failed to send message to server (" << WSAGetLastError()
                << ")\n";
      return EXIT_FAILURE;
    }
  }

  std::cout << "you can now write messages (empty message to disconnect)"
            << std::endl;
  bool running = true;
  std::thread readThread([&]() {
    std::vector<std::uint8_t> pendingData;
    while (running) {
      char buffer[1024];
      int byteRead = recv(sock, buffer, sizeof(buffer), 0);
      if (byteRead == 0 || byteRead == SOCKET_ERROR) {
        if (byteRead == 0)
          std::cout << "server disconnected" << std::endl;
        else
          std::cerr << "failed to read from server (" << WSAGetLastError()
                    << "), disconnecting..." << std::endl;

        running = false;
      } else {
        std::size_t oldSize = pendingData.size();
        pendingData.resize(oldSize + byteRead);
        std::memcpy(&pendingData[oldSize], buffer, byteRead);

        while (pendingData.size() >= sizeof(std::uint16_t)) {
          std::uint16_t messageSize;
          std::memcpy(&messageSize, &pendingData[0], sizeof(messageSize));

          messageSize = ntohs(messageSize);

          if (pendingData.size() - sizeof(messageSize) < messageSize)
            break;

          std::uint8_t opcode = pendingData[sizeof(std::uint16_t)];

          std::size_t stringSize = messageSize - 1;
          if (opcode == std::uint8_t(1)) {
            std::string message(stringSize, ' ');
            std::memcpy(
              &message[0],
              &pendingData[sizeof(messageSize) + sizeof(std::uint8_t)],
              stringSize);
            std::cout << message << std::endl;
          } else
            std::cout << "unexpected opcode " << +opcode << std::endl;
          std::size_t handledSize = sizeof(messageSize) + messageSize;
          pendingData.erase(pendingData.begin(),
                            pendingData.begin() + handledSize);
        }
      }
    }
  });

  std::srand(std::time(nullptr));
  game();

  readThread.join();

  closesocket(sock);
  WSACleanup();
}

void
game()
{
  Resources resources;
  if (!LoadResources(resources))
    return;
  sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Snake");
  window.setVerticalSyncEnabled(true);
  sf::Vector2f viewSize(windowWidth, windowHeight);
  sf::Vector2f viewCenter =
    viewSize / 2.f - sf::Vector2f(cellSize, cellSize) / 2.f;
  window.setView(sf::View(viewCenter, viewSize));
  Grid grid(gridWidth, gridHeight);
  Snake snake(sf::Vector2i(gridWidth / 2, gridHeight / 2),
              sf::Vector2i(1, 0),
              sf::Color::Green);
  sf::Clock clock;
  sf::Time tickInterval = sf::seconds(tickDelay);
  sf::Time nextTick = clock.getElapsedTime() + tickInterval;
  sf::Time appleInterval = sf::seconds(5.f);
  sf::Time nextApple = clock.getElapsedTime() + appleInterval;

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      switch (event.type) {
        case sf::Event::Closed:
          window.close();
          break;
        case sf::Event::KeyPressed: {
          sf::Vector2i direction = sf::Vector2i(0, 0);
          switch (event.key.code) {
            case sf::Keyboard::Up:
              direction = sf::Vector2i(0, -1);
              break;

            case sf::Keyboard::Down:
              direction = sf::Vector2i(0, 1);
              break;

            case sf::Keyboard::Left:
              direction = sf::Vector2i(-1, 0);
              break;

            case sf::Keyboard::Right:
              direction = sf::Vector2i(1, 0);
              break;

            default:
              break;
          }
          if (direction != sf::Vector2i(0, 0)) {
            if (direction != -snake.GetCurrentDirection())
              snake.SetFollowingDirection(direction);
          }
          break;
        }

        default:
          break;
      }
    }

    sf::Time now = clock.getElapsedTime();
    if (now >= nextTick) {
      tick(grid, snake);
      nextTick += tickInterval;
    }
    if (now >= nextApple) {
      int x = 1 + rand() % (gridWidth - 2);
      int y = 1 + rand() % (gridHeight - 2);
      if (!snake.TestCollision(sf::Vector2i(x, y), true)) {
        grid.SetCell(x, y, CellType::Apple);
        nextApple += appleInterval;
      }
    }
    window.clear(sf::Color(247, 230, 151));
    grid.Draw(window, resources);
    snake.Draw(window, resources);
    window.display();
  }
}

void
tick(Grid& grid, Snake& snake)
{
  snake.Advance();
  sf::Vector2i headPos = snake.GetHeadPosition();
  switch (grid.GetCell(headPos.x, headPos.y)) {
    case CellType::Apple: {
      grid.SetCell(headPos.x, headPos.y, CellType::None);
      snake.Grow();
      break;
    }

    case CellType::Wall: {
      snake.Respawn(sf::Vector2i(gridWidth / 2, gridHeight / 2),
                    sf::Vector2i(1, 0));
      break;
    }

    case CellType::None:
      break;
  }
  if (snake.TestCollision(headPos, false)) {
    snake.Respawn(sf::Vector2i(gridWidth / 2, gridHeight / 2),
                  sf::Vector2i(1, 0));
  }

  std::string message = "queue";
  std::vector<std::uint8_t> buffer(sizeof(std::uint16_t) +
                                   sizeof(std::uint8_t) + message.size());
  std::uint16_t size = htons(sizeof(std::uint8_t) + message.size());
  std::memcpy(&buffer[0], &size, sizeof(std::uint16_t));
  buffer[sizeof(std::uint16_t)] = std::uint8_t(1);
  std::memcpy(&buffer[sizeof(std::uint16_t) + sizeof(std::uint8_t)],
              message.data(),
              message.size());
  if (send(sock, (char*)buffer.data(), buffer.size(), 0) == SOCKET_ERROR) {
    std::cerr << "failed to send message to server (" << WSAGetLastError()
              << ")\n";
    return;
  }
}

int
client(SOCKET sock)
{
  return EXIT_SUCCESS;
}