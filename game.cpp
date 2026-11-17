#include "raylib.h"
#include <vector>
#include <string>
#include <fstream>
#include <map>

using namespace std;

// --- 1. Lightweight Database ---
class Database
{
public:
    map<string, string> users;
    struct Stats
    {
        int played = 0, won = 0, lost = 0;
        float bestEasy = 9999.0f, bestMedium = 9999.0f, bestHard = 9999.0f;
    };
    map<string, Stats> records;

    Database()
    {
        ifstream f1("security.txt");
        string u, p;
        while (f1 >> u >> p)
            users[u] = p;
        f1.close();
        ifstream f2("record.txt");
        Stats s;
        while (f2 >> u >> s.played >> s.won >> s.lost >> s.bestEasy >> s.bestMedium >> s.bestHard)
            records[u] = s;
        f2.close();
    }

    void saveAll()
    {
        ofstream f1("security.txt");
        for (auto const &pair : users)
            f1 << pair.first << " " << pair.second << "\n";
        f1.close();
        ofstream f2("record.txt");
        for (auto const &pair : records)
            f2 << pair.first << " " << pair.second.played << " " << pair.second.won << " " << pair.second.lost << " " << pair.second.bestEasy << " " << pair.second.bestMedium << " " << pair.second.bestHard << "\n";
        f2.close();
    }

    string attemptAuth(string u, string p, bool isSignup)
    {
        if (u.empty() || p.empty())
            return "Fields cannot be empty!";
        if (isSignup)
        {
            if (users.count(u))
                return "User exists!";
            if (p.length() < 8)
                return "Password: Min 8 chars!";
            users[u] = p;
            records[u] = Stats();
            saveAll();
            return "Signup Success! Login now.";
        }
        return (users.count(u) && users[u] == p) ? "SUCCESS" : "Invalid User or Password!";
    }

    void updateRecord(string u, int level, bool isWin, float time)
    {
        records[u].played++;
        if (isWin)
        {
            records[u].won++;
            if (level == 1 && time < records[u].bestEasy)
                records[u].bestEasy = time;
            else if (level == 2 && time < records[u].bestMedium)
                records[u].bestMedium = time;
            else if (level == 3 && time < records[u].bestHard)
                records[u].bestHard = time;
        }
        else
            records[u].lost++;
        saveAll();
    }
};

// --- 2. Optimized Minesweeper Game Logic ---
struct Cell
{
    bool isMine = false, isRev = false, isFlag = false;
    int num = 0;
};

class Minesweeper
{
private:
    int rows, cols, totalMines, cellSize;
    vector<vector<Cell>> grid;
    bool isGameOver = false, isGameWon = false, firstClick = true;
    float startTime = 0, finalTime = 0;
    int revCount = 0, flagCount = 0;

public:
    int difficultyLevel;
    bool wantsRestart = false, wantsMenu = false;

    Minesweeper(int level)
    {
        difficultyLevel = level;
        if (level == 1)
        {
            rows = 10;
            cols = 10;
            totalMines = 10;
            cellSize = 70;
        }
        else if (level == 2)
        {
            rows = 16;
            cols = 16;
            totalMines = 40;
            cellSize = 50;
        }
        else
        {
            rows = 16;
            cols = 30;
            totalMines = 99;
            cellSize = 45;
        }
        grid.assign(rows, vector<Cell>(cols));
        startTime = GetTime();
    }

    void placeMines(int fR, int fC)
    {
        int placed = 0;
        while (placed < totalMines)
        {
            int r = GetRandomValue(0, rows - 1), c = GetRandomValue(0, cols - 1);
            if (!grid[r][c].isMine && (abs(r - fR) > 1 || abs(c - fC) > 1))
            {
                grid[r][c].isMine = true;
                placed++;
                for (int dr = -1; dr <= 1; dr++)
                    for (int dc = -1; dc <= 1; dc++)
                        if (r + dr >= 0 && r + dr < rows && c + dc >= 0 && c + dc < cols)
                            grid[r + dr][c + dc].num++;
            }
        }
    }

    void revealCell(int r, int c)
    {
        if (r < 0 || r >= rows || c < 0 || c >= cols || grid[r][c].isRev || grid[r][c].isFlag)
            return;
        if (firstClick)
        {
            placeMines(r, c);
            firstClick = false;
            startTime = GetTime();
        }

        grid[r][c].isRev = true;
        revCount++;
        if (grid[r][c].isMine)
        {
            isGameOver = true;
            isGameWon = false;
            finalTime = GetTime() - startTime;
            return;
        }
        if (revCount == (rows * cols) - totalMines)
        {
            isGameOver = true;
            isGameWon = true;
            finalTime = GetTime() - startTime;
        }

        if (grid[r][c].num == 0)
            for (int dr = -1; dr <= 1; dr++)
                for (int dc = -1; dc <= 1; dc++)
                    revealCell(r + dr, c + dc);
    }

