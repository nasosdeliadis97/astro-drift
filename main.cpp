#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <vector>

constexpr unsigned int WINDOW_WIDTH = 1280;
constexpr unsigned int WINDOW_HEIGHT = 720;
constexpr float PI = 3.14159265f;

const sf::Color SPACE_BACKGROUND(5, 7, 18);
const sf::Color PLAYER_NORMAL(255, 255, 255);
const sf::Color PLAYER_HIT_RED(255, 90, 90);

enum class GameState {
    MainMenu,
    ShipSelect,
    HighScore,
    Playing,
    Paused,
    GameOver
};

enum class PowerUpType {
    Shield,
    RapidFire
};

enum class ObstacleType {
    NormalAsteroid,
    RareAsteroid,
    ShipDebris
};

struct Asteroid {
    sf::Sprite sprite;
    float speed;
    float rotationSpeed;
    ObstacleType type;
};

struct Star {
    sf::CircleShape shape;
    float speed;
};

struct Bullet {
    sf::Sprite sprite;
    float speed;
};

struct Explosion {
    sf::CircleShape shape;
    float lifetime;
    float maxLifetime;
};

struct PowerUp {
    sf::Sprite sprite;
    PowerUpType type;
    float speed;
};

struct Fragment {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    float rotationSpeed;
    float lifetime;
    float maxLifetime;
};

float randomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 generator(rd());
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(generator);
}

int randomInt(int min, int max) {
    static std::random_device rd;
    static std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

float lerp(float start, float end, float t) {
    return start + (end - start) * t;
}

bool containsPoint(const sf::FloatRect& rect, sf::Vector2f point) {
    return point.x >= rect.position.x &&
           point.x <= rect.position.x + rect.size.x &&
           point.y >= rect.position.y &&
           point.y <= rect.position.y + rect.size.y;
}

void centerTextOrigin(sf::Text& text) {
    sf::FloatRect bounds = text.getLocalBounds();

    text.setOrigin({
        bounds.position.x + bounds.size.x / 2.f,
        bounds.position.y + bounds.size.y / 2.f
    });
}

void centerSpriteOrigin(sf::Sprite& sprite) {
    sf::FloatRect bounds = sprite.getLocalBounds();

    sprite.setOrigin({
        bounds.position.x + bounds.size.x / 2.f,
        bounds.position.y + bounds.size.y / 2.f
    });
}

void updateLetterboxView(sf::View& view, unsigned int windowWidth, unsigned int windowHeight) {
    float windowRatio =
        static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

    float gameRatio =
        static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);

    float viewportWidth = 1.f;
    float viewportHeight = 1.f;
    float viewportLeft = 0.f;
    float viewportTop = 0.f;

    if (windowRatio > gameRatio) {
        viewportWidth = gameRatio / windowRatio;
        viewportLeft = (1.f - viewportWidth) / 2.f;
    } else if (windowRatio < gameRatio) {
        viewportHeight = windowRatio / gameRatio;
        viewportTop = (1.f - viewportHeight) / 2.f;
    }

    view.setViewport(sf::FloatRect(
        {viewportLeft, viewportTop},
        {viewportWidth, viewportHeight}
    ));
}

sf::Vector2f playerStartPosition() {
    return {
        static_cast<float>(WINDOW_WIDTH) / 2.f,
        static_cast<float>(WINDOW_HEIGHT) - 70.f
    };
}

std::array<sf::FloatRect, 3> getShipCardBounds() {
    return {
        sf::FloatRect({230.f, 210.f}, {220.f, 260.f}),
        sf::FloatRect({530.f, 210.f}, {220.f, 260.f}),
        sf::FloatRect({830.f, 210.f}, {220.f, 260.f})
    };
}

int loadHighScore() {
    std::ifstream file("highscore.txt");

    int highScore = 0;

    if (file >> highScore) {
        return highScore;
    }

    return 0;
}

void saveHighScore(int highScore) {
    std::ofstream file("highscore.txt");

    if (file) {
        file << highScore;
    }
}

bool updateHighScoreIfNeeded(int score, int& highScore) {
    if (score > highScore) {
        highScore = score;
        saveHighScore(highScore);
        return true;
    }

    return false;
}

std::vector<Star> createStars(unsigned int count) {
    std::vector<Star> stars;

    for (unsigned int i = 0; i < count; i++) {
        float radius = randomFloat(1.f, 2.4f);

        Star star;
        star.shape = sf::CircleShape(radius);
        star.shape.setFillColor(sf::Color(
            210,
            220,
            255,
            static_cast<std::uint8_t>(randomFloat(70.f, 170.f))
        ));

        star.shape.setPosition({
            randomFloat(0.f, static_cast<float>(WINDOW_WIDTH)),
            randomFloat(0.f, static_cast<float>(WINDOW_HEIGHT))
        });

        star.speed = randomFloat(20.f, 85.f);
        stars.push_back(star);
    }

    return stars;
}

void updateStars(std::vector<Star>& stars, float deltaTime) {
    for (auto& star : stars) {
        star.shape.move({0.f, star.speed * deltaTime});

        if (star.shape.getPosition().y > static_cast<float>(WINDOW_HEIGHT)) {
            star.shape.setPosition({
                randomFloat(0.f, static_cast<float>(WINDOW_WIDTH)),
                -10.f
            });
        }
    }
}

Asteroid createAsteroid(
    const sf::Texture& asteroidTexture,
    const std::array<sf::Texture, 3>& shipTextures,
    float difficulty
) {
    int roll = randomInt(1, 100);

    ObstacleType type = ObstacleType::NormalAsteroid;

    if (roll <= 7) {
        type = ObstacleType::ShipDebris;
    } else if (roll <= 17) {
        type = ObstacleType::RareAsteroid;
    }

    const sf::Texture* selectedTexture = &asteroidTexture;

    if (type == ObstacleType::ShipDebris) {
        selectedTexture = &shipTextures[randomInt(0, 2)];
    }

    Asteroid asteroid{sf::Sprite(*selectedTexture), 0.f, 0.f, type};

    centerSpriteOrigin(asteroid.sprite);

    if (type == ObstacleType::NormalAsteroid) {
        float scale = randomFloat(0.45f, 0.8f);
        asteroid.sprite.setScale({scale, scale});
        asteroid.sprite.setColor(sf::Color(255, 255, 255, 255));

        float minSpeed = lerp(160.f, 280.f, difficulty);
        float maxSpeed = lerp(290.f, 540.f, difficulty);

        asteroid.speed = randomFloat(minSpeed, maxSpeed);
        asteroid.rotationSpeed = randomFloat(-140.f, 140.f);
    } else if (type == ObstacleType::RareAsteroid) {
        float scale = randomFloat(0.55f, 0.92f);
        asteroid.sprite.setScale({scale, scale});
        asteroid.sprite.setColor(sf::Color(255, 145, 95, 245));

        float minSpeed = lerp(210.f, 340.f, difficulty);
        float maxSpeed = lerp(340.f, 620.f, difficulty);

        asteroid.speed = randomFloat(minSpeed, maxSpeed);
        asteroid.rotationSpeed = randomFloat(-220.f, 220.f);
    } else {
        float scale = randomFloat(0.17f, 0.26f);
        asteroid.sprite.setScale({scale, scale});
        asteroid.sprite.setColor(sf::Color(160, 170, 185, 215));

        float minSpeed = lerp(190.f, 310.f, difficulty);
        float maxSpeed = lerp(320.f, 570.f, difficulty);

        asteroid.speed = randomFloat(minSpeed, maxSpeed);
        asteroid.rotationSpeed = randomFloat(-330.f, 330.f);
    }

    asteroid.sprite.setPosition({
        randomFloat(50.f, static_cast<float>(WINDOW_WIDTH) - 50.f),
        -80.f
    });

    return asteroid;
}

