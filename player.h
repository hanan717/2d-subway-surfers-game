#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>

extern const float laneX[3];
extern const float playerBaseY;

class Player {
private:
    sf::Sprite sprite;
    sf::Sprite shieldSprite;
    int lane = 1;
    bool leftLock = false, rightLock = false, jumpLock = false, slideLock = false;
    float vy = 0;
    bool sliding = false;
    bool isJumping = false;
    float jumpHeight = 0.f;
    float maxJumpHeight = 150.f; // Maximum jump height
    float shieldTimeRemaining = 0.f;
    float coinDoubleTimeRemaining = 0.f;
    float jetpackTimeRemaining = 0.f;
    bool shieldActive = false;
    bool coinDoubleActive = false;
    bool jetpackActive = false;
    sf::FloatRect collisionBox;

public:
    void init(sf::Texture &playerTex, sf::Texture &shieldTex);
    void handleInput(float dt);
    void update(float dt);
    void draw(sf::RenderWindow &win);
    sf::FloatRect getHitbox() const;
    int getLane() const;
    bool getIsJumping() const;
    bool getSliding() const;
    float getJumpHeight() const; // New method to get current jump height
    
    // Power-up methods
    void activateShield(float duration);
    void activateCoinDouble(float duration);
    void activateJetpack(float duration);
    void useShield();
    void deactivateShield();
    void deactivateCoinDouble();
    void deactivateJetpack();
    bool isShieldActive() const;
    bool isCoinDoubleActive() const;
    bool isJetpackActive() const;
    bool isFlying() const; // ADDED: Check if jetpack is active
    float getShieldTimeRemaining() const;
    float getCoinDoubleTimeRemaining() const;
    float getJetpackTimeRemaining() const;
    int getCoinMultiplier() const;
    
    void updateHitbox();
};

#endif