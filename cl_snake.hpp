#pragma once

#include "cl_resources.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <vector>

// La classe Snake repr�sente un serpent en jeu, ainsi que toutes ses pi�ces
// celui-ci poss�de toujours une taille de trois � l'apparition, et peut grandir,
// se d�placer dans une direction pr�cise, et r�apparaitre en remettant sa taille � z�ro

class Snake
{
public:
	Snake();
	Snake(const sf::Vector2i& spawnPosition, const sf::Vector2i& direction, const sf::Color& color);

	// Fait avancer le serpent dans la direction suivie
	void Advance();
	
	// G�re l'affichage du serpent (ainsi que l'orientation des sprites qui le composent en fonction de la direction de chaque partie du corps)
	void Draw(sf::RenderTarget& renderTarget, Resources& resources) const;
	
	// Retourne la liste des positions occup�es par le serpent
	const std::vector<sf::Vector2i>& GetBody() const;

	// Retourne la direction actuelle, celle vers laquelle pointe la t�te du serpent
	sf::Vector2i GetCurrentDirection() const;
	
	// Retourne la direction suivie, celle vers laquelle le serpent va aller au prochain d�placement
	sf::Vector2i GetFollowingDirection() const;
	
	// Retourne la position de la t�te du serpent
	sf::Vector2i GetHeadPosition() const;

	// Agrandit notre serpent en faisant apparaitre une nouvelle pi�ce apr�s sa queue
	void Grow();
	
	// Fait r�apparaitre le serpent (avec sa taille initiale de trois) � l'endroit et la direction indiqu�e
	void Respawn(const sf::Vector2i& spawnPosition, const sf::Vector2i& direction);

	// Met � jour le corps du serpent avec une liste de positions occup�es
	void SetBody(const std::vector<sf::Vector2i>& body);

	// D�finit la prochaine direction � suivre (direction doit avoir x ou y � 1/-1 et l'autre � z�ro)
	void SetFollowingDirection(const sf::Vector2i& direction);

	// Teste si le serpent poss�de une partie de son corps sur la cellule � cette position-l�
	// on ajoute un petit bool�en pour savoir s'il faut tester la t�te ou non
	bool TestCollision(const sf::Vector2i& position, bool testHead);

private:
	sf::Color m_color;
	sf::Vector2i m_followingDir;
	std::vector<sf::Vector2i> m_body; //< doit au moins avoir trois �l�ments quoiqu'il arrive
};
