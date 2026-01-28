#include "Map.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

Map::Map() {
  tileShape.setSize({TILE_SIZE, TILE_SIZE});
  tileShape.setFillColor(sf::Color::White);

  // Load Textures
  if (!finishTexture.loadFromFile("assets/tiles/flag.png")) {
    std::cerr << "Failed to load flag.png" << std::endl;
  }
  if (!spawnTexture.loadFromFile("assets/tiles/void.png")) {
    std::cerr << "Failed to load void.png" << std::endl;
  }
  if (!wallTexture.loadFromFile("assets/tiles/planks.png")) {
    std::cerr << "Failed to load planks.png" << std::endl;
  }

  // Load font for text objects
  fontLoaded = font.openFromFile("assets/fonts/font.ttf");
  if (!fontLoaded) {
    std::cerr << "Failed to load font for map text" << std::endl;
  }
}

bool Map::loadFromFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open map file: " << filename << std::endl;
    return false;
  }

  // Read entire file content
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  // Check file extension
  bool isTMX = filename.substr(filename.find_last_of(".") + 1) == "tmx";
  if (!isTMX) {
    std::cerr << "Error: Only .tmx map files are supported." << std::endl;
    return false;
  }

  return parseTMX(content);
}

// Helper function to extract attribute value from XML tag
std::string extractAttribute(const std::string &tag,
                             const std::string &attrName) {
  std::string search = attrName + "=\"";
  size_t start = tag.find(search);
  if (start == std::string::npos)
    return "";
  start += search.length();
  size_t end = tag.find("\"", start);
  if (end == std::string::npos)
    return "";
  return tag.substr(start, end - start);
}

// Helper to extract content between tags
std::string extractBetweenTags(const std::string &content,
                               const std::string &startTag,
                               const std::string &endTag, size_t searchFrom = 0,
                               size_t *foundEnd = nullptr) {
  size_t start = content.find(startTag, searchFrom);
  if (start == std::string::npos)
    return "";
  start += startTag.length();
  size_t end = content.find(endTag, start);
  if (end == std::string::npos)
    return "";
  if (foundEnd)
    *foundEnd = end + endTag.length();
  return content.substr(start, end - start);
}

bool Map::parseTMX(const std::string &content) {
  mainGrid.clear();
  textureGrid.clear();
  textObjects.clear();
  finishAreas.clear();

  // Extract map dimensions
  size_t mapTagStart = content.find("<map ");
  size_t mapTagEnd = content.find(">", mapTagStart);
  std::string mapTag = content.substr(mapTagStart, mapTagEnd - mapTagStart);

  int mapWidth = std::stoi(extractAttribute(mapTag, "width"));
  int mapHeight = std::stoi(extractAttribute(mapTag, "height"));

  // Parse layers
  size_t pos = 0;
  while (true) {
    size_t layerStart = content.find("<layer ", pos);
    if (layerStart == std::string::npos)
      break;

    size_t layerTagEnd = content.find(">", layerStart);
    std::string layerTag = content.substr(layerStart, layerTagEnd - layerStart);
    std::string layerName = extractAttribute(layerTag, "name");

    // Find the CSV data within this layer
    size_t dataStart = content.find("<data encoding=\"csv\">", layerStart);
    size_t dataEnd = content.find("</data>", dataStart);

    if (dataStart != std::string::npos && dataEnd != std::string::npos) {
      std::string csvData = content.substr(
          dataStart + 21, dataEnd - (dataStart + 21)); // 21 = length of opening
                                                       // tag

      auto grid = parseLayerData(csvData, mapWidth, mapHeight);

      if (layerName == "main") {
        mainGrid = grid;

        // Find spawn and finish in main grid
        // After -1 adjustment: 0=spawn, 1=finish, 2=wall, 3=text
        int spawnCount = 0;
        for (size_t y = 0; y < mainGrid.size(); ++y) {
          for (size_t x = 0; x < mainGrid[y].size(); ++x) {
            int id = mainGrid[y][x];

            if (id == 0) { // Spawn (Tiled ID 1)
              if (spawnCount > 0) {
                std::cerr << "Warning: Multiple spawn points found!"
                          << std::endl;
              }
              startPosition = {
                  static_cast<float>(x) * TILE_SIZE + TILE_SIZE / 2.f,
                  static_cast<float>(y) * TILE_SIZE + TILE_SIZE / 2.f};
              spawnCount++;
            } else if (id == 1) { // Finish (Tiled ID 2)
              finishAreas.push_back(
                  sf::FloatRect({static_cast<float>(x) * TILE_SIZE,
                                 static_cast<float>(y) * TILE_SIZE},
                                {TILE_SIZE, TILE_SIZE}));
            }
          }
        }
      } else if (layerName == "textures") {
        textureGrid = grid;
      }
    }

    pos = dataEnd + 7; // Move past </data>
  }

  // Parse object groups (for text)
  parseObjectGroup(content);

  // Prepare cached text objects (optimization: avoid allocation in render loop)
  prepareTextObjects();

  std::cout << "Loaded TMX map: " << mapWidth << "x" << mapHeight << " tiles"
            << std::endl;
  std::cout << "Text objects found: " << textObjects.size() << std::endl;

  return !mainGrid.empty();
}

