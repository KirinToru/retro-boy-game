#include "Game.hpp"
#include <iostream>

const sf::Time Game::TimePerFrame = sf::seconds(1.f / 60.f);

Game::Game()
    : mWindow(sf::VideoMode({1280, 720}), "Journey to the Clouds"),
      mCamera({0.f, 0.f}, {640.f, 360.f}), mPlayer(), mMap(),
      mBackgroundSprite(mBackgroundTexture) {

  if (!mBackgroundTexture.loadFromFile("assets/backgrounds/bg_bricks.png")) {
    std::cerr << "Failed to load bg_bricks.png" << std::endl;
  }

  // Enable texture repeating for tiled background
  mBackgroundTexture.setRepeated(true);
  mBackgroundSprite.setTexture(mBackgroundTexture, true);

  // Scale the background (2x for pixel art look)
  mBackgroundSprite.setScale({2.f, 2.f});

  // Load font for FPS counter
  mFPSFontLoaded = mFPSFont.openFromFile("assets/fonts/font.ttf");

  loadLevel("assets/maps/tutorial.tmx");

  // Cap framerate at 60 FPS (use both methods for reliability)
  mWindow.setFramerateLimit(60);
  mWindow.setVerticalSyncEnabled(true);
}

void Game::run() {
  sf::Clock clock;
  sf::Time timeSinceLastUpdate = sf::Time::Zero;

  while (mWindow.isOpen()) {
    sf::Time dt = clock.restart();
    timeSinceLastUpdate += dt;

    while (timeSinceLastUpdate > TimePerFrame) {
      timeSinceLastUpdate -= TimePerFrame;

      processEvents();
      update(TimePerFrame);
    }
    render();
  }
}

void Game::processEvents() {
  while (const std::optional event = mWindow.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      mWindow.close();
    }

    // Handle Resizing
    if (const auto *resized = event->getIf<sf::Event::Resized>()) {
      sf::Vector2f newSize(static_cast<float>(resized->size.x),
                           static_cast<float>(resized->size.y));
      // Maintain 2x Zoom
      mCamera.setSize({newSize.x / 2.f, newSize.y / 2.f});
    }

    // Key Presses
    if (const auto *keyPress = event->getIf<sf::Event::KeyPressed>()) {
      if (keyPress->code == sf::Keyboard::Key::R) {
        mPlayer.reset(mMap.getStartPosition());
      }
      // F1 - Toggle hitbox visibility
      if (keyPress->code == sf::Keyboard::Key::F1) {
        mShowHitbox = !mShowHitbox;
      }
      // F2 - Toggle FPS counter
      if (keyPress->code == sf::Keyboard::Key::F2) {
        mShowFPS = !mShowFPS;
      }
      // F4 - Cycle window mode
      if (keyPress->code == sf::Keyboard::Key::F4) {
        cycleWindowMode();
      }
    }
  }
}

void Game::update(sf::Time dt) {
  mPlayer.update(dt.asSeconds(), mMap);

  // Death Logic (Falling off map)
  if (mPlayer.getPosition().y > mMap.getHeight() + 200.f) {
    mPlayer.reset(mMap.getStartPosition());
  }

  // Finish Logic
  if (mMap.checkFinish(mPlayer.getBounds())) {
    std::cout << "Level Finished! Resetting..." << std::endl;
    mPlayer.reset(mMap.getStartPosition());
  }

  // Camera Logic
  sf::Vector2f playerPos = mPlayer.getPosition();
  sf::Vector2f viewSize = mCamera.getSize();
  sf::Vector2f currentCenter = mCamera.getCenter();

  float targetX, targetY;

  // Calculate TARGET X (Clamped to map)
  float mapW = mMap.getWidth();
  if (mapW < viewSize.x) {
    targetX = mapW / 2.f;
  } else {
    targetX = std::max(playerPos.x, viewSize.x / 2.f);
    targetX = std::min(targetX, mapW - viewSize.x / 2.f);
  }

  // Calculate TARGET Y (Clamped to map)
  float mapH = mMap.getHeight();
  if (mapH < viewSize.y) {
    targetY = mapH / 2.f;
  } else {
    targetY = std::max(playerPos.y, viewSize.y / 2.f);
    targetY = std::min(targetY, mapH - viewSize.y / 2.f);
  }

  // Smoothly interpolate current center towards target
  // Factor 5.0f determines the "tightness" of the rubber band
  float lerpSpeed = 5.0f;
  float newX = currentCenter.x +
               (targetX - currentCenter.x) * lerpSpeed * dt.asSeconds();
  float newY = currentCenter.y +
               (targetY - currentCenter.y) * lerpSpeed * dt.asSeconds();

  mCamera.setCenter({newX, newY});
  mWindow.setView(mCamera);
}

