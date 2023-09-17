#include "cl_message.hpp"



void Cl_message::SendData(SOCKET sock, const void* data, std::size_t dataLength)
{
  if (send(sock, static_cast<const char*>(data), static_cast<int>(dataLength), 0) == SOCKET_ERROR)
  {
    std::cerr << "failed to send data to server (" << WSAGetLastError() << ")" << std::endl;
    throw std::runtime_error("failed to send data");
  }
}


void Cl_message::SendMessageToServer(SOCKET sock, Opcodes _opcode, InputMessageStruct structMessage /*temp struct message*/)
{
  std::vector<std::uint8_t> data(sizeof(std::uint16_t) + sizeof(Opcodes) + sizeof(InputMessageStruct));
  std::uint16_t size = htons(data.size());
  std::memcpy(&data[0], &size, sizeof(std::uint16_t));
  std::memcpy(&data[sizeof(std::uint16_t)], &_opcode, sizeof(std::uint8_t));

  std::memcpy(&data[sizeof(std::uint16_t) + sizeof(std::uint8_t)], &structMessage, sizeof(std::uint8_t));
  SendData(sock, data.data(), size);
}

void Cl_message::SendMessageToServer(SOCKET sock, Opcodes _opcode, SpawnMessageStruct structMessage)
{
}

void Cl_message::SendMessageToServer(SOCKET sock, Opcodes _opcode, CollideMessageStruct structMessage)
{
}
