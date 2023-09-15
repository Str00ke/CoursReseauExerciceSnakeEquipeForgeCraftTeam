#pragma once

#include "cl_resources.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <vector>

// Une enum class est comme une enum en C++ classique, � l'exception du fait qu'il est obligatoire d'�crire le nom de l'enum
// pour acc�der � ses �l�ments (CellType::Apple plut�t que juste Apple), et qu'il n'est pas possible de convertir implicitement
// la valeur en entier (il suffit d'un static_cast pour cela).

enum class CellType
{
	Apple,
	Wall,
	None
};

// La classe grid repr�sente les �l�ments immobiles du terrain, comme les pommes et les murs, dans une grille d'une certaine taille
class Grid
{
public:
	Grid(int width, int height);

	// Affiche le contenu de la grille
	void Draw(sf::RenderTarget& renderTarget, Resources& resources) const;

	// R�cup�re le contenu d'une cellule de la grille � une position d�finie
	CellType GetCell(int x, int y) const;

	// D�fini le contenu d'une cellule de la grille � une position d�finie
	void SetCell(int x, int y, CellType cellType);

private:
	std::vector<CellType> m_content;
	int m_height;
	int m_width;
};
