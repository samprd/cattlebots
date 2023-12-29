#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

// enums for game screens
typedef enum game_screen { MENU, BATTLE, SETTINGS } GAME_SCREEN;

// enums for moves
typedef enum move { ATTACK, BLOCK, HEAL, SPECIAL, DODGE, MAGIC_HEAL, DO_NOTHING, DEBUG_WIN, DEBUG_LOSE } MOVE;

// enums for states (changes how the cows look in the battlefield)
// states are purely visual and do not affect gameplay
// except for DEAD which is used to determine when the game ends
typedef enum states { 
    ATTACKING, 
    BLOCKING, 
    HEALING, 
    SPECIAL_ATTACKING, 
    DODGING, 
    MAGIC_HEALING, 
    DOING_NOTHING, 
    DEAD,
} STATES;

// enum for damage states (changes how much damage cows take)
typedef enum damage { NORMAL, DOUBLE, NONE } DAMAGE;

// enum for ai type (changes how enemy behaves)
typedef enum ai_type { 
    NO_AI, 
    AGGRESSIVE, 
    DEFENSIVE, 
    CAUTIOUS, 
    RANDOM, 
    BERSERKER, 
    SURVIVOR, 
    ESCAPIST,
    PACIFIST,
    RISKY,
    PATIENT,
} AI_TYPE;

// enum for cow type (changes some stats)
typedef enum cow_type { 
    DEFAULT,
    CLUMSY, // -ACCURACY
    PRECISE, // +ACCURACY
    SWIFT, // +DODGE, -BLOCK
    TOUGH, // +BLOCK, NO DODGE
    HEALER, // +MAGIC HEAL, -BLOCK, -DODGE
    MAGIC, // 100% chance MAGIC HEAL, 100% chance SPECIAL, NO BLOCK, NO DODGE
    BRUTE, // 100% chance SPECIAL, -MAGIC HEAL, -BLOCK, -DODGE
    GHOST, // 100% chance DODGE, +MAGIC HEAL, NO BLOCK, NO SPECIAL
    AGENT, // 100% chance SPECIAL, 100% chance BLOCK, NO DODGE, NO MAGIC HEAL
} COW_TYPE;

typedef enum accuracyStat { 
    ALWAYS_MISS,
    LOW, 
    NORMALA, 
    HIGH,
    GUARANTEED,
} ACCURACY;

struct cow {
    int eyeType;
    int legType;

    int health;

    MOVE attempted_move;
    STATES state;
    
    DAMAGE damage;

    AI_TYPE ai_type;

    COW_TYPE cow_type;
};

#define BAR_WIDTH 10

#define AI_TYPES 12
#define LIST_SIZE 6

#define ACCURACY_MOVES 4 // Block, Special, Dodge, Magic Heal
#define ACCURACY_TYPES 10 
// Default, Clumsy, Precise, Swift, Tough, Healer, Magic, Brute, Ghost, Agent

// Define list type for ai odds
typedef struct{
    int items[LIST_SIZE];
} LIST;

// Defines ai odds dictionary type
typedef struct {
    AI_TYPE key;
    LIST value;
} AI_ODDS;

typedef struct {
    COW_TYPE key;
    ACCURACY value;
} ACCURACY_LIST;

typedef struct {
    MOVE key;
    ACCURACY_LIST value[ACCURACY_TYPES];
} MOVE_ACCURACY;

// Order: {ATTACK, BLOCK, HEAL, SPECIAL, DODGE, MAGIC_HEAL}
const AI_ODDS aiOdds[AI_TYPES] = {
    {NO_AI, {100, 0, 0, 0, 0, 0}}, // only attack, no other moves
    {AGGRESSIVE, {80, 0, 0, 20, 0, 0}}, // mostly attack, some special attacks
    {DEFENSIVE, {30, 40, 30, 0, 0, 0}}, // mostly block and heal, some attacks
    {CAUTIOUS, {30, 0, 40, 0, 30, 0}}, // mostly heals and dodges, some attacks
    {RANDOM, {34, 33, 33, 0, 0, 0}}, // random basic moves
    {BERSERKER, {35, 0, 0, 65, 0, 0}}, // special attack priority
    {SURVIVOR, {50, 50, 0, 0, 0, 0}}, // tries to defend itself
    {ESCAPIST, {70, 0, 0, 0, 30, 0}}, // mostly attack, some dodges
    {PACIFIST, {15, 30, 30, 0, 15, 10}}, // little attacking
    {RISKY, {50, 20, 0, 30, 10, 0}}, // mostly attacking and blocking, some risky moves
    {PATIENT, {10, 50, 30, 0, 10, 0}}, // priority for defending
};

const AI_ODDS aiRageOdds[AI_TYPES] = {
    {NO_AI, {100, 0, 0, 0, 0, 0}}, // only attack, no other moves
    {AGGRESSIVE, {60, 0, 0, 40, 0, 0}}, // more special attacks
    {DEFENSIVE, {10, 40, 40, 0, 0, 10}}, // attacks are rarer
    {CAUTIOUS, {10, 0, 40, 0, 35, 15}}, // attacks are rarer
    {RANDOM, {17, 17, 17, 16, 17, 16}}, // random moves 
    {BERSERKER, {15, 0, 0, 85, 0, 0}}, // even higher special attack priority
    {SURVIVOR, {15, 0, 65, 0, 0, 20}}, // healing priority
    {ESCAPIST, {40, 0, 0, 0, 60, 0}}, // higher dodge priority
    {PACIFIST, {0, 30, 30, 0, 20, 20}}, // no attacking
    {RISKY, {0, 0, 0, 45, 25, 30}}, // priority for risky moves
    {PATIENT, {10, 0, 0, 90, 0, 0}}, // priority for heavy attacking
};

