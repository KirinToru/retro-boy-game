#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

// --- CONFIGURATION ---
// Define time per frame for a 60 FPS target
const sf::Time TIME_PER_FRAME = sf::seconds(1.f / 60.f);

int main()
{
	// 1. Window Creation
	sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Retro Boy");
	window.setFramerateLimit(60);

	// 2. Clock for tracking time
	sf::Clock clock;
	sf::Time timeSinceLastUpdate = sf::Time::Zero;

	// --- GAME LOOP ---
	while (window.isOpen())
	{
		// A. Time Management
		sf::Time dt = clock.restart();
		timeSinceLastUpdate += dt;

		// B. Update at fixed time steps
		while (const std::optional event = window.pollEvent())
		{
			if(event->is<sf::Event::Closed>())
				window.close();
		}

		// C. PHYSICS (Fixed Time Step)
		while (timeSinceLastUpdate >= TIME_PER_FRAME)
		{
			// Here would go the physics update code, using TIME_PER_FRAME as delta time
			timeSinceLastUpdate -= TIME_PER_FRAME;
		}
		
		// D. RENDERING
		window.clear(sf::Color::Black);

		// Here would go the rendering code
		// player.draw(window);

		window.display();
	}
	return 0;
}
