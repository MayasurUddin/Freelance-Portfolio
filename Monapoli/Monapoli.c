#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define TOTAL_SPACES 40
#define MAX_PLAYERS 4
#define MIN_PLAYERS 2

#define PLAYER_COLOR_1 (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define PLAYER_COLOR_2 (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define PLAYER_COLOR_3 (FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define PLAYER_COLOR_4 (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)

#define NUM_CHANCE_CARDS 16
#define NUM_CHEST_CARDS 16

typedef struct {
    char name[50];
    int position;
    int money;
    int numProperties;
    int inJail;
    int turnsInJail;
    int getOutOfJailCards;
    char symbol;
    WORD color;
} Player;

typedef struct {
    char name[50];
    int price;
    int rent;
    int houses;
    int hotels;
    int owner;
} Property;

typedef struct {
    Property spaces[TOTAL_SPACES];
} Board;

typedef struct {
    char description[300];
    void (*effect)(Player *player, Player players[], Board *board, int numPlayers);
} Card;


void InitializeBoard(Board *board);
void InitializePlayers(Player players[], int numPlayers);
void DisplaySquareBoard(Board *board, Player players[], int numPlayers);
void SetConsoleColor(WORD color);
void ResetConsoleColor();
int RollDice();
void MovePlayer(Player *player, Board *board, Player players[], int numPlayers, Card chance[], Card chest[]);
void TakeTurn(Player *player, Player players[], Board *board, int numPlayers, Card chance[], Card chest[]);
void GoToJail(Player *player);
void TurnInJail(Player *player);
void ViewStatus(Player players[], int numPlayers);
void BuyOrBuild(Player *player, Board *board, Player players[], int numPlayers);
void Menu(Player *player, Player players[], Board *board, int numPlayers, Card chance[], Card chest[]);
void DrawChanceCard(Player *p, Player ps[], Board *b, int n, Card cards[]);
void DrawCommunityCard(Player *p, Player ps[], Board *b, int n, Card cards[]);
void InitializeChanceCards(Card c[]);
void InitializeCommunityCards(Card c[]);

void MoveTo(Player *player, int destination, Board *board) {
    if (destination < player->position) player->money += 200;
    player->position = destination;
    printf("Moved to %s\n", board->spaces[destination].name);}

void PayForBuildings(Player *player, Board *board, int perHouse, int perHotel) {
    int total = 0;
    for (int i = 0; i < TOTAL_SPACES; i++) {
        if (board->spaces[i].owner == player - &player[0]) {
            total += board->spaces[i].houses * perHouse;
            total += board->spaces[i].hotels * perHotel;
        }
    }
    player->money -= total;
    printf("Paid $%d for building costs.\n", total);
}

void TurnInJail(Player *player) {
    printf("%s is in jail.\n", player->name);

    if (player->getOutOfJailCards > 0) {
        printf("You have a 'Get Out of Jail Free' card. Use it to skip jail? (1=Yes, 0=No): ");
        int useCard; scanf("%d", &useCard);
        if (useCard == 1) {
            player->getOutOfJailCards--;
            player->inJail = 0;
            player->turnsInJail = 0;
            printf("Used the 'Get Out of Jail Free' card. You're free!\n");
            system("pause");
            return;
        }
    }

    if (player->turnsInJail < 2) {
        player->turnsInJail++;
        printf("Skipping turn %d in jail.\n", player->turnsInJail);
        system("pause");
        return;
    }

    printf("1. Roll for doubles\n2. Pay $50\nChoose: ");
    int choice; scanf("%d", &choice);

    if (choice == 1) {
        int a = rand() % 6 + 1;
        int b = rand() % 6 + 1;
        printf("Rolled %d and %d.\n", a, b);
        if (a == b) {
            player->inJail = 0;
            player->turnsInJail = 0;
            printf("Rolled doubles! You're free!\n");
        } else {
            player->turnsInJail++;
            printf("No doubles. Turn %d in jail.\n", player->turnsInJail);
            if (player->turnsInJail >= 3 && player->money >= 50) {
                player->money -= 50;
                player->inJail = 0;
                player->turnsInJail = 0;
                printf("Paid $50 after 3 turns. You're free!\n");
            }
        }
    } else if (choice == 2) {
        if (player->money >= 50) {
            player->money -= 50;
            player->inJail = 0;
            player->turnsInJail = 0;
            printf("Paid $50. You're free!\n");
        } else {
            printf("Can't pay. Stay jailed.\n");
        }
    } else {
        printf("Invalid input.\n");
    }
    system("pause");
}



