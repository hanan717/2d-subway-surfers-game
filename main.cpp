#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>
#include <sstream>
#include <cmath>

#include "player.h"

using namespace std;
using namespace sf;

const int WINDOW_WIDTH = 900;
const int WINDOW_HEIGHT = 480;

const float laneX[3] = {333.f, 463.f, 587.f};
const float playerBaseY = 410.f;

// Minimum vertical gap between obstacles
const float MIN_OBSTACLE_GAP = 150.f;
const float MIN_POWERUP_GAP = 80.f;

// Structure for leaderboard entries
struct LeaderboardEntry {
    string name;
    int score;
};

// Global leaderboard array (max 10 entries)
LeaderboardEntry leaderboard[10];
int leaderboardSize = 0;

// Helper function to convert integer to string
string intToString(int num) {
    stringstream ss;
    ss << num;
    return ss.str();
}

// Function to read leaderboard from file
void readLeaderboard() {
    ifstream file("leaderboard.txt");
    leaderboardSize = 0;
    
    if (file.is_open()) {
        string line;
        while (getline(file, line) && leaderboardSize < 10) {
            size_t commaPos = line.find(',');
            if (commaPos != string::npos) {
                leaderboard[leaderboardSize].name = line.substr(0, commaPos);
                
                string scoreStr = line.substr(commaPos + 1);
                int score = 0;
                for (char c : scoreStr) {
                    if (c >= '0' && c <= '9') {
                        score = score * 10 + (c - '0');
                    }
                }
                leaderboard[leaderboardSize].score = score;
                leaderboardSize++;
            }
        }
        file.close();
    }
    
    // Bubble sort for leaderboard
    for (int i = 0; i < leaderboardSize - 1; i++) {
        for (int j = 0; j < leaderboardSize - i - 1; j++) {
            if (leaderboard[j].score < leaderboard[j + 1].score) {
                LeaderboardEntry temp = leaderboard[j];
                leaderboard[j] = leaderboard[j + 1];
                leaderboard[j + 1] = temp;
            }
        }
    }
}

// Function to write leaderboard to file
void writeLeaderboard() {
    ofstream file("leaderboard.txt");
    
    if (file.is_open()) {
        for (int i = 0; i < leaderboardSize; i++) {
            file << leaderboard[i].name << "," << leaderboard[i].score << endl;
        }
        file.close();
    }
}

// Function to add a score to the leaderboard
void addToLeaderboard(const string& playerName, int score) {
    readLeaderboard();
    
    LeaderboardEntry newEntry;
    newEntry.name = playerName;
    newEntry.score = score;
    
    int insertPos = leaderboardSize;
    for (int i = 0; i < leaderboardSize; i++) {
        if (score > leaderboard[i].score) {
            insertPos = i;
            break;
        }
    }
    
    if (leaderboardSize < 10) leaderboardSize++;
    
    for (int i = leaderboardSize - 1; i > insertPos; i--) {
        leaderboard[i] = leaderboard[i - 1];
    }
    
    if (insertPos < 10) {
        leaderboard[insertPos] = newEntry;
    }
    
    if (leaderboardSize > 10) {
        leaderboardSize = 10;
    }
    
    writeLeaderboard();
}

// Template-based GameList class for storing game objects
template <typename T>
class GameList {
private:
    static const int MAX_CAPACITY = 50;
    T items[MAX_CAPACITY];
    int count;
    
public:
    GameList() : count(0) {}
    
    void add(const T& item) {
        if (count < MAX_CAPACITY) {
            items[count++] = item;
        }
    }
    
    T& get(int index) {
        return items[index];
    }
    
    const T& get(int index) const {
        return items[index];
    }
    
    int size() const {
        return count;
    }
    
    void remove(int index) {
        if (index >= 0 && index < count) {
            for (int i = index; i < count - 1; i++) {
                items[i] = items[i + 1];
            }
            count--;
        }
    }
    
    void clear() {
        count = 0;
    }
    
    // Find first item that satisfies condition
    template <typename Predicate>
    T* findFirst(Predicate pred) {
        for (int i = 0; i < count; i++) {
            if (pred(items[i])) {
                return &items[i];
            }
        }
        return nullptr;
    }
    
    // Apply function to all items
    template <typename Function>
    void forEach(Function func) {
        for (int i = 0; i < count; i++) {
            func(items[i]);
        }
    }
    
    // Check if any item satisfies condition
    template <typename Predicate>
    bool any(Predicate pred) {
        for (int i = 0; i < count; i++) {
            if (pred(items[i])) {
                return true;
            }
        }
        return false;
    }
};

// Abstract Obstacle class
class Obstacle {
protected:
    bool active;
    int lane;
    Sprite sprite;
    bool canJumpOver;
    bool canSlideUnder;
    float yPosition;
    
public:
    Obstacle() : active(false), lane(0), canJumpOver(false), canSlideUnder(false), yPosition(0.f) {}
    virtual ~Obstacle() {}
    
    virtual void init(Texture &tex, bool jumpable, bool slidable) {
        sprite.setTexture(tex);
        canJumpOver = jumpable;
        canSlideUnder = slidable;
    }
    
    virtual void spawn() = 0;
    virtual void spawn(int preferredLane) = 0;
    
    virtual void update(float dt, float speed) {
        if (!active) return;
        sprite.move(0, speed * dt);
        yPosition = sprite.getPosition().y;
        if (sprite.getPosition().y > WINDOW_HEIGHT + 80) active = false;
    }
    
    virtual void draw(RenderWindow &win) {
        if (active) win.draw(sprite);
    }
    
    bool isActive() const { return active; }
    
    FloatRect getHitbox() {
        return sprite.getGlobalBounds();
    }
    
    float getYPosition() const { return yPosition; }
    int getLane() const { return lane; }
    
    bool canBeJumpedOver() const { return canJumpOver; }
    bool canBeSlidUnder() const { return canSlideUnder; }
    
    virtual bool checkCollision(const Player &player) {
        if (!active) return false;
        if (lane != player.getLane()) return false;
        
        // Don't check collision if player is flying (jetpack active)
        if (player.isFlying()) {
            return false;
        }
        
        FloatRect obstacleBox = getHitbox();
        FloatRect playerBox = player.getHitbox();
        
        if (!playerBox.intersects(obstacleBox)) return false;
        
        if (canJumpOver && player.getIsJumping()) {
            if (playerBox.top + playerBox.height < obstacleBox.top + 20.f) {
                return false;
            }
        }
        
        if (canSlideUnder && player.getSliding()) {
            return false;
        }
        
        return true;
    }

    void changeState(){
        active = false;
    }
};

// Derived obstacle classes
class TrainObstacle : public Obstacle {
public:
    void init(Texture &tex) {
        Obstacle::init(tex, false, false);
        sprite.setScale(0.25f, 0.25f);
    }
    
    void spawn() override {
        active = true;
        lane = rand() % 3;
        sprite.setPosition(laneX[lane], -60.f);
        yPosition = -60.f;
    }
    
