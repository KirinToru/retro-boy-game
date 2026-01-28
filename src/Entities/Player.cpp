#include "Player.hpp"

#include <iostream>

Player::Player() : sprite(texture) {
  // Load Texture (idle.png is 64x32, 2 frames of 32x32)
  if (!texture.loadFromFile("assets/player/idle.png")) {
    std::cerr << "Failed to load player texture!" << std::endl;
  }
  sprite.setTexture(texture, true);

  // Set initial texture rect (first frame)
  sprite.setTextureRect(sf::IntRect({0, 0}, {32, 32}));

  // Set origin to bottom-center for proper positioning relative to hitbox
  sprite.setOrigin({16.f, 32.f});

  facingRight = true;

  // Animation setup
  currentFrame = 0;
  animationTimer = 0.f;
  animationSpeed = 0.5f; // Switch frame every 0.5 seconds

  // Hitbox at feet
  shape.setSize({24.f, 32.f});
  shape.setFillColor(sf::Color(0, 0, 0, 0));
  shape.setOutlineThickness(1.f);
  shape.setOutlineColor(sf::Color::Green);
  shape.setPosition({100.f, 0.f}); // Start position

  // Physics parameters
  moveSpeed = 300.f;
  acceleration = 1500.f;
  friction = 1200.f;

  gravity = 1000.f;
  jumpStrength = 500.f;

  // Wall mechanics
  wallSlideSpeed = 80.f;
  wallJumpForce = {320.f, 480.f};
  isWallSliding = false;
  wallDir = 0;

  velocity = {0.f, 0.f};
  isGrounded = false;
}

// Include Map for collision checks
#include "../World/Map.hpp"

