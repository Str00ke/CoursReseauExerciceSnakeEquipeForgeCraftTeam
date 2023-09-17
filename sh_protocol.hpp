#pragma once
#include <SFML/Graphics.hpp>

// Ce fichier contient tout ce qui va �tre li� au protocole du jeu, � la fa�on dont le client et le serveur vont communiquer

enum Opcodes : int8_t
{
  Input = 0,
  Spawn,
  Collide
};


struct InputMessageStruct
{
  InputMessageStruct(int8_t _input) : input(_input){}
  //add if needed
  int8_t input = 0;
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