    void spawn(int preferredLane) override {
        active = true;
        lane = preferredLane;
        sprite.setPosition(laneX[lane], -60.f);
        yPosition = -60.f;
    }
};

class BarrierObstacle : public Obstacle {
public:
    void init(Texture &tex) {
        Obstacle::init(tex, true, false);
        sprite.setScale(0.2f, 0.2f);
    }
    
    void spawn() override {
        active = true;
        lane = rand() % 3;
        sprite.setPosition(laneX[lane], -60.f);
        yPosition = -60.f;
    }
    
    void spawn(int preferredLane) override {
        active = true;
        lane = preferredLane;
        sprite.setPosition(laneX[lane], -60.f);
        yPosition = -60.f;
    }
};

class ConeObstacle : public Obstacle {
public:
    void init(Texture &tex) {
        Obstacle::init(tex, true, false);
        sprite.setScale(0.15f, 0.15f);
    }
    
    void spawn() override {
        active = true;
        lane = rand() % 3;
        sprite.setPosition(laneX[lane], -60.f);
        yPosition = -60.f;
    }
    
    void spawn(int preferredLane) override {
        active = true;
        lane = preferredLane;
        sprite.setPosition(laneX[lane], -60.f);
        yPosition = -60.f;
    }
};

class FenceObstacle : public Obstacle {
public:
    void init(Texture &tex) {
        Obstacle::init(tex, false, true);
        sprite.setScale(0.05f, 0.05f);
    }
    
    void spawn() override {
        active = true;
        lane = rand() % 3;
        sprite.setPosition(laneX[lane], -60.f);
        yPosition = -60.f;
    }
    
    void spawn(int preferredLane) override {
        active = true;
        lane = preferredLane;
        sprite.setPosition(laneX[lane], -60.f);
        yPosition = -60.f;
    }
};

// Forward declaration for GameEngine
class GameEngine;

// Abstract PowerUp class
class PowerUp {
protected:
    bool active;
    int lane;
    Sprite sprite;
    float rotation;
    string type;
    float yPosition;
    
    public:
    PowerUp() : active(false), lane(0), rotation(0.f), yPosition(0.f) {}
    virtual ~PowerUp() {}
    
    virtual void init(Texture &tex, const string& powerupType) {
        sprite.setTexture(tex);
        sprite.setScale(0.2f, 0.2f);
        sprite.setOrigin(tex.getSize().x / 2.f, tex.getSize().y / 2.f);
        type = powerupType;
    }
    
    virtual void spawn() = 0;
    virtual void spawn(int preferredLane) = 0;
    
    virtual void update(float dt, float speed) {
        if (!active) return;
        sprite.move(0, speed * dt);
        yPosition = sprite.getPosition().y;
        
        rotation += 180.f * dt;
        if (rotation >= 360.f) rotation -= 360.f;
        sprite.setRotation(rotation);
        
        if (sprite.getPosition().y > WINDOW_HEIGHT + 40) active = false;
    }
    
    virtual void draw(RenderWindow &win) {
        if (active) win.draw(sprite);
    }
    
    bool isActive() const { return active; }
    
    FloatRect getHitbox() {
        if (!active) return {0, 0, 0, 0};
        FloatRect bounds = sprite.getGlobalBounds();
        bounds.left += bounds.width * 0.2f;
        bounds.top += bounds.height * 0.2f;
        bounds.width *= 0.6f;
        bounds.height *= 0.6f;
        return bounds;
    }
    
    float getYPosition() const { return yPosition; }
    int getLane() { return lane; }
    const string& getType() { return type; }
    void collect() { active = false; }
    
    virtual void activateEffect(class Player& player, class GameEngine& game) = 0;
    virtual void deactivateEffect(class Player& player, class GameEngine& game) = 0;
};

// Derived power-up classes
class MagnetPower : public PowerUp {
public:
void spawn() override {
        active = true;
        lane = rand() % 3;
        sprite.setPosition(laneX[lane] + 25.f, -40.f);
        yPosition = -40.f;
        rotation = 0.f;
    }
    
    void spawn(int preferredLane) override {
        active = true;
        lane = preferredLane;
        sprite.setPosition(laneX[lane] + 25.f, -40.f);
        yPosition = -40.f;
        rotation = 0.f;
    }
    
    void activateEffect(Player& player, GameEngine& game) override;
    void deactivateEffect(Player& player, GameEngine& game) override;
};

class JetpackPower : public PowerUp {
public:
void spawn() override {
    active = true;
    lane = rand() % 3;
        sprite.setPosition(laneX[lane] + 25.f, -40.f);
        yPosition = -40.f;
        rotation = 0.f;
    }
    
    void spawn(int preferredLane) override {
        active = true;
        lane = preferredLane;
        sprite.setPosition(laneX[lane] + 25.f, -40.f);
        yPosition = -40.f;
        rotation = 0.f;
    }
    
    void activateEffect(Player& player, GameEngine& game) override;
    void deactivateEffect(Player& player, GameEngine& game) override;
};

class ShieldPower : public PowerUp {
    public:
    void spawn() override {
        active = true;
        lane = rand() % 3;
        sprite.setPosition(laneX[lane] + 25.f, -40.f);
        yPosition = -40.f;
        rotation = 0.f;
    }
    
    void spawn(int preferredLane) override {
        active = true;
        lane = preferredLane;
        sprite.setPosition(laneX[lane] + 25.f, -40.f);
        yPosition = -40.f;
        rotation = 0.f;
    }
    
    void activateEffect(Player& player, GameEngine& game) override;
    void deactivateEffect(Player& player, GameEngine& game) override;
};

class DoubleCoinPower : public PowerUp {
public:
    void spawn() override {
        active = true;
        lane = rand() % 3;
        sprite.setPosition(laneX[lane] + 25.f, -40.f);
        yPosition = -40.f;
        rotation = 0.f;
    }
    
    void spawn(int preferredLane) override {
        active = true;
        lane = preferredLane;
        sprite.setPosition(laneX[lane] + 25.f, -40.f);
        yPosition = -40.f;
        rotation = 0.f;
    }
    
    void activateEffect(Player& player, GameEngine& game) override;
    void deactivateEffect(Player& player, GameEngine& game) override;
};

class Coin {
private:
    bool active;
    int lane;
    Sprite sprite;
    Vector2f position;
    float yPosition;
    
    public:
    Coin() : active(false), lane(0), yPosition(0.f) {}
    
    void init(Texture &tex) {
        sprite.setTexture(tex);
        sprite.setScale(0.25f, 0.25f);
    }
    
    Sprite& getSprite() { return sprite; }
    
    void spawn() {
        active = true;
        lane = rand() % 3;
        position = Vector2f(laneX[lane] + 15.f, -40.f);
        yPosition = -40.f;
        sprite.setPosition(position);
    }
    
    void spawn(int preferredLane) {
        active = true;
        lane = preferredLane;
        position = Vector2f(laneX[lane] + 15.f, -40.f);
        yPosition = -40.f;
        sprite.setPosition(position);
    }

