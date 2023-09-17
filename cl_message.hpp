#pragma once
#include "sh_protocol.hpp"
#include <winsock2.h>
#include <iostream>
#include <vector>


class Cl_message
{
private:
  static void SendData(SOCKET sock, const void* data, std::size_t dataLength);


public:
  static void SendMessageToServer(SOCKET sock, Opcodes _opcode, InputMessageStruct structMessage);
  static void SendMessageToServer(SOCKET sock, Opcodes _opcode, SpawnMessageStruct structMessage);
  static void SendMessageToServer(SOCKET sock, Opcodes _opcode, CollideMessageStruct structMessage);
};