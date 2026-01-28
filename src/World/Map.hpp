#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// Structure for text objects from Tiled object layer
struct MapText {
  sf::Vector2f position;
  sf::Vector2f size;
  std::string content;
  std::string name;
};

class Map {
public:
  // Tile size: 32px
  static constexpr float TILE_SIZE = 32.f;

  Map();

  // Loads map from a TMX file (Tiled format)
  bool loadFromFile(const std::string &filename);

  // Getters for map dimensions (in pixels)
  float getWidth() const {
    return mainGrid.empty() ? 0.f : mainGrid[0].size() * TILE_SIZE;
  }
  float getHeight() const { return mainGrid.size() * TILE_SIZE; }

  // Returns the player spawn position extracted from the map file
  sf::Vector2f getStartPosition() const { return startPosition; }

  // Renders only the visible portion of the map (view culling)
  void render(sf::RenderWindow &window);

  // Checks for collisions between an entity's bounding box and the map walls.
  std::vector<sf::FloatRect> checkCollision(const sf::FloatRect &bounds) const;

  // Checks if the player bounds intersect with the finish tile
  bool checkFinish(const sf::FloatRect &bounds) const;

private:
  // Parse TMX XML content
  bool parseTMX(const std::string &content);

  // Parse a single layer's CSV data
  std::vector<std::vector<int>> parseLayerData(const std::string &csvData,
                                               int width, int height);

  // Parse object group for text objects
  void parseObjectGroup(const std::string &content);

  // Prepare cached text objects for rendering (called after parsing)
  void prepareTextObjects();

  // Main grid for collision detection (from "main" layer)
  std::vector<std::vector<int>> mainGrid;

  // Texture grid for rendering (from "textures" layer)
  std::vector<std::vector<int>> textureGrid;

  // Text objects from object layer (raw data)
  std::vector<MapText> textObjects;

  // Cached sf::Text objects for rendering (avoid allocation in render loop)
  std::vector<sf::Text> cachedTexts;

  sf::Vector2f startPosition{100.f, 100.f};
  std::vector<sf::FloatRect> finishAreas;

  // Reusable tile shape (avoid creating new shapes)
  sf::RectangleShape tileShape;

  // Textures for tiles
  sf::Texture finishTexture;
  sf::Texture spawnTexture;
  sf::Texture wallTexture;

  // Font for text rendering
  sf::Font font;
  bool fontLoaded;
};
