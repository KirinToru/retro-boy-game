#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class Map {
public:
  // Tile size: 40px (Same as player for now)
  static constexpr float TILE_SIZE = 40.f;

  Map();

  // Loads map from a CSV text file
  void loadFromFile(const std::string &filename);

  // Getters for map dimensions (in pixels)
  float getWidth() const {
    return grid.empty() ? 0.f : grid[0].size() * TILE_SIZE;
  }
  float getHeight() const { return grid.size() * TILE_SIZE; }

  // Returns the player spawn position extracted from the map file
  sf::Vector2f getStartPosition() const { return startPosition; }

  // Renders the map tiles to the window
  void render(sf::RenderWindow &window);

  // Checks for collisions between an entity's bounding box and the map walls.
  // Returns a vector of bounding boxes of the tiles that intersect.
  std::vector<sf::FloatRect> checkCollision(const sf::FloatRect &bounds) const;

private:
  // 2D grid storing tile IDs. 0 = empty, 1 = solid block.
  // Outer vector is rows (Y), inner is columns (X).
  std::vector<std::vector<int>> grid;

  sf::Vector2f startPosition{100.f, 100.f}; // Default if not found

  // Helper to setup shapes for rendering
  sf::RectangleShape tileShape;
};