int main() {
    srand((unsigned int)time(NULL));
    int numPlayers;

    Card chance[NUM_CHANCE_CARDS];
    Card chest[NUM_CHEST_CARDS];

    InitializeChanceCards(chance);
    InitializeCommunityCards(chest);
    printf("===== WELCOME TO MONOPOLY IN C =====\n");
    do {
        printf("Enter number of players (%d-%d): ", MIN_PLAYERS, MAX_PLAYERS);
        scanf("%d", &numPlayers);
        if (numPlayers < MIN_PLAYERS || numPlayers > MAX_PLAYERS)
            printf("Invalid number of players! Try again.\n");
    } while (numPlayers < MIN_PLAYERS || numPlayers > MAX_PLAYERS);
    getchar();

    Player players[numPlayers];
    for (int i = 0; i < numPlayers; i++) {
        printf("Enter name for Player %d: ", i + 1);
        fgets(players[i].name, sizeof(players[i].name), stdin);
        size_t len = strlen(players[i].name);
        if (len > 0 && players[i].name[len - 1] == '\n')
            players[i].name[len - 1] = '\0';
    }

    InitializePlayers(players, numPlayers);

    Board board;
    InitializeBoard(&board);

    int currentPlayerIndex = 0;
    int gameOver = 0;

    while (!gameOver) {
        Player *currentPlayer = &players[currentPlayerIndex];

        if (currentPlayer->money <= 0) {
            printf("%s is bankrupt and out of the game!\n", currentPlayer->name);
            currentPlayerIndex = (currentPlayerIndex + 1) % numPlayers;
            continue;
        }

        Menu(currentPlayer, players, &board, numPlayers, chance, chest);


        if (currentPlayer->money <= 0)
            printf("%s went bankrupt!\n", currentPlayer->name);

        int active = 0, last = -1;
        for (int i = 0; i < numPlayers; i++) {
            if (players[i].money > 0) {
                active++;
                last = i;
            }
        }
        if (active == 1) {
            printf("\nGame over! %s wins!\n", players[last].name);
            gameOver = 1;
        } else {
            currentPlayerIndex = (currentPlayerIndex + 1) % numPlayers;
        }
    }

    return 0;
}

void InitializeBoard(Board *board) {
    const char *names[TOTAL_SPACES] = {
        "GO","LAMA","CHEST","MIRA","INCOME","SYLHET","AMBER","CHANCE","ZINDA","BONDOR",
        "JAIL","AKUYA","ELECTRIC","NONDI","COLLEGE","MYMENSINGH","PANDIT","CHEST","KEUYAT","BORO",
        "REST","BATALI","CHANCE","PATHOR","MEHEDI","CHITTAGONG","LALDIGHIR","AGRABAAD","WATER","KULSI",
        "GOTOJAIL","WAARI","MOTIJHIL","CHEST","DHANMONDI","KOMLAPUR","CHANCE","BANANI","LUXURY","GULSHAN"
    };
    int prices[TOTAL_SPACES] = {
        0,60,0,60,200,200,100,0,100,120,0,140,150,140,160,200,180,0,100,200,
        0,220,0,220,240,200,260,260,150,280,0,300,300,0,320,200,0,350,100,400
    };
    int rents[TOTAL_SPACES] = {
        0,5,0,5,0,25,5,0,5,10,0,10,0,10,12,25,14,0,14,16,
        0,18,0,18,20,25,22,22,0,24,0,26,26,0,28,25,0,35,0,50
    };

    for (int i = 0; i < TOTAL_SPACES; i++) {
        strncpy(board->spaces[i].name, names[i], sizeof(board->spaces[i].name) - 1);
        board->spaces[i].name[sizeof(board->spaces[i].name) - 1] = '\0';
        board->spaces[i].price = prices[i];
        board->spaces[i].rent = rents[i];
        board->spaces[i].houses = 0;
        board->spaces[i].hotels = 0;
        board->spaces[i].owner = -1;
    }
}

