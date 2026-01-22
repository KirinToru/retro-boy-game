#include "Entities/Boy.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "World/Map.hpp"

// --- CONFIGURATION ---
// Define time per frame for a 60 FPS target
const sf::Time TIME_PER_FRAME = sf::seconds(1.f / 60.f);

int main() {
  // 1. Window Creation
  sf::RenderWindow window(sf::VideoMode({1280, 720}), "Retro Boy");
  // 2. Create game entities
  Map map;
  // map.loadDemoMap(); // REMOVED
  map.loadFromFile("assets/levels/level1.txt");

  Boy retroBoy;
  // Start position from Map
  retroBoy.reset(map.getStartPosition());

  // Camera
  sf::View camera = window.getDefaultView();

  // 3. Clock for tracking time
  sf::Clock clock;
  sf::Time timeSinceLastUpdate = sf::Time::Zero;

  // --- GAME LOOP ---
  while (window.isOpen()) {
    // A. Time Management
    sf::Time dt = clock.restart();
    timeSinceLastUpdate += dt;

    // B. Update at fixed time steps
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>())
        window.close();

      // Restart input
      if (const auto *keyPress = event->getIf<sf::Event::KeyPressed>()) {
        if (keyPress->code == sf::Keyboard::Key::R) {
          retroBoy.reset(map.getStartPosition());
        }
      }
    }

    // C. PHYSICS (Fixed Time Step)
    while (timeSinceLastUpdate >= TIME_PER_FRAME) {
      timeSinceLastUpdate -= TIME_PER_FRAME;

      retroBoy.update(TIME_PER_FRAME.asSeconds(), map);

      // Death Logic (Falling off map)
      // + 200 buffer
      if (retroBoy.getPosition().y > map.getHeight() + 200.f) {
        retroBoy.reset(map.getStartPosition());
      }
    }

    // Camera Logic
    sf::Vector2f playerPos = retroBoy.getPosition();
    sf::Vector2f viewSize = camera.getSize();
    float camX, camY;

    // CENTER MAP X: If map is smaller than view, center coordinate is MapWidth
    // / 2. SCROLL X: Else, clamp camera to player, keeping within map bounds.
    float mapW = map.getWidth();
    if (mapW < viewSize.x) {
      camX = mapW / 2.f;
    } else {
      camX = std::max(playerPos.x, viewSize.x / 2.f);
      camX = std::min(camX, mapW - viewSize.x / 2.f);
    }

    // CENTER MAP Y: Similar logic
    float mapH = map.getHeight();
    if (mapH < viewSize.y) {
      camY = mapH / 2.f;
    } else {
      camY = std::max(playerPos.y, viewSize.y / 2.f);
      camY = std::min(camY, mapH - viewSize.y / 2.f);
    }

    camera.setCenter({camX, camY});
    window.setView(camera);

    // D. RENDERING
    window.clear(
        sf::Color::White); // Changed background to White to see voids better

    map.render(window);
    retroBoy.render(window);

    window.display();
  }
  return 0;
}