    void updateAndDraw(Database &db, string user)
    {
        int offX = (1600 - cols * cellSize) / 2, offY = (900 - rows * cellSize) / 2 + 30;

        if (!isGameOver)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
            {
                int c = (GetMouseX() - offX) / cellSize, r = (GetMouseY() - offY) / cellSize;
                if (r >= 0 && r < rows && c >= 0 && c < cols)
                {
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                        revealCell(r, c);
                    else if (!grid[r][c].isRev)
                    {
                        grid[r][c].isFlag = !grid[r][c].isFlag;
                        flagCount += grid[r][c].isFlag ? 1 : -1;
                    }
                    if (isGameOver)
                        db.updateRecord(user, difficultyLevel, isGameWon, finalTime);
                }
            }
        }
        else
        {
            if (IsKeyPressed(KEY_R))
                wantsRestart = true;
            if (IsKeyPressed(KEY_M))
                wantsMenu = true;
        }

        DrawText(TextFormat("Mines Left: %d", totalMines - flagCount), 100, 40, 35, DARKBLUE);
        DrawText(TextFormat("Time: %.1f s", firstClick ? 0 : (isGameOver ? finalTime : GetTime() - startTime)), 1300, 40, 35, DARKBLUE);

        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < cols; c++)
            {
                Rectangle rect = {(float)(offX + c * cellSize), (float)(offY + r * cellSize), (float)cellSize, (float)cellSize};
                Cell cell = grid[r][c];

                if (cell.isRev)
                {
                    DrawRectangleRec(rect, LIGHTGRAY);
                    if (cell.isMine)
                        DrawCircle(rect.x + cellSize / 2, rect.y + cellSize / 2, cellSize / 3, RED);
                    else if (cell.num > 0)
                        DrawText(TextFormat("%d", cell.num), rect.x + cellSize / 3, rect.y + cellSize / 5, cellSize / 1.5, BLUE);
                }
                else
                {
                    DrawRectangleRec(rect, DARKGRAY);
                    if (isGameOver && !isGameWon)
                    {
                        if (cell.isMine && !cell.isFlag)
                            DrawCircle(rect.x + cellSize / 2, rect.y + cellSize / 2, cellSize / 3, BLACK);
                        else if (!cell.isMine && cell.isFlag)
                            DrawText("X", rect.x + cellSize / 3, rect.y + cellSize / 5, cellSize / 1.5, BLACK);
                        else if (cell.isMine && cell.isFlag)
                            DrawText("F", rect.x + cellSize / 3, rect.y + cellSize / 5, cellSize / 1.5, DARKGREEN);
                    }
                    else if (cell.isFlag)
                        DrawText("F", rect.x + cellSize / 3, rect.y + cellSize / 5, cellSize / 1.5, RED);
                }
                DrawRectangleLinesEx(rect, 1, cell.isRev ? GRAY : BLACK);
            }
        }

        if (isGameOver)
        {
            DrawRectangle(550, 350, 500, 180, Fade(BLACK, 0.8f));
            DrawText(isGameWon ? "YOU WIN!" : "GAME OVER!", 700, 380, 40, isGameWon ? GREEN : RED);
            DrawText("Press 'R' to Restart | 'M' for Menu", 590, 450, 25, WHITE);
        }
    }
};

// --- 3. Main Loop ---
enum GameState
{
    AUTH,
    MENU,
    RULES,
    PLAYING
};

// Helper function for button drawing
void DrawMenuButton(Rectangle btn, const char *text, Vector2 mouse)
{
    if (CheckCollisionPointRec(mouse, btn))
        DrawRectangleRec(btn, LIGHTGRAY);
    else
        DrawRectangleRec(btn, RAYWHITE);
    DrawRectangleLinesEx(btn, 2, DARKGRAY);
    DrawText(text, btn.x + (btn.width - MeasureText(text, 20)) / 2, btn.y + 12, 20, BLACK);
}