void InitializePlayers(Player players[], int numPlayers) {
    char symbols[MAX_PLAYERS] = {'@','$','&','%'};
    WORD colors[MAX_PLAYERS] = {PLAYER_COLOR_1,PLAYER_COLOR_2,PLAYER_COLOR_3,PLAYER_COLOR_4};


    for (int i = 0; i < numPlayers; i++) {
        players[i].money = 1500;
        players[i].position = 0;
        players[i].numProperties = 0;
        players[i].inJail = 0;
        players[i].turnsInJail = 0;
        players[i].getOutOfJailCards = 0;
        players[i].symbol = symbols[i];
        players[i].color = colors[i];
    }
}

void SetConsoleColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}
void ResetConsoleColor() {
    SetConsoleColor(FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
}

void DisplaySquareBoard(Board *board, Player players[], int numPlayers) {
    system("cls");
    printf("\n=== MONOPOLY BOARD ===\n\n");

    char grid[11][11][8];
    for (int i=0;i<11;i++) for (int j=0;j<11;j++) strcpy(grid[i][j],"     ");

    for (int i=1;i<=10;i++) {
        int idx = i-1;
        snprintf(grid[0][i],6,"%5.5s",board->spaces[idx].name);
    }

    for (int i=1;i<=9;i++) {
        int idx = 10 + (i - 1);
        snprintf(grid[i][10],6,"%5.5s",board->spaces[idx].name);
    }

    for (int i = 0; i <= 10; i++) {
    int idx = 29 - i;
    snprintf(grid[10][i], 6, "%5.5s", board->spaces[idx].name);
}

    for (int i = 0; i <= 9; i++) {
    int idx = 30 + i;
    snprintf(grid[9 - i][0], 6, "%5.5s", board->spaces[idx].name);
}
    for (int p = 0; p < numPlayers; p++) {
    int pos = players[p].position, row, col;

    if (pos <= 9) {
        row = 0;
        col = pos + 1;
    } else if (pos <= 18) {
        row = pos - 9;
        col = 10;
    } else if (pos <= 29) {
        row = 10;
        col = 29 - pos;
    } else {
        row = 39 - pos;
        col = 0;
    }

    int len = strlen(grid[row][col]);
    if (len < 6) {
        grid[row][col][len] = players[p].symbol;
        grid[row][col][len + 1] = '\0';
    }
}

    for(int i=0;i<11;i++){
        for(int j=0;j<11;j++){
            int colored=0;
            for(int p=0;p<numPlayers;p++){
                if(strchr(grid[i][j],players[p].symbol)){
                    SetConsoleColor(players[p].color);
                    printf("%-7s",grid[i][j]);
                    ResetConsoleColor();
                    colored=1;
                    break;
                }
            }
            if(!colored) printf("%-7s",grid[i][j]);
        }
        printf("\n");
    }
    printf("\nPlayers: ");
    for(int p=0;p<numPlayers;p++){
        SetConsoleColor(players[p].color);
        printf("%s(%c) ",players[p].name,players[p].symbol);
        ResetConsoleColor();
    }
    printf("\n\n");
}

int RollDice(){ return (rand()%6+1)+(rand()%6+1); }

void MovePlayer(Player *player, Board *board, Player players[], int numPlayers, Card chance[], Card chest[]) {
    int d = RollDice();
    printf("%s rolled %d.\n", player->name, d);
    player->position += d;
    if (player->position >= TOTAL_SPACES) {
        player->position -= TOTAL_SPACES;
        player->money += 200;
        printf("%s passed GO and got $200.\n", player->name);
    }
    printf("%s landed on %s.\n", player->name, board->spaces[player->position].name);

    if (player->position == 30) {
        GoToJail(player);
        return;
    } else if (player->position == 2 || player->position == 17 || player->position == 33) {
        DrawCommunityCard(player, players, board, numPlayers, chest);
    } else if (player->position == 7 || player->position == 22 || player->position == 36) {
        DrawChanceCard(player, players, board, numPlayers, chance);
    }
}


void TakeTurn(Player *player, Player players[], Board *board, int numPlayers, Card chance[], Card chest[]) {
    if (player->inJail) {
        TurnInJail(player);
        if (player->inJail) {
            printf("Stuck in jail this turn.\n");
            system("pause");
            return;
        }
    }
    MovePlayer(player, board, players, numPlayers, chance, chest);
    system("pause");
}

void GoToJail(Player *player){
    printf("%s is sent to jail!\n",player->name);
    player->position=10;
    player->inJail=1;
    player->turnsInJail=0;
}

void ViewStatus(Player players[], int n){
    printf("\n--- Status ---\n");
    for(int i=0;i<n;i++){
        printf("%s: $%d | Pos %d | Props %d | Jail Cards: %d | %s\n",
    players[i].name, players[i].money, players[i].position,
    players[i].numProperties, players[i].getOutOfJailCards,
    players[i].inJail ? "In Jail" : "Free");
    }
    system("pause");
}

void BuyOrBuild(Player *player, Board *board, Player players[], int n){
    int pos=player->position;
    Property *pr=&board->spaces[pos];
    int idx=-1;
    for(int i=0;i<n;i++) if(&players[i]==player) idx=i;
    if(idx<0){ printf("Error\n"); system("pause"); return; }
    if(pr->price==0){ printf("Can't buy/build here.\n"); system("pause"); return; }
    if(pr->owner==-1){
        printf("%s costs $%d. Buy?1=Yes:0=No ",pr->name,pr->price);
        int c; scanf("%d",&c);
        if(c==1 && player->money>=pr->price){
            player->money-=pr->price;
            pr->owner=idx;
            player->numProperties++;
            printf("Bought %s!\n",pr->name);
        } else printf(c==1?"Not enough $\n":"Skipped\n");
        system("pause");
        return;
    }
    if(pr->owner==idx){
        printf("Own %s. Houses:%d Hotels:%d\n1=House($40) 2=Hotel($100,4 houses) 0=No ",
            pr->name,pr->houses,pr->hotels);
        int c; scanf("%d",&c);
        if(c==1 && player->money>=40){ pr->houses++; player->money-=40; printf("House built\n"); }
        else if(c==2 && pr->houses>=4 && player->money>=100){
            pr->houses=0; pr->hotels++; player->money-=100; printf("Hotel built\n");
        }
        else printf("Can't build\n");
        system("pause");
        return;
    }
    printf("%s owned by %s. Rent $%d\n",pr->name,players[pr->owner].name,pr->rent);
    if(player->money>=pr->rent){
        player->money-=pr->rent;
        players[pr->owner].money+=pr->rent;
    } else { printf("Bankrupt!\n"); player->money=0; }
    system("pause");
}

void Menu(Player *player, Player players[], Board *board, int n,Card chance[],Card chest[]){
    int choice,rolled=0;
    while(1){
        DisplaySquareBoard(board,players,n);
        printf("%s: $%d%s\n1=Roll/Move 2=Buy/Build 3=Status 4=End\nChoose: ",
            player->name,player->money,player->inJail?" [Jail]":"");
        scanf("%d",&choice);
        if(choice==1){
            if(rolled){ printf("Already rolled!\n"); system("pause"); }
           else{ TakeTurn(player, players, board, n, chance, chest); rolled = 1; }

        }
        else if(choice==2){
            if(!rolled){ printf("Roll first!\n"); system("pause"); }
            else BuyOrBuild(player,board,players,n);
        }
        else if(choice==3) ViewStatus(players,n);
        else if(choice==4){
            if(!rolled){ printf("Roll first!\n"); system("pause"); }
            else{ printf("Turn end\n"); system("pause"); break; }
        }
        else{ printf("Invalid\n"); system("pause"); }
    }
}
void Chance01(Player *p, Player ps[], Board *b, int n) { p->position = (p->position + TOTAL_SPACES - 3) % TOTAL_SPACES; }
void Chance02(Player *p, Player ps[], Board *b, int n) { MoveTo(p, 39, b); }
void Chance03(Player *p, Player ps[], Board *b, int n) { MoveTo(p, 11, b); }
void Chance04(Player *p, Player ps[], Board *b, int n) {
    p->getOutOfJailCards++;
    printf("You received a 'Get Out of Jail Free' card!\n");
}
void Chance05(Player *p, Player ps[], Board *b, int n) { PayForBuildings(p, b, 40, 115); }
void Chance06(Player *p, Player ps[], Board *b, int n) { p->money += 100; }
void Chance07(Player *p, Player ps[], Board *b, int n) { PayForBuildings(p, b, 25, 100); }
void Chance08(Player *p, Player ps[], Board *b, int n) { p->money += 150; }
void Chance09(Player *p, Player ps[], Board *b, int n) { MoveTo(p, 15, b); }
void Chance10(Player *p, Player ps[], Board *b, int n) { p->money += 50; }
void Chance11(Player *p, Player ps[], Board *b, int n) { p->money -= 150; }
void Chance12(Player *p, Player ps[], Board *b, int n) { p->money -= 20; }
void Chance13(Player *p, Player ps[], Board *b, int n) { MoveTo(p, 0, b); }
void Chance14(Player *p, Player ps[], Board *b, int n) { MoveTo(p, 24, b); }
void Chance15(Player *p, Player ps[], Board *b, int n) { p->money -= 15; }
void Chance16(Player *p, Player ps[], Board *b, int n) { GoToJail(p); }

void Chest01(Player *p, Player ps[], Board *b, int n) { p->money -= 50; }
void Chest02(Player *p, Player ps[], Board *b, int n) { p->money -= 10; }
void Chest03(Player *p, Player ps[], Board *b, int n) { p->money += 50; }
void Chest04(Player *p, Player ps[], Board *b, int n) { p->money += 100; }
void Chest05(Player *p, Player ps[], Board *b, int n) { p->money -= 50; }
void Chest06(Player *p, Player ps[], Board *b, int n) { MoveTo(p, 1, b); }
void Chest07(Player *p, Player ps[], Board *b, int n) {
    p->getOutOfJailCards++;
    printf("You received a 'Get Out of Jail Free' card!\n");
}
void Chest08(Player *p, Player ps[], Board *b, int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        if (&ps[i] != p && ps[i].money >= 10) {
            ps[i].money -= 10;
            total += 10;
        }
    }
    p->money += total;
}
void Chest09(Player *p, Player ps[], Board *b, int n) { p->money += 20; }
void Chest10(Player *p, Player ps[], Board *b, int n) { p->money += 100; }
void Chest11(Player *p, Player ps[], Board *b, int n) { p->money += 200; }
void Chest12(Player *p, Player ps[], Board *b, int n) { p->money += 10; }
void Chest13(Player *p, Player ps[], Board *b, int n) { MoveTo(p, 0, b); }
void Chest14(Player *p, Player ps[], Board *b, int n) { GoToJail(p); }
void Chest15(Player *p, Player ps[], Board *b, int n) { p->money += 25; }
void Chest16(Player *p, Player ps[], Board *b, int n) { p->money -= 100; }

