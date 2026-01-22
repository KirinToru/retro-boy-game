#include "Map.hpp"

Map::Map() {
  tileShape.setSize({TILE_SIZE, TILE_SIZE});
  tileShape.setFillColor(sf::Color(100, 100, 100)); // Grey walls
  tileShape.setOutlineColor(sf::Color::White);
  tileShape.setOutlineThickness(-1.f);
}

#include <fstream>
#include <iostream>

void Map::loadFromFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open map file: " << filename << std::endl;
    return;
  }

  grid.clear();
  std::string line;
  int y = 0;
  while (std::getline(file, line)) {
    std::vector<int> row;
    for (int x = 0; x < line.length(); ++x) {
      char c = line[x];
      if (c == '#') {
        row.push_back(1);
      } else if (c == '*') {
        row.push_back(0);
        startPosition = {x * TILE_SIZE, y * TILE_SIZE};
      } else {
        row.push_back(0); // Treat everything else as empty
      }
    }

    if (!row.empty()) {
      grid.push_back(row);
      y++;
    }
  }
}

void Map::render(sf::RenderWindow &window) {
  for (size_t y = 0; y < grid.size(); ++y) {
    for (size_t x = 0; x < grid[y].size(); ++x) {
      if (grid[y][x] == 1) {
        tileShape.setPosition({x * TILE_SIZE, y * TILE_SIZE});
        window.draw(tileShape);
      }
    }
  }
}

std::vector<sf::FloatRect>
Map::checkCollision(const sf::FloatRect &bounds) const {
  std::vector<sf::FloatRect> collisions;

  // Calculate tile range to check
  int left_tile = static_cast<int>(bounds.position.x / TILE_SIZE);
  int top_tile = static_cast<int>(bounds.position.y / TILE_SIZE);
  int right_tile =
      static_cast<int>((bounds.position.x + bounds.size.x) / TILE_SIZE);
  int bottom_tile =
      static_cast<int>((bounds.position.y + bounds.size.y) / TILE_SIZE);

  // Clamp to map bounds
  if (left_tile < 0)
    left_tile = 0;
  if (top_tile < 0)
    top_tile = 0;
  if (right_tile >= static_cast<int>(grid[0].size()))
    right_tile = static_cast<int>(grid[0].size()) - 1;
  if (bottom_tile >= static_cast<int>(grid.size()))
    bottom_tile = static_cast<int>(grid.size()) - 1;

  for (int y = top_tile; y <= bottom_tile; ++y) {
    for (int x = left_tile; x <= right_tile; ++x) {
      if (y >= 0 && y < grid.size() && x >= 0 && x < grid[y].size()) {
        if (grid[y][x] == 1) {
          collisions.push_back(sf::FloatRect({x * TILE_SIZE, y * TILE_SIZE},
                                             {TILE_SIZE, TILE_SIZE}));
        }
      }
    }
  }

  return collisions;
}
