#pragma once

// Ce fichier contient tout ce qui va être lié au protocole du jeu, à la façon dont le client et le serveur vont communiquer

enum Opcodes
{
  Input = 0,
  Spawn,
  Collide
};