    void update(float dt, float speed) {
        if (!active) return;
        position.y += speed * dt;
        yPosition = position.y;
        sprite.setPosition(position);
        if (position.y > WINDOW_HEIGHT + 40) active = false;
    }

    bool isActive() const { return active; }

    FloatRect getHitbox() {
        if (!active) return {0, 0, 0, 0};
        return sprite.getGlobalBounds();
    }
    
    Vector2f getPosition() const { return position; }
    float getYPosition() const { return yPosition; }
    
    void setPosition(const Vector2f& pos) { 
        position = pos; 
        yPosition = pos.y;
        sprite.setPosition(position);
    }

    int getLane() { return lane; }

    void collect() { active = false; }

    void draw(RenderWindow &win) {
        if (active) win.draw(sprite);
    }
};

// ScoreManager class
class ScoreManager {
    private:
    int score;
    int coinScore;
    int highScore;
    string highScoreName;
    
    public:
    ScoreManager() : score(0), coinScore(0), highScore(0) {}
    
    void addScore(int points) {
        score += points;
    }
    
    void addCoinScore(int coins) {
        coinScore += coins * 50;
    }
    
    int getTotalScore() const {
        return score + coinScore;
    }
    
    int getCoinCount() const {
        return coinScore / 50;
    }
    
    void loadHighScore() {
        ifstream file("highscore.txt");
        if (file.is_open()) {
            string line;
            if (getline(file, line)) {
                size_t commaPos = line.find(',');
                if (commaPos != string::npos) {
                    highScoreName = line.substr(0, commaPos);
                    
                    string scoreStr = line.substr(commaPos + 1);
                    int score = 0;
                    for (char c : scoreStr) {
                        if (c >= '0' && c <= '9') {
                            score = score * 10 + (c - '0');
                        }
                    }
                    highScore = score;
                }
            }
            file.close();
        }
    }
    
    void saveHighScore(const string& name) {
        if (getTotalScore() > highScore) {
            highScore = getTotalScore();
            highScoreName = name;
            
            ofstream file("highscore.txt");
            if (file.is_open()) {
                file << name << "," << highScore << endl;
                file.close();
            }
            
            // Also add to leaderboard
            addToLeaderboard(name, highScore);
        }
    }
    
    int getHighScore() const { return highScore; }
    const string& getHighScoreName() const { return highScoreName; }
    
    void reset() {
        score = 0;
        coinScore = 0;
    }
};

// TrackManager class
class TrackManager {
    private:
    Sprite bg1, bg2;
    Texture bgTex;
    float baseSpeed;
    float currentSpeed;
    
public:
    TrackManager() : baseSpeed(280.f), currentSpeed(280.f) {}
    
    bool loadTexture(const string& filename) {
        return bgTex.loadFromFile(filename);
    }
    
    void init() {
        bg1.setTexture(bgTex);
        bg2.setTexture(bgTex);
        bg1.setPosition(0, 0);
        bg2.setPosition(0, -bg1.getGlobalBounds().height);
    }
    
    void update(float dt) {
        bg1.move(0, currentSpeed * dt);
        bg2.move(0, currentSpeed * dt);
        
        if (bg1.getPosition().y >= WINDOW_HEIGHT) {
            bg1.setPosition(0, bg2.getPosition().y - bg1.getGlobalBounds().height);
        }
        if (bg2.getPosition().y >= WINDOW_HEIGHT) {
            bg2.setPosition(0, bg1.getPosition().y - bg2.getGlobalBounds().height);
        }
    }
    
    void setSpeed(float speed) {
        currentSpeed = speed;
    }
    
    float getSpeed() const {
        return currentSpeed;
    }
    
    void increaseSpeed(float increment) {
        currentSpeed += increment;
    }
    
    void draw(RenderWindow& window) {
        window.draw(bg1);
        window.draw(bg2);
    }
    
    void reset() {
        currentSpeed = baseSpeed;
        bg1.setPosition(0, 0);
        bg2.setPosition(0, -bg1.getGlobalBounds().height);
    }
};

// GameEngine class
class GameEngine {
    private:
    const static int OB_MAX = 10;
    const static int COIN_MAX = 12;
    const static int POWERUP_MAX = 8;
    
    RenderWindow window;
    Clock clock;
    Font font;
    
    Player player;
    TrackManager trackManager;
    ScoreManager scoreManager;
    
    sf::Texture menuBgTexture;
    sf::Sprite menuBgSprite;
    // Obstacle arrays
    TrainObstacle trains[OB_MAX];
    BarrierObstacle barriers[OB_MAX];
    ConeObstacle cones[OB_MAX];
    FenceObstacle fences[OB_MAX];
    
    // Powerup arrays
    MagnetPower magnetPowers[POWERUP_MAX/4];
    JetpackPower jetpackPowers[POWERUP_MAX/4];
    ShieldPower shieldPowers[POWERUP_MAX/4];
    DoubleCoinPower doubleCoinPowers[POWERUP_MAX/4];
    
    Coin coins[COIN_MAX];
    
    Texture playerTex, coinTex, trainTex, barrierTex, bgTex;
    Texture coneTex, fenceTex, shieldTex, coinDoubleTex, magnetTex, jetpackTex;
    
    float obstacleSpeed = 280.f;
    float baseObstacleSpeed = 280.f;
    float speedIncrement = 0.5f;
    float spawnTimer = 0.f;
    float barrierTimer = 0.f;
    float coneTimer = 0.f;
    float fenceTimer = 0.f;
    float coinTimer = 0.f;
    float powerupTimer = 0.f;
    float gameTime = 0.f;
    bool gameRunning = true;
    bool inGame = false;
    bool inLeaderboard = false;
    string playerName;
    bool gettingName = false;
    string inputName;
    int menuSelection = 0;
    int leaderboardPage = 0;
    
    // Powerup states
    bool magnetActive = false;
    bool jetpackActive = false;
    float magnetTimeRemaining = 0.f;
    float jetpackTimeRemaining = 0.f;
    
    // Lane management
    int laneOccupiedCount[3] = {0, 0, 0};
    