const MOVE_ACCURACY accuracyList[ACCURACY_MOVES] = {
    {BLOCK, {
        {DEFAULT, NORMALA}, 
        {CLUMSY, LOW}, 
        {PRECISE, HIGH}, 
        {SWIFT, LOW}, 
        {TOUGH, HIGH}, 
        {HEALER, LOW}, 
        {MAGIC, ALWAYS_MISS}, 
        {BRUTE, LOW},
        {GHOST, ALWAYS_MISS},
        {AGENT, GUARANTEED}}},
    {SPECIAL, {
        {DEFAULT, NORMALA}, 
        {CLUMSY, LOW}, 
        {PRECISE, HIGH}, 
        {SWIFT, NORMALA}, 
        {TOUGH, NORMALA}, 
        {HEALER, NORMALA}, 
        {MAGIC, GUARANTEED}, 
        {BRUTE, GUARANTEED},
        {GHOST, ALWAYS_MISS},
        {AGENT, GUARANTEED}}},
    {DODGE, {
        {DEFAULT, NORMALA}, 
        {CLUMSY, LOW}, 
        {PRECISE, HIGH}, 
        {SWIFT, HIGH}, 
        {TOUGH, LOW}, 
        {HEALER, LOW}, 
        {MAGIC, ALWAYS_MISS}, 
        {BRUTE, LOW},
        {GHOST, GUARANTEED},
        {AGENT, ALWAYS_MISS}}},
    {MAGIC_HEAL, {
        {DEFAULT, NORMALA}, 
        {CLUMSY, LOW}, 
        {PRECISE, HIGH}, 
        {SWIFT, NORMALA}, 
        {TOUGH, NORMALA}, 
        {HEALER, HIGH}, 
        {MAGIC, GUARANTEED}, 
        {BRUTE, LOW},
        {GHOST, HIGH},
        {AGENT, ALWAYS_MISS}}},
};

// finds ai odds by key
const LIST* findAiOdds(AI_TYPE key){
    for (int i = 0; i < AI_TYPES; i++) {
        if (aiOdds[i].key == key) {
            return &aiOdds[i].value;
        }
    }
    return NULL; // Return NULL if key not found
}

const LIST* findRageAiOdds(AI_TYPE key){
    for (int i = 0; i < AI_TYPES; i++) {
        if (aiRageOdds[i].key == key) {
            return &aiRageOdds[i].value;
        }
    }
    return NULL; // Return NULL if key not found
}

const ACCURACY findAccuracy(COW_TYPE cow_type, MOVE move){
    for (int i = 0; i < ACCURACY_MOVES; i++) {
        MOVE_ACCURACY move_accuracy = accuracyList[i]; // {MOVE, ACCURACY_LIST}
        if (move_accuracy.key == move) {            
            for (int j = 0; j < ACCURACY_TYPES; j++) {
                ACCURACY_LIST accuracy_list = move_accuracy.value[j]; // {COW_TYPE, ACCURACY}
                if (accuracy_list.key == cow_type) {
                    return accuracy_list.value; // ACCURACY
                }
            }
        }
    }
    return NORMALA; // Return NORMAL if key not found
}

void clear(){
    system("cls");
}