std::vector<std::vector<int>> Map::parseLayerData(const std::string &csvData,
                                                  int width, int height) {
  std::vector<std::vector<int>> grid;
  std::stringstream ss(csvData);
  std::string line;

  while (std::getline(ss, line)) {
    // Skip empty lines
    if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos)
      continue;

    std::vector<int> row;
    std::stringstream lineStream(line);
    std::string cell;

    while (std::getline(lineStream, cell, ',')) {
      // Remove whitespace
      cell.erase(std::remove_if(cell.begin(), cell.end(), ::isspace),
                 cell.end());
      if (!cell.empty()) {
        try {
          // Tiled uses 1-based IDs, 0 means empty
          // Subtract 1 to get our 0-based IDs, but handle 0 specially
          int rawId = std::stoi(cell);
          int tileId = (rawId == 0) ? -1 : rawId - 1;
          row.push_back(tileId);
        } catch (...) {
          row.push_back(-1);
        }
      }
    }

    if (!row.empty()) {
      grid.push_back(row);
    }
  }

  return grid;
}

void Map::parseObjectGroup(const std::string &content) {
  size_t pos = 0;

  while (true) {
    size_t objGroupStart = content.find("<objectgroup ", pos);
    if (objGroupStart == std::string::npos)
      break;

    size_t objGroupEnd = content.find("</objectgroup>", objGroupStart);
    if (objGroupEnd == std::string::npos)
      break;

    std::string objGroupContent =
        content.substr(objGroupStart, objGroupEnd - objGroupStart);

    // Check if this is the text layer
    size_t tagEnd = objGroupContent.find(">");
    std::string groupTag = objGroupContent.substr(0, tagEnd);
    std::string groupName = extractAttribute(groupTag, "name");

    if (groupName == "text") {
      // Parse each object in this group
      size_t objPos = 0;
      while (true) {
        size_t objStart = objGroupContent.find("<object ", objPos);
        if (objStart == std::string::npos)
          break;

        size_t objEnd = objGroupContent.find("</object>", objStart);
        if (objEnd == std::string::npos)
          break;

        std::string objContent =
            objGroupContent.substr(objStart, objEnd - objStart);

        // Extract object attributes
        size_t objTagEnd = objContent.find(">");
        std::string objTag = objContent.substr(0, objTagEnd);

        MapText text;
        text.name = extractAttribute(objTag, "name");

        std::string xStr = extractAttribute(objTag, "x");
        std::string yStr = extractAttribute(objTag, "y");
        std::string wStr = extractAttribute(objTag, "width");
        std::string hStr = extractAttribute(objTag, "height");

        if (!xStr.empty())
          text.position.x = std::stof(xStr);
        if (!yStr.empty())
          text.position.y = std::stof(yStr);
        if (!wStr.empty())
          text.size.x = std::stof(wStr);
        if (!hStr.empty())
          text.size.y = std::stof(hStr);

        // Extract text content between <text> tags
        size_t textStart = objContent.find("<text");
        if (textStart != std::string::npos) {
          size_t textContentStart = objContent.find(">", textStart) + 1;
          size_t textContentEnd = objContent.find("</text>", textContentStart);
          if (textContentEnd != std::string::npos) {
            text.content = objContent.substr(textContentStart,
                                             textContentEnd - textContentStart);
          }
        }

        if (!text.content.empty()) {
          textObjects.push_back(text);
        }

        objPos = objEnd + 9; // Move past </object>
      }
    }

    pos = objGroupEnd + 14; // Move past </objectgroup>
  }
}