    // Helper function to check if lane is safe for spawning
    bool isLaneSafeForSpawn(int lane, bool forPowerup = false) {
        // During jetpack, don't spawn obstacles, only coins
        if (jetpackActive && !forPowerup && lane != -2) {
            return false;
        }
        
        // Always leave at least one lane free
        int occupiedLanes = 0;
        for (int i = 0; i < 3; i++) {
            if (laneOccupiedCount[i] > 0) {
                occupiedLanes++;
            }
        }
        
        // If already 2 lanes occupied, don't occupy the third
        if (occupiedLanes >= 2 && laneOccupiedCount[lane] == 0) {
            return false;
        }
        
        float minGap = forPowerup ? MIN_POWERUP_GAP : MIN_OBSTACLE_GAP;
        
        // Check for obstacle overlap in this lane
        for (int i = 0; i < OB_MAX; i++) {
            if (trains[i].isActive() && trains[i].getLane() == lane) {
                if (trains[i].getYPosition() < minGap) return false;
            }
            if (barriers[i].isActive() && barriers[i].getLane() == lane) {
                if (barriers[i].getYPosition() < minGap) return false;
            }
            if (cones[i].isActive() && cones[i].getLane() == lane) {
                if (cones[i].getYPosition() < minGap) return false;
            }
            if (fences[i].isActive() && fences[i].getLane() == lane) {
                if (fences[i].getYPosition() < minGap) return false;
            }
        }
        
        // Check for powerup overlap in this lane
        if (!forPowerup) {
            for (int i = 0; i < POWERUP_MAX/4; i++) {
                if (magnetPowers[i].isActive() && magnetPowers[i].getLane() == lane) {
                    if (magnetPowers[i].getYPosition() < MIN_POWERUP_GAP) return false;
                }
                if (jetpackPowers[i].isActive() && jetpackPowers[i].getLane() == lane) {
                    if (jetpackPowers[i].getYPosition() < MIN_POWERUP_GAP) return false;
                }
                if (shieldPowers[i].isActive() && shieldPowers[i].getLane() == lane) {
                    if (shieldPowers[i].getYPosition() < MIN_POWERUP_GAP) return false;
                }
                if (doubleCoinPowers[i].isActive() && doubleCoinPowers[i].getLane() == lane) {
                    if (doubleCoinPowers[i].getYPosition() < MIN_POWERUP_GAP) return false;
                }
            }
        }
        
        // Check for coin overlap in this lane
        for (int i = 0; i < COIN_MAX; i++) {
            if (coins[i].isActive() && coins[i].getLane() == lane) {
                if (coins[i].getYPosition() < (forPowerup ? MIN_POWERUP_GAP : MIN_OBSTACLE_GAP/2)) {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    // Helper function to update lane occupied count
    void updateLaneOccupiedCount() {
        // Reset counts
        for (int i = 0; i < 3; i++) {
            laneOccupiedCount[i] = 0;
        }
        
        // Count obstacles in each lane
        for (int i = 0; i < OB_MAX; i++) {
            if (trains[i].isActive() && trains[i].getYPosition() < WINDOW_HEIGHT/2) {
                laneOccupiedCount[trains[i].getLane()]++;
            }
            if (barriers[i].isActive() && barriers[i].getYPosition() < WINDOW_HEIGHT/2) {
                laneOccupiedCount[barriers[i].getLane()]++;
            }
            if (cones[i].isActive() && cones[i].getYPosition() < WINDOW_HEIGHT/2) {
                laneOccupiedCount[cones[i].getLane()]++;
            }
            if (fences[i].isActive() && fences[i].getYPosition() < WINDOW_HEIGHT/2) {
                laneOccupiedCount[fences[i].getLane()]++;
            }
        }
    }
    
    // Helper function to find a free lane with preference
    int findFreeLane(bool forPowerup = false) {
        // Special case: during jetpack, only spawn coins more frequently
        if (jetpackActive && !forPowerup) {
            return -1; // Don't spawn obstacles during jetpack
        }
        
        int preferredLane = -1;
        
        // Create an array of lanes in random order
        int laneOrder[3] = {0, 1, 2};
        for (int i = 0; i < 3; i++) {
            int j = rand() % 3;
            int temp = laneOrder[i];
            laneOrder[i] = laneOrder[j];
            laneOrder[j] = temp;
        }
        
        // First try: find a completely safe lane
        for (int i = 0; i < 3; i++) {
            int lane = laneOrder[i];
            if (isLaneSafeForSpawn(lane, forPowerup)) {
                preferredLane = lane;
                break;
            }
        }
        
        // If no lane is safe, return -1 (don't spawn)
        return preferredLane;
    }
    
    // Helper function to handle text input
    void handleTextInput(Event& event) {
        if (event.type == Event::TextEntered) {
            if (event.text.unicode == '\b') { // Backspace
                if (!inputName.empty()) {
                    inputName.pop_back();
                }
            } else if (event.text.unicode == '\r' || event.text.unicode == '\n') { // Enter
                if (!inputName.empty()) {
                    playerName = inputName;
                    gettingName = false;
                    inputName.clear();
                    startGame();
                }
            } else if (event.text.unicode >= 32 && event.text.unicode < 128) {
                char c = static_cast<char>(event.text.unicode);
                // Only allow letters, numbers, and underscores, no spaces
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
                    (c >= '0' && c <= '9') || c == '_') {
                    if (inputName.length() < 15) { // Limit name length
                        inputName += c;
                    }
                }
            }
        }
    }
    
    void updatePowerupTimers(float dt) {
        if (magnetActive) {
            magnetTimeRemaining -= dt;
            if (magnetTimeRemaining <= 0.f) {
                magnetActive = false;
            }
        }
        
        if (jetpackActive) {
            jetpackTimeRemaining -= dt;
            if (jetpackTimeRemaining <= 0.f) {
                jetpackActive = false;
                // Restore normal speed when jetpack ends
                obstacleSpeed = baseObstacleSpeed;
                trackManager.setSpeed(obstacleSpeed);
            }
        }
    }

public:
    GameEngine() {
        window.create({WINDOW_WIDTH, WINDOW_HEIGHT}, "Subway Surfers");
        srand(time(0));
    }

    void loadAssets() {

        if (!menuBgTexture.loadFromFile("bg.jpeg")) {
         std::cout << "Menu background load fail\n";
        } else {
        menuBgSprite.setTexture(menuBgTexture);
        menuBgSprite.setScale(
        WINDOW_WIDTH / (float)menuBgTexture.getSize().x,
        WINDOW_HEIGHT / (float)menuBgTexture.getSize().y
    );
}
        if (!trackManager.loadTexture("trackg.jpeg")) cout << "BG load fail\n";
        if (!playerTex.loadFromFile("man.jpeg")) cout << "Player load fail\n";
        if (!coinTex.loadFromFile("Coin1.png")) cout << "Coin load fail\n";
        if (!trainTex.loadFromFile("train1.jpeg")) cout << "Train load fail\n";
        if (!barrierTex.loadFromFile("blocker2.jpeg")) cout << "Barrier load fail\n";
        
        if (!coneTex.loadFromFile("cone.png")) cout << "Cone texture load fail\n";
        if (!fenceTex.loadFromFile("fence.png")) cout << "Fence texture load fail\n";
        if (!shieldTex.loadFromFile("sheild.png")) cout << "Shield texture load fail\n";
        if (!coinDoubleTex.loadFromFile("coindouble.png")) cout << "CoinDouble texture load fail\n";
        if (!magnetTex.loadFromFile("Coin-Magnet.png")) cout << "Magnet texture load fail\n";
        if (!jetpackTex.loadFromFile("jetpack.png")) cout << "Jetpack texture load fail\n";
        if (!font.loadFromFile("Arial.ttf")) cout << "Font load fail\n";

        trackManager.init();
        
        player.init(playerTex, shieldTex);
        
        // Initialize all obstacles
        for (int i = 0; i < OB_MAX; i++) {
            trains[i].init(trainTex);
            barriers[i].init(barrierTex);
            cones[i].init(coneTex);
            fences[i].init(fenceTex);
        }
        
        // Initialize powerups
        for (int i = 0; i < POWERUP_MAX/4; i++) {
            magnetPowers[i].init(magnetTex, "magnet");
            jetpackPowers[i].init(jetpackTex, "jetpack");
            shieldPowers[i].init(shieldTex, "shield");
            doubleCoinPowers[i].init(coinDoubleTex, "doublecoin");
        }
        
        // Initialize coins
        for (int i = 0; i < COIN_MAX; i++) {
            coins[i].init(coinTex);
        }
        
        scoreManager.loadHighScore();
    }
    
    void activateMagnet(float duration) {
        magnetActive = true;
        magnetTimeRemaining = duration;
    }
    
    void deactivateMagnet() {
        magnetActive = false;
    }
    
    void activateJetpack(float duration) {
        jetpackActive = true;
        jetpackTimeRemaining = duration;
        player.activateJetpack(duration);
        
        // Increase speed during jetpack
        baseObstacleSpeed = obstacleSpeed;
        obstacleSpeed *= 1.5f; // 50% faster during jetpack
        trackManager.setSpeed(obstacleSpeed);
        
        // Deactivate all active obstacles during jetpack
        for (int i = 0; i < OB_MAX; i++) {
            if (trains[i].isActive()) {
                trains[i].spawn(1); // Deactivate by moving off screen
            }
            if (barriers[i].isActive()) {
                barriers[i].spawn(1);
            }
            if (cones[i].isActive()) {
                cones[i].spawn(1);
            }
            if (fences[i].isActive()) {
                fences[i].spawn(1);
            }
        }
    }
    
    void deactivateJetpack() {
        jetpackActive = false;
        player.deactivateJetpack();
        // Speed will be restored in updatePowerupTimers
    }
    
    void startGame() {
        inGame = true;
        gameRunning = true;
        gameTime = 0.f;
        obstacleSpeed = 280.f;
        baseObstacleSpeed = 280.f;
        spawnTimer = 0.f;
        barrierTimer = 0.f;
        coneTimer = 0.f;
        fenceTimer = 0.f;
        coinTimer = 0.f;
        powerupTimer = 0.f;
        
        // Reset lane counts
        for (int i = 0; i < 3; i++) {
            laneOccupiedCount[i] = 0;
        }
        
        scoreManager.reset();
        
        // Reset all game objects
        for (int i = 0; i < OB_MAX; i++) {
            trains[i].spawn(1);
            barriers[i].spawn(1);
            cones[i].spawn(1);
            fences[i].spawn(1);
        }
        
        for (int i = 0; i < POWERUP_MAX/4; i++) {
            magnetPowers[i].collect();
            jetpackPowers[i].collect();
            shieldPowers[i].collect();
            doubleCoinPowers[i].collect();
        }
        
        for (int i = 0; i < COIN_MAX; i++) coins[i].collect();
        
        player.deactivateShield();
        player.deactivateCoinDouble();
        player.deactivateJetpack();
        magnetActive = false;
        jetpackActive = false;
        speedIncrement = 0.5f;
        
        trackManager.reset();
    }
    
    void showMainMenu() {
        window.clear(Color(50, 50, 100));
        window.draw(menuBgSprite);
        Text titleText;
       
        
        vector<string> options = {"Start Game", "Leaderboard", "Exit"};
        
        for (int i = 0; i < options.size(); i++) {
            Text optionText;
            optionText.setFont(font);
            optionText.setCharacterSize(50);
            
            float yPos = 200 + i * 100;
            FloatRect textBounds;
            optionText.setString(options[i]);
            optionText.setStyle(sf::Text::Bold); 
            textBounds = optionText.getLocalBounds();
            optionText.setOrigin(textBounds.left + textBounds.width / 2.0f, 
                                textBounds.top + textBounds.height / 2.0f);
            optionText.setPosition(WINDOW_WIDTH / 2, yPos);
            
            if (i == menuSelection) {
                optionText.setFillColor(Color::Yellow);
            } else {
                optionText.setFillColor(Color(255, 165, 0));
            }
            
            window.draw(optionText);
        }
        
        Text instructions;
        instructions.setFont(font);
        instructions.setCharacterSize(20);
        instructions.setFillColor(Color::Cyan);
        instructions.setString("Use A/D to move left/right, W/S to jump/slide. Use UP/DOWN to navigate menu. ENTER to select.");
        instructions.setPosition(WINDOW_WIDTH / 2 - instructions.getGlobalBounds().width / 2, 450);
        window.draw(instructions);
        
        window.display();
        
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Escape) {
                    window.close();
                }
                else if (event.key.code == Keyboard::Up) {
                    menuSelection--;
                    if (menuSelection < 0) menuSelection = 2;
                }
                else if (event.key.code == Keyboard::Down) {
                    menuSelection++;
                    if (menuSelection > 2) menuSelection = 0;
                }
                else if (event.key.code == Keyboard::Enter) {
                    if (menuSelection == 0) {
                        gettingName = true;
                        inputName.clear();
                    } else if (menuSelection == 1) {
                        readLeaderboard();
                        inLeaderboard = true;
                        leaderboardPage = 0;
                    } else if (menuSelection == 2) {
                        window.close();
                    }
                }
            }
        }
    }
    