void waitForEnter(){
    printf("Press Enter to continue...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF); // clear the input buffer
}

void killCow(struct cow *cow){
    cow->health = 0;
    cow->state = DEAD;
    cow->damage = NONE;
}

// sets the cow's sprite based on its state
void setSprite(struct cow *cow){
    switch (cow->state){
        case DOING_NOTHING: {
            cow->eyeType = 0;
            cow->legType = 0;
            break; 
        }
        case ATTACKING: {
            cow->eyeType = 7;
            cow->legType = 0;
            break; 
        }
        case BLOCKING: {
            cow->eyeType = 2;
            cow->legType = 0;
            break; 
        }
        case HEALING: {
            cow->eyeType = 6;
            cow->legType = 0;
            break;
        }
        case SPECIAL_ATTACKING: {
            cow->eyeType = 3;
            cow->legType = 0;
            break;
        }
        case DODGING: {
            cow->eyeType = 5;
            cow->legType = 2;
            break;
        }
        case MAGIC_HEALING: {
            cow->eyeType = 4;
            cow->legType = 0;
            break;
        }
        case DEAD: {
            cow->eyeType = 1;
            cow->legType = 1;
            break;
        }
    }
}

// prints the player cow's sprite
void printCow(struct cow *player){
    setSprite(player);

    char eyes[8][3] = {"oo", "xx", "><", "**", "@@", "OO", "..", "II"};
    char legs[3][10] = {"||w--||", "//w--\\\\", "//w--//"};
    
    printf("\n\\|/          (__)           \\|/\n");
    printf("     `\\------(%s) \n", eyes[player->eyeType]);
    printf("       ||    (__) \n");
    printf("       %s      \\|/ \n", legs[player->legType]);
    printf("   \\|/ \n");
}

// prints the enemy cow's sprite
void printEnemyCow(struct cow *enemy){
    setSprite(enemy);

    char eyes[8][3] = {"oo", "xx", "><", "**", "@@", "OO", "..", "II"};
    char legs[3][12] = {"||--w||", "//w--\\\\", "\\\\w--\\\\"};
    
    printf("                    (__)          \\|/ \n");
    printf("                    (%s)------/`      \n", eyes[enemy->eyeType]);
    printf("                    (__)    ||    \n");
    printf("                       %s          \\|/ \n", legs[enemy->legType]);
    printf("          \\|/ \n");
}

// sets the health bar based on the current health
void setBar(char healthBar[], int currentHealth, char prefix, char suffix){
    int numColons = currentHealth;
    if (numColons > 10){
        numColons = 10;
    }
    if (numColons < 0){
        numColons = 0;
    }
    int numSpaces = BAR_WIDTH - numColons;

    char bar[BAR_WIDTH + 3];
    bar[0] = '\0';

    char prefixStr[2] = {prefix, '\0'};
    char suffixStr[2] = {suffix, '\0'};

    strcat(bar, prefixStr);
    for (int i = 0; i < numColons; i++) {
        strcat(bar, ":");
    }
    for (int i = 0; i < numSpaces; i++) {
        strcat(bar, " ");
    }
    strcat(bar, suffixStr);

    strcpy(healthBar, bar);
}

// prints the health bars
void printHealth(int playerH, int enemyH){
    char playerHealthBar[13];
    char enemyHealthBar[13];

    setBar(playerHealthBar, playerH, '{', '}');
    setBar(enemyHealthBar, enemyH, '{', '}');

    printf("Player\t%s\t(%d)\n", playerHealthBar, playerH);
    printf("Enemy\t%s\t(%d)\n", enemyHealthBar, enemyH);
}

// corrects health to be between 0 and 10
int correctHealth(int health){
    if (health < 0){
        health = 0;
    }
    if (health > 10){
        health = 10;
    }
    return health;
}

// gets player's chosen ai type
AI_TYPE getComType(){
    clear();

    char input;
    char line[2];
    
    printf("-> Aggresive\n");
    printf("-> Defensive\n");
    printf("-> Cautious\n");
    printf("-> Random\n");
    printf("-> Berserker\n");
    printf("-> Survivor\n");
    printf("-> Escapist\n");
    printf("-> Pacifist\n");
    printf("-> Wild\n");
    printf("-> Quiet\n");

    printf("\nChoose your opponent's strategy: ");
    if (fgets(line, sizeof(line), stdin) && line[0] != '\n') {
        input = line[0];
        // discard the rest of the line
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    } else {
        input = 'n'; // n means do nothing
    }

    AI_TYPE chosen_type;

    switch (tolower(input)){
        case 'a':
            chosen_type = AGGRESSIVE;
            break;
        case 'd':
            chosen_type = DEFENSIVE;
            break;
        case 'c':
            chosen_type = CAUTIOUS;
            break;
        case 'r':
            chosen_type = RANDOM;
            break;
        case 'b':
            chosen_type = BERSERKER;
            break;
        case 's':
            chosen_type = SURVIVOR;
            break;
        case 'e':
            chosen_type = ESCAPIST;
            break;
        case 'p':
            chosen_type = PACIFIST;
            break;
        case 'w':
            chosen_type = RISKY;
            break;
        case 'q':
            chosen_type = PATIENT;
            break;
        default:
            chosen_type = AGGRESSIVE;
            break;
    }

    return chosen_type;
}

// gets chosen cow type
COW_TYPE getCowType(){
    clear();

    char input;
    char line[2];
    
    printf("-> DEFAULT\n");
    printf("-> CLUMSY\t(-ACCURACY)\n");
    printf("-> PRECISE\t(+ACCURACY)\n");
    printf("-> SWIFT\t(+DODGE, -BLOCK)\n");
    printf("-> TOUGH\t(+BLOCK, NO DODGE)\n");
    printf("-> HEALER\t(+MAGIC HEAL, -BLOCK, -DODGE)\n");
    printf("-> MAGIC\t(100%% chance MAGIC HEAL, 100%% chance SPECIAL, NO BLOCK, NO DODGE)\n");
    printf("-> BRUTE\t(100%% chance SPECIAL, -MAGIC HEAL, -BLOCK, -DODGE)\n");
    printf("-> GHOST\t(100%% chance DODGE, +MAGIC HEAL, NO BLOCK, NO SPECIAL)\n");
    printf("-> AGENT\t(100%% chance SPECIAL, 100%% chance BLOCK, NO DODGE, NO MAGIC HEAL)\n");

    printf("\nChoose your cow type: ");
    if (fgets(line, sizeof(line), stdin) && line[0] != '\n') {
        input = line[0];
        // discard the rest of the line
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    } else {
        input = 'n'; // n means do nothing
    }

    COW_TYPE chosen_type;

    switch (tolower(input)){ 
        case 'd':
            chosen_type = DEFAULT;
            break;
        case 'c':
            chosen_type = CLUMSY;
            break;
        case 'p':
            chosen_type = PRECISE;
            break;
        case 's':
            chosen_type = SWIFT;
            break;
        case 't':
            chosen_type = TOUGH;
            break;
        case 'h':
            chosen_type = HEALER;
            break;
        case 'm':
            chosen_type = MAGIC;
            break;
        case 'b':
            chosen_type = BRUTE;
            break;
        case 'g':
            chosen_type = GHOST;
            break;
        case 'a':
            chosen_type = AGENT;
            break;
        default:
            chosen_type = DEFAULT;
            break;
    }

    return chosen_type;
}

// gets the player's move
MOVE getInput(){
    char input;
    char line[2];

    printf("\nMoves:\n");
    printf("-> Attack\t(1 Damage)\n");
    printf("-> Block\t(50%% chance of blocking attack)\n");
    printf("-> Heal\t\t(+1 HP)\n");
    printf("-> Special\t(2 Damage, 50%% chance fail)\n");
    printf("-> Dodge\t(75%% chance of dodging attack, 25%% chance of 2x damage)\n");
    printf("-> Magic Heal\t(+3 HP, 50%% chance fail)\n");

    printf("\n[DEBUG ONLY]\n");
    printf("[&]   \tdebug.win\n");
    printf("[*]   \tdebug.lose\n");

    printf("\nChoose your move: ");
    if (fgets(line, sizeof(line), stdin) && line[0] != '\n') {
        input = line[0];
        // discard the rest of the line
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    } else {
        input = 'n'; // n means do nothing
    }

    MOVE pMove;

    switch (tolower(input)){
        case 'a':
        case '1':
            pMove = ATTACK;
            break;
        case 'b':
        case '2':
            pMove = BLOCK;
            break;
        case 'h':
        case '3':
            pMove = HEAL;
            break;
        case 's':
        case '4':
            pMove = SPECIAL;
            break;
        case 'd':
        case '5':
            pMove = DODGE;
            break;
        case 'm':
        case '6':
            pMove = MAGIC_HEAL;
            break;
        case '&':
            pMove = DEBUG_WIN;
            break;
        case '*':
            pMove = DEBUG_LOSE;
            break;
        default:
            pMove = DO_NOTHING;
            break;
    }

    return pMove;
}

// gets the name of an ai type
void getAiTypeName(char *str, AI_TYPE ai_type){
    switch (ai_type){
        case NO_AI:
            strcpy(str, "No AI");
            break;
        case AGGRESSIVE:
            strcpy(str, "Aggressive");
            break;
        case DEFENSIVE:
            strcpy(str, "Defensive");
            break;
        case CAUTIOUS:
            strcpy(str, "Cautious");
            break;
        case RANDOM:
            strcpy(str, "Random");
            break;
        case BERSERKER:
            strcpy(str, "Berserker");
            break;
        case SURVIVOR:
            strcpy(str, "Survivor");
            break;
        case ESCAPIST:
            strcpy(str, "Escapist");
            break;
        case PACIFIST:
            strcpy(str, "Pacifist");
            break;
        case RISKY:
            strcpy(str, "Wild");
            break;
        case PATIENT:
            strcpy(str, "Quiet");
            break;
        default:
            strcpy(str, "Unknown");
            break;
    }
}

// gets the name of a cow type
void getCowTypeName(char *str, COW_TYPE cow_type){
    switch (cow_type){
        case DEFAULT:
            strcpy(str, "Default");
            break;
        case CLUMSY:
            strcpy(str, "Clumsy");
            break;
        case PRECISE:
            strcpy(str, "Precise");
            break;
        case SWIFT:
            strcpy(str, "Swift");
            break;
        case TOUGH:
            strcpy(str, "Tough");
            break;
        case HEALER:
            strcpy(str, "Healer");
            break;
        case MAGIC:
            strcpy(str, "Magic");
            break;
        case BRUTE:
            strcpy(str, "Brute");
            break;
        case GHOST:
            strcpy(str, "Ghost");
            break;
        case AGENT:
            strcpy(str, "Agent");
            break;
        default:
            strcpy(str, "Unknown");
            break;
    }
}

// gets the name of an accuracy stat
void getAccuracyName(char *str, ACCURACY accuracy){
    switch (accuracy){
        case ALWAYS_MISS:
            strcpy(str, "Always Misses");
            break;
        case LOW:
            strcpy(str, "Low");
            break;
        case NORMALA:
            strcpy(str, "Normal");
            break;
        case HIGH:
            strcpy(str, "High");
            break;
        case GUARANTEED:
            strcpy(str, "Guaranteed");
            break;
        default:
            strcpy(str, "Unknown");
            break;
    }
}

// gets the name of move
void getMoveName(char *str, MOVE move){
    switch (move){
        case ATTACK:
            strcpy(str, "Attack");
            break;
        case BLOCK:
            strcpy(str, "Block");
            break;
        case HEAL:
            strcpy(str, "Heal");
            break;
        case SPECIAL:
            strcpy(str, "Special");
            break;
        case DODGE:
            strcpy(str, "Dodge");
            break;
        case MAGIC_HEAL:
            strcpy(str, "Magic Heal");
            break;
        case DO_NOTHING:
            strcpy(str, "Do Nothing");
            break;
        case DEBUG_WIN:
            strcpy(str, "DEBUG WIN");
            break;
        case DEBUG_LOSE:
            strcpy(str, "DEBUG LOSE");
            break;
        default:
            strcpy(str, "Unknown");
            break;
    }
}

// gets modifier as a string
void getModifier(char *str, int modifier){
    if (modifier > 0) {
        sprintf(str, "+%d", modifier);
    } else {
        sprintf(str, "%d", modifier);
    }
}

// handles the player's move
void playerMove(MOVE pMove, struct cow *player, struct cow *enemy){
    // handles player moves
    switch (pMove){
        case DEBUG_WIN: {
            killCow(enemy);
            player->attempted_move = DEBUG_WIN;
            break;
        }
        case DEBUG_LOSE: {
            killCow(player);
            player->attempted_move = DEBUG_LOSE;
            break;
        }
        case ATTACK: {
            player->state = ATTACKING;            
            player->damage = NORMAL;
            player->attempted_move = ATTACK;
            break;
            }
        case BLOCK: {
            player->attempted_move = BLOCK;
            const ACCURACY move_accuracy = findAccuracy(player->cow_type, BLOCK);
            int num;
            switch (move_accuracy){
                case ALWAYS_MISS:
                    player->state = DOING_NOTHING;
                    player->damage = NORMAL;
                    break;
                case LOW:
                    num = rand() % 3; // 1/3 chance
                    if (num == 0) { // 1/3 chance
                        player->state = BLOCKING;
                        player->damage = NONE;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = NORMAL;
                    }
                    break;
                case NORMALA:
                    num = rand() % 2; // 1/2 chance
                    if (num == 0) { // 1/2 chance
                        player->state = BLOCKING;
                        player->damage = NONE;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = NORMAL;
                    }
                    break;
                case HIGH:
                    num = rand() % 3; // 2/3 chance
                    if (num != 0) { // 2/3 chance
                        player->state = BLOCKING;
                        player->damage = NONE;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = NORMAL;
                    }
                    break;
                case GUARANTEED:
                    player->state = BLOCKING;
                    player->damage = NONE;
                    break;
            }
            
            break;
        }
        case HEAL: {
            player->state = HEALING;
            player->damage = NORMAL;
            player->attempted_move = HEAL;
            
            break;
        }
        case SPECIAL: {
            player->attempted_move = SPECIAL;
            const ACCURACY move_accuracy = findAccuracy(player->cow_type, SPECIAL);
            int num;
            switch (move_accuracy){
                case ALWAYS_MISS:
                    player->state = DOING_NOTHING;
                    player->damage = NORMAL;
                    break;
                case LOW:
                    num = rand() % 3; // 1/3 chance
                    if (num == 0) { // 1/3 chance
                        player->state = SPECIAL_ATTACKING;
                        player->damage = NORMAL;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = NORMAL;
                    }
                    break;
                case NORMALA:
                    num = rand() % 2; // 1/2 chance
                    if (num == 0) { // 1/2 chance
                        player->state = SPECIAL_ATTACKING;
                        player->damage = NORMAL;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = NORMAL;
                    }
                    break;
                case HIGH:
                    num = rand() % 3; // 2/3 chance
                    if (num != 0) { // 2/3 chance
                        player->state = SPECIAL_ATTACKING;
                        player->damage = NORMAL;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = NORMAL;
                    }
                    break;
                case GUARANTEED:
                    player->state = SPECIAL_ATTACKING;
                    player->damage = NORMAL;
                    break;
            }
            
            break;
        }
        case DODGE: {
            player->attempted_move = DODGE;
            const ACCURACY move_accuracy = findAccuracy(player->cow_type, DODGE);
            int num;
            switch (move_accuracy){
                case ALWAYS_MISS:
                    player->state = DOING_NOTHING;
                    player->damage = DOUBLE;
                    break;
                case LOW:
                    num = rand() % 2; // 1/2 chance
                    if (num == 0) { // 1/2 chance
                        player->state = DODGING;
                        player->damage = NONE;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = DOUBLE;
                    }
                    break;
                case NORMALA:
                    num = rand() % 4; // 3/4 chance
                    if (num != 0) { // 3/4 chance
                        player->state = DODGING;
                        player->damage = NONE;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = DOUBLE;
                    }
                    break;
                case HIGH:
                    num = rand() % 8; // 7/8 chance
                    if (num != 0) { // 7/8 chance
                        player->state = DODGING;
                        player->damage = NONE;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = DOUBLE;
                    }
                    break;
                case GUARANTEED:
                    player->state = DODGING;
                    player->damage = NONE;
                    break;
            }
            
            break;
        }
        case MAGIC_HEAL: {
            player->attempted_move = MAGIC_HEAL;
            const ACCURACY move_accuracy = findAccuracy(player->cow_type, MAGIC_HEAL);

            int num;
            switch (move_accuracy){
                case ALWAYS_MISS:
                    player->state = DOING_NOTHING;
                    player->damage = NORMAL;
                    break;
                case LOW:
                    num = rand() % 3; // 1/3 chance
                    if (num == 0) { // 1/3 chance
                        player->state = MAGIC_HEALING;
                        player->damage = NORMAL;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = NORMAL;
                    }
                    break;
                case NORMALA:
                    num = rand() % 2; // 1/2 chance
                    if (num == 0) { // 1/2 chance
                        player->state = MAGIC_HEALING;
                        player->damage = NORMAL;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = NORMAL;
                    }
                    break;
                case HIGH:
                    num = rand() % 3; // 2/3 chance
                    if (num != 0) { // 2/3 chance
                        player->state = MAGIC_HEALING;
                        player->damage = NORMAL;
                    } else {
                        player->state = DOING_NOTHING;
                        player->damage = NORMAL;
                    }
                    break;
                case GUARANTEED:
                    player->state = MAGIC_HEALING;
                    player->damage = NORMAL;
                    break;
            }
            
            break;
        }
        default: {
            player->state = DOING_NOTHING;
            player->damage = NORMAL;
            player->attempted_move = DO_NOTHING;

            printf("Player does nothing.\n");
            break;
        }
    }
}

// handles the enemy's move
void enemyMove(struct cow *player, struct cow *enemy){
    MOVE eMove;

    // gets the odds for the enemy's ai type
    const LIST* odds = findAiOdds(enemy->ai_type);
    const LIST* rageOdds = findRageAiOdds(enemy->ai_type);

    // generates a random number between 0 and 99
    int random_num = rand() % 100;

    // checks whether player health is low enough to trigger rage mode
    if (enemy->health <= 5) {
        const int* ro = rageOdds->items;

        if (random_num < ro[0]) {
            eMove = ATTACK;
        } else if (random_num < ro[0] + ro[1]) {
            eMove = BLOCK;
        } else if (random_num < ro[0] + ro[1] + ro[2]) {
            eMove = HEAL;
        } else if (random_num < ro[0] + ro[1] + ro[2] + ro[3]) {
            eMove = SPECIAL;
        } else if (random_num < ro[0] + ro[1] + ro[2] + ro[3] + ro[4]) {
            eMove = DODGE;
        } else {
            eMove = MAGIC_HEAL;
        }
    } else {
        const int* o = odds->items;

        if (random_num < o[0]) {
            eMove = ATTACK;
        } else if (random_num < o[0] + o[1]) {
            eMove = BLOCK;
        } else if (random_num < o[0] + o[1] + o[2]) {
            eMove = HEAL;
        } else if (random_num < o[0] + o[1] + o[2] + o[3]) {
            eMove = SPECIAL;
        } else if (random_num < o[0] + o[1] + o[2] + o[3] + o[4]) {
            eMove = DODGE;
        } else {
            eMove = MAGIC_HEAL;
        }
    }

    switch (eMove){
        case ATTACK: {
            enemy->state = ATTACKING;            
            enemy->damage = NORMAL;
            enemy->attempted_move = ATTACK;
            break;
            }
        case BLOCK: {
            enemy->attempted_move = BLOCK;
            const ACCURACY move_accuracy = findAccuracy(enemy->cow_type, BLOCK);
            int num;
            switch (move_accuracy){
                case ALWAYS_MISS:
                    enemy->state = BLOCKING;
                    enemy->damage = NORMAL;
                    break;
                case LOW:
                    num = rand() % 3;
                    if (num == 0) { // 1/3 chance
                        enemy->state = BLOCKING;
                        enemy->damage = NONE;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = NORMAL;
                    }
                    break;
                case NORMALA:
                    num = rand() % 2;
                    if (num == 0) { // 1/2 chance
                        enemy->state = BLOCKING;
                        enemy->damage = NONE;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = NORMAL;
                    }
                    break;
                case HIGH:
                    num = rand() % 3;
                    if (num != 0) { // 2/3 chance
                        enemy->state = BLOCKING;
                        enemy->damage = NONE;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = NORMAL;
                    }
                    break;
                case GUARANTEED:
                    enemy->state = BLOCKING;
                    enemy->damage = NONE;
                    break;
            }
            
            break;
        }
        case HEAL: {
            enemy->state = HEALING;
            enemy->damage = NORMAL;
            enemy->attempted_move = HEAL;
            
            break;
        }
        case SPECIAL: {
            enemy->attempted_move = SPECIAL;
            const ACCURACY move_accuracy = findAccuracy(enemy->cow_type, SPECIAL);
            int num;
            switch (move_accuracy){
                case ALWAYS_MISS:
                    enemy->state = DOING_NOTHING;
                    enemy->damage = NORMAL;
                    break;
                case LOW:
                    num = rand() % 3;
                    if (num == 0) { // 1/3 chance
                        enemy->state = SPECIAL_ATTACKING;
                        enemy->damage = NORMAL;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = NORMAL;
                    }
                    break;
                case NORMALA:
                    num = rand() % 2;
                    if (num == 0) { // 1/2 chance
                        enemy->state = SPECIAL_ATTACKING;
                        enemy->damage = NORMAL;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = NORMAL;
                    }
                    break;
                case HIGH:
                    num = rand() % 3;
                    if (num != 0) { // 2/3 chance
                        enemy->state = SPECIAL_ATTACKING;
                        enemy->damage = NORMAL;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = NORMAL;
                    }
                    break;
                case GUARANTEED:
                    enemy->state = SPECIAL_ATTACKING;
                    enemy->damage = NORMAL;
                    break;
            }
            
            break;
        }
        case DODGE: {
            enemy->attempted_move = DODGE;
            const ACCURACY move_accuracy = findAccuracy(enemy->cow_type, DODGE);
            int num;
            switch (move_accuracy){
                case ALWAYS_MISS:
                    enemy->state = DOING_NOTHING;
                    enemy->damage = DOUBLE;
                    break;
                case LOW:
                    num = rand() % 2;
                    if (num == 0) { // 1/2 chance
                        enemy->state = DODGING;
                        enemy->damage = NONE;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = DOUBLE;
                    }
                    break;
                case NORMALA:
                    num = rand() % 4;
                    if (num != 0) { // 3/4 chance
                        enemy->state = DODGING;
                        enemy->damage = NONE;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = DOUBLE;
                    }
                    break;
                case HIGH:
                    num = rand() % 8;
                    if (num != 0) { // 7/8 chance
                        enemy->state = DODGING;
                        enemy->damage = NONE;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = DOUBLE;
                    }
                    break;
                case GUARANTEED:
                    enemy->state = DODGING;
                    enemy->damage = NONE;
                    break;
            }
            
            break;
        }
        case MAGIC_HEAL: {
            enemy->attempted_move = MAGIC_HEAL;
            const ACCURACY move_accuracy = findAccuracy(enemy->cow_type, MAGIC_HEAL);

            int num;
            switch (move_accuracy){
                case ALWAYS_MISS:
                    enemy->state = DOING_NOTHING;
                    enemy->damage = NORMAL;
                    break;
                case LOW:
                    num = rand() % 3;
                    if (num == 0) { // 1/3 chance
                        enemy->state = MAGIC_HEALING;
                        enemy->damage = NORMAL;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = NORMAL;
                    }
                    break;
                case NORMALA:
                    num = rand() % 2;
                    if (num == 0) { // 1/2 chance
                        enemy->state = MAGIC_HEALING;
                        enemy->damage = NORMAL;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = NORMAL;
                    }
                    break;
                case HIGH:
                    num = rand() % 3;
                    if (num != 0) { // 2/3 chance
                        enemy->state = MAGIC_HEALING;
                        enemy->damage = NORMAL;
                    } else {
                        enemy->state = DOING_NOTHING;
                        enemy->damage = NORMAL;
                    }
                    break;
                case GUARANTEED:
                    enemy->state = MAGIC_HEALING;
                    enemy->damage = NORMAL;
                    break;
            }
            
            break;
        }
        default: {
            enemy->state = DOING_NOTHING;
            enemy->damage = NORMAL;
            enemy->attempted_move = DO_NOTHING;

            printf("Player does nothing.\n");
            break;
        }
    }
}

void printStart(){
    clear();

    printf("  ______     ___   .___________.___________. __       _______ .______     ______   .___________.    _______.\n");
    printf(" /      |   /   \\  |           |           ||  |     |   ____||   _  \\   /  __  \\  |           |   /       |\n");
    printf("|  ,----'  /  ^  \\ `---|  |----`---|  |----`|  |     |  |__   |  |_)  | |  |  |  | `---|  |----`  |   (----`\n");
    printf("|  |      /  /_\\  \\    |  |        |  |     |  |     |   __|  |   _  <  |  |  |  |     |  |        \\   \\    \n");
    printf("|  `----./  _____  \\   |  |        |  |     |  `----.|  |____ |  |_)  | |  `--'  |     |  |    .----)   |   \n");
    printf(" \\______/__/     \\__\\  |__|        |__|     |_______||_______||______/   \\______/      |__|    |_______/    \n");
    printf("                                                                                                            \n");
    printf("\n");
}

void printSettings(){
    clear();
    
    printf("     _______. _______ .___________.___________. __  .__   __.   _______      _______.\n");
    printf("    /       ||   ____||           |           ||  | |  \\ |  |  /  _____|    /       |\n");
    printf("   |   (----`|  |__   `---|  |----`---|  |----`|  | |   \\|  | |  |  __     |   (----`\n");
    printf("    \\   \\    |   __|      |  |        |  |     |  | |  . `  | |  | |_ |     \\   \\    \n");
    printf(".----)   |   |  |____     |  |        |  |     |  | |  |\\   | |  |__| | .----)   |   \n");
    printf("|_______/    |_______|    |__|        |__|     |__| |__| \\__|  \\______| |_______/    \n");
    printf("                                                                                     \n");
    printf("\n");
}

void printBattlefield(struct cow *player, struct cow *enemy){
    clear();

    struct cow default_player = { 0, 0, 10, DO_NOTHING, DOING_NOTHING, NORMAL, NO_AI, player->cow_type };
    struct cow default_enemy = { 0, 0, 10, DO_NOTHING, DOING_NOTHING, NORMAL, enemy->ai_type, enemy->cow_type };
    
    printCow(&default_player);
    printEnemyCow(&default_enemy);

    printHealth(player->health, enemy->health);
}

void printCalculations(struct cow *player, struct cow *enemy){
    clear();

    printCow(player);
    printEnemyCow(enemy);

    if (player->state != DEAD && enemy->state != DEAD){
        int enemyDamageMultiplier;
        int playerDamageMultiplier;

        if (enemy->damage == DOUBLE) {
            enemyDamageMultiplier = 2;
        } else if (enemy->damage == NONE) {
            enemyDamageMultiplier = 0;
        } else {
            enemyDamageMultiplier = 1;        
        }

        if (player->damage == DOUBLE) {
            playerDamageMultiplier = 2;
        } else if (player->damage == NONE) {
            playerDamageMultiplier = 0;
        } else {
            playerDamageMultiplier = 1;        
        }

        switch (player->state){
            case ATTACKING:
                enemy->health -= enemyDamageMultiplier;
                break;
            case HEALING:
                player->health += 1; // heals player by 1 health
                break;
            case SPECIAL_ATTACKING:
                enemy->health -= 2 * enemyDamageMultiplier;
                break;
            case MAGIC_HEALING:
                player->health += 3; // heals player by 3 health
                break;
        }

        switch (enemy->state){
            case ATTACKING:
                player->health -= playerDamageMultiplier;
                break;
            case HEALING:
                enemy->health += 1; // heals enemy by 1 health
                break;
            case SPECIAL_ATTACKING:
                player->health -= 2 * playerDamageMultiplier;
                break;
            case MAGIC_HEALING:
                enemy->health += 3; // heals enemy by 3 health
                break;
        }
    
        player->health = correctHealth(player->health);
        enemy->health = correctHealth(enemy->health);
    }

    printHealth(player->health, enemy->health);

    if (player->health <= 0) {
        killCow(player);

        printf("\nPlayer is dead!\n\n");
        return;
    }

    if (enemy->health <= 0) {
        killCow(enemy);

        printf("\nEnemy is dead!\n\n");
        return;
    }

    if (player->state != DEAD && enemy->state != DEAD){
        int enemyDamageMultiplier;
        int playerDamageMultiplier;

        if (enemy->damage == DOUBLE) {
            enemyDamageMultiplier = 2;
        } else if (enemy->damage == NONE) {
            enemyDamageMultiplier = 0;
        } else {
            enemyDamageMultiplier = 1;        
        }

        if (player->damage == DOUBLE) {
            playerDamageMultiplier = 2;
        } else if (player->damage == NONE) {
            playerDamageMultiplier = 0;
        } else {
            playerDamageMultiplier = 1;        
        }

        switch (player->state){
            case ATTACKING:
                printf("Player attacks! Enemy takes %i damage!\n", enemyDamageMultiplier);
                break;
            case BLOCKING:
                printf("Player is blocking!\n");
                break;
            case HEALING:
                printf("Player heals! Player gains 1 health!\n");
                break;
            case SPECIAL_ATTACKING:
                printf("Player uses special move! Enemy takes %i damage!\n", 2 * enemyDamageMultiplier);
                break;
            case DODGING:
                printf("Player dodges!\n");
                break;
            case MAGIC_HEALING:
                printf("Player uses magic heal! Player gains 3 health!\n");
                break;
            default:
                switch (player->attempted_move){
                    case BLOCK:
                        printf("Player tries to block, but fails!\n");
                        break;
                    case SPECIAL:
                        printf("Player tries to use special move, but fails!\n");
                        break;
                    case DODGE:
                        printf("Player tries to dodge, but fails!\n");
                        break;
                    case MAGIC_HEAL:
                        printf("Player tries to use magic heal, but fails!\n");
                        break;
                    default:
                        printf("Player does nothing.\n");
                        break;
                }
                break;
        }

        switch (enemy->state){
            case ATTACKING:
                printf("Enemy attacks! Player takes %i damage!\n", playerDamageMultiplier);
                break;
            case BLOCKING:
                printf("Enemy is blocking!\n");
                break;
            case HEALING:
                printf("Enemy heals! Enemy gains 1 health!\n");
                break;
            case SPECIAL_ATTACKING:
                printf("Enemy uses special move! Player takes %i damage!\n", 2 * playerDamageMultiplier);
                break;
            case DODGING:
                printf("Enemy dodges!\n");
                break;
            case MAGIC_HEALING:
                printf("Enemy uses magic heal! Enemy gains 3 health!\n");
                break;
            default:
                switch (enemy->attempted_move){
                    case BLOCK:
                        printf("Enemy tries to block, but fails!\n");
                        break;
                    case SPECIAL:
                        printf("Enemy tries to use special move, but fails!\n");
                        break;
                    case DODGE:
                        printf("Enemy tries to dodge, but fails!\n");
                        break;
                    case MAGIC_HEAL:
                        printf("Enemy tries to use magic heal, but fails!\n");
                        break;
                    default:
                        printf("Enemy does nothing.\n");
                        break;
                }
                break;
        }        
    }
}

// handles the battle mechanics
void battle(struct cow *player, struct cow *enemy){
    printBattlefield(player, enemy);

    playerMove(getInput(), player, enemy);

    enemyMove(player, enemy);

    if (player->state != DEAD && enemy->state != DEAD){
        printCalculations(player, enemy);

        waitForEnter();
    }
}

int main(){
    srand(time(NULL)); // sets random seed

    COW_TYPE playerCowType = DEFAULT;

    AI_TYPE enemyComType = AGGRESSIVE;
    COW_TYPE enemyCowType = DEFAULT;
    
    GAME_SCREEN screen = MENU;

    while (1) {
        while (screen == MENU){
            printStart();

            char input;
            char line[2];

            printf("-> Play\n");
            printf("-> Settings\n");
            printf("-> Quit\n");

            printf("\nChoose an option: ");

            if (fgets(line, sizeof(line), stdin) && line[0] != '\n') {
                input = line[0];
                // discard the rest of the line
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
            } else {
                input = 'n'; // n means do nothing
            }

            switch (tolower(input)){
                case 'p':
                    screen = BATTLE;
                    break;
                case 's':
                    screen = SETTINGS;
                    break;
                case 'q':
                    return 0;
                default:
                    break;
            }
        }
        while (screen == SETTINGS){
            printSettings();

            char input;
            char line[2];

            char aiTypeName[12];
            char playerCowTypeName[12];
            char enemyCowTypeName[12];

            getAiTypeName(aiTypeName, enemyComType);
            getCowTypeName(playerCowTypeName, playerCowType);
            getCowTypeName(enemyCowTypeName, enemyCowType);
            
            printf("-> Robot Strategy\t(%s)\n", aiTypeName);
            printf("-> Player Cow Type\t(%s)\n", playerCowTypeName);
            printf("-> Enemy Cow Type\t(%s)\n", enemyCowTypeName);
            printf("-> Back\n");

            printf("\nChoose an option: ");

            if (fgets(line, sizeof(line), stdin) && line[0] != '\n') {
                input = line[0];
                // discard the rest of the line
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
            } else {
                input = 'n'; // n means do nothing
            }

            switch (tolower(input)){
                case 'r':
                    enemyComType = getComType();
                    break;
                case 'p':
                    playerCowType = getCowType();
                    break;
                case 'e':
                    enemyCowType = getCowType();
                    break;
                case 'b':
                    screen = MENU;
                    break;
                default:
                    break;
            }
        }
        while (screen == BATTLE){
            // creates cows
            struct cow player = { 0, 0, 10, DO_NOTHING, DOING_NOTHING, NORMAL, NO_AI, playerCowType };
            struct cow enemy = { 0, 0, 10, DO_NOTHING, DOING_NOTHING, NORMAL, enemyComType, enemyCowType };

            // loops until either cow dies
            while (player.state != DEAD && enemy.state != DEAD){
                battle(&player, &enemy);
            }

            // win/lose screen
            printCalculations(&player, &enemy);

            if (player.state == DEAD) {
                printf("You lose!\n");
            } else {
                printf("You win!\n");
            }

            char input;
            char line[2];

            printf("\nWould you like to play again? (y/n): ");

            if (fgets(line, sizeof(line), stdin) && line[0] != '\n') {
                input = line[0];
                // discard the rest of the line
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
            } else {
                input = 'n'; // n means back to menu
            }

            if (tolower(input) != 'y') {
                screen = MENU;
            }
        }
    }
    
    return 0;
}
