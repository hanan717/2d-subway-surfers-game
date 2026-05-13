#include "player.h"
#include <iostream>

void Player::init(sf::Texture &playerTex, sf::Texture &shieldTex) {
    sprite.setTexture(playerTex);
    sprite.setScale(0.1f, 0.1f);
    sprite.setPosition(laneX[lane], playerBaseY);
    
    shieldSprite.setTexture(shieldTex);
    shieldSprite.setScale(0.3f, 0.3f);
    shieldSprite.setOrigin(shieldTex.getSize().x / 2.f, shieldTex.getSize().y / 2.f);
    
    updateHitbox();
}
// Add this method at the end of the file, before the last closing brace


void Player::handleInput(float dt) {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && !leftLock) {
        if (lane > 0) lane--;
        sprite.setPosition(laneX[lane], sprite.getPosition().y);
        leftLock = true;
    }
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::A)) leftLock = false;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && !rightLock) {
        if (lane < 2) lane++;
        sprite.setPosition(laneX[lane], sprite.getPosition().y);
        rightLock = true;
    }
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::D)) rightLock = false;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && !jumpLock) {
        if (!sliding && !isJumping) {
            if (jetpackActive) {
                vy = -800.f; // Higher jump with jetpack
            } else {
                vy = -600.f;
            }
            isJumping = true;
            jumpHeight = 0.f;
        }
        jumpLock = true;
    }
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::W)) jumpLock = false;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && !slideLock) {
        if (!isJumping) {
            sliding = true;
        }
        slideLock = true;
    }
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        sliding = false;
        slideLock = false;
    }
}

void Player::update(float dt) {
    sprite.move(0, vy * dt);
    jumpHeight -= vy * dt; // Track jump height (negative vy means going up)
    
    vy += 1400 * dt;
    
    if (sprite.getPosition().y >= playerBaseY) {
        sprite.setPosition(sprite.getPosition().x, playerBaseY);
        vy = 0;
        isJumping = false;
        jumpHeight = 0.f;
    }

    // Update shield position
    if (shieldActive) {
        shieldSprite.setPosition(sprite.getPosition().x + sprite.getGlobalBounds().width / 2,
                                sprite.getPosition().y + sprite.getGlobalBounds().height / 2);
        shieldTimeRemaining -= dt;
        if (shieldTimeRemaining <= 0.f) {
            shieldActive = false;
        }
    }
    
    // Update coin double timer
    if (coinDoubleActive) {
        coinDoubleTimeRemaining -= dt;
        if (coinDoubleTimeRemaining <= 0.f) {
            coinDoubleActive = false;
        }
    }
    
    // Update jetpack timer
    if (jetpackActive) {
        jetpackTimeRemaining -= dt;
        if (jetpackTimeRemaining <= 0.f) {
            jetpackActive = false;
        }
    }
}

void Player::draw(sf::RenderWindow &win) {
    win.draw(sprite);
    if (shieldActive) {
        win.draw(shieldSprite);
    }
}

sf::FloatRect Player::getHitbox() const {
    sf::FloatRect box = sprite.getGlobalBounds();
    box.left += 12.f;
    box.top += 6.f;
    box.width *= 0.6f;
    
    if (sliding) {
        box.height = sprite.getGlobalBounds().height * 0.5f;
    } else {
        box.height *= 0.9f;
    }
    
    // Adjust hitbox for jumping - smaller hitbox when jumping
    if (isJumping) {
        box.height *= 0.7f;
        // Move hitbox up slightly when jumping to represent being higher
        box.top -= 10.f;
    }
    
    return box;
}

int Player::getLane() const { 
    return lane; 
}

bool Player::getIsJumping() const { 
    return isJumping; 
}

bool Player::getSliding() const { 
    return sliding; 
}

float Player::getJumpHeight() const {
    return jumpHeight;
}

void Player::activateShield(float duration) {
    shieldActive = true;
    shieldTimeRemaining = duration;
}

void Player::activateCoinDouble(float duration) {
    coinDoubleActive = true;
    coinDoubleTimeRemaining = duration;
}

void Player::activateJetpack(float duration) {
    jetpackActive = true;
    jetpackTimeRemaining = duration;
}

void Player::useShield() {
    shieldActive = false;
}

void Player::deactivateShield() {
    shieldActive = false;
    shieldTimeRemaining = 0.f;
}

void Player::deactivateCoinDouble() {
    coinDoubleActive = false;
    coinDoubleTimeRemaining = 0.f;
}

void Player::deactivateJetpack() {
    jetpackActive = false;
    jetpackTimeRemaining = 0.f;
}

bool Player::isShieldActive() const {
    return shieldActive;
}

bool Player::isCoinDoubleActive() const {
    return coinDoubleActive;
}

bool Player::isJetpackActive() const {
    return jetpackActive;
}

float Player::getShieldTimeRemaining() const {
    return shieldTimeRemaining;
}

float Player::getCoinDoubleTimeRemaining() const {
    return coinDoubleTimeRemaining;
}

float Player::getJetpackTimeRemaining() const {
    return jetpackTimeRemaining;
}

int Player::getCoinMultiplier() const {
    return coinDoubleActive ? 2 : 1;
}

void Player::updateHitbox() {
    collisionBox = sprite.getGlobalBounds();
    collisionBox.width *= 0.2f;
    collisionBox.height *= 0.2f;
}

bool Player::isFlying() const {
    return jetpackActive;
}