    void showNameInput() {
        window.clear(Color(50, 50, 100));
        
        Text titleText;
        titleText.setFont(font);
        titleText.setCharacterSize(60);
        titleText.setFillColor(Color::Yellow);
        titleText.setString("ENTER YOUR NAME");
        titleText.setPosition(WINDOW_WIDTH / 2 - titleText.getGlobalBounds().width / 2, 50);
        window.draw(titleText);
        
        Text instructions;
        instructions.setFont(font);
        instructions.setCharacterSize(24);
        instructions.setFillColor(Color::White);
        instructions.setString("No spaces allowed. Press Enter when done.");
        instructions.setPosition(WINDOW_WIDTH / 2 - instructions.getGlobalBounds().width / 2, 120);
        window.draw(instructions);
        
        RectangleShape inputBox(Vector2f(400, 60));
        inputBox.setFillColor(Color(30, 30, 30));
        inputBox.setOutlineThickness(3);
        inputBox.setOutlineColor(Color::White);
        inputBox.setPosition(WINDOW_WIDTH / 2 - 200, 200);
        window.draw(inputBox);
        
        // Input text with cursor
        string displayText = inputName + "_";
        Text inputText;
        inputText.setFont(font);
        inputText.setCharacterSize(36);
        inputText.setFillColor(Color(255,165,0));
        inputText.setString(displayText);
        inputText.setPosition(WINDOW_WIDTH / 2 - inputText.getGlobalBounds().width / 2, 210);
        window.draw(inputText);
        
        window.display();
        
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Escape) {
                    gettingName = false;
                    inLeaderboard = false;
                    inGame = false;
                }
            }
            handleTextInput(event);
        }
    }
    
    void showLeaderboard() {
        window.clear(Color(50, 50, 100));
        
        Text titleText;
        titleText.setFont(font);
        titleText.setCharacterSize(60);
        titleText.setFillColor(Color::Yellow);
        titleText.setString("LEADERBOARD");
        titleText.setPosition(WINDOW_WIDTH / 2 - titleText.getGlobalBounds().width / 2, 30);
        window.draw(titleText);
        
        Text entryText;
        entryText.setFont(font);
        entryText.setCharacterSize(30);
        entryText.setFillColor(Color::White);
        
        if (leaderboardSize == 0) {
            entryText.setString("No scores yet!");
            entryText.setPosition(WINDOW_WIDTH / 2 - entryText.getGlobalBounds().width / 2, 150);
            window.draw(entryText);
        } else {
            int startIndex = leaderboardPage * 5;
            int endIndex = startIndex + 5;
            if (endIndex > leaderboardSize) endIndex = leaderboardSize;
            
            for (int i = startIndex; i < endIndex; i++) {
                string entryStr = intToString(i + 1) + ". " + leaderboard[i].name + " - " + intToString(leaderboard[i].score);
                entryText.setString(entryStr);
                
                // Color coding for top 3
                if (i == 0) entryText.setFillColor(Color::Yellow);
                else if (i == 1) entryText.setFillColor(Color::Cyan);
                else if (i == 2) entryText.setFillColor(Color(205, 127, 50)); // Bronze
                else entryText.setFillColor(Color::White);
                
                entryText.setPosition(WINDOW_WIDTH / 2 - entryText.getGlobalBounds().width / 2, 
                                      120 + (i - startIndex) * 40);
                window.draw(entryText);
            }
        }
        
        // Page indicator
        Text pageText;
        pageText.setFont(font);
        pageText.setCharacterSize(20);
        pageText.setFillColor(Color::Cyan);
        
        string pageStr = "Page " + intToString(leaderboardPage + 1) + " of " + 
                        intToString((leaderboardSize + 4) / 5);
        pageText.setString(pageStr);
        pageText.setPosition(WINDOW_WIDTH / 2 - pageText.getGlobalBounds().width / 2, 350);
        window.draw(pageText);
        
        // Back instruction
        Text backText;
        backText.setFont(font);
        backText.setCharacterSize(24);
        backText.setFillColor(Color::Cyan);
        backText.setString("Press ESC to return to menu. LEFT/RIGHT to navigate pages.");
        backText.setPosition(WINDOW_WIDTH / 2 - backText.getGlobalBounds().width / 2, 400);
        window.draw(backText);
        
        window.display();
        
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Escape) {
                    inLeaderboard = false;
                }
                else if (event.key.code == Keyboard::Left) {
                    if (leaderboardPage > 0) leaderboardPage--;
                }
                else if (event.key.code == Keyboard::Right) {
                    if ((leaderboardPage + 1) * 5 < leaderboardSize) leaderboardPage++;
                }
            }
        }
    }

    void loop() {
        while (window.isOpen()) {
            if (gettingName) {
                showNameInput();
                continue;
            }
            
            if (inLeaderboard) {
                showLeaderboard();
                continue;
            }
            
            if (!inGame) {
                showMainMenu();
                continue;
            }
            
            float dt = clock.restart().asSeconds();

            Event e;
            while (window.pollEvent(e)) {
                if (e.type == Event::Closed) window.close();
                
                if (e.type == Event::KeyPressed && e.key.code == Keyboard::Escape) {
                    inGame = false;
                    menuSelection = 0;
                    continue;
                }
                
                if (e.type == Event::KeyPressed && e.key.code == Keyboard::R && !gameRunning) {
                    resetGame();
                }
            }

            if (!gameRunning) {
                render();
                continue;
            }

            gameTime += dt;

            // Increase speed over time (only if not in jetpack mode)
            if (!jetpackActive) {
                obstacleSpeed += speedIncrement * dt;
                baseObstacleSpeed = obstacleSpeed;
            }
            trackManager.setSpeed(obstacleSpeed);

            // Update track
            trackManager.update(dt);
            
            // Update lane occupied count
            updateLaneOccupiedCount();

            // Add survival score (10 points per second)
            scoreManager.addScore((int)(dt * 10));

            // Spawn trains (not during jetpack)
            spawnTimer += dt;
            if (spawnTimer > 1.5f && !jetpackActive) {
                int freeLane = findFreeLane();
                if (freeLane != -1) {
                    for (int i = 0; i < OB_MAX; i++) {
                        if (!trains[i].isActive()) {
                            trains[i].spawn(freeLane);
                            break;
                        }
                    }
                }
                spawnTimer = 0;
            }

            // Spawn barriers (not during jetpack)
            barrierTimer += dt;
            if (barrierTimer > 3.0f && !jetpackActive) {
                int freeLane = findFreeLane();
                if (freeLane != -1) {
                    for (int i = 0; i < OB_MAX; i++) {
                        if (!barriers[i].isActive()) {
                            barriers[i].spawn(freeLane);
                            break;
                        }
                    }
                }
                barrierTimer = 0;
            }

            // Spawn cones (not during jetpack)
            coneTimer += dt;
            if (coneTimer > 4.0f && !jetpackActive) {
                int freeLane = findFreeLane();
                if (freeLane != -1) {
                    for (int i = 0; i < OB_MAX; i++) {
                        if (!cones[i].isActive()) {
                            cones[i].spawn(freeLane);
                            break;
                        }
                    }
                }
                coneTimer = 0;
            }

            // Spawn fences (not during jetpack)
            fenceTimer += dt;
            if (fenceTimer > 4.0f && !jetpackActive) { 
                int freeLane = findFreeLane();
                if (freeLane != -1) {
                    for (int i = 0; i < OB_MAX; i++) {
                        if (!fences[i].isActive()) {
                            fences[i].spawn(freeLane);
                            break;
                        }
                    }
                }
                fenceTimer = 0;
            }

            // Spawn coins (more frequently during jetpack)
            coinTimer += dt;
            float coinSpawnTime = jetpackActive ? 0.3f : 0.8f; // Faster coin spawn during jetpack
            if (coinTimer > coinSpawnTime) {
                int freeLane = findFreeLane(true); // For coins, use powerup spacing
                if (freeLane != -1) {
                    for (int i = 0; i < COIN_MAX; i++) {
                        if (!coins[i].isActive()) {
                            coins[i].spawn(freeLane);
                            break;
                        }
                    }
                }
                coinTimer = 0;
            }

            // Spawn powerups (not during jetpack)
            powerupTimer += dt;
            if (powerupTimer > 8.0f && !jetpackActive) {
                int freeLane = findFreeLane(true);
                if (freeLane != -1) {
                    // Randomly choose which powerup to spawn
                    int powerupType = rand() % 4;
                    bool spawned = false;
                    
                    switch(powerupType) {
                        case 0: // Magnet
                            for (int i = 0; i < POWERUP_MAX/4; i++) {
                                if (!magnetPowers[i].isActive()) {
                                    magnetPowers[i].spawn(freeLane);
                                    spawned = true;
                                    break;
                                }
                            }
                            break;
                        case 1: // Jetpack
                            for (int i = 0; i < POWERUP_MAX/4; i++) {
                                if (!jetpackPowers[i].isActive()) {
                                    jetpackPowers[i].spawn(freeLane);
                                    spawned = true;
                                    break;
                                }
                            }
                            break;
                        case 2: // Shield
                            for (int i = 0; i < POWERUP_MAX/4; i++) {
                                if (!shieldPowers[i].isActive()) {
                                    shieldPowers[i].spawn(freeLane);
                                    spawned = true;
                                    break;
                                }
                            }
                            break;
                        case 3: // Double Coin
                            for (int i = 0; i < POWERUP_MAX/4; i++) {
                                if (!doubleCoinPowers[i].isActive()) {
                                    doubleCoinPowers[i].spawn(freeLane);
                                    spawned = true;
                                    break;
                                }
                            }
                            break;
                    }
                }
                powerupTimer = 0;
            }

            player.handleInput(dt);
            player.update(dt);
            
            updatePowerupTimers(dt);

            // Update all game objects
            for (int i = 0; i < OB_MAX; i++) {
                trains[i].update(dt, obstacleSpeed);
                barriers[i].update(dt, obstacleSpeed);
                cones[i].update(dt, obstacleSpeed);
                fences[i].update(dt, obstacleSpeed);
            }
            
            for (int i = 0; i < POWERUP_MAX/4; i++) {
                magnetPowers[i].update(dt, obstacleSpeed);
                jetpackPowers[i].update(dt, obstacleSpeed);
                shieldPowers[i].update(dt, obstacleSpeed);
                doubleCoinPowers[i].update(dt, obstacleSpeed);
            }
            
            for (int i = 0; i < COIN_MAX; i++) coins[i].update(dt, obstacleSpeed);

            // Check collisions with all obstacles (only if player is not flying)
            if (!player.isFlying()) {
                for (int i = 0; i < OB_MAX; i++) {
                    if (trains[i].checkCollision(player)) {
                        if (player.isShieldActive()) {
                            player.useShield();
                            trains[i].changeState();
                            // Shield protects from collision - don't game over
                        } else {
                            gameRunning = false;
                            scoreManager.saveHighScore(playerName);
                        }
                    }
                    
                    if (barriers[i].checkCollision(player)) {
                        if (player.isShieldActive()) {
                            player.useShield();
                            barriers[i].changeState();
                            // Shield protects from collision - don't game over
                        } else {
                            gameRunning = false;
                            scoreManager.saveHighScore(playerName);
                        }
                    }
                    
                    if (cones[i].checkCollision(player)) {
                        if (player.isShieldActive()) {
                            player.useShield();
                            cones[i].changeState();
                            // Shield protects from collision - don't game over
                        } else {
                            gameRunning = false;
                            scoreManager.saveHighScore(playerName);
                        }
                    }
                    
                    if (fences[i].checkCollision(player)) {
                        if (player.isShieldActive()) {
                            player.useShield();
                            fences[i].changeState();
                            // Shield protects from collision - don't game over
                        } else {
                            gameRunning = false;
                            scoreManager.saveHighScore(playerName);
                        }
                    }
                }
            }

            // Check coin collection
            for (int i = 0; i < COIN_MAX; i++) {
                if (coins[i].isActive()) {
                    bool collected = false;
                    
                    // Check direct collision
                    if (coins[i].getHitbox().intersects(player.getHitbox())) {
                        collected = true;
                    }
                    
                    // Magnet effect (only if magnet is active and player is not flying)
                    if (magnetActive && !collected && !player.isFlying()) {
                        FloatRect coinBox = coins[i].getHitbox();
                        FloatRect playerBox = player.getHitbox();
                        
                        // Get centers of coin and player
                        Vector2f coinCenter(
                            coinBox.left + coinBox.width / 2,
                            coinBox.top + coinBox.height / 2
                        );
                        Vector2f playerCenter(
                            playerBox.left + playerBox.width / 2,
                            playerBox.top + playerBox.height / 2
                        );
                        
                        // Calculate distance
                        float dx = playerCenter.x - coinCenter.x;
                        float dy = playerCenter.y - coinCenter.y;
                        float distance = sqrt(dx*dx + dy*dy);
                        
                        // Magnet radius
                        if (distance < 200.f && distance > 10.f) {
                            // Move coin toward player
                            float attractionSpeed = 500.f * dt;
                            Vector2f coinPos = coins[i].getPosition();
                            
                            // Normalize direction vector and move coin
                            coinPos.x += (dx / distance) * attractionSpeed;
                            coinPos.y += (dy / distance) * attractionSpeed;
                            
                            // Update coin position
                            coins[i].setPosition(coinPos);
                            
                            // Check collision again with new position
                            if (coins[i].getHitbox().intersects(player.getHitbox())) {
                                collected = true;
                            }
                        }
                    }
                    
                    if (collected) {
                        coins[i].collect();
                        int coinValue = 50 * player.getCoinMultiplier();
                        scoreManager.addCoinScore(coinValue / 50);
                    }
                }
            }
            
            // Check powerup collection (only if player is not flying)
            if (!player.isFlying()) {
                for (int i = 0; i < POWERUP_MAX/4; i++) {
                    if (magnetPowers[i].isActive() && 
                        magnetPowers[i].getHitbox().intersects(player.getHitbox())) {
                        magnetPowers[i].activateEffect(player, *this);
                        magnetPowers[i].collect();
                    }
                    
                    if (jetpackPowers[i].isActive() && 
                        jetpackPowers[i].getHitbox().intersects(player.getHitbox())) {
                        jetpackPowers[i].activateEffect(player, *this);
                        jetpackPowers[i].collect();
                    }
                    
                    if (shieldPowers[i].isActive() && 
                        shieldPowers[i].getHitbox().intersects(player.getHitbox())) {
                        shieldPowers[i].activateEffect(player, *this);
                        shieldPowers[i].collect();
                    }
                    
                    if (doubleCoinPowers[i].isActive() && 
                        doubleCoinPowers[i].getHitbox().intersects(player.getHitbox())) {
                        doubleCoinPowers[i].activateEffect(player, *this);
                        doubleCoinPowers[i].collect();
                    }
                }
            }

            render();
        }
    }

    void resetGame() {
        gameRunning = true;
        gameTime = 0.f;
        obstacleSpeed = 280.f;
        baseObstacleSpeed = 280.f;
        spawnTimer = 0.f;
        barrierTimer = 0.f;
        coneTimer = 0.f;
        fenceTimer = 0.f;
        coinTimer = 0.f;
        powerupTimer = 0.f;
        
        // Reset lane counts
        for (int i = 0; i < 3; i++) {
            laneOccupiedCount[i] = 0;
        }
        
        scoreManager.reset();
        
        for (int i = 0; i < OB_MAX; i++) {
            trains[i].spawn(1);
            barriers[i].spawn(1);
            cones[i].spawn(1);
            fences[i].spawn(1);
        }
        
        for (int i = 0; i < POWERUP_MAX/4; i++) {
            magnetPowers[i].collect();
            jetpackPowers[i].collect();
            shieldPowers[i].collect();
            doubleCoinPowers[i].collect();
        }
        
        for (int i = 0; i < COIN_MAX; i++) coins[i].collect();
        
        player.deactivateShield();
        player.deactivateCoinDouble();
        player.deactivateJetpack();
        magnetActive = false;
        jetpackActive = false;
        speedIncrement = 0.5f;
        
        trackManager.reset();
    }

    void render() {
        window.clear();
        trackManager.draw(window);
        
        // Draw all game objects
        for (int i = 0; i < OB_MAX; i++) {
            trains[i].draw(window);
            barriers[i].draw(window);
            cones[i].draw(window);
            fences[i].draw(window);
        }
        
        for (int i = 0; i < POWERUP_MAX/4; i++) {
            magnetPowers[i].draw(window);
            jetpackPowers[i].draw(window);
            shieldPowers[i].draw(window);
            doubleCoinPowers[i].draw(window);
        }
        
        for (int i = 0; i < COIN_MAX; i++) coins[i].draw(window);
        
        player.draw(window);

        // Left side HUD
        Text leftHud;
        leftHud.setFont(font);
        leftHud.setCharacterSize(20);
        leftHud.setFillColor(Color::White);
        leftHud.setPosition(20, 20);
        
        string leftHudText = "Player: " + playerName + "\n";
        
        if (player.isShieldActive()) {
            leftHudText += "Shield: " + intToString((int)player.getShieldTimeRemaining()) + "s\n";
        }
        
        if (player.isCoinDoubleActive()) {
            leftHudText += "2x Coins: " + intToString((int)player.getCoinDoubleTimeRemaining()) + "s\n";
        }
        
        if (magnetActive) {
            leftHudText += "Magnet: " + intToString((int)magnetTimeRemaining) + "s\n";
        }
        
        if (jetpackActive) {
            leftHudText += "JETPACK ACTIVE! " + intToString((int)jetpackTimeRemaining) + "s\n";
            leftHudText += "Speed: " + intToString((int)obstacleSpeed) + " (FAST!)";
        } else if (jetpackTimeRemaining <= 0.f) {
            leftHudText += "Speed: " + intToString((int)obstacleSpeed);
        }
        
        leftHud.setString(leftHudText);
        window.draw(leftHud);

        // Right side HUD for scores
        Text hud;
        hud.setFont(font);
        hud.setCharacterSize(24);
        hud.setFillColor(Color::White);
        hud.setString("Score: " + intToString(scoreManager.getTotalScore()));
        hud.setPosition(650, 20);
        window.draw(hud);

        Text coinHud;
        coinHud.setFont(font);
        coinHud.setCharacterSize(24);
        coinHud.setFillColor(Color::Yellow);
        coinHud.setString("Coins: " + intToString(scoreManager.getCoinCount()));
        coinHud.setPosition(650, 60);
        window.draw(coinHud);

        Text speedHud;
        speedHud.setFont(font);
        speedHud.setCharacterSize(20);
        speedHud.setFillColor(Color::Cyan);
        speedHud.setString("Speed: " + intToString((int)obstacleSpeed));
        speedHud.setPosition(650, 100);
        window.draw(speedHud);

        // High score display
        Text highScoreHud;
        highScoreHud.setFont(font);
        highScoreHud.setCharacterSize(18);
        highScoreHud.setFillColor(Color::Green);
        highScoreHud.setString("High Score: " + scoreManager.getHighScoreName() + " - " + 
                               intToString(scoreManager.getHighScore()));
        highScoreHud.setPosition(650, 140);
        window.draw(highScoreHud);

        // Jetpack mode indicator
        if (jetpackActive) {
            RectangleShape jetpackOverlay(Vector2f(WINDOW_WIDTH, 30));
            jetpackOverlay.setFillColor(Color(255, 165, 0, 150)); // Orange transparent
            jetpackOverlay.setPosition(0, WINDOW_HEIGHT - 30);
            window.draw(jetpackOverlay);
            
            Text jetpackText;
            jetpackText.setFont(font);
            jetpackText.setCharacterSize(20);
            jetpackText.setFillColor(Color::Yellow);
            jetpackText.setString("JETPACK MODE ACTIVE! FLYING OVER OBSTACLES!");
            jetpackText.setPosition(WINDOW_WIDTH / 2 - jetpackText.getGlobalBounds().width / 2, WINDOW_HEIGHT - 25);
            window.draw(jetpackText);
        }

        // Game Over screen
        if (!gameRunning) {
            RectangleShape overlay(Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            overlay.setFillColor(Color(0, 0, 0, 150));
            window.draw(overlay);

            Text gameOverText;
            gameOverText.setFont(font);
            gameOverText.setCharacterSize(60);
            gameOverText.setFillColor(Color::Red);
            gameOverText.setString("GAME OVER");
            gameOverText.setPosition(300, 150);
            window.draw(gameOverText);

            Text restartText;
            restartText.setFont(font);
            restartText.setCharacterSize(30);
            restartText.setFillColor(Color::White);
            restartText.setString("Press R to Restart or ESC for Menu");
            restartText.setPosition(230, 250);
            window.draw(restartText);

            Text finalScore;
            finalScore.setFont(font);
            finalScore.setCharacterSize(28);
            finalScore.setFillColor(Color::Yellow);
            finalScore.setString("Final Score: " + intToString(scoreManager.getTotalScore()));
            finalScore.setPosition(340, 320);
            window.draw(finalScore);
        }

        window.display();
    }
};

// Power-up effect implementations - AFTER GameEngine class definition
void MagnetPower::activateEffect(Player& player, GameEngine& game) {
    game.activateMagnet(5.f);
}

void MagnetPower::deactivateEffect(Player& player, GameEngine& game) {
    game.deactivateMagnet();
}

void JetpackPower::activateEffect(Player& player, GameEngine& game) {
    game.activateJetpack(5.f);
}

void JetpackPower::deactivateEffect(Player& player, GameEngine& game) {
    game.deactivateJetpack();
}

void ShieldPower::activateEffect(Player& player, GameEngine& game) {
    player.activateShield(5.f);
}

void ShieldPower::deactivateEffect(Player& player, GameEngine& game) {
    player.deactivateShield();
}

void DoubleCoinPower::activateEffect(Player& player, GameEngine& game) {
    player.activateCoinDouble(5.f);
}

void DoubleCoinPower::deactivateEffect(Player& player, GameEngine& game) {
    player.deactivateCoinDouble();
}

int main() {
    GameEngine game;
    game.loadAssets();
    game.loop();
    return 0;
}