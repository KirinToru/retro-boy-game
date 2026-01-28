#pragma once
#include <SFML/Graphics.hpp>

class Player {
public:
  Player(); // Constructor

  // Func that activates physics
  void update(float dt, const class Map &map);

  // Func that is rendering player on the screen
  void render(sf::RenderWindow &window);

  // Resets player state
  void reset(sf::Vector2f position);

  sf::Vector2f getPosition() const { return shape.getPosition(); }
  sf::Vector2f getVelocity() const { return velocity; }
  sf::FloatRect getBounds() const { return shape.getGlobalBounds(); }

private:
  sf::RectangleShape shape; // Player's shape

  // Physics variables
  sf::Vector2f velocity; // Velocity vector
  bool isGrounded;       // Is player on the ground

  // Player movement parameters
  float moveSpeed;    // Max speed
  float acceleration; // Horizontal acceleration
  float friction;     // Horizontal friction
  float gravity;
  float jumpStrength;

  // Wall mechanics
  float wallSlideSpeed;
  sf::Vector2f wallJumpForce;
  bool isWallSliding;
  int wallDir; // -1 left, 1 right, 0 none

  sf::Texture texture;
  sf::Sprite sprite;
  bool facingRight;

  // Animation
  int currentFrame;
  float animationTimer;
  float animationSpeed; // seconds per frame

  bool wasJumpPressed;
};