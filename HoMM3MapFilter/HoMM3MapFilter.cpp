#include <windows.h>
#include <tchar.h> 
#include <cstdio>
#include <iostream>
#include <fstream> 
#include <sstream>
#include <strsafe.h>
#include <vector>
#include <iomanip>
#include <stdio.h>
#include <zlib.h>

using namespace std;

//Global filters
int gameVer, mapSize, difficulty, numPlayers, hasDungeon, isAllied, hasTEvents, hasEvents, hasRumors;
bool verbose = true;
struct MapDescriptor {
    char* fileName;
    int players;
    int difficulty;
    int eventCnt;
    int timedEventCnt;
};

MapDescriptor* matchDescription(char* filename) {
    unsigned char thisVersion, thisSize, thisDungeon, thisDifficulty, thisPlayers;
    int thisEventCnt = 0, thisTEventCnt = 0;
    std::ifstream file(filename);
    if (!file.is_open()) {
        cout << "Couldn't open file: " << filename << "\n";
        return NULL;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string& s = ss.str();
    vector<unsigned char> buf(s.begin(), s.end());
    file.close(); 
    //Check if starts with 1f 8b, then it must be gzipped
    if (buf[0] == 0x1f && buf[1] == 0x8b) {
        gzFile inFileZ = gzopen(filename, "rb");
        //inf(file, stdout);
        if (inFileZ == NULL) {
            printf("Error: Failed to gzopen %s\n", filename);
            return NULL;
        }
        unsigned char unzipBuffer[8192];
        unsigned int unzippedBytes;
        std::vector<unsigned char> unzippedData;
        while (true) {
            unzippedBytes = gzread(inFileZ, unzipBuffer, 8192);
            if (unzippedBytes > 0) {
                for (int i = 0; i < unzippedBytes; i++) {
                    unzippedData.push_back(unzipBuffer[i]);
                }
            } else {
                break;
            }
        }
        gzclose(inFileZ);
        buf = unzippedData;
    }

    //########## CHECK GAME VERSION ##########
    if (gameVer != 3) {
        thisVersion = buf[0];
        if (thisVersion == 0x0E && gameVer != 0) return NULL;
        if (thisVersion == 0x15 && gameVer != 1) return NULL;
        if (thisVersion == 0x1C && gameVer != 2) return NULL;
        if (thisVersion != 0x0E && thisVersion != 0x15 && thisVersion != 0x1C) return NULL; //WOG and wtf?
    }
    //########## CHECK MAP SIZE ##########
    thisSize = buf[5];
    if (thisSize == 0x24 && mapSize != 36) return NULL;
    if (thisSize == 0x48 && mapSize != 72) return NULL;
    if (thisSize == 0x6C && mapSize != 108) return NULL;
    if (thisSize == 0x90 && mapSize != 144) return NULL;

    //########## CHECK IF MAP HAS DUNGEON ##########
    thisDungeon = buf[9];
    if (hasDungeon != 2 && (thisDungeon != hasDungeon)) return NULL;

    int nameOffset = (buf[13] << 24) |(buf[12] << 16) | (buf[11] << 8) | buf[10]; //skip the name
    nameOffset += 14;
    //skip the description
    int descOffset = (buf[nameOffset+3] << 24) |(buf[nameOffset+2] << 16) | (buf[nameOffset+1] << 8) | buf[nameOffset]; 
    descOffset += nameOffset+3;
    //########## CHECK DIFFICULTY ##########
    thisDifficulty = buf[descOffset + 1];
    if (thisDifficulty < difficulty) return NULL;

    // ########## PLAYER INFO ##########
    int infoOffset = descOffset + 3;
    struct Players {
        int canBeHuman;
        int canBeComputer;
    } players[8];
    int thisCanBeHuman = 0, thisCanBeComputer = 0; 
    for (int i = 0; i < 8; ++i) {
        //Player info
        players[i].canBeHuman = buf[infoOffset]; thisCanBeHuman += players[i].canBeHuman;
        players[i].canBeComputer = buf[++infoOffset]; thisCanBeComputer += players[i].canBeComputer;
        infoOffset += 6;
        if (buf[infoOffset] != 0) {
            infoOffset += 5+1;
        } else {
            infoOffset++;
        }
        infoOffset++; //IsRandomHero?
        if (buf[infoOffset] != 0xFF) {
            int nameOffset = (buf[infoOffset+5] << 24) |(buf[infoOffset+4] << 16) 
                | (buf[infoOffset+3] << 8) | buf[infoOffset+2];
            infoOffset += 5 + nameOffset + 2;
            int numberOfHeroes = (buf[infoOffset+3] << 24) |(buf[infoOffset+2] << 16) 
                | (buf[infoOffset+1] << 8) | buf[infoOffset];
            infoOffset += 4;
            for (int j = 0; j < numberOfHeroes; ++j) { //Number of heroes
               infoOffset++; //skip id
               int nameOffset = (buf[infoOffset+3] << 24) |(buf[infoOffset+2] << 16) 
                   | ( buf[infoOffset + 1] << 8 ) | buf[infoOffset];
               infoOffset += nameOffset + 4;
            }
        } else {
            infoOffset+=6; //+junk+heroCount+1
        }
    }
    // ########## Victory Condition ##########
    // This is skipped.
    unsigned char victoryCondition = buf[infoOffset];
    switch (victoryCondition) {
        case 0xFF: infoOffset += 1; break;
        case 0x00: infoOffset += 1+3; break;
        case 0x01: infoOffset += 1+8; break;
        case 0x02: infoOffset += 1+7; break;
        case 0x03: infoOffset += 1+7; break;
        case 0x04: infoOffset += 1+5; break;
        case 0x05: infoOffset += 1+5; break;
        case 0x06: infoOffset += 1+5; break;
        case 0x07: infoOffset += 1+5; break;
        case 0x08: infoOffset += 1+2; break;
        case 0x09: infoOffset += 1+2; break;
        case 0x0A: infoOffset += 1+6; break;
        default: break;
    }
    unsigned char lossCondition = buf[infoOffset];
    switch (lossCondition) {
        case 0xFF: infoOffset += 1; break;
        case 0x00: infoOffset += 1+3; break;
        case 0x01: infoOffset += 1+3; break;
        case 0x02: infoOffset += 1+2; break;
        default: break;
    }
    // ########## CHECK TEAMS AND PLAYER AMOUNT ##########
    unsigned char enableTeams = buf[infoOffset];
    unsigned char teams[8];
    if (enableTeams == 0x00) {
        infoOffset++; //No team info
    } else {
        infoOffset++;
        for (int i = 0; i < 8; ++i) {
            teams[i] = buf[infoOffset++];
        }
    }
    
    if (isAllied != 2) {
        //This is a very simple test, if map is allied. TODO: do a proper cross check for teams that consist of human players.
        int thisMapIsAllied = ( enableTeams != 0x00 && thisCanBeHuman >= 2 ) ? 1 : 0;
        if (thisMapIsAllied != isAllied) return NULL;
    }

    thisPlayers = ( thisCanBeComputer >= thisCanBeHuman ) ? thisCanBeComputer : thisCanBeHuman;
    if (thisPlayers <= numPlayers) return NULL;

    // ########## FREE HEROES ##########
    // This is skipped.
    infoOffset += 20;
    infoOffset += 4;
    unsigned char amountOfHeroes = buf[infoOffset++];
    if (amountOfHeroes != 0x00) {
        //TODO!!11
        int nameOffset = (buf[infoOffset+3] << 24) |(buf[infoOffset+2] << 16) 
                | (buf[infoOffset+1] << 8) | buf[infoOffset];
    }
    infoOffset += 31; //junk
    // ########## ARTIFACTS AND SPELLS ##########
    // This is skipped
    infoOffset += 18;   //artifacts
    infoOffset += 9;    //spells
    infoOffset += 4;    //sec skills
    // ########## RUMORS ##########
    // TODO
    int amountOfRumors = (buf[infoOffset+3] << 24) |(buf[infoOffset+2] << 16) 
                | (buf[infoOffset+1] << 8) | buf[infoOffset];
    infoOffset += 3;
    if (amountOfRumors != 0) {

    }
    // ########## MAP DATA ##########
    // This is skipped
    int multiplier = ( thisDungeon != 0 ) ? 2 : 1;
    infoOffset += multiplier*thisSize*thisSize*7;
    // ########## ALL OBJECTS + EVENTS ##########
    // TODO collect info about events
    MapDescriptor* descriptor = new MapDescriptor { filename, thisPlayers, difficulty, thisEventCnt, thisTEventCnt };
    return descriptor;
}

int main() {
    
    vector<char*> files, nonMatched;
    vector<MapDescriptor*> matched;

    cout << "Welcome to HoMM3 map filter tool. \nUsage: place it in maps directory of your homm3 game and follow the"<<
        "instructions. \nAll non-matched maps will be copied to the 'non_matched' directory."<< 
        "\n(!) Please ensure such directory already exists!" <<
        "\nAll maps that are left have matched the search criteria."<<
        "\nWarning! Input is not fool-proof!\n" ;
    //cout << "Game version: 0: RoE, 1: AB, 2: SoD, 3: DC\n"; cin >> gameVer;
    gameVer = 2;

    //cout << "Map size: 0: S, 1: M, 2: L, 3: XL\n"; cin >> mapSize;
    //mapSize = ( mapSize + 1 ) * 36; //36,72,108,144
    //cout << "Difficulty (min. value): 0: Easy, 1: Normal, 2: Hard, 3: Expert, 4: Impossible\n"; cin >> difficulty;
    //cout << "Players (min. value):\n"; cin >> numPlayers;
    //cout << "Has dungeon?: 0: No, 1: Yes, 2: DC\n"; cin >> hasDungeon;
    //cout << "Is Allied?: 0: No, 1: Yes, 2: DC\n"; cin >> isAllied;
    //cout << "Has timed events?: 0: No, 1: Yes, 2: DC\n"; cin >> hasTEvents;
    //cout << "Has map events?: 0: No, 1: Yes, 2: DC\n"; cin >> hasEvents;
    // 
    // 
    //cout << "Sort output by: 0: difficulty, 1: Yes, 2: DC\n"; cin >> sortCriteria;
    //cout << "Verbose mode? 0: No, 1: Yes\n"; cin >> verbose;

    // TEST DATA
    gameVer = 2; mapSize = 144; numPlayers = 4; hasDungeon = 1; isAllied = 1; 
    hasTEvents = 1; hasEvents = 1; difficulty = 1;

    //####################### List all files in current directory #######################
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR szDir[4] = { '.', '\\', '*', '\0' };
    hFind = FindFirstFile(szDir, &ffd);
    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char* ch = new char[260];
            char DefChar = ' ';
            WideCharToMultiByte(CP_ACP, 0, ffd.cFileName, -1, ch, 260, &DefChar, NULL);
            char* found=strstr(ch,".h3m");
            char* found2=strstr(ch,".H3M");
            if (found!=NULL || found2!= NULL) files.push_back(ch);
            //_tprintf(TEXT("  %s   \n"), ffd.cFileName);
        }
    } while (FindNextFile(hFind, &ffd) != 0);
    FindClose(hFind);

    //####################### Filter all files #######################
    int i = 0, len = files.size();
    for (auto file : files) {
        if (verbose) {
            cout << std::setiosflags(std::ios::fixed) 
                << "\nMatching: "  << setw(40) << std::left << file  << " (" << i << " of "<< len << ")";
        }
        i++;
        MapDescriptor* match = matchDescription(file);
        if (match != NULL) {
            matched.push_back(match);
        } else {
            cout << "...moved!";
            char* newFile = new char[strlen(file) + 14];
            strcpy(newFile, "./non_matched/");
            strcat(newFile, file);
            rename(file, newFile);
        }
    }

    //####################### Output results #######################
    cout << "\n===============================================================================\n";
    cout << "| Name       | Difficulty   | Events   | TimedEvents   |\n"; //TODO (max 80)
    //TODO: SORT THE VECTOR BY CRITERIA
    for (auto match : matched) {
        cout << match->fileName << " | " << match->difficulty << "\n"; //TODO proper formatted output (max80)
    }   
    system("pause");
    return 0;
}