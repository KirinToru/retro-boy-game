#pragma once
#include <SFML/Graphics.hpp>

class Boy {
public:
  Boy(); // Constructor

  // Func that activates physics
  // Now requires Map for collision checks
  void update(float dt, const class Map &map);

  // Func that is rendering player on the screen
  void render(sf::RenderWindow &window);

  // Resets player state
  void reset(sf::Vector2f position);

  sf::Vector2f getPosition() const { return shape.getPosition(); }
  sf::Vector2f getVelocity() const { return velocity; }

private:
  sf::RectangleShape shape; // Player's shape

  // Physics variables
  sf::Vector2f velocity; // Velocity vector
  bool isGrounded;       // Is player on the ground

  // Player movement parameters
  float moveSpeed;
  float gravity;
  float jumpStrength;
};