void InitializeChanceCards(Card c[]) {
    const char *texts[NUM_CHANCE_CARDS] = {
        "STEP BACK 3 MOVES.",
        "GO TO GULSHAN.",
        "GO TO AKUYA. COLLECT $200 IF PASS GO.",
        "PRISON RELEASE PASS.",
        "PAY -40 PER HOUSE & -115 PER HOTEL.",
        "WIN DEBATE, RECEIVE $100.",
        "RENOVATION FEES: HOUSE -25, HOTEL -100.",
        "BANK LOAN $150 TO BUILD.",
        "GO TO MYMENSINGH. COLLECT $200 IF PASS GO.",
        "RECEIVE DIVIDEND $50.",
        "PAY SCHOOL FEES -150.",
        "FINE FOR ALCOHOL -20.",
        "GO TO START.",
        "GO TO MEHEDI BAGH. COLLECT $200 IF PASS GO.",
        "SPEEDING FINE -15.",
        "GO TO JAIL. DO NOT PASS GO."
    };
    void (*fx[])(Player*,Player[],Board*,int) = {
        Chance01,Chance02,Chance03,Chance04,Chance05,Chance06,Chance07,Chance08,
        Chance09,Chance10,Chance11,Chance12,Chance13,Chance14,Chance15,Chance16
    };
    for(int i=0;i<NUM_CHANCE_CARDS;i++){ strcpy(c[i].description,texts[i]); c[i].effect=fx[i]; }
}