void Player::update(float dt, const Map &map) {
  // 1. Input Handling & Dynamic Speed (Acceleration/Friction)
  bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
  bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
               sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
  bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);

  // Horizontal Movement with Acceleration
  if (left && !right) {
    velocity.x -= acceleration * dt;
  } else if (right && !left) {
    velocity.x += acceleration * dt;
  } else {
    // Friction
    if (velocity.x > 0) {
      velocity.x -= friction * dt;
      if (velocity.x < 0)
        velocity.x = 0;
    } else if (velocity.x < 0) {
      velocity.x += friction * dt;
      if (velocity.x > 0)
        velocity.x = 0;
    }
  }

  // Cap speed
  if (velocity.x > moveSpeed)
    velocity.x = moveSpeed;
  if (velocity.x < -moveSpeed)
    velocity.x = -moveSpeed;

  // 2. Wall Detection Logic
  sf::FloatRect bounds = shape.getGlobalBounds();
  sf::FloatRect leftCheck = bounds;
  leftCheck.position.x -= 2.f;
  sf::FloatRect rightCheck = bounds;
  rightCheck.position.x += 2.f;

  bool touchingLeft = !map.checkCollision(leftCheck).empty();
  bool touchingRight = !map.checkCollision(rightCheck).empty();

  // Reset wall state
  isWallSliding = false;
  wallDir = 0;

  if (touchingLeft)
    wallDir = -1;
  if (touchingRight)
    wallDir = 1;

  // Wall Slide
  if (wallDir != 0 && velocity.y > 0 && !isGrounded) {
    if ((wallDir == -1 && left) || (wallDir == 1 && right)) {
      isWallSliding = true;
      if (velocity.y > wallSlideSpeed) {
        velocity.y = wallSlideSpeed;
      }
    }
  }

  // 3. Jump and Wall Jump
  // Only trigger on rising edge (first frame of press)
  bool jumpJustPressed = jumpPressed && !wasJumpPressed;

  if (jumpJustPressed) {
    // Normal Jump
    if (isGrounded) {
      velocity.y = -jumpStrength;
      isGrounded = false;
    }
    // Wall Jump
    else if (isWallSliding || (wallDir != 0 && !isGrounded)) {
      velocity.y = -wallJumpForce.y;
      velocity.x = -wallDir * wallJumpForce.x;
    }
  }

  // Update previous state for next frame
  wasJumpPressed = jumpPressed;

  // 4. Variable Gravity (Dynamic Acceleration) with Gravity Halt at Peak
  float currentGravity = gravity;

  // Gravity Halt: near the peak of the jump (velocity close to 0), reduce
  // gravity
  const float peakThreshold = 50.f; // velocity range considered "peak"
  if (std::abs(velocity.y) < peakThreshold && !isGrounded && !isWallSliding) {
    currentGravity *= 0.7f; // Reduced gravity at peak for floaty feel
  }
  // Variable gravity relies on holding the button
  else if (velocity.y < 0.f && !jumpPressed) {
    // Rising but button released: heavier gravity (shorter jump)
    currentGravity *= 2.0f;
  } else if (velocity.y > 0.f) {
    // Falling: heavier gravity (fast fall), unless sliding
    if (!isWallSliding) {
      currentGravity *= 1.8f;
    } else {
      currentGravity = 0; // Handled by slide constant speed
    }
  }

  velocity.y += currentGravity * dt;

  // 5. Physics & Collision Resolution

  // --- X-AXIS ---
  shape.move({velocity.x * dt, 0.f});

  // Check collisions after X move
  std::vector<sf::FloatRect> walls =
      map.checkCollision(shape.getGlobalBounds());
  for (const auto &wall : walls) {
    sf::FloatRect playerBounds = shape.getGlobalBounds();

    // Calculate Intersection Overlap using SFML 3 struct members
    float overlapY = std::min(playerBounds.position.y + playerBounds.size.y,
                              wall.position.y + wall.size.y) -
                     std::max(playerBounds.position.y, wall.position.y);

    // Ignore "snagging" on floor/ceiling seams
    if (overlapY < 5.f)
      continue;

    // Resolve X collision
    float playerCenter = shape.getPosition().x + shape.getSize().x / 2.f;
    float wallCenter = wall.position.x + wall.size.x / 2.f;

    if (velocity.x > 0) { // Moving Right
      // Only resolve if wall is to the right
      if (wallCenter > playerCenter) {
        shape.setPosition(
            {wall.position.x - shape.getSize().x, shape.getPosition().y});
        velocity.x = 0; // Stop on wall
      }
    } else if (velocity.x < 0) { // Moving Left
      // Only resolve if wall is to the left
      if (wallCenter < playerCenter) {
        shape.setPosition(
            {wall.position.x + wall.size.x, shape.getPosition().y});
        velocity.x = 0; // Stop on wall
      }
    }
  }

  // --- Y-AXIS ---
  // Reset grounded (will be set true if we land on something)
  isGrounded = false;

  float prevBottom = shape.getPosition().y + shape.getSize().y;
  shape.move({0.f, velocity.y * dt});

  // Check collisions after Y move
  walls = map.checkCollision(shape.getGlobalBounds());
  for (const auto &wall : walls) {
    sf::FloatRect playerBounds = shape.getGlobalBounds();

    // Calculate Intersection Overlap X to distinguish Wall from Floor
    float overlapX = std::min(playerBounds.position.x + playerBounds.size.x,
                              wall.position.x + wall.size.x) -
                     std::max(playerBounds.position.x, wall.position.x);

    // Ignore walls (vertical surfaces) when resolving Y collisions
    if (overlapX < 2.f)
      continue;

    // Resolve Y collision
    if (velocity.y > 0) { // Falling
      // Only snap to top if we were previously ABOVE the wall
      // Tolerance allows for fast falling, but prevents snapping from
      // side/bottom
      if (prevBottom > wall.position.y + 15.f)
        continue;

      shape.setPosition(
          {shape.getPosition().x, wall.position.y - shape.getSize().y});
      velocity.y = 0.f;
      isGrounded = true;
    } else if (velocity.y < 0) { // Jumping up
      // Upwards Corner Correction: Try to wiggle player horizontally
      const float cornerMargin = 6.f; // Pixels to check for nudge
      sf::FloatRect playerBounds = shape.getGlobalBounds();

      // Check if we can nudge left
      sf::FloatRect nudgeLeft = playerBounds;
      nudgeLeft.position.x -= cornerMargin;
      if (map.checkCollision(nudgeLeft).empty()) {
        shape.move({-cornerMargin, 0.f});
      } else {
        // Check if we can nudge right
        sf::FloatRect nudgeRight = playerBounds;
        nudgeRight.position.x += cornerMargin;
        if (map.checkCollision(nudgeRight).empty()) {
          shape.move({cornerMargin, 0.f});
        } else {
          // Can't nudge, stop upward movement
          shape.setPosition(
              {shape.getPosition().x, wall.position.y + wall.size.y});
          velocity.y = 0.f;
        }
      }
    }
  }

  // Idle Animation Update (only when grounded and not moving)
  if (isGrounded && std::abs(velocity.x) < 10.f) {
    animationTimer += dt;
    if (animationTimer >= animationSpeed) {
      animationTimer = 0.f;
      currentFrame = (currentFrame + 1) % 2; // 2 frames
      sprite.setTextureRect(sf::IntRect({currentFrame * 32, 0}, {32, 32}));
    }
  } else {
    // Reset to first frame when moving/in air
    currentFrame = 0;
    animationTimer = 0.f;
    sprite.setTextureRect(sf::IntRect({0, 0}, {32, 32}));
  }

  // Visual Update
  // Position sprite at bottom-center of hitbox (origin is bottom-center)
  sf::Vector2f bottomCenter = {shape.getPosition().x + shape.getSize().x / 2.f,
                               shape.getPosition().y + shape.getSize().y};
  sprite.setPosition(bottomCenter);

  // Flip Logic
  if (velocity.x > 1.f) {
    facingRight = true;
  } else if (velocity.x < -1.f) {
    facingRight = false;
  }

  if (facingRight) {
    sprite.setScale({1.5f, 1.5f});
  } else {
    sprite.setScale({-1.5f, 1.5f});
  }
}

void Player::render(sf::RenderWindow &window) {
  window.draw(sprite);
  // uncomment to visualize hitbox
  // window.draw(shape);
}

void Player::reset(sf::Vector2f position) {
  // Center the hitbox on the provided position (which is center of tile)
  shape.setPosition({position.x - shape.getSize().x / 2.f,
                     position.y - shape.getSize().y / 2.f});
  velocity = {0.f, 0.f};
  isGrounded = false;
}