void Map::render(sf::RenderWindow &window) {
  // Get the current view bounds for culling
  sf::View view = window.getView();
  sf::Vector2f viewCenter = view.getCenter();
  sf::Vector2f viewSize = view.getSize();

  // Calculate visible tile range (with 1 tile margin for safety)
  int startX = std::max(
      0, static_cast<int>((viewCenter.x - viewSize.x / 2.f) / TILE_SIZE) - 1);
  int startY = std::max(
      0, static_cast<int>((viewCenter.y - viewSize.y / 2.f) / TILE_SIZE) - 1);
  int endX = std::min(
      static_cast<int>(textureGrid.empty() ? 0 : textureGrid[0].size()),
      static_cast<int>((viewCenter.x + viewSize.x / 2.f) / TILE_SIZE) + 2);
  int endY = std::min(
      static_cast<int>(textureGrid.size()),
      static_cast<int>((viewCenter.y + viewSize.y / 2.f) / TILE_SIZE) + 2);

  // Render only visible tiles (view culling optimization)
  for (int y = startY; y < endY; ++y) {
    for (int x = startX; x < endX; ++x) {
      if (y >= 0 && y < static_cast<int>(textureGrid.size()) && x >= 0 &&
          x < static_cast<int>(textureGrid[y].size())) {
        int id = textureGrid[y][x];

        // Skip empty tiles
        if (id < 3 || id > 5)
          continue;

        tileShape.setPosition({static_cast<float>(x) * TILE_SIZE,
                               static_cast<float>(y) * TILE_SIZE});

        // Set texture based on tile type (switch is more efficient for multiple
        // cases)
        switch (id) {
        case 5: // Wall texture
          tileShape.setTexture(&wallTexture);
          break;
        case 4: // Finish texture
          tileShape.setTexture(&finishTexture);
          break;
        case 3: // Spawn texture
          tileShape.setTexture(&spawnTexture);
          break;
        default:
          continue;
        }
        tileShape.setFillColor(sf::Color::White);
        window.draw(tileShape);
      }
    }
  }

  // Render cached text objects (no allocation in render loop)
  for (auto &text : cachedTexts) {
    window.draw(text);
  }
}

// Prepare text objects once (called after map loading)
void Map::prepareTextObjects() {
  cachedTexts.clear();

  if (!fontLoaded)
    return;

  // Reserve space to avoid reallocations
  cachedTexts.reserve(textObjects.size());

  for (const auto &textObj : textObjects) {
    sf::Text text(font);
    text.setCharacterSize(12);
    text.setFillColor(sf::Color::White);
    text.setOutlineColor(sf::Color::Black);
    text.setOutlineThickness(1.f);

    // Manual word wrap based on object width from Tiled
    std::string wrappedText;
    std::string currentLine;
    std::string word;
    float maxWidth = textObj.size.x > 0 ? textObj.size.x : 100.f;

    for (size_t i = 0; i <= textObj.content.size(); ++i) {
      char c = (i < textObj.content.size()) ? textObj.content[i] : ' ';

      if (c == ' ' || c == '\n' || i == textObj.content.size()) {
        std::string testLine =
            currentLine.empty() ? word : currentLine + " " + word;
        text.setString(testLine);
        float lineWidth = text.getLocalBounds().size.x;

        if (lineWidth > maxWidth && !currentLine.empty()) {
          if (!wrappedText.empty())
            wrappedText += "\n";
          wrappedText += currentLine;
          currentLine = word;
        } else {
          currentLine = testLine;
        }
        word.clear();

        if (c == '\n') {
          if (!wrappedText.empty())
            wrappedText += "\n";
          wrappedText += currentLine;
          currentLine.clear();
        }
      } else {
        word += c;
      }
    }

    if (!currentLine.empty()) {
      if (!wrappedText.empty())
        wrappedText += "\n";
      wrappedText += currentLine;
    }

    text.setString(wrappedText);
    text.setPosition(textObj.position);
    cachedTexts.push_back(std::move(text));
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
  if (mainGrid.empty())
    return collisions;
  if (right_tile >= static_cast<int>(mainGrid[0].size()))
    right_tile = static_cast<int>(mainGrid[0].size()) - 1;
  if (bottom_tile >= static_cast<int>(mainGrid.size()))
    bottom_tile = static_cast<int>(mainGrid.size()) - 1;

  for (int y = top_tile; y <= bottom_tile; ++y) {
    for (int x = left_tile; x <= right_tile; ++x) {
      if (y >= 0 && y < mainGrid.size() && x >= 0 && x < mainGrid[y].size()) {
        // Main layer ID 2 = wall (after -1 adjustment from Tiled's 3)
        if (mainGrid[y][x] == 2) {
          collisions.push_back(sf::FloatRect({x * TILE_SIZE, y * TILE_SIZE},
                                             {TILE_SIZE, TILE_SIZE}));
        }
      }
    }
  }

  return collisions;
}

bool Map::checkFinish(const sf::FloatRect &bounds) const {
  for (const auto &finishArea : finishAreas) {
    if (bounds.findIntersection(finishArea).has_value()) {
      return true;
    }
  }
  return false;
}