int main()
{
    InitWindow(1600, 900, "Minesweeper - ReClaim Hub Dev");
    SetTargetFPS(60);

    GameState state = AUTH;
    Database db;
    Minesweeper *game = nullptr;
    string user = "", pass = "", msg = "";
    int activeField = 0;

    Rectangle uBox = {720, 340, 300, 50}, pBox = {720, 410, 300, 50};
    Rectangle btnLog = {720, 520, 140, 45}, btnSign = {880, 520, 140, 45};

    // Menu Buttons
    Rectangle btnEasy = {650, 400, 300, 45}, btnMed = {650, 470, 300, 45}, btnHard = {650, 540, 300, 45};
    Rectangle btnRules = {650, 610, 300, 45}, btnLogout = {650, 700, 300, 45};

    // Rules Button
    Rectangle btnBack = {650, 750, 300, 45};

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        Vector2 mouse = GetMousePosition();

        if (state == AUTH)
        {
            DrawText("LOGIN / SIGN UP", 680, 220, 35, DARKGRAY);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse, uBox))
                    activeField = 0;
                else if (CheckCollisionPointRec(mouse, pBox))
                    activeField = 1;
                else if (CheckCollisionPointRec(mouse, btnLog))
                {
                    msg = db.attemptAuth(user, pass, false);
                    if (msg == "SUCCESS")
                        state = MENU;
                }
                else if (CheckCollisionPointRec(mouse, btnSign))
                    msg = db.attemptAuth(user, pass, true);
            }
            int key = GetCharPressed();
            while (key > 0)
            {
                if (key >= 32 && key <= 125)
                {
                    if (activeField == 0 && user.length() < 15)
                        user += (char)key;
                    else if (activeField == 1 && pass.length() < 15)
                        pass += (char)key;
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE))
            {
                if (activeField == 0 && user.length() > 0)
                    user.pop_back();
                else if (activeField == 1 && pass.length() > 0)
                    pass.pop_back();
            }

            DrawText("Username:", 580, 350, 25, BLACK);
            DrawRectangleLinesEx(uBox, 2, activeField == 0 ? BLUE : BLACK);
            DrawText(user.c_str(), 730, 355, 25, DARKBLUE);
            DrawText("Password:", 580, 420, 25, BLACK);
            DrawRectangleLinesEx(pBox, 2, activeField == 1 ? BLUE : BLACK);
            DrawText(string(pass.length(), '*').c_str(), 730, 425, 25, DARKBLUE);
            DrawText(msg.c_str(), 580, 480, 20, RED);

            DrawMenuButton(btnLog, "LOGIN", mouse);
            DrawMenuButton(btnSign, "SIGN UP", mouse);
        }
        else if (state == MENU)
        {
            DrawText(TextFormat("Welcome, %s!", user.c_str()), 100, 100, 40, DARKBLUE);
            Database::Stats s = db.records[user];
            DrawText(TextFormat("Played: %d | Won: %d | Lost: %d", s.played, s.won, s.lost), 100, 170, 25, BLACK);
            DrawText(TextFormat("Best Times: Easy(%.1fs) | Medium(%.1fs) | Hard(%.1fs)", (s.bestEasy == 9999.0f ? 0 : s.bestEasy), (s.bestMedium == 9999.0f ? 0 : s.bestMedium), (s.bestHard == 9999.0f ? 0 : s.bestHard)), 100, 220, 25, DARKGREEN);

            DrawText("SELECT OPTION:", 680, 330, 30, BLACK);

            DrawMenuButton(btnEasy, "1. EASY (10x10)", mouse);
            DrawMenuButton(btnMed, "2. MEDIUM (16x16)", mouse);
            DrawMenuButton(btnHard, "3. HARD (30x16)", mouse);
            DrawMenuButton(btnRules, "HOW TO PLAY (RULES)", mouse);
            DrawMenuButton(btnLogout, "LOGOUT", mouse);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse, btnEasy))
                {
                    game = new Minesweeper(1);
                    state = PLAYING;
                }
                else if (CheckCollisionPointRec(mouse, btnMed))
                {
                    game = new Minesweeper(2);
                    state = PLAYING;
                }
                else if (CheckCollisionPointRec(mouse, btnHard))
                {
                    game = new Minesweeper(3);
                    state = PLAYING;
                }
                else if (CheckCollisionPointRec(mouse, btnRules))
                    state = RULES;
                else if (CheckCollisionPointRec(mouse, btnLogout))
                {
                    user = "";
                    pass = "";
                    msg = "";
                    state = AUTH;
                }
            }
        }
        else if (state == RULES)
        {
            DrawText("HOW TO PLAY MINESWEEPER", 550, 100, 35, DARKBLUE);
            DrawText("1. Goal: Reveal all safe cells without clicking on a mine.", 350, 200, 25, BLACK);
            DrawText("2. Numbers: A number on a cell indicates how many mines are hidden", 350, 260, 25, BLACK);
            DrawText("   in the 8 neighboring cells around it.", 350, 300, 25, BLACK);
            DrawText("3. Left Click: Use left click to reveal a cell.", 350, 360, 25, BLACK);
            DrawText("4. Right Click: Use right click to place a Flag (F) on a suspected mine.", 350, 420, 25, BLACK);
            DrawText("5. Safe Start: Your first click will NEVER be a mine.", 350, 480, 25, BLACK);
            DrawText("6. Win: Place flags correctly and reveal all non-mine cells!", 350, 540, 25, DARKGREEN);

            DrawMenuButton(btnBack, "BACK TO MENU", mouse);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, btnBack))
            {
                state = MENU;
            }
        }
        else if (state == PLAYING && game)
        {
            game->updateAndDraw(db, user);
            if (game->wantsRestart)
            {
                int lvl = game->difficultyLevel;
                delete game;
                game = new Minesweeper(lvl);
            }
            else if (game->wantsMenu)
            {
                delete game;
                game = nullptr;
                state = MENU;
            }
        }
        EndDrawing();
    }
    if (game)
        delete game;
    CloseWindow();
    return 0;
}