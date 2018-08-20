#include <Arduboy.h>
#include "bitmaps.h"

Arduboy arduboy;

// Game variables
// const int DEFAULT_COLUMNS = 24; //128 - 4 * 2 (2 walls + 1px) = 120 / 5 = 24
const int COLUMNS = 24; //128 - 4 * 2 (2 walls + 1px) = 120 / 5 = 24
const int LINES = 12;
const int PADDING = 4;
const int WALLSIZE = 3;
const int BLOCKSIZE = 5;
const unsigned char* skinList[][3] = {
    {marshm, marshmL, marshmR}, 
    {rabbit, rabbitL, rabbitR}, 
    {knight, knightL, knightR}, 
    {viking, vikingL, vikingR}, 
    {dog, dogL, dogR}
};
const unsigned char* blockList[] = {
    block, spike, bomb, bombTicking
};

// int COLUMNS;
int gameState;
int i, j;
int timer;
int blockFallLevel;
int tempoBlockX, tempoBlockY;
int gameLevel;
int blockType;
int playerX, playerY;
int counter;
int skin;
int currentSkin;
int wallGap;
int score;

bool justPressed;
bool blockIsFalling;
bool playerIsFalling;
bool playerIsJumping;
bool gameOver;

int matrix[LINES][COLUMNS];

void skinSlection()
{
    if (arduboy.pressed(LEFT_BUTTON) && !justPressed) {
        arduboy.drawBitmap(WIDTH/2 - 2 - BLOCKSIZE*2 - 1, 50 - 1, arrowLx2, 4, 7, WHITE);
        skin--;
        justPressed = true;
    } else if (arduboy.pressed(RIGHT_BUTTON) && !justPressed) {
        arduboy.drawBitmap(WIDTH/2 - 2 + BLOCKSIZE*2 + 1, 50 - 1, arrowRx2, 4, 7, WHITE);
        skin++;
        justPressed = true;
    }
    if (skin < 0) {
        skin = 4;
    } else if (skin > 4) {
        skin = 0;
    }
}

void wallSizeSelection()
{
    if (arduboy.pressed(UP_BUTTON) && !justPressed) {
        if (wallGap < BLOCKSIZE * 6) {
            wallGap += 5;
        }
        justPressed = true;
    } else if (arduboy.pressed(DOWN_BUTTON) && !justPressed) {
        if (wallGap >= BLOCKSIZE) {
            wallGap -= 5;
        }
        justPressed = true;
    }
}

void drawMenu()
{
    arduboy.drawBitmap(PADDING - WALLSIZE + wallGap, 0, wallL, WALLSIZE, HEIGHT, WHITE);
    arduboy.drawBitmap(WIDTH - PADDING - wallGap, 0, wallR, WALLSIZE, HEIGHT, WHITE);

    arduboy.drawBitmap(WIDTH/2 - 2, 50, skinList[skin][0], BLOCKSIZE, BLOCKSIZE, WHITE);

    arduboy.drawBitmap(WIDTH/2 - 2 - BLOCKSIZE*2, 50, arrowL, 3, 5, WHITE);
    arduboy.drawBitmap(WIDTH/2 - 2 + BLOCKSIZE*2 + 1, 50, arrowR, 3, 5, WHITE);

    skinSlection();

    wallSizeSelection();

    arduboy.setCursor(10, 10);
    arduboy.print("pit");

    if (arduboy.notPressed(LEFT_BUTTON) && arduboy.notPressed(RIGHT_BUTTON) && arduboy.notPressed(UP_BUTTON) && arduboy.notPressed(DOWN_BUTTON) && arduboy.notPressed(A_BUTTON) && arduboy.notPressed(B_BUTTON))
        justPressed = false;
}