void Game::render() {
  mWindow.clear(sf::Color(50, 50, 80)); // Dark blue fallback color

  // Set world view for all game elements
  mWindow.setView(mCamera);

  // Parallax background - scroll texture based on camera position
  // Factor 0.3 = background moves at 30% of camera speed (further away =
  // slower)
  float parallaxFactor = 0.3f;
  sf::Vector2f cameraCenter = mCamera.getCenter();
  sf::Vector2f viewSize = mCamera.getSize();
  float bgScale = mBackgroundSprite.getScale().x;

  // Calculate texture offset for parallax (scroll the texture)
  int texOffsetX =
      static_cast<int>((cameraCenter.x * parallaxFactor) / bgScale);
  int texOffsetY =
      static_cast<int>((cameraCenter.y * parallaxFactor) / bgScale);

  // Size of visible area in texture coordinates
  int texWidth = static_cast<int>(viewSize.x / bgScale) + 64; // Extra margin
  int texHeight = static_cast<int>(viewSize.y / bgScale) + 64;

  // Update texture rect with parallax offset
  mBackgroundSprite.setTextureRect(
      sf::IntRect({texOffsetX, texOffsetY}, {texWidth, texHeight}));

  // Position sprite at camera top-left corner
  mBackgroundSprite.setPosition(
      {cameraCenter.x - viewSize.x / 2.f, cameraCenter.y - viewSize.y / 2.f});

  // Draw tiled background (behind everything)
  mWindow.draw(mBackgroundSprite);

  // Draw map and player
  mMap.render(mWindow);
  mPlayer.render(mWindow, mShowHitbox);

  // FPS Counter
  mFrameCount++;
  if (mFPSClock.getElapsedTime().asSeconds() >= 0.1f) { // Update every 100ms
    mCurrentFPS =
        static_cast<int>(mFrameCount / mFPSClock.getElapsedTime().asSeconds());
    mFrameCount = 0;
    mFPSClock.restart();
  }

  // Draw FPS (in screen space)
  if (mShowFPS && mFPSFontLoaded) {
    mWindow.setView(mWindow.getDefaultView());
    sf::Text fpsText(mFPSFont);
    fpsText.setString(std::to_string(mCurrentFPS));
    fpsText.setCharacterSize(16);
    fpsText.setFillColor(sf::Color::Green);
    fpsText.setOutlineColor(sf::Color::Black);
    fpsText.setOutlineThickness(1.f);
    // Position at top-right
    float textWidth = fpsText.getLocalBounds().size.x;
    fpsText.setPosition({mWindow.getSize().x - textWidth - 10.f, 10.f});
    mWindow.draw(fpsText);
  }

  mWindow.display();
}

void Game::loadLevel(const std::string &filename) {
  // Only reload if needed, but for now simple reload
  if (mMap.loadFromFile(filename)) {
    mPlayer.reset(mMap.getStartPosition());

    sf::Vector2f playerPos = mPlayer.getPosition();
    sf::Vector2f viewSize = mCamera.getSize();
    float mapW = mMap.getWidth();
    float mapH = mMap.getHeight();

    float camX = std::max(playerPos.x, viewSize.x / 2.f);
    camX = std::min(camX, mapW - viewSize.x / 2.f);
    float camY = std::max(playerPos.y, viewSize.y / 2.f);
    camY = std::min(camY, mapH - viewSize.y / 2.f);
    mCamera.setCenter({camX, camY});

    // Set background texture rect to cover the map
    float bgScale = mBackgroundSprite.getScale().x;
    int texWidth = static_cast<int>((mMap.getWidth() * 1.0f) / bgScale);
    int texHeight = static_cast<int>((mMap.getHeight() * 1.0f) / bgScale);
    mBackgroundSprite.setTextureRect(
        sf::IntRect({0, 0}, {texWidth, texHeight}));
  } else {
    std::cerr << "Failed to load level: " << filename << std::endl;
  }
}

void Game::cycleWindowMode() {
  mWindowMode = (mWindowMode + 1) % 3;

  switch (mWindowMode) {
  case 0: // Windowed
    mWindow.create(sf::VideoMode({1280, 720}), "Journey to the Clouds",
                   sf::Style::Default);
    break;
  case 1: // Maximized (borderless windowed at desktop resolution)
  {
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    mWindow.create(desktop, "Journey to the Clouds", sf::Style::None);
  } break;
  case 2: // Fullscreen
    mWindow.create(sf::VideoMode::getDesktopMode(), "Journey to the Clouds",
                   sf::State::Fullscreen);
    break;
  }

  // Restore camera size based on new window size
  sf::Vector2u winSize = mWindow.getSize();
  mCamera.setSize({winSize.x / 2.f, winSize.y / 2.f});

  // Restore FPS limit
  mWindow.setFramerateLimit(60);
  mWindow.setVerticalSyncEnabled(true);
}
