#pragma once
#include <SFML/Graphics.hpp>

// Ce fichier contient tout ce qui va être lié au protocole du jeu, à la façon dont le client et le serveur vont communiquer

enum Opcodes : uint8_t
{
  Input = 0,
  Nickname,
  Spawn,
  Collide,
  RequestApple,
  ConfirmApple,
  DistributApple
};


struct InputMessageStruct
{
  InputMessageStruct(uint8_t _input) : input(_input){}
  //add if needed
  uint8_t input = 0;
};

struct SpawnMessageStruct
{
  SpawnMessageStruct(const sf::Vector2i& _position) : position(_position){}
  sf::Vector2i position;
};

//If effects on collide added
struct CollideMessageStruct
{
  CollideMessageStruct(const sf::Vector2i& _position, bool _collided) 
    : position(_position), collided(_collided){}

  sf::Vector2i position;
  bool collided;
};