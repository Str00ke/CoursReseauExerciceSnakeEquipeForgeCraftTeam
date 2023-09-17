#include "sh_constants.hpp"
#include "cl_resources.hpp"
#include "cl_grid.hpp"
#include "cl_snake.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <winsock2.h> 
#include <ws2tcpip.h> 

const int windowWidth = cellSize * gridWidth;
const int windowHeight = cellSize * gridHeight;

void game();
void tick(Grid& grid, Snake& snake);
int ConnectToServer(SOCKET sock);

int main()
{
	// Initialisation du g�n�rateur al�atoire
	// Note : en C++ moderne on dispose de meilleurs outils pour g�n�rer des nombres al�atoires,
	// mais ils sont aussi plus verbeux / complexes à utiliser, ce n'est pas tr�s int�ressant ici.
	std::srand(std::time(nullptr));
	
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data); 

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		std::cerr << "failed to open socket (" << WSAGetLastError() << ")\n";
		return EXIT_FAILURE;
	};

	//ConnectToServer(sock);

	/*
	// D�sactivation de l'algorithme de naggle sur la socket `sock`
	BOOL option = 1;
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&option), sizeof(option)) == SOCKET_ERROR)
	{
		std::cerr << "failed to disable Naggle's algorithm (" << WSAGetLastError() << ")\n";
		return EXIT_FAILURE;
	}
	*/

	game();
}

int ConnectToServer(SOCKET sock) 
{
	sockaddr_in clientIp;
	clientIp.sin_family = AF_INET;
	clientIp.sin_port = htons(port);
	for (;;)
	{
		std::cout << "please enter the server address: " << std::flush;

		std::string serverIP;
		std::cin >> serverIP;
		if (serverIP.empty())
			continue;

		if (inet_pton(clientIp.sin_family, serverIP.c_str(), &clientIp.sin_addr) == 1)
			break;

		std::cout << "invalid IP!" << std::endl;
	}

	if (connect(sock, reinterpret_cast<sockaddr*>(&clientIp), sizeof(clientIp)) == SOCKET_ERROR)
	{
		std::cout << "failed to connect to server (" << WSAGetLastError() << ")";
		return EXIT_FAILURE;
	}

	std::cout << "connected to server" << std::endl;

	//// On demande au client de renseigner un pseudo
	//std::string nickname;
	//do
	//{
	//	std::cout << "enter your nickname: " << std::flush;
	//	std::cin >> nickname;

	//	// On vide le buffer d'entrée (comme on ne lit que le premier mot, ça évite de garder le second en mémoire)
	//	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	//} while (nickname.empty());

	//// Envoi du pseudo du joueur
	//// On préfixe la taille du message et l'opcode avant celui-ci
	//std::vector<std::uint8_t> buffer(sizeof(std::uint16_t) + sizeof(std::uint8_t) + nickname.size());

	//// On sérialise l'entier 16bits
	//std::uint16_t size = htons(sizeof(std::uint8_t) + nickname.size());
	//std::memcpy(&buffer[0], &size, sizeof(std::uint16_t));

	//// On écrit l'opcode
	//buffer[sizeof(std::uint16_t)] = OpcodeNickname;

	//// On écrit le pseudo
	//std::memcpy(&buffer[sizeof(std::uint16_t) + sizeof(std::uint8_t)], nickname.data(), nickname.size());

	//// On envoie le pseudo au serveur
	//if (send(sock, (char*)buffer.data(), buffer.size(), 0) == SOCKET_ERROR)
	//{
	//	std::cerr << "failed to send message to server (" << WSAGetLastError() << ")\n";
	//	return EXIT_FAILURE;
	//}

}

void Disconnect(SOCKET sock) 
{
	closesocket(sock);

	WSACleanup();
}

