#include "Map.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

Map::Map() {
  tileShape.setSize({TILE_SIZE, TILE_SIZE});
  tileShape.setFillColor(sf::Color::White); // Default to white for textures

  // Load Textures
  if (!finishTexture.loadFromFile("assets/tiles/flag.png")) {
    std::cerr << "Failed to load flag.png" << std::endl;
  }
  if (!spawnTexture.loadFromFile("assets/tiles/void.png")) {
    std::cerr << "Failed to load void.png" << std::endl;
  }
  if (!wallTexture.loadFromFile("assets/tiles/stone.png")) {
    std::cerr << "Failed to load stone.png" << std::endl;
  }

  hasFinish = false;
}

bool Map::loadFromFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open map file: " << filename << std::endl;
    return false;
  }

  grid.clear();
  hasFinish = false;
  int spawnCount = 0;

  std::string line;
  int y = 0;

  // Check file extension
  bool isCSV = filename.substr(filename.find_last_of(".") + 1) == "csv";
  if (!isCSV) {
    std::cerr << "Error: Only .csv map files are supported." << std::endl;
    return false;
  }

  while (std::getline(file, line)) {
    std::vector<int> row;

    if (isCSV) {
      std::stringstream ss(line);
      std::string cell;
      while (std::getline(ss, cell, ',')) {
        try {
          int tileID = std::stoi(cell);
          // Map IDs:
          // -1: Empty
          // 0: Spawn
          // 1: Finish
          // 2: Wall

          if (tileID == 0) {
            if (spawnCount > 0) {
              std::cerr << "Warning: Multiple spawn points found!" << std::endl;
            }
            startPosition = {
                static_cast<float>(row.size()) * TILE_SIZE + TILE_SIZE / 2.f,
                static_cast<float>(y) * TILE_SIZE + TILE_SIZE / 2.f};
            spawnCount++;
          } else if (tileID == 1) {
            if (hasFinish) {
              std::cerr << "Warning: Multiple finish points found!"
                        << std::endl;
            }
            finishArea =
                sf::FloatRect({static_cast<float>(row.size()) * TILE_SIZE,
                               static_cast<float>(y) * TILE_SIZE},
                              {TILE_SIZE, TILE_SIZE});
            hasFinish = true;
          }

          row.push_back(tileID);
        } catch (...) {
          row.push_back(-1); // Error fallback
        }
      }
    }

    if (!row.empty()) {
      grid.push_back(row);
      y++;
    }
  }

  // Validation
  if (spawnCount != 1) {
    std::cerr << "Error: Map must have exactly one spawn point. Found: "
              << spawnCount << std::endl;
    return false;
  }
  return true;
}

void Map::render(sf::RenderWindow &window) {
  for (size_t y = 0; y < grid.size(); ++y) {
    for (size_t x = 0; x < grid[y].size(); ++x) {
      int id = grid[y][x];
      tileShape.setPosition({x * TILE_SIZE, y * TILE_SIZE});

      if (id == 2) { // Wall
        tileShape.setTexture(&wallTexture);
        tileShape.setFillColor(
            sf::Color::White); // Reset color if texture is used
        window.draw(tileShape);
      } else if (id == 1) { // Finish
        tileShape.setTexture(&finishTexture);
        tileShape.setFillColor(sf::Color::White);
        window.draw(tileShape);
      } else if (id == 0) { // Spawn
        // Do not render spawn point
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
  if (grid.empty())
    return collisions;
  if (right_tile >= static_cast<int>(grid[0].size()))
    right_tile = static_cast<int>(grid[0].size()) - 1;
  if (bottom_tile >= static_cast<int>(grid.size()))
    bottom_tile = static_cast<int>(grid.size()) - 1;

  for (int y = top_tile; y <= bottom_tile; ++y) {
    for (int x = left_tile; x <= right_tile; ++x) {
      if (y >= 0 && y < grid.size() && x >= 0 && x < grid[y].size()) {
        if (grid[y][x] == 2) { // Wall ONLY collision
          collisions.push_back(sf::FloatRect({x * TILE_SIZE, y * TILE_SIZE},
                                             {TILE_SIZE, TILE_SIZE}));
        }
      }
    }
  }

  return collisions;
}

bool Map::checkFinish(const sf::FloatRect &bounds) const {
  if (!hasFinish)
    return false;
  return bounds.findIntersection(finishArea).has_value();
}