Bullet createBullet(const sf::Texture& laserTexture, const sf::Sprite& player) {
    Bullet bullet{sf::Sprite(laserTexture), 740.f};

    centerSpriteOrigin(bullet.sprite);

    bullet.sprite.setScale({0.58f, 0.58f});
    bullet.sprite.setPosition({
        player.getPosition().x,
        player.getPosition().y - 35.f
    });

    return bullet;
}

Explosion createExplosion(sf::Vector2f position) {
    Explosion explosion;

    explosion.shape = sf::CircleShape(8.f);
    explosion.shape.setOrigin({8.f, 8.f});
    explosion.shape.setPosition(position);

    explosion.shape.setFillColor(sf::Color(255, 170, 40, 180));
    explosion.shape.setOutlineThickness(3.f);
    explosion.shape.setOutlineColor(sf::Color(255, 230, 120, 220));

    explosion.lifetime = 0.f;
    explosion.maxLifetime = 0.35f;

    return explosion;
}

std::vector<Fragment> createFragments(
    const sf::Texture& texture,
    sf::Vector2f position,
    int count,
    float minScale,
    float maxScale,
    sf::Color color,
    float minSpeed,
    float maxSpeed,
    float minLifetime,
    float maxLifetime
) {
    std::vector<Fragment> fragments;

    for (int i = 0; i < count; i++) {
        Fragment fragment{sf::Sprite(texture), {0.f, 0.f}, 0.f, 0.f, 0.f};

        centerSpriteOrigin(fragment.sprite);

        float scale = randomFloat(minScale, maxScale);
        fragment.sprite.setScale({scale, scale});
        fragment.sprite.setPosition(position);
        fragment.sprite.setRotation(sf::degrees(randomFloat(0.f, 360.f)));
        fragment.sprite.setColor(color);

        float angle = randomFloat(0.f, PI * 2.f);
        float speed = randomFloat(minSpeed, maxSpeed);

        fragment.velocity = {
            std::cos(angle) * speed,
            std::sin(angle) * speed
        };

        fragment.rotationSpeed = randomFloat(-420.f, 420.f);
        fragment.lifetime = 0.f;
        fragment.maxLifetime = randomFloat(minLifetime, maxLifetime);

        fragments.push_back(fragment);
    }

    return fragments;
}

PowerUp createPowerUp(
    const sf::Texture& shieldTexture,
    const sf::Texture& rapidTexture
) {
    PowerUpType type = randomInt(0, 1) == 0
        ? PowerUpType::Shield
        : PowerUpType::RapidFire;

    const sf::Texture& selectedTexture = type == PowerUpType::Shield
        ? shieldTexture
        : rapidTexture;

    PowerUp powerUp{sf::Sprite(selectedTexture), type, 160.f};

    centerSpriteOrigin(powerUp.sprite);
    powerUp.sprite.setScale({0.28f, 0.28f});
    powerUp.sprite.setPosition({
        randomFloat(60.f, static_cast<float>(WINDOW_WIDTH) - 60.f),
        -40.f
    });

    if (type == PowerUpType::Shield) {
        powerUp.sprite.setColor(sf::Color(120, 210, 255));
    } else {
        powerUp.sprite.setColor(sf::Color(255, 235, 100));
    }

    return powerUp;
}

void setupPlayer(sf::Sprite& player, const sf::Texture& texture) {
    player.setTexture(texture, true);
    centerSpriteOrigin(player);
    player.setScale({0.38f, 0.38f});
    player.setPosition(playerStartPosition());
    player.setRotation(sf::degrees(0.f));
    player.setColor(PLAYER_NORMAL);
}

void resetGameplay(
    sf::Sprite& player,
    const sf::Texture& selectedShipTexture,
    std::vector<Asteroid>& asteroids,
    std::vector<Bullet>& bullets,
    std::vector<Explosion>& explosions,
    std::vector<PowerUp>& powerUps,
    std::vector<Fragment>& fragments,
    int& score,
    int& lives,
    bool& invincible,
    bool& shieldActive,
    bool& rapidFireActive,
    bool& playerVisible,
    bool& newHighScore,
    float& shieldTime,
    float& rapidFireTime,
    float& shakeTimer,
    float& survivalTime
) {
    setupPlayer(player, selectedShipTexture);

    asteroids.clear();
    bullets.clear();
    explosions.clear();
    powerUps.clear();
    fragments.clear();

    score = 0;
    lives = 3;
    invincible = false;
    shieldActive = false;
    rapidFireActive = false;
    playerVisible = true;
    newHighScore = false;
    shieldTime = 0.f;
    rapidFireTime = 0.f;
    shakeTimer = 0.f;
    survivalTime = 0.f;
}

void drawTextWithShadow(
    sf::RenderWindow& window,
    sf::Text& text,
    sf::Vector2f shadowOffset = {2.f, 2.f}
) {
    sf::Vector2f originalPosition = text.getPosition();
    sf::Color originalColor = text.getFillColor();

    text.setPosition(originalPosition + shadowOffset);
    text.setFillColor(sf::Color(0, 0, 0, 190));
    window.draw(text);

    text.setPosition(originalPosition);
    text.setFillColor(originalColor);
    window.draw(text);
}

void styleTextButton(sf::Text& text, bool selected) {
    if (selected) {
        text.setFillColor(sf::Color(255, 230, 120));
        text.setScale({1.08f, 1.08f});
    } else {
        text.setFillColor(sf::Color(235, 240, 255));
        text.setScale({1.f, 1.f});
    }
}

void drawTextButtonDecoration(
    sf::RenderWindow& window,
    const sf::Text& text,
    bool selected
) {
    if (!selected) {
        return;
    }

    sf::FloatRect bounds = text.getGlobalBounds();

    sf::RectangleShape underline({
        bounds.size.x + 48.f,
        3.f
    });

    underline.setOrigin({
        (bounds.size.x + 48.f) / 2.f,
        1.5f
    });

    underline.setPosition({
        bounds.position.x + bounds.size.x / 2.f,
        bounds.position.y + bounds.size.y + 14.f
    });

    underline.setFillColor(sf::Color(255, 230, 120, 210));
    window.draw(underline);

    sf::ConvexShape pointer;
    pointer.setPointCount(3);
    pointer.setPoint(0, {0.f, 0.f});
    pointer.setPoint(1, {16.f, 9.f});
    pointer.setPoint(2, {0.f, 18.f});
    pointer.setFillColor(sf::Color(255, 230, 120, 220));

    pointer.setPosition({
        bounds.position.x - 48.f,
        bounds.position.y + bounds.size.y / 2.f - 5.f
    });

    window.draw(pointer);
}

void drawSpriteGlow(
    sf::RenderWindow& window,
    const sf::Sprite& source,
    sf::Color color,
    float scaleMultiplier
) {
    sf::Sprite glow = source;

    sf::Vector2f scale = source.getScale();
    glow.setScale({
        scale.x * scaleMultiplier,
        scale.y * scaleMultiplier
    });

    glow.setColor(color);
    window.draw(glow);
}

void drawBackground(
    sf::RenderWindow& window,
    const sf::Sprite& backgroundSprite,
    const std::vector<Star>& stars
) {
    window.clear(SPACE_BACKGROUND);

    window.draw(backgroundSprite);

    sf::RectangleShape softDarkOverlay({
        static_cast<float>(WINDOW_WIDTH),
        static_cast<float>(WINDOW_HEIGHT)
    });

    softDarkOverlay.setPosition({0.f, 0.f});
    softDarkOverlay.setFillColor(sf::Color(0, 0, 0, 18));
    window.draw(softDarkOverlay);

    for (const auto& star : stars) {
        window.draw(star.shape);
    }
}