void game()
{
	enum GameState
	{
		MENU,
		GAME
	};

	GameState _gS = GameState::MENU;

	// Chargement des assets du jeu
	Resources resources;
	if (!LoadResources(resources))
		return;

	// Cr�ation et ouverture d'une fen�tre
	sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Snake");
	window.setVerticalSyncEnabled(true);

	// �tant donn� que l'origine de tous les objets est au centre, il faut d�caler la cam�ra d'autant pour garder
	// une logique de grille à l'affichage
	sf::Vector2f viewSize(windowWidth, windowHeight);
	sf::Vector2f viewCenter = viewSize / 2.f - sf::Vector2f(cellSize, cellSize) / 2.f;
	window.setView(sf::View(viewCenter, viewSize));

	// On déclare la grille des �l�ments statiques (murs, pommes)
	// Les �l�ments statiques sont ceux qui �voluent tr�s peu dans le jeu, comparativement aux serpents qui �voluent à chaque
	// tour de jeu ("tick")
	Grid grid(gridWidth, gridHeight);

	// On déclare le serpent du joueur qu'on fait apparaitre au milieu de la grille, pointant vers la droite
	// Note : les directions du serpent sont repr�sent�es par le d�calage en X ou en Y n�cessaire pour passer à la case suivante.
	// Ces valeurs doivent toujours �tre à 1 ou -1 et la valeur de l'autre axe doit �tre à z�ro (nos serpents ne peuvent pas se d�placer
	// en diagonale)
	Snake snake(sf::Vector2i(gridWidth / 2, gridHeight / 2), sf::Vector2i(1, 0), sf::Color::Green);

	// On déclare quelques petits outils pour g�rer le temps
	sf::Clock clock;
	
	// Temps entre les ticks (tours de jeu)
	sf::Time tickInterval = sf::seconds(tickDelay);
	sf::Time nextTick = clock.getElapsedTime() + tickInterval;

	// Temps entre les apparitions de pommes
	sf::Time appleInterval = sf::seconds(5.f);
	sf::Time nextApple = clock.getElapsedTime() + appleInterval;

	while (window.isOpen())
	{
		if (_gS == GameState::GAME) 
		{

			// On traite les évènements fen�tre qui se sont produits depuis le dernier tour de boucles
			sf::Event event;
			while (window.pollEvent(event))
			{
				switch (event.type)
				{
					// L'utilisateur souhaite fermer la fen�tre, fermons-la
				case sf::Event::Closed:
					window.close();
					break;
					// Une touche a été enfonc�e par l'utilisateur
				case sf::Event::KeyPressed:
				{
					sf::Vector2i direction = sf::Vector2i(0, 0);

					// On met à jour la direction si la touche sur laquelle l'utilisateur a appuy�e est une fl�che directionnelle
					switch (event.key.code)
					{
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

					// On applique la direction, si modifi�e, au serpent
					if (direction != sf::Vector2i(0, 0))
					{
						// On interdit au serpent de faire un demi-tour complet
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

			// On v�rifie si assez de temps s'est �coul� pour faire avancer la logique du jeu
			if (now >= nextTick)
			{
				// On met à jour la logique du jeu
				tick(grid, snake);

				// On pr�voit la prochaine mise à jour
				nextTick += tickInterval;
			}

			// On v�rifie si assez de temps s'est �coul� pour faire apparaitre une nouvelle pomme
			if (now >= nextApple)
			{
				// On �vite de remplacer un mur par une pomme ...
				int x = 1 + rand() % (gridWidth - 2);
				int y = 1 + rand() % (gridHeight - 2);

				// On �vite de faire apparaitre une pomme sur un serpent
				// si c'est le cas on retentera au prochain tour de boucle
				if (!snake.TestCollision(sf::Vector2i(x, y), true))
				{
					grid.SetCell(x, y, CellType::Apple);

					// On pr�voit la prochaine apparition de pomme
					nextApple += appleInterval;
				}
			}

			// On remplit la sc�ne d'une couleur plus jolie pour les yeux
			window.clear(sf::Color(247, 230, 151));

			// On affiche les �l�ments statiques
			grid.Draw(window, resources);

			// On affiche le serpent
			snake.Draw(window, resources);

			// On actualise l'affichage de la fen�tre
			window.display();
		}
		else if (_gS == GameState::MENU) 
		{
			// On remplit la sc�ne d'une couleur plus jolie pour les yeux
			window.clear(sf::Color(247, 230, 151));

			// On affiche les �l�ments statiques
			grid.Draw(window, resources);

			// On actualise l'affichage de la fen�tre
			window.display();
		}
	}
}


void tick(Grid& grid, Snake& snake)
{
	snake.Advance();

	// On teste la collision de la t�te du serpent avec la grille
	sf::Vector2i headPos = snake.GetHeadPosition();
	switch (grid.GetCell(headPos.x, headPos.y))
	{
		case CellType::Apple:
		{
			// Le serpent mange une pomme, faisons-la disparaitre et faisons grandir le serpent !
			grid.SetCell(headPos.x, headPos.y, CellType::None);
			snake.Grow();
			break;
		}

		case CellType::Wall:
		{
			// Le serpent s'est pris un mur, on le fait r�apparaitre
			snake.Respawn(sf::Vector2i(gridWidth / 2, gridHeight / 2), sf::Vector2i(1, 0));
			break;
		}

		case CellType::None:
			break; //< rien à faire
	}

	// On v�rifie maintenant que le serpent n'est pas en collision avec lui-m�me
	if (snake.TestCollision(headPos, false))
	{
		// Le serpent s'est tu� tout seul, on le fait r�apparaitre
		snake.Respawn(sf::Vector2i(gridWidth / 2, gridHeight / 2), sf::Vector2i(1, 0));
	}
}