void InitializeCommunityCards(Card c[]) {
    const char *texts[NUM_CHEST_CARDS] = {
        "PAY -50 INSURANCE PREMIUM.",
        "PAY -10 OR TAKE CHANCE.",
        "GET $50 FOR SELLING STOCK.",
        "RECEIVE $100 SCHOLARSHIP.",
        "PAY $50 DOCTOR'S FEE.",
        "GO TO LAMA BAZAR.",
        "PRISON RELEASE PASS.",
        "BIRTHDAY! GET $10 FROM EACH PLAYER.",
        "INCOME TAX REFUND $20.",
        "INHERIT $100.",
        "BANK ERROR IN YOUR FAVOR $200.",
        "BEAUTY CONTEST PRIZE $10.",
        "GO TO START.",
        "GO TO JAIL.",
        "RECEIVE 7% INTEREST = $25.",
        "PAY HOSPITAL BILL $100."
    };
    void (*fx[])(Player*,Player[],Board*,int) = {
        Chest01,Chest02,Chest03,Chest04,Chest05,Chest06,Chest07,Chest08,
        Chest09,Chest10,Chest11,Chest12,Chest13,Chest14,Chest15,Chest16
    };
    for(int i=0;i<NUM_CHEST_CARDS;i++){ strcpy(c[i].description,texts[i]); c[i].effect=fx[i]; }
}

void DrawChanceCard(Player *p, Player ps[], Board *b, int n, Card cards[]) {
    printf("\n-- Spinning CHANCE card...\n");
    for (int i = 0; i < 10; i++) {
        int r = rand() % NUM_CHANCE_CARDS;
        printf("\r%s", cards[r].description);
        Sleep(100);
    }
    int index = rand() % NUM_CHANCE_CARDS;
    printf("\n--- CHANCE CARD ---\n%s\n", cards[index].description);
    cards[index].effect(p, ps, b, n);
    system("pause");
}

void DrawCommunityCard(Player *p, Player ps[], Board *b, int n, Card cards[]) {
    printf("\n-- Spinning COMMUNITY CHEST card...\n");
    for (int i = 0; i < 10; i++) {
        int r = rand() % NUM_CHEST_CARDS;
        printf("\r%s", cards[r].description);
        Sleep(100);
    }
    int index = rand() % NUM_CHEST_CARDS;
    printf("\n--- COMMUNITY CHEST ---\n%s\n", cards[index].description);
    cards[index].effect(p, ps, b, n);
    system("pause");

}