int main() {
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();

    sf::RenderWindow window(
        desktopMode,
        "Astro Drift",
        sf::Style::Default,
        sf::State::Fullscreen
    );

    window.setFramerateLimit(60);

    sf::View gameView(sf::FloatRect(
        {0.f, 0.f},
        {static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)}
    ));

    updateLetterboxView(gameView, window.getSize().x, window.getSize().y);

    sf::View camera = gameView;

    GameState gameState = GameState::MainMenu;

    std::array<sf::Texture, 3> shipTextures;
    sf::Texture asteroidTexture;
    sf::Texture laserTexture;
    sf::Texture shieldPowerUpTexture;
    sf::Texture rapidPowerUpTexture;
    sf::Texture backgroundTexture;
    sf::Texture logoTexture;

    if (!backgroundTexture.loadFromFile("assets/background.png")) {
        return 1;
    }

    if (!logoTexture.loadFromFile("assets/logo.png")) {
        return 1;
    }

    if (!shipTextures[0].loadFromFile("assets/spaceship_1.png")) {
        return 1;
    }

    if (!shipTextures[1].loadFromFile("assets/spaceship_2.png")) {
        return 1;
    }

    if (!shipTextures[2].loadFromFile("assets/spaceship_3.png")) {
        return 1;
    }

    if (!asteroidTexture.loadFromFile("assets/asteroid.png")) {
        return 1;
    }

    if (!laserTexture.loadFromFile("assets/laser.png")) {
        return 1;
    }

    if (!shieldPowerUpTexture.loadFromFile("assets/powerup_shield.png")) {
        return 1;
    }

    if (!rapidPowerUpTexture.loadFromFile("assets/powerup_rapid.png")) {
        return 1;
    }

    sf::SoundBuffer shootBuffer;
    sf::SoundBuffer explosionBuffer;
    sf::SoundBuffer powerUpBuffer;
    sf::SoundBuffer hitBuffer;
    sf::SoundBuffer deathBuffer;
    sf::SoundBuffer menuSelectBuffer;

    bool hasShootSound = shootBuffer.loadFromFile("assets/shoot.wav");
    bool hasExplosionSound = explosionBuffer.loadFromFile("assets/explosion.wav");
    bool hasPowerUpSound = powerUpBuffer.loadFromFile("assets/powerup.wav");
    bool hasHitSound = hitBuffer.loadFromFile("assets/hit.wav");
    bool hasDeathSound = deathBuffer.loadFromFile("assets/death.wav");
    bool hasMenuSelectSound = menuSelectBuffer.loadFromFile("assets/menu_select.wav");

    sf::Sound shootSound(shootBuffer);
    sf::Sound explosionSound(explosionBuffer);
    sf::Sound powerUpSound(powerUpBuffer);
    sf::Sound hitSound(hitBuffer);
    sf::Sound deathSound(deathBuffer);
    sf::Sound menuSelectSound(menuSelectBuffer);

    shootSound.setVolume(35.f);
    explosionSound.setVolume(55.f);
    powerUpSound.setVolume(55.f);
    hitSound.setVolume(60.f);
    deathSound.setVolume(75.f);
    menuSelectSound.setVolume(35.f);

    sf::Music menuMusic;
    bool hasMenuMusic = menuMusic.openFromFile("assets/menu_music.ogg");

    if (hasMenuMusic) {
        menuMusic.setLooping(true);
        menuMusic.setVolume(22.f);
    }

    bool soundEnabled = true;

    auto playSound = [&](sf::Sound& sound, bool loaded) {
        if (soundEnabled && loaded) {
            sound.stop();
            sound.play();
        }
    };

    auto stopAllSounds = [&]() {
        shootSound.stop();
        explosionSound.stop();
        powerUpSound.stop();
        hitSound.stop();
        deathSound.stop();
        menuSelectSound.stop();

        if (hasMenuMusic) {
            menuMusic.pause();
        }
    };

    auto updateMenuMusic = [&]() {
        bool shouldPlayMenuMusic =
            soundEnabled &&
            hasMenuMusic &&
            (
                gameState == GameState::MainMenu ||
                gameState == GameState::ShipSelect ||
                gameState == GameState::HighScore
            );

        if (shouldPlayMenuMusic) {
            if (menuMusic.getStatus() != sf::SoundSource::Status::Playing) {
                menuMusic.play();
            }
        } else {
            if (hasMenuMusic &&
                menuMusic.getStatus() == sf::SoundSource::Status::Playing) {
                menuMusic.pause();
            }
        }
    };

    sf::Sprite backgroundSprite(backgroundTexture);
    backgroundSprite.setScale({
        static_cast<float>(WINDOW_WIDTH) / static_cast<float>(backgroundTexture.getSize().x),
        static_cast<float>(WINDOW_HEIGHT) / static_cast<float>(backgroundTexture.getSize().y)
    });

    sf::Sprite logoSprite(logoTexture);
    centerSpriteOrigin(logoSprite);
    logoSprite.setScale({0.32f, 0.32f});
    logoSprite.setPosition({
        static_cast<float>(WINDOW_WIDTH) / 2.f,
        140.f
    });

    int selectedShipIndex = 0;
    int highScore = loadHighScore();

    sf::Sprite player(shipTextures[selectedShipIndex]);
    setupPlayer(player, shipTextures[selectedShipIndex]);

    sf::Font font;

    if (!font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
        if (!font.openFromFile("/System/Library/Fonts/SFNS.ttf")) {
            return 1;
        }
    }

    sf::Font sciFiFont;
    bool hasSciFiFont = sciFiFont.openFromFile("assets/orbitron.ttf");

    const sf::Font& displayFont = hasSciFiFont ? sciFiFont : font;

    sf::Text playText(displayFont);
    playText.setString("PLAY");
    playText.setCharacterSize(38);
    centerTextOrigin(playText);

    sf::Text chooseShipText(displayFont);
    chooseShipText.setString("CHOOSE SHIP");
    chooseShipText.setCharacterSize(38);
    centerTextOrigin(chooseShipText);

    sf::Text highScoreMenuText(displayFont);
    highScoreMenuText.setString("HIGH SCORE");
    highScoreMenuText.setCharacterSize(38);
    centerTextOrigin(highScoreMenuText);

    sf::Text quitText(displayFont);
    quitText.setString("QUIT");
    quitText.setCharacterSize(38);
    centerTextOrigin(quitText);

    std::array<sf::FloatRect, 4> menuHitBoxes = {
        sf::FloatRect({440.f, 300.f}, {400.f, 56.f}),
        sf::FloatRect({440.f, 365.f}, {400.f, 56.f}),
        sf::FloatRect({440.f, 430.f}, {400.f, 56.f}),
        sf::FloatRect({440.f, 495.f}, {400.f, 56.f})
    };

    std::array<sf::Text*, 4> menuTexts = {
        &playText,
        &chooseShipText,
        &highScoreMenuText,
        &quitText
    };

    for (int i = 0; i < 4; i++) {
        float y = 328.f + static_cast<float>(i) * 65.f;

        menuTexts[i]->setPosition({
            static_cast<float>(WINDOW_WIDTH) / 2.f,
            y
        });
    }

    sf::Text shipSelectTitle(displayFont);
    shipSelectTitle.setString("CHOOSE YOUR SHIP");
    shipSelectTitle.setCharacterSize(52);
    shipSelectTitle.setFillColor(sf::Color(245, 248, 255));
    centerTextOrigin(shipSelectTitle);
    shipSelectTitle.setPosition({
        static_cast<float>(WINDOW_WIDTH) / 2.f,
        105.f
    });

    sf::Text shipBackText(displayFont);
    shipBackText.setString("BACK");
    shipBackText.setCharacterSize(32);
    centerTextOrigin(shipBackText);
    shipBackText.setPosition({
        static_cast<float>(WINDOW_WIDTH) / 2.f,
        635.f
    });

    sf::FloatRect shipBackBounds({500.f, 605.f}, {280.f, 70.f});

    sf::Text highScoreTitleText(displayFont);
    highScoreTitleText.setString("HIGH SCORE");
    highScoreTitleText.setCharacterSize(60);
    highScoreTitleText.setFillColor(sf::Color(255, 230, 120));
    centerTextOrigin(highScoreTitleText);
    highScoreTitleText.setPosition({
        static_cast<float>(WINDOW_WIDTH) / 2.f,
        160.f
    });

    sf::Text highScoreValueText(displayFont);
    highScoreValueText.setCharacterSize(42);
    highScoreValueText.setFillColor(sf::Color(245, 248, 255));

    sf::Text backButtonText(displayFont);
    backButtonText.setString("BACK");
    backButtonText.setCharacterSize(36);
    centerTextOrigin(backButtonText);
    backButtonText.setPosition({
        static_cast<float>(WINDOW_WIDTH) / 2.f,
        500.f
    });

    sf::FloatRect backButtonBounds({500.f, 470.f}, {280.f, 70.f});

    sf::Text hudText(font);
    hudText.setCharacterSize(20);
    hudText.setFillColor(sf::Color(245, 248, 255));
    hudText.setPosition({18.f, 16.f});

    sf::Text soundButtonText(displayFont);
    soundButtonText.setCharacterSize(18);

    sf::FloatRect soundButtonBounds({
        static_cast<float>(WINDOW_WIDTH) - 165.f,
        static_cast<float>(WINDOW_HEIGHT) - 52.f
    }, {150.f, 40.f});

    sf::Text gameOverText(displayFont);
    gameOverText.setCharacterSize(62);
    gameOverText.setFillColor(sf::Color(255, 100, 100));
    gameOverText.setString("GAME OVER");
    centerTextOrigin(gameOverText);
    gameOverText.setPosition({
        static_cast<float>(WINDOW_WIDTH) / 2.f,
        static_cast<float>(WINDOW_HEIGHT) / 2.f - 150.f
    });

    sf::Text newHighScoreText(displayFont);
    newHighScoreText.setCharacterSize(34);
    newHighScoreText.setFillColor(sf::Color(255, 230, 120));
    newHighScoreText.setString("NEW HIGH SCORE!");
    centerTextOrigin(newHighScoreText);
    newHighScoreText.setPosition({
        static_cast<float>(WINDOW_WIDTH) / 2.f,
        static_cast<float>(WINDOW_HEIGHT) / 2.f - 85.f
    });

    sf::Text finalScoreText(displayFont);
    finalScoreText.setCharacterSize(28);
    finalScoreText.setFillColor(sf::Color(245, 248, 255));

    std::array<sf::Text, 2> gameOverButtonTexts = {
        sf::Text(displayFont),
        sf::Text(displayFont)
    };

    gameOverButtonTexts[0].setString("RESTART");
    gameOverButtonTexts[1].setString("MAIN MENU");

    std::array<sf::FloatRect, 2> gameOverButtonBounds = {
        sf::FloatRect({340.f, 405.f}, {280.f, 70.f}),
        sf::FloatRect({660.f, 405.f}, {280.f, 70.f})
    };

    for (int i = 0; i < 2; i++) {
        float x = i == 0
            ? static_cast<float>(WINDOW_WIDTH) / 2.f - 160.f
            : static_cast<float>(WINDOW_WIDTH) / 2.f + 160.f;

        gameOverButtonTexts[i].setCharacterSize(32);
        centerTextOrigin(gameOverButtonTexts[i]);
        gameOverButtonTexts[i].setPosition({
            x,
            static_cast<float>(WINDOW_HEIGHT) / 2.f + 80.f
        });
    }

    sf::Text pausedTitleText(displayFont);
    pausedTitleText.setString("PAUSED");
    pausedTitleText.setCharacterSize(62);
    pausedTitleText.setFillColor(sf::Color(255, 230, 120));
    centerTextOrigin(pausedTitleText);
    pausedTitleText.setPosition({
        static_cast<float>(WINDOW_WIDTH) / 2.f,
        210.f
    });

    std::array<sf::Text, 3> pauseButtonTexts = {
        sf::Text(displayFont),
        sf::Text(displayFont),
        sf::Text(displayFont)
    };

    pauseButtonTexts[0].setString("RESUME");
    pauseButtonTexts[1].setString("MAIN MENU");
    pauseButtonTexts[2].setString("QUIT");

    std::array<sf::FloatRect, 3> pauseButtonBounds = {
        sf::FloatRect({440.f, 310.f}, {400.f, 56.f}),
        sf::FloatRect({440.f, 385.f}, {400.f, 56.f}),
        sf::FloatRect({440.f, 460.f}, {400.f, 56.f})
    };

    for (int i = 0; i < 3; i++) {
        pauseButtonTexts[i].setCharacterSize(34);
        centerTextOrigin(pauseButtonTexts[i]);
        pauseButtonTexts[i].setPosition({
            static_cast<float>(WINDOW_WIDTH) / 2.f,
            338.f + static_cast<float>(i) * 75.f
        });
    }

    sf::ConvexShape exhaust;
    exhaust.setPointCount(3);
    exhaust.setPoint(0, {-12.f, 0.f});
    exhaust.setPoint(1, {0.f, 34.f});
    exhaust.setPoint(2, {12.f, 0.f});
    exhaust.setFillColor(sf::Color(255, 130, 40, 170));

    sf::CircleShape shieldRing(52.f);
    shieldRing.setOrigin({52.f, 52.f});
    shieldRing.setFillColor(sf::Color(60, 180, 255, 35));
    shieldRing.setOutlineThickness(3.f);
    shieldRing.setOutlineColor(sf::Color(100, 220, 255, 190));

    std::vector<Asteroid> asteroids;
    std::vector<Bullet> bullets;
    std::vector<Explosion> explosions;
    std::vector<PowerUp> powerUps;
    std::vector<Fragment> fragments;
    std::vector<Star> stars = createStars(80);

    sf::Clock frameClock;
    sf::Clock spawnClock;
    sf::Clock scoreClock;
    sf::Clock hitClock;
    sf::Clock shootClock;
    sf::Clock powerUpSpawnClock;

    const float playerSpeed = 470.f;

    int score = 0;
    int lives = 3;
    int selectedMenuIndex = 0;
    int selectedPauseIndex = 0;
    int selectedGameOverIndex = 0;

    bool invincible = false;
    bool shieldActive = false;
    bool rapidFireActive = false;
    bool playerVisible = true;
    bool newHighScore = false;

    float shieldTime = 0.f;
    float rapidFireTime = 0.f;
    float shakeTimer = 0.f;
    float survivalTime = 0.f;
    float nextPowerUpDelay = randomFloat(7.f, 12.f);
    float nextAsteroidDelay = randomFloat(0.78f, 1.08f);

    auto startGame = [&]() {
        resetGameplay(
            player,
            shipTextures[selectedShipIndex],
            asteroids,
            bullets,
            explosions,
            powerUps,
            fragments,
            score,
            lives,
            invincible,
            shieldActive,
            rapidFireActive,
            playerVisible,
            newHighScore,
            shieldTime,
            rapidFireTime,
            shakeTimer,
            survivalTime
        );

        spawnClock.restart();
        scoreClock.restart();
        hitClock.restart();
        shootClock.restart();
        powerUpSpawnClock.restart();
        nextPowerUpDelay = randomFloat(7.f, 12.f);
        nextAsteroidDelay = randomFloat(0.78f, 1.08f);

        gameState = GameState::Playing;
    };

    auto goToMainMenu = [&]() {
        gameState = GameState::MainMenu;
        asteroids.clear();
        bullets.clear();
        explosions.clear();
        powerUps.clear();
        fragments.clear();
        playerVisible = true;
        shakeTimer = 0.f;
    };

    auto finishGame = [&]() {
        newHighScore = updateHighScoreIfNeeded(score, highScore);
        selectedGameOverIndex = 0;

        playerVisible = false;

        explosions.push_back(createExplosion(player.getPosition()));

        auto shipFragments = createFragments(
            shipTextures[selectedShipIndex],
            player.getPosition(),
            14,
            0.09f,
            0.19f,
            sf::Color(255, 235, 190, 230),
            130.f,
            430.f,
            0.65f,
            1.15f
        );

        fragments.insert(fragments.end(), shipFragments.begin(), shipFragments.end());

        playSound(deathSound, hasDeathSound);
        shakeTimer = 0.45f;
        gameState = GameState::GameOver;
    };

    auto activateMenuItem = [&](int index) {
        playSound(menuSelectSound, hasMenuSelectSound);

        if (index == 0) {
            startGame();
        } else if (index == 1) {
            gameState = GameState::ShipSelect;
        } else if (index == 2) {
            gameState = GameState::HighScore;
        } else if (index == 3) {
            window.close();
        }
    };

    auto activatePauseItem = [&](int index) {
        playSound(menuSelectSound, hasMenuSelectSound);

        if (index == 0) {
            gameState = GameState::Playing;
            frameClock.restart();
        } else if (index == 1) {
            goToMainMenu();
        } else if (index == 2) {
            window.close();
        }
    };

    auto activateGameOverButton = [&](int index) {
        playSound(menuSelectSound, hasMenuSelectSound);

        if (index == 0) {
            startGame();
        } else {
            goToMainMenu();
        }
    };

    updateMenuMusic();

    while (window.isOpen()) {
        float deltaTime = frameClock.restart().asSeconds();

        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
        sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel, gameView);

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                updateLetterboxView(gameView, resized->size.x, resized->size.y);
                updateLetterboxView(camera, resized->size.x, resized->size.y);
            }

            if (event->is<sf::Event::MouseButtonPressed>()) {
                const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>();

                if (mouse->button == sf::Mouse::Button::Left) {
                    GameState clickedState = gameState;

                    sf::Vector2f clickPosition = window.mapPixelToCoords(
                        mouse->position,
                        gameView
                    );

                    if (containsPoint(soundButtonBounds, clickPosition)) {
                        soundEnabled = !soundEnabled;

                        if (!soundEnabled) {
                            stopAllSounds();
                        } else {
                            playSound(menuSelectSound, hasMenuSelectSound);
                        }

                        continue;
                    }

                    if (clickedState == GameState::MainMenu) {
                        for (int i = 0; i < 4; i++) {
                            if (containsPoint(menuHitBoxes[i], clickPosition)) {
                                selectedMenuIndex = i;
                                activateMenuItem(i);
                                break;
                            }
                        }
                    } else if (clickedState == GameState::ShipSelect) {
                        auto shipCards = getShipCardBounds();

                        bool clickedCard = false;

                        for (int i = 0; i < 3; i++) {
                            if (containsPoint(shipCards[i], clickPosition)) {
                                selectedShipIndex = i;
                                setupPlayer(player, shipTextures[selectedShipIndex]);
                                playSound(menuSelectSound, hasMenuSelectSound);
                                gameState = GameState::MainMenu;
                                clickedCard = true;
                                break;
                            }
                        }

                        if (!clickedCard && containsPoint(shipBackBounds, clickPosition)) {
                            playSound(menuSelectSound, hasMenuSelectSound);
                            gameState = GameState::MainMenu;
                        }
                    } else if (clickedState == GameState::HighScore) {
                        if (containsPoint(backButtonBounds, clickPosition)) {
                            playSound(menuSelectSound, hasMenuSelectSound);
                            gameState = GameState::MainMenu;
                        }
                    } else if (clickedState == GameState::Paused) {
                        for (int i = 0; i < 3; i++) {
                            if (containsPoint(pauseButtonBounds[i], clickPosition)) {
                                selectedPauseIndex = i;
                                activatePauseItem(i);
                                break;
                            }
                        }
                    } else if (clickedState == GameState::GameOver) {
                        for (int i = 0; i < 2; i++) {
                            if (containsPoint(gameOverButtonBounds[i], clickPosition)) {
                                selectedGameOverIndex = i;
                                activateGameOverButton(i);
                                break;
                            }
                        }
                    }
                }
            }

            if (event->is<sf::Event::KeyPressed>()) {
                const auto* key = event->getIf<sf::Event::KeyPressed>();

                if (key->code == sf::Keyboard::Key::S) {
                    soundEnabled = !soundEnabled;

                    if (!soundEnabled) {
                        stopAllSounds();
                    } else {
                        playSound(menuSelectSound, hasMenuSelectSound);
                    }
                }

                if (gameState == GameState::MainMenu) {
                    if (key->code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }

                    if (key->code == sf::Keyboard::Key::Up) {
                        selectedMenuIndex--;

                        if (selectedMenuIndex < 0) {
                            selectedMenuIndex = 3;
                        }

                        playSound(menuSelectSound, hasMenuSelectSound);
                    }

                    if (key->code == sf::Keyboard::Key::Down) {
                        selectedMenuIndex++;

                        if (selectedMenuIndex > 3) {
                            selectedMenuIndex = 0;
                        }

                        playSound(menuSelectSound, hasMenuSelectSound);
                    }

                    if (key->code == sf::Keyboard::Key::Enter) {
                        activateMenuItem(selectedMenuIndex);
                    }
                } else if (gameState == GameState::ShipSelect) {
                    if (key->code == sf::Keyboard::Key::Escape ||
                        key->code == sf::Keyboard::Key::Backspace) {
                        setupPlayer(player, shipTextures[selectedShipIndex]);
                        playSound(menuSelectSound, hasMenuSelectSound);
                        gameState = GameState::MainMenu;
                    }

                    if (key->code == sf::Keyboard::Key::Left) {
                        selectedShipIndex--;

                        if (selectedShipIndex < 0) {
                            selectedShipIndex = 2;
                        }

                        playSound(menuSelectSound, hasMenuSelectSound);
                    }

                    if (key->code == sf::Keyboard::Key::Right) {
                        selectedShipIndex++;

                        if (selectedShipIndex > 2) {
                            selectedShipIndex = 0;
                        }

                        playSound(menuSelectSound, hasMenuSelectSound);
                    }

                    if (key->code == sf::Keyboard::Key::Enter) {
                        setupPlayer(player, shipTextures[selectedShipIndex]);
                        playSound(menuSelectSound, hasMenuSelectSound);
                        gameState = GameState::MainMenu;
                    }
                } else if (gameState == GameState::HighScore) {
                    if (key->code == sf::Keyboard::Key::Escape ||
                        key->code == sf::Keyboard::Key::Backspace ||
                        key->code == sf::Keyboard::Key::Enter) {
                        playSound(menuSelectSound, hasMenuSelectSound);
                        gameState = GameState::MainMenu;
                    }
                } else if (gameState == GameState::Playing) {
                    if (key->code == sf::Keyboard::Key::Escape) {
                        selectedPauseIndex = 0;
                        playSound(menuSelectSound, hasMenuSelectSound);
                        gameState = GameState::Paused;
                    }
                } else if (gameState == GameState::Paused) {
                    if (key->code == sf::Keyboard::Key::Escape ||
                        key->code == sf::Keyboard::Key::Backspace) {
                        playSound(menuSelectSound, hasMenuSelectSound);
                        gameState = GameState::Playing;
                        frameClock.restart();
                    }

                    if (key->code == sf::Keyboard::Key::Up) {
                        selectedPauseIndex--;

                        if (selectedPauseIndex < 0) {
                            selectedPauseIndex = 2;
                        }

                        playSound(menuSelectSound, hasMenuSelectSound);
                    }

                    if (key->code == sf::Keyboard::Key::Down) {
                        selectedPauseIndex++;

                        if (selectedPauseIndex > 2) {
                            selectedPauseIndex = 0;
                        }

                        playSound(menuSelectSound, hasMenuSelectSound);
                    }

                    if (key->code == sf::Keyboard::Key::Enter) {
                        activatePauseItem(selectedPauseIndex);
                    }
                } else if (gameState == GameState::GameOver) {
                    if (key->code == sf::Keyboard::Key::Left ||
                        key->code == sf::Keyboard::Key::Right) {
                        selectedGameOverIndex = 1 - selectedGameOverIndex;
                        playSound(menuSelectSound, hasMenuSelectSound);
                    }

                    if (key->code == sf::Keyboard::Key::Enter) {
                        activateGameOverButton(selectedGameOverIndex);
                    }

                    if (key->code == sf::Keyboard::Key::R) {
                        playSound(menuSelectSound, hasMenuSelectSound);
                        startGame();
                    }

                    if (key->code == sf::Keyboard::Key::M ||
                        key->code == sf::Keyboard::Key::Escape) {
                        playSound(menuSelectSound, hasMenuSelectSound);
                        goToMainMenu();
                    }
                }
            }
        }

        updateMenuMusic();

        if (gameState != GameState::Paused) {
            updateStars(stars, deltaTime);
        }

        if (gameState == GameState::MainMenu) {
            for (int i = 0; i < 4; i++) {
                if (containsPoint(menuHitBoxes[i], mouseWorld)) {
                    selectedMenuIndex = i;
                }
            }
        }

        if (gameState == GameState::ShipSelect) {
            auto shipCards = getShipCardBounds();

            for (int i = 0; i < 3; i++) {
                if (containsPoint(shipCards[i], mouseWorld)) {
                    selectedShipIndex = i;
                }
            }
        }

        if (gameState == GameState::HighScore) {
            if (containsPoint(backButtonBounds, mouseWorld)) {
                styleTextButton(backButtonText, true);
            } else {
                styleTextButton(backButtonText, false);
            }
        }

        if (gameState == GameState::Paused) {
            for (int i = 0; i < 3; i++) {
                if (containsPoint(pauseButtonBounds[i], mouseWorld)) {
                    selectedPauseIndex = i;
                }
            }
        }

        if (gameState == GameState::GameOver) {
            for (int i = 0; i < 2; i++) {
                if (containsPoint(gameOverButtonBounds[i], mouseWorld)) {
                    selectedGameOverIndex = i;
                }
            }
        }

        if (gameState == GameState::Playing) {
            survivalTime += deltaTime;

            float baseDifficulty = std::clamp((survivalTime - 8.f) / 95.f, 0.f, 1.f);

            float difficultyCap = 1.f;

            if (survivalTime < 15.f) {
                difficultyCap = 0.18f;
            } else if (survivalTime < 35.f) {
                difficultyCap = 0.42f;
            } else if (survivalTime < 65.f) {
                difficultyCap = 0.70f;
            }

            float currentDifficulty = std::clamp(
                baseDifficulty + randomFloat(-0.10f, 0.18f),
                0.f,
                difficultyCap
            );

            if (shieldActive) {
                shieldTime -= deltaTime;

                if (shieldTime <= 0.f) {
                    shieldActive = false;
                    shieldTime = 0.f;
                }
            }

            if (rapidFireActive) {
                rapidFireTime -= deltaTime;

                if (rapidFireTime <= 0.f) {
                    rapidFireActive = false;
                    rapidFireTime = 0.f;
                }
            }

            sf::Vector2f position = player.getPosition();

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                position.x -= playerSpeed * deltaTime;
                player.setRotation(sf::degrees(-8.f));
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                position.x += playerSpeed * deltaTime;
                player.setRotation(sf::degrees(8.f));
            } else {
                player.setRotation(sf::degrees(0.f));
            }

            position.x = std::clamp(
                position.x,
                35.f,
                static_cast<float>(WINDOW_WIDTH) - 35.f
            );

            player.setPosition(position);

            float shootDelay = rapidFireActive ? 0.07f : 0.15f;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) &&
                shootClock.getElapsedTime().asSeconds() > shootDelay) {
                bullets.push_back(createBullet(laserTexture, player));
                playSound(shootSound, hasShootSound);
                shootClock.restart();
            }

            if (spawnClock.getElapsedTime().asSeconds() > nextAsteroidDelay) {
                asteroids.push_back(createAsteroid(
                    asteroidTexture,
                    shipTextures,
                    currentDifficulty
                ));

                float baseDelay = lerp(0.86f, 0.26f, currentDifficulty);
                nextAsteroidDelay = baseDelay * randomFloat(0.72f, 1.38f);
                nextAsteroidDelay = std::clamp(nextAsteroidDelay, 0.22f, 1.15f);

                spawnClock.restart();
            }

            if (powerUpSpawnClock.getElapsedTime().asSeconds() > nextPowerUpDelay) {
                powerUps.push_back(createPowerUp(
                    shieldPowerUpTexture,
                    rapidPowerUpTexture
                ));

                powerUpSpawnClock.restart();
                nextPowerUpDelay = randomFloat(8.f, 14.f);
            }

            if (invincible) {
                float elapsed = hitClock.getElapsedTime().asSeconds();

                if (elapsed > 0.15f) {
                    player.setColor(PLAYER_NORMAL);
                }

                if (elapsed > 1.0f) {
                    invincible = false;
                }
            }

            for (auto& bullet : bullets) {
                bullet.sprite.move({0.f, -bullet.speed * deltaTime});
            }

            for (auto& powerUp : powerUps) {
                powerUp.sprite.move({0.f, powerUp.speed * deltaTime});
                powerUp.sprite.rotate(sf::degrees(90.f * deltaTime));

                if (powerUp.sprite.getGlobalBounds().findIntersection(player.getGlobalBounds())) {
                    if (powerUp.type == PowerUpType::Shield) {
                        shieldActive = true;
                        shieldTime = 6.f;
                    } else {
                        rapidFireActive = true;
                        rapidFireTime = 6.f;
                    }

                    explosions.push_back(createExplosion(powerUp.sprite.getPosition()));
                    playSound(powerUpSound, hasPowerUpSound);

                    powerUp.sprite.setPosition({
                        powerUp.sprite.getPosition().x,
                        static_cast<float>(WINDOW_HEIGHT + 200)
                    });
                }
            }

            for (auto& asteroid : asteroids) {
                asteroid.sprite.move({0.f, asteroid.speed * deltaTime});
                asteroid.sprite.rotate(sf::degrees(asteroid.rotationSpeed * deltaTime));

                if (!invincible &&
                    asteroid.sprite.getGlobalBounds().findIntersection(player.getGlobalBounds())) {
                    explosions.push_back(createExplosion(asteroid.sprite.getPosition()));

                    const sf::Texture& fragmentTexture =
                        asteroid.type == ObstacleType::ShipDebris
                            ? shipTextures[selectedShipIndex]
                            : asteroidTexture;

                    sf::Color fragmentColor =
                        asteroid.type == ObstacleType::RareAsteroid
                            ? sf::Color(255, 150, 90, 230)
                            : asteroid.type == ObstacleType::ShipDebris
                                ? sf::Color(170, 180, 195, 220)
                                : sf::Color(210, 210, 215, 220);

                    auto asteroidFragments = createFragments(
                        fragmentTexture,
                        asteroid.sprite.getPosition(),
                        asteroid.type == ObstacleType::ShipDebris ? 6 : 8,
                        asteroid.type == ObstacleType::ShipDebris ? 0.06f : 0.13f,
                        asteroid.type == ObstacleType::ShipDebris ? 0.14f : 0.27f,
                        fragmentColor,
                        90.f,
                        asteroid.type == ObstacleType::RareAsteroid ? 390.f : 310.f,
                        0.45f,
                        0.95f
                    );

                    fragments.insert(
                        fragments.end(),
                        asteroidFragments.begin(),
                        asteroidFragments.end()
                    );

                    if (lives > 1 || shieldActive) {
                        playSound(hitSound, hasHitSound);
                    }

                    asteroid.sprite.setPosition({
                        asteroid.sprite.getPosition().x,
                        static_cast<float>(WINDOW_HEIGHT + 140)
                    });

                    shakeTimer = 0.28f;

                    if (shieldActive) {
                        shieldActive = false;
                        shieldTime = 0.f;
                    } else {
                        lives--;
                        invincible = true;
                        hitClock.restart();
                        player.setColor(PLAYER_HIT_RED);

                        if (lives <= 0) {
                            finishGame();
                        }
                    }
                }
            }

            for (auto& bullet : bullets) {
                for (auto& asteroid : asteroids) {
                    if (bullet.sprite.getGlobalBounds().findIntersection(
                            asteroid.sprite.getGlobalBounds()
                        )) {
                        explosions.push_back(createExplosion(asteroid.sprite.getPosition()));

                        const sf::Texture& fragmentTexture =
                            asteroid.type == ObstacleType::ShipDebris
                                ? shipTextures[selectedShipIndex]
                                : asteroidTexture;

                        sf::Color fragmentColor =
                            asteroid.type == ObstacleType::RareAsteroid
                                ? sf::Color(255, 150, 90, 235)
                                : asteroid.type == ObstacleType::ShipDebris
                                    ? sf::Color(175, 185, 200, 225)
                                    : sf::Color(220, 220, 225, 230);

                        auto asteroidFragments = createFragments(
                            fragmentTexture,
                            asteroid.sprite.getPosition(),
                            asteroid.type == ObstacleType::ShipDebris ? 7 : 9,
                            asteroid.type == ObstacleType::ShipDebris ? 0.06f : 0.12f,
                            asteroid.type == ObstacleType::ShipDebris ? 0.15f : 0.26f,
                            fragmentColor,
                            130.f,
                            asteroid.type == ObstacleType::RareAsteroid ? 460.f : 390.f,
                            0.5f,
                            1.05f
                        );

                        fragments.insert(
                            fragments.end(),
                            asteroidFragments.begin(),
                            asteroidFragments.end()
                        );

                        playSound(explosionSound, hasExplosionSound);

                        bullet.sprite.setPosition({
                            bullet.sprite.getPosition().x,
                            -100.f
                        });

                        asteroid.sprite.setPosition({
                            asteroid.sprite.getPosition().x,
                            static_cast<float>(WINDOW_HEIGHT + 140)
                        });

                        score += 5;
                        shakeTimer = 0.08f;
                    }
                }
            }

            if (scoreClock.getElapsedTime().asSeconds() >= 1.f) {
                score++;
                scoreClock.restart();
            }
        }

        if (gameState != GameState::Paused) {
            for (auto& explosion : explosions) {
                explosion.lifetime += deltaTime;

                float t = explosion.lifetime / explosion.maxLifetime;
                float radius = 8.f + t * 42.f;

                explosion.shape.setRadius(radius);
                explosion.shape.setOrigin({radius, radius});

                std::uint8_t alpha = static_cast<std::uint8_t>(
                    std::max(0.f, 220.f * (1.f - t))
                );

                explosion.shape.setFillColor(sf::Color(255, 150, 40, alpha));
                explosion.shape.setOutlineColor(sf::Color(255, 230, 120, alpha));
            }

            for (auto& fragment : fragments) {
                fragment.lifetime += deltaTime;

                fragment.sprite.move(fragment.velocity * deltaTime);
                fragment.sprite.rotate(sf::degrees(fragment.rotationSpeed * deltaTime));

                fragment.velocity.x *= 0.985f;
                fragment.velocity.y *= 0.985f;

                float t = fragment.lifetime / fragment.maxLifetime;
                std::uint8_t alpha = static_cast<std::uint8_t>(
                    std::max(0.f, 255.f * (1.f - t))
                );

                sf::Color color = fragment.sprite.getColor();
                color.a = alpha;
                fragment.sprite.setColor(color);
            }
        }

        asteroids.erase(
            std::remove_if(
                asteroids.begin(),
                asteroids.end(),
                [](const Asteroid& asteroid) {
                    return asteroid.sprite.getPosition().y >
                           static_cast<float>(WINDOW_HEIGHT) + 120.f;
                }
            ),
            asteroids.end()
        );

        bullets.erase(
            std::remove_if(
                bullets.begin(),
                bullets.end(),
                [](const Bullet& bullet) {
                    return bullet.sprite.getPosition().y < -100.f;
                }
            ),
            bullets.end()
        );

        explosions.erase(
            std::remove_if(
                explosions.begin(),
                explosions.end(),
                [](const Explosion& explosion) {
                    return explosion.lifetime >= explosion.maxLifetime;
                }
            ),
            explosions.end()
        );

        fragments.erase(
            std::remove_if(
                fragments.begin(),
                fragments.end(),
                [](const Fragment& fragment) {
                    return fragment.lifetime >= fragment.maxLifetime;
                }
            ),
            fragments.end()
        );

        powerUps.erase(
            std::remove_if(
                powerUps.begin(),
                powerUps.end(),
                [](const PowerUp& powerUp) {
                    return powerUp.sprite.getPosition().y >
                           static_cast<float>(WINDOW_HEIGHT) + 80.f;
                }
            ),
            powerUps.end()
        );

        camera = gameView;

        if (shakeTimer > 0.f &&
            (gameState == GameState::Playing || gameState == GameState::GameOver)) {
            shakeTimer -= deltaTime;

            float shakeAmount = 7.f;
            camera.move({
                randomFloat(-shakeAmount, shakeAmount),
                randomFloat(-shakeAmount, shakeAmount)
            });
        }

        window.setView(camera);
        drawBackground(window, backgroundSprite, stars);

        if (gameState == GameState::MainMenu) {
            window.setView(gameView);

            drawSpriteGlow(window, logoSprite, sf::Color(80, 180, 255, 50), 1.05f);
            window.draw(logoSprite);

            for (int i = 0; i < 4; i++) {
                styleTextButton(*menuTexts[i], i == selectedMenuIndex);
                drawTextButtonDecoration(window, *menuTexts[i], i == selectedMenuIndex);
                drawTextWithShadow(window, *menuTexts[i], {2.f, 2.f});
            }
        }

        if (gameState == GameState::ShipSelect) {
            window.setView(gameView);
            drawTextWithShadow(window, shipSelectTitle, {3.f, 3.f});

            std::array<sf::Vector2f, 3> positions = {
                sf::Vector2f{340.f, 340.f},
                sf::Vector2f{640.f, 340.f},
                sf::Vector2f{940.f, 340.f}
            };

            for (int i = 0; i < 3; i++) {
                sf::RectangleShape card({220.f, 260.f});
                card.setOrigin({110.f, 130.f});
                card.setPosition(positions[i]);
                card.setFillColor(sf::Color(5, 10, 25, 95));

                if (i == selectedShipIndex) {
                    card.setOutlineThickness(3.f);
                    card.setOutlineColor(sf::Color(255, 230, 120, 190));
                } else {
                    card.setOutlineThickness(1.f);
                    card.setOutlineColor(sf::Color(150, 170, 230, 85));
                }

                window.draw(card);

                sf::Sprite preview(shipTextures[i]);
                centerSpriteOrigin(preview);
                preview.setScale({0.55f, 0.55f});
                preview.setPosition(positions[i]);

                drawSpriteGlow(
                    window,
                    preview,
                    sf::Color(80, 180, 255, i == selectedShipIndex ? 90 : 35),
                    i == selectedShipIndex ? 1.25f : 1.12f
                );

                window.draw(preview);

                sf::Text label(displayFont);
                label.setString("SHIP " + std::to_string(i + 1));
                label.setCharacterSize(22);
                label.setFillColor(i == selectedShipIndex
                    ? sf::Color(255, 230, 120)
                    : sf::Color(220, 230, 250)
                );

                centerTextOrigin(label);
                label.setPosition({
                    positions[i].x,
                    positions[i].y + 105.f
                });

                drawTextWithShadow(window, label, {1.5f, 1.5f});
            }

            bool backSelected = containsPoint(shipBackBounds, mouseWorld);
            styleTextButton(shipBackText, backSelected);
            drawTextButtonDecoration(window, shipBackText, backSelected);
            drawTextWithShadow(window, shipBackText, {2.f, 2.f});
        }

        if (gameState == GameState::HighScore) {
            window.setView(gameView);

            highScoreValueText.setString("Best Score: " + std::to_string(highScore));
            centerTextOrigin(highScoreValueText);
            highScoreValueText.setPosition({
                static_cast<float>(WINDOW_WIDTH) / 2.f,
                300.f
            });

            drawTextWithShadow(window, highScoreTitleText, {3.f, 3.f});
            drawTextWithShadow(window, highScoreValueText, {2.f, 2.f});

            bool backSelected = containsPoint(backButtonBounds, mouseWorld);
            styleTextButton(backButtonText, backSelected);
            drawTextButtonDecoration(window, backButtonText, backSelected);
            drawTextWithShadow(window, backButtonText, {2.f, 2.f});
        }

        if (gameState == GameState::Playing ||
            gameState == GameState::Paused ||
            gameState == GameState::GameOver) {
            if (gameState == GameState::Playing && playerVisible) {
                exhaust.setPosition({
                    player.getPosition().x,
                    player.getPosition().y + 25.f
                });

                window.draw(exhaust);
            }

            if (shieldActive && playerVisible) {
                shieldRing.setPosition(player.getPosition());
                window.draw(shieldRing);
            }

            if (playerVisible) {
                drawSpriteGlow(window, player, sf::Color(80, 180, 255, 45), 1.18f);
                window.draw(player);
            }

            for (const auto& bullet : bullets) {
                drawSpriteGlow(window, bullet.sprite, sf::Color(255, 220, 90, 85), 1.8f);
                window.draw(bullet.sprite);
            }

            for (const auto& powerUp : powerUps) {
                sf::Color glowColor = powerUp.type == PowerUpType::Shield
                    ? sf::Color(80, 190, 255, 85)
                    : sf::Color(255, 230, 90, 85);

                drawSpriteGlow(window, powerUp.sprite, glowColor, 1.65f);
                window.draw(powerUp.sprite);
            }

            for (const auto& fragment : fragments) {
                window.draw(fragment.sprite);
            }

            for (const auto& explosion : explosions) {
                window.draw(explosion.shape);
            }

            for (const auto& asteroid : asteroids) {
                if (asteroid.type == ObstacleType::RareAsteroid) {
                    drawSpriteGlow(window, asteroid.sprite, sf::Color(255, 90, 50, 45), 1.16f);
                } else if (asteroid.type == ObstacleType::ShipDebris) {
                    drawSpriteGlow(window, asteroid.sprite, sf::Color(120, 180, 255, 35), 1.18f);
                }

                window.draw(asteroid.sprite);
            }

            window.setView(gameView);

            std::string effectsText;

            if (shieldActive) {
                effectsText += "    Shield: " + std::to_string(static_cast<int>(shieldTime + 1.f)) + "s";
            }

            if (rapidFireActive) {
                effectsText += "    Rapid: " + std::to_string(static_cast<int>(rapidFireTime + 1.f)) + "s";
            }

            hudText.setString(
                "Score: " + std::to_string(score) +
                "    High: " + std::to_string(highScore) +
                "    Lives: " + std::to_string(lives) +
                effectsText
            );

            drawTextWithShadow(window, hudText, {2.f, 2.f});

            if (gameState == GameState::Paused) {
                sf::RectangleShape overlay({
                    static_cast<float>(WINDOW_WIDTH),
                    static_cast<float>(WINDOW_HEIGHT)
                });

                overlay.setFillColor(sf::Color(0, 0, 0, 140));
                overlay.setPosition({0.f, 0.f});
                window.draw(overlay);

                drawTextWithShadow(window, pausedTitleText, {3.f, 3.f});

                for (int i = 0; i < 3; i++) {
                    styleTextButton(pauseButtonTexts[i], i == selectedPauseIndex);
                    drawTextButtonDecoration(window, pauseButtonTexts[i], i == selectedPauseIndex);
                    drawTextWithShadow(window, pauseButtonTexts[i], {2.f, 2.f});
                }
            }

            if (gameState == GameState::GameOver) {
                finalScoreText.setString(
                    "Score: " + std::to_string(score) +
                    "    High Score: " + std::to_string(highScore)
                );
                centerTextOrigin(finalScoreText);
                finalScoreText.setPosition({
                    static_cast<float>(WINDOW_WIDTH) / 2.f,
                    static_cast<float>(WINDOW_HEIGHT) / 2.f - 35.f
                });

                sf::RectangleShape overlay({
                    static_cast<float>(WINDOW_WIDTH),
                    static_cast<float>(WINDOW_HEIGHT)
                });
                overlay.setFillColor(sf::Color(0, 0, 0, 105));
                overlay.setPosition({0.f, 0.f});
                window.draw(overlay);

                drawTextWithShadow(window, gameOverText, {3.f, 3.f});

                if (newHighScore) {
                    drawTextWithShadow(window, newHighScoreText, {2.f, 2.f});
                }

                drawTextWithShadow(window, finalScoreText, {2.f, 2.f});

                for (int i = 0; i < 2; i++) {
                    styleTextButton(
                        gameOverButtonTexts[i],
                        i == selectedGameOverIndex
                    );

                    drawTextButtonDecoration(
                        window,
                        gameOverButtonTexts[i],
                        i == selectedGameOverIndex
                    );

                    drawTextWithShadow(window, gameOverButtonTexts[i], {2.f, 2.f});
                }
            }
        }

        window.setView(gameView);

        bool soundHover = containsPoint(soundButtonBounds, mouseWorld);

        sf::RectangleShape soundButtonBackground(soundButtonBounds.size);
        soundButtonBackground.setPosition(soundButtonBounds.position);
        soundButtonBackground.setFillColor(soundHover
            ? sf::Color(10, 18, 40, 150)
            : sf::Color(5, 10, 25, 100)
        );
        soundButtonBackground.setOutlineThickness(1.f);
        soundButtonBackground.setOutlineColor(soundEnabled
            ? sf::Color(120, 220, 255, 150)
            : sf::Color(255, 100, 100, 150)
        );

        window.draw(soundButtonBackground);

        soundButtonText.setString(soundEnabled ? "SOUND: ON" : "SOUND: OFF");
        soundButtonText.setFillColor(soundEnabled
            ? sf::Color(210, 245, 255)
            : sf::Color(255, 145, 145)
        );
        centerTextOrigin(soundButtonText);
        soundButtonText.setPosition({
            soundButtonBounds.position.x + soundButtonBounds.size.x / 2.f,
            soundButtonBounds.position.y + soundButtonBounds.size.y / 2.f
        });

        drawTextWithShadow(window, soundButtonText, {1.5f, 1.5f});

        window.display();
        window.setTitle("Astro Drift");
    }

    return 0;
}