void initGame()
{
    timer = 0;
    blockFallLevel = 0;
    tempoBlockY = -5;
    tempoBlockX = 0;
    gameLevel = 0;
    blockIsFalling = false;
    blockType = 0;

    for (i = 0; i < LINES; i++) {
        for (j = 0; j < COLUMNS; j++) {
            if (i >= 10  && j >= 6 && j < 18) {
                matrix[i][j] = 1;
            } else {
                matrix[i][j] = 0;
            }
        }
    }

    playerX = WIDTH/2 - 2;
    playerY = HEIGHT/2 - 2;
    playerIsFalling = true;
    playerIsJumping = false;
    currentSkin = 0;
    gameOver = false;
    counter = 0;
    score = 0;
}

void drawGame()
{
    //Draw walls
    arduboy.drawBitmap(PADDING - WALLSIZE, 0, wallL, WALLSIZE, HEIGHT, WHITE);
    arduboy.drawBitmap(WIDTH - PADDING, 0, wallR, WALLSIZE, HEIGHT, WHITE);
    
    //Draw the matrix
    for (i = 0; i < LINES; i++) {
        for (j = 0; j < COLUMNS; j++) {
            if (matrix[i][j] != 0) {
                arduboy.drawBitmap(PADDING + j*BLOCKSIZE, PADDING + i*BLOCKSIZE + (matrix[i][j]%100)/10, blockList[(matrix[i][j]-1) % 10 + (matrix[i][j]/100) % 2], BLOCKSIZE, BLOCKSIZE, WHITE);
            } 
        }
    }

    //Draw the falling block
    if (blockIsFalling) {
        arduboy.drawBitmap(tempoBlockX, tempoBlockY, blockList[blockType-1], BLOCKSIZE, BLOCKSIZE, WHITE);
    }

    //Draw the player
    arduboy.drawBitmap(playerX, playerY, skinList[skin][currentSkin], BLOCKSIZE, BLOCKSIZE, WHITE);
}

void checkGameOver()
{
      //If has fallen
    if (playerY >= HEIGHT) {
        gameOver = true;
    } //If under a spike
    else if (matrix[(playerY+BLOCKSIZE-PADDING-blockFallLevel)/BLOCKSIZE][(playerX+2-PADDING)/BLOCKSIZE] % 10 == 2) {
        gameOver = true;
    } //If only 2 pixels are on a spike
    else if (matrix[(playerY+BLOCKSIZE-PADDING-blockFallLevel)/BLOCKSIZE][(playerX+2-PADDING)/BLOCKSIZE] % 10 == 0 && (matrix[(playerY+BLOCKSIZE-PADDING-blockFallLevel)/BLOCKSIZE][(playerX+BLOCKSIZE-1-PADDING)/BLOCKSIZE] % 10 == 2 || matrix[(playerY+BLOCKSIZE-PADDING-blockFallLevel)/BLOCKSIZE][(playerX-PADDING)/BLOCKSIZE] % 10 == 2)) {
        gameOver = true;
    } //If near the walls
    else if (playerX == PADDING || playerX + BLOCKSIZE-1 == WIDTH - (PADDING + 1)) {
        gameOver = true;
    } //If crushed by a block
    else if (arduboy.getPixel(playerX, playerY - 1) == 1 || arduboy.getPixel(playerX + BLOCKSIZE-1, playerY - 1) == 1) {
        gameOver = true;
    } else {
        gameOver = false;
    }
}

void blockGenerator()
{
    //One block at a time
    if (!blockIsFalling && timer % (60 + 1 - (gameLevel * 2 / 3)) == 0 && timer >= 60) {

        //Prevent adding a block where it's full
        do {
            i = random(COLUMNS);
        } while (matrix[0][i] != 0); 

        //Set X and Y
        tempoBlockX = (i*BLOCKSIZE) + PADDING;
        tempoBlockY = -5;
        blockIsFalling = true;

        //Random type of the block
        i = random(100);
        if (i < 5) {
            blockType = 3;
        } else if (i < 25) {
            blockType = 2;
        } else {
            blockType = 1;
        }

    //If block created, make it fall until near the matrix
    } else if (blockIsFalling && tempoBlockY < PADDING + blockFallLevel) {
        tempoBlockY++;

    //Then add it to it
    } else if (blockIsFalling) {
        matrix[(tempoBlockY - PADDING - blockFallLevel)/BLOCKSIZE][(tempoBlockX - PADDING)/BLOCKSIZE] = blockType + blockFallLevel*10;
        blockIsFalling = false;
    }
}

void checkBombsStatue()
{
    for (i = 0; i < LINES; i++) {
        for (j = 0; j < COLUMNS; j++) {

            //If bomb activated
            if (matrix[i][j] % 10 == 3 && matrix[i][j] / 100 != 0) {

                //Tick
                if (matrix[i][j] / 100 < 7) {
                    if (timer % 20 == 0) {
                        matrix[i][j] += 100;
                    }
                } 

                //If ready to explode, boom !
                else {
                    boom();
                }
            }
        }
    }
}

void boom()
{
    int g, h;
    for (g = i-1; g <= i+1; g++) {
        for (h = j-1; h <= j+1; h++) {
            if (g >= 0 && h >= 0 && g < LINES && h < COLUMNS) {
                matrix[g][h] = 0;
            }

            //If player near the explosion
            if (playerX >= j*BLOCKSIZE + PADDING - BLOCKSIZE - 4 && playerX <= j*BLOCKSIZE + PADDING + BLOCKSIZE + 4 && playerY >= i*BLOCKSIZE + PADDING + blockFallLevel - BLOCKSIZE - 4 && playerY <= i*BLOCKSIZE + PADDING + blockFallLevel + BLOCKSIZE + 4) {
                gameOver = true;
            }
        }
    }
}

void dropBlockLevel()
{
    for (i = LINES-1; i >= 0; i--) {
        for (j = 0; j < COLUMNS; j++) {
            if (matrix[i][j] != 0) {
                if (matrix[i][j] % 100 != 40 + (matrix[i][j] % 10)) {
                    matrix[i][j] = matrix[i][j] + 10;
                } else if (i != LINES-1) {
                    matrix[i+1][j] = matrix[i][j] % 10 + (matrix[i][j]/100) * 100;
                    matrix[i][j] = 0;
                } else {
                    matrix[i][j] = 0;
                }
            }
        }
    }

    blockFallLevel++;
    if (blockFallLevel == 5) {
        blockFallLevel = 0;
    }
}

void blocksGravity()
{
    for (i = LINES-1; i >= 0; i--) {
        for (j = 0; j < COLUMNS; j++) {
            if (matrix[i][j] != 0) {
                if (i == LINES-1) {
                    if ((matrix[i][j] % 100) / 10 != blockFallLevel) {
                        matrix[i][j] = matrix[i][j] + 10;
                    }
                } else {
                    if (arduboy.getPixel(j*BLOCKSIZE + PADDING + 2, i*BLOCKSIZE + PADDING + BLOCKSIZE + (matrix[i][j] % 100) / 10) == 0) {
                        if (matrix[i][j] % 100 != 40 + (matrix[i][j] % 10)) {
                            matrix[i][j] = matrix[i][j] + 10;
                        } else {
                            matrix[i+1][j] = matrix[i][j] % 10 + (matrix[i][j]/100) * 100;
                            matrix[i][j] = 0;
                        }
                    }
                }
            }
        }
    }
}

void checkIfNearABomb(int a, bool aBis, int b, bool bBis)
{
    bool doneChecking = false;

    i = 0;
    int c = 0;
    int d = 0;

    if (aBis) {
        c = 4;
    } else {
        d = 4;
    }

    while (!(doneChecking || i == 2)) {
        if (arduboy.getPixel(playerX + a + i*c, playerY + b + i*d) != 0) {
            if (matrix[(playerY + b + i*d - blockFallLevel - PADDING)/BLOCKSIZE][(playerX + a + i*c - PADDING)/BLOCKSIZE] % 10 == 3) {
                if (matrix[(playerY + b + i*d - blockFallLevel - PADDING)/BLOCKSIZE][(playerX + a + i*c - PADDING)/BLOCKSIZE] / 100 == 0) {
                    matrix[(playerY + b + i*d - blockFallLevel - PADDING)/BLOCKSIZE][(playerX + a + i*c - PADDING)/BLOCKSIZE] += 100;
                    doneChecking = true;
                }
            }
        }
        i++;
    }
}

void playersGravity()
{
    if (!playerIsJumping && arduboy.getPixel(playerX, playerY + BLOCKSIZE) == 0 && arduboy.getPixel(playerX + BLOCKSIZE-1, playerY + BLOCKSIZE) == 0) {
        playerIsFalling = true;
    } else {
        //Check if on top of a bomb
        checkIfNearABomb(0, true, 5, false);

        playerIsFalling = false;
    }

    if (playerIsFalling) {
        playerY++;
    } else if ((playerY + BLOCKSIZE) >= HEIGHT) {
        playerY++;
    } else {
        playerIsFalling = false;
    }
}

void jumping()
{
    if (arduboy.pressed(B_BUTTON) && justPressed == 0 && !playerIsJumping && !playerIsFalling) {
        playerIsJumping = true;
    }
    if (playerIsJumping) {
        if (counter <= BLOCKSIZE*2) {
            playerY--;
            counter++;
        } else {
            playerIsJumping = false;
            counter = 0;
        }
    }
}

void moving()
{
    int direction;

    if (arduboy.pressed(LEFT_BUTTON) && arduboy.getPixel(playerX - 1, playerY + 4) == 0 && arduboy.getPixel(playerX - 1, playerY) == 0) {
        currentSkin = 1;
        direction = -1;
    } else if (arduboy.pressed(RIGHT_BUTTON) && arduboy.getPixel(playerX + 5, playerY + 4) == 0 && arduboy.getPixel(playerX + 5, playerY) == 0) {
        currentSkin = 2;
        direction = 1;
    } else {
        //Check if colliding with a bomb
        checkIfNearABomb(5, false, 0, true);
        checkIfNearABomb(-1, false, 0, true);

        currentSkin = 0;
        direction = 0;
    }

    playerX += direction;
}

void setup() 
{
    arduboy.begin();

    //Random seed
    srand(7/8); 

    //Set framerate at 60 
    arduboy.setFrameRate(60);

    i = 0;
    j = 0;
    gameState = 0;
    currentSkin = 0;
    skin = 0;
    wallGap = 0;
    // COLUMNS = 24;
}

void loop()
{

    if (!arduboy.nextFrame())
        return;

    arduboy.clear();

    switch(gameState) {
        case 0:
            //Title screen
            drawMenu();

            if (arduboy.pressed(A_BUTTON) && !justPressed) {
                initGame();

                justPressed = true;
                gameState = 1;
            }
            break;
        
        case 1:
            //Gameplay screen
            drawGame();

            timer++;

            checkGameOver();

            blockGenerator();

            checkBombsStatue();

            if (timer % (180 - gameLevel) == 0) {
                dropBlockLevel();

                if (gameLevel < 90) {
                    gameLevel++;
                }

                score += 100;
            } else {
                blocksGravity();
            }

            playersGravity();

            jumping();

            moving();

            if (gameOver) {
                gameState = 3;
            }

            //Go back to menu
            if (arduboy.pressed(A_BUTTON) && !justPressed) {
                justPressed = true;
                gameState = 2;
            }

            break;

        case 2:
            //Win screen
            arduboy.setCursor(10, 10);
            arduboy.print("Pause");

            if (arduboy.pressed(A_BUTTON) && !justPressed) {
                justPressed = true;
                gameState = 1;
            }

            break;

        case 3:
            //Gameover screen
            arduboy.setCursor(10, 10);
            arduboy.print("Gameover Screen");

            arduboy.setCursor(10, 40);
            arduboy.print(score);

            if (arduboy.pressed(A_BUTTON) && !justPressed) {
                justPressed = true;
                gameState = 0;
            }

            break;
    }

    if (arduboy.notPressed(A_BUTTON) && arduboy.notPressed(B_BUTTON) && gameState != 0)
        justPressed = false;

    arduboy.display();
}
