#include <windows.h>
#include <tchar.h> 
#include <cstdio>
#include <iostream>
#include <fstream> 
#include <sstream>
#include <strsafe.h>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <stdio.h>
#include <zlib.h>

using namespace std;

//Global filters
int gameVer, mapSize, difficulty, numPlayers, hasDungeon, isAllied, hasRumors;
bool verbose = true;
struct MapDescriptor {
    char* fileName;
    int score;
    int eventCnt;
    int rumors;
    int timedEventCnt;
    int grail;
    int seer;
    int utopia;
    int questGuard;
    int dragons;
    bool operator < (const MapDescriptor& str) const { return (score < str.score); }
};

bool comparePtrMapDescriptor(MapDescriptor* a, MapDescriptor* b) { return (*a < *b); }

//############################## SKIPPING PROTOTYPES ######################################
inline void skipName(int &offset, vector<unsigned char> &buf);
inline int getInt(int &offset, vector<unsigned char> &buf);
inline int getShort(int &offset, vector<unsigned char> &buf);
inline char* getName(int &offset, vector<unsigned char> &buf);
inline unsigned char getChar(int &offset, vector<unsigned char> &buf);
inline void skipAtrifact(int &offset, vector<unsigned char> &buf);
inline void skipPandora(int &offset, vector<unsigned char> &buf);
inline void skipTown(int &offset, vector<unsigned char> &buf);
inline void skipQuestGuard(int &offset, vector<unsigned char> &buf);
inline void skipHero(int &offset, vector<unsigned char> &buf);
inline void skipMonster(int &offset, vector<unsigned char> &buf);
inline void skipSeer(int &offset, vector<unsigned char> &buf);


MapDescriptor* matchDescription(char* filename) {
    unsigned char thisVersion, thisSize, thisDungeon, thisDifficulty, thisPlayers;
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

    int infoOffset = 10; //This offset is used to navigate the file. First 10 bytes of the file are static.

    //############################## CHECK GAME VERSION ######################################
    if (gameVer != 3) {
        thisVersion = buf[0];
        if (thisVersion == 0x0E && gameVer != 0) return NULL;
        if (thisVersion == 0x15 && gameVer != 1) return NULL;
        if (thisVersion == 0x1C && gameVer != 2) return NULL;
        if (thisVersion != 0x0E && thisVersion != 0x15 && thisVersion != 0x1C) return NULL; //WOG and wtf?
    }

    //############################## CHECK MAP SIZE ##############################
    thisSize = buf[5];
    if (thisSize == 0x24 && mapSize != 36) return NULL;
    if (thisSize == 0x48 && mapSize != 72) return NULL;
    if (thisSize == 0x6C && mapSize != 108) return NULL;
    if (thisSize == 0x90 && mapSize != 144) return NULL;

    //############################## CHECK IF MAP HAS DUNGEON ##############################
    thisDungeon = buf[9];
    if (hasDungeon != 2 && (thisDungeon != hasDungeon)) return NULL;
    
    skipName(infoOffset, buf);//skip the name
    skipName(infoOffset, buf);//skip the description

    //############################## CHECK DIFFICULTY ##############################
    thisDifficulty = buf[infoOffset++];
    if (thisDifficulty < difficulty) return NULL;

    //############################## PLAYER INFO ##############################
    struct Players {
        int canBeHuman;
        int canBeComputer;
    } players[8];
    int thisCanBeHuman = 0, thisCanBeComputer = 0; 
    infoOffset++;//skip maxHeroLevel
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
            //TODO: replace with skipName
            int nameOffset = (buf[infoOffset+5] << 24) |(buf[infoOffset+4] << 16) 
                | (buf[infoOffset+3] << 8) | buf[infoOffset+2];
            infoOffset += 5 + nameOffset + 2;
        } else {
            infoOffset+=2; //+junk+heroCount+1
        }
        int numberOfHeroes = getInt(infoOffset, buf);
        for (int j = 0; j < numberOfHeroes; ++j) { //Number of heroes
            infoOffset++; //skip id
            skipName(infoOffset, buf);
        }
    }

    //############################## VICTORY CONDITION ##############################
    unsigned char victoryCondition = buf[infoOffset];
    switch (victoryCondition) {
        case 0xFF: infoOffset += 1; break;
        case 0x00: infoOffset += 1+3+1; break;
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

    //############################## CHECK TEAMS AND PLAYER AMOUNT ##############################
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

    //############################## FREE HEROES ##############################
    infoOffset += 24; //Skip heroes availability info
    unsigned char amountOfHeroes = buf[infoOffset++];
    if (amountOfHeroes != 0x00) {
        for (int i = 0; i < amountOfHeroes; ++i) {
            infoOffset += 2;
            skipName(infoOffset, buf);
            infoOffset++; //skip availability byte
        }
    }
    infoOffset += 31; //junk

    //############################## ARTIFACTS AND SPELLS ##############################
    infoOffset += 18;   //artifacts
    infoOffset += 9;    //spells
    infoOffset += 4;    //sec skills

    //############################## RUMORS ##############################
    int amountOfRumors = getInt(infoOffset, buf);
    if (amountOfRumors != 0) {
        for (int i = 0; i < amountOfRumors; ++i) {
            skipName(infoOffset, buf);//skip name
            skipName(infoOffset, buf);//skip rumor text
        }
    }

    //############################## HERO SETTINGS ##############################
    for (int i = 0; i < 156; ++i) {
        if (buf[infoOffset++] != 0x00) {
            if (buf[infoOffset++] != 0x00) infoOffset += 4; //exp
            if (buf[infoOffset++] != 0x00) {//sec skills
                int secSkillsCount = getInt(infoOffset, buf);
                if (secSkillsCount != 0) infoOffset += secSkillsCount * 2;
            }
            if (buf[infoOffset++] != 0x00) { //artefacts
                infoOffset += 19*2; //equipped
                int backPackCount = getShort(infoOffset,buf);
                if (backPackCount != 0) infoOffset += 2 * backPackCount;
            }
            if (buf[infoOffset++] != 0x00) { //bio
                skipName(infoOffset, buf);//skip bio
            }
            infoOffset += 1; //sex
            if (buf[infoOffset++] != 0x00) infoOffset += 9; //spells
            if (buf[infoOffset++] != 0x00) infoOffset += 4; //attributes
        }
    }

    //############################## MAP DATA ##############################
    int multiplier = ( thisDungeon != 0 ) ? 2 : 1;
    infoOffset += multiplier*thisSize*thisSize*7; // This is skipped

    //############################## ALL OBJECTS ##############################
    int objectCount = getInt(infoOffset, buf);
    struct Object {
        char* objName;
        unsigned int objClass;
        unsigned int objNumber;
        unsigned char objGroup;
    };

    int dragonDwellings = 0, dragonUtopias = 0;
    Object* objects = new Object[objectCount];
    for (int i = 0; i < objectCount; ++i) {
        objects[i].objName = getName(infoOffset, buf);
        if (strstr(objects[i].objName, "AVGcdrg.def") != NULL) dragonDwellings++;
        if (strstr(objects[i].objName, "AVGfdrg.def") != NULL) dragonDwellings++;
        if (strstr(objects[i].objName, "AVGazur.def") != NULL) dragonDwellings++;
        if (strstr(objects[i].objName, "AVGrust.def") != NULL) dragonDwellings++;
        if (strstr(objects[i].objName, "AVSutop0.def") != NULL) dragonUtopias++;
        infoOffset += 6 + 6 + 2 + 2;//passability6, actions6, landscape, editGroup
        objects[i].objClass = getInt(infoOffset, buf);
        objects[i].objNumber = getInt(infoOffset, buf);
        objects[i].objGroup = buf[infoOffset++];
        infoOffset += 1 +16; //overlay, junk      
    }

    //############################## ALL TUNED OBJECTS ##############################
    int garrissons = 0, isGrail = 0, locEvents = 0, questGuard = 0, seerHuts = 0;
    int tunedObjects = getInt(infoOffset, buf);
    for (int i = 0; i < tunedObjects; ++i) {
        if (i == 0x65e) {
            int a = 5;
        }
        infoOffset += 3; //x,y,z  
        int objectID = getInt(infoOffset, buf); 
        infoOffset += 5; //junk
        int objClass = objects[objectID].objClass;
        switch(objClass) {
             case 5:
             case 65:
             case 66:
             case 67:
             case 68:
             case 69:
                 //ObjectArtefact artefact;
                 skipAtrifact(infoOffset, buf);
                 break;
             case 6:
                 //ObjectPandora pandora;
                 skipPandora(infoOffset, buf);
                 break;
             case 17:
             case 20:
             case 42: //lighthouse
                 //ObjectDwelling dwelling;
                 infoOffset += 4; 
                 break;
             case 26:
                 //ObjectEvent localevent;
                 locEvents++;
                 skipPandora(infoOffset, buf);
                 infoOffset += 1+1+1; 
                 infoOffset += 4; //junk
                 break;
             case 33:
             case 219:
                 //ObjectGarrison garrison;
                 garrissons++;
                 infoOffset += 4;
                 infoOffset += (2+2)*7; //GuardTag: id,count, seven units
                 infoOffset += 1+4*2;
                 break;
             case 34:
             case 70:
                 //ObjectHero hero;
                 skipHero(infoOffset, buf);
                 break;
             case 62:
                 //ObjectHero hero;
                 skipHero(infoOffset, buf);
                 break;
             case 36:
                 //ObjectGrail grail;
                 isGrail++;
                 infoOffset += 1+3;
                 break;
             case 53:
                 switch(objects[objectID].objNumber) {
                 case 7:
                     //ObjectAbandonedMine abandoned;
                     infoOffset += 1+3;
                     break;
                 default:
                     //ObjectMine mine;
                     infoOffset += 4;
                     break;
                 }
                 break;
             case 54:
             case 71:
             case 72:
             case 73:
             case 74:
             case 75:
             case 162:
             case 163:
             case 164:
                 //ObjectMonster monster;
                 skipMonster(infoOffset, buf);
                 break;
             case 76:
             case 79:
                 //ObjectResource res;
                 skipAtrifact(infoOffset, buf);
                 infoOffset += 4; //qty
                 infoOffset += 4; //junk
                 break;              
             case 81:
                 //ObjectScientist scientist;
                 infoOffset += 1;
                 infoOffset += 4;
                 infoOffset += 3;
                 break;
             case 83:
                 //ObjectProphet seer;
                 seerHuts++;
                 skipSeer(infoOffset, buf);
                 break;
             case 87:
                 //ObjectShipyard shipyard;
                 infoOffset += 4;
                 break;
             case 88:
             case 89:
             case 90:
                 //ObjectShrine shrine;
                 infoOffset += 4; //SpellID
                 break;
             case 91:
             case 59:
                 //ObjectSign sign;
                 skipName(infoOffset, buf);
                 infoOffset += 4; //junk
                 break;
             case 93:
                 //ObjectSpell spell;
                 skipAtrifact(infoOffset, buf);
                 infoOffset += 4; //SpellID        
                 break;
             case 98:
             case 77:
                 //ObjectTown town;
                 skipTown(infoOffset, buf);
                 break;
             case 113:
                 //ObjectWitchHut whut;
                 infoOffset += 4;
                 break;
             case 215:
                 //ObjectQuestionGuard qguard;
                 questGuard++;
                 skipQuestGuard(infoOffset, buf);
                 break;
             case 216:
                 //ObjectGeneralRandomDwelling dwelling;
                 infoOffset += 4;
                 if (getInt(infoOffset, buf)==0) infoOffset += 2;
                 infoOffset += 1+1;
                 break;
             case 217:
                 //ObjectLevelRandomDwelling dwelling;
                 infoOffset += 4;
                 if (getInt(infoOffset, buf)==0) infoOffset += 2;
                 break;
             case 218:
                 //ObjectTownRandomDwelling dwelling;
                 infoOffset += 4;
                 infoOffset += 1+1;
                 break;
             case 220:
                 //ObjectAbandonedMine abandoned;
                 infoOffset += 1+3;
                 break;
             default: break;
        };
    }
    //############################## GLOBAL TIMED EVENTS ##############################
    int timedEventCnt = getInt(infoOffset, buf);
    for (int i = 0; i < timedEventCnt; ++i) {
        skipName(infoOffset, buf);
        skipName(infoOffset, buf);
        infoOffset += 4*7 + 3*1 + 2*2 + 16; //resources, players, humans, ai, day, iteration, junk
    }

    infoOffset += 124; //junk
    if (infoOffset != buf.size()) cout << "\nSize match error!\n";

    //############################## COMPUTE SCORE VALUE ##############################
    int thisScore = 0;
    if (amountOfRumors != 0) thisScore += 50 + 2 * amountOfRumors;
    if (locEvents != 0) thisScore += 50 + 1 * locEvents;
    if (timedEventCnt != 0) thisScore += 50 + 2 * timedEventCnt;
    if (isGrail != 0) thisScore += 200;
    if (seerHuts != 0) thisScore += 50 + 5 * seerHuts;
    if (questGuard != 0) thisScore += 50 + 7 * questGuard;
    if (garrissons != 0) thisScore += 10 + 3 * garrissons;
    if (dragonDwellings != 0) thisScore += 100 + 30 * dragonDwellings;
    if (dragonUtopias != 0) thisScore += 250 + 30 * dragonUtopias;

    MapDescriptor* descriptor = new MapDescriptor { filename, thisScore, locEvents, amountOfRumors,
        timedEventCnt, isGrail, seerHuts, dragonUtopias, questGuard, dragonDwellings };
    return descriptor;
}

int main() { 
    vector<char*> files, nonMatched;
    vector<MapDescriptor*> matched;

    cout << "Welcome to HoMM3 map filter tool. \nUsage: place it in maps directory of your homm3 game and follow "<<
        "the instructions \nAll non-matched maps will be copied to the 'non_matched' directory."<< 
        "\n(!) Please ensure such directory already exists!" <<
        "\nAll maps that are left have matched the search criteria."<<
        "\nWarning! Input is not fool-proof!\n" ;
    //cout << "Game version: 0: RoE, 1: AB, 2: SoD, 3: DC\n"; cin >> gameVer;
    gameVer = 2;

    cout << "Map size: 0: S, 1: M, 2: L, 3: XL\n"; cin >> mapSize;
    mapSize = ( mapSize + 1 ) * 36; //36,72,108,144
    cout << "Difficulty (min. value): 0: Easy, 1: Normal, 2: Hard, 3: Expert, 4: Impossible\n"; cin >> difficulty;
    cout << "Players (min. value):\n"; cin >> numPlayers;
    cout << "Has dungeon?: 0: No, 1: Yes, 2: DC\n"; cin >> hasDungeon;
    cout << "Is Allied?: 0: No, 1: Yes, 2: DC\n"; cin >> isAllied;
    cout << "Verbose mode? 0: No, 1: Yes\n"; cin >> verbose;

    // TEST DATA
    //gameVer = 2; mapSize = 144; numPlayers = 3; hasDungeon = 2; isAllied = 1; difficulty = 1;

    //####################### List all files in current directory #######################
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR szDir[4] = { '.', '\\', '*', '\0' };
    hFind = FindFirstFile(szDir, &ffd);
    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char* ch = new char[260];
            char DefChar = ' ';
            //This effectively transforms non-latin characters to their latin equivalent. Fix this?
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
            if (verbose) cout << "...moved!";
            char* newFile = new char[strlen(file) + 14];
            strcpy(newFile, "./non_matched/");
            strcat(newFile, file);
            rename(file, newFile);
        }
    }

    //####################### Output results #######################
    cout << "\n===============================================================================\n";
    cout <<   "| Name           | Score | Evt | Rumr | TEvt | QGrd | Grl | Seer | Utop | Drg |\n";
    std::sort (matched.begin(), matched.end(), comparePtrMapDescriptor); 
    for (auto match : matched) {
        //TODO proper formatted columns.
        cout << "| " << setw(15)<< std::string(match->fileName).substr(0,13) << "| " 
            << setw(6) << match->score << "| " 
            << setw(4) << match->eventCnt << "| " 
            << setw(5) << match->rumors << "| " 
            << setw(5) << match->timedEventCnt << "| " 
            << setw(5) << match->questGuard << "| " 
            << setw(4) << match->grail << "| " 
            << setw(5) << match->seer << "| " 
            << setw(5) << match->utopia << "| " 
            << setw(4) << match->dragons << "|\n";
    }   
    cout << "===============================================================================\n";
    system("pause");
    return 0;
}
inline char* getName(int &offset, vector<unsigned char> &buf) {
    int nameLength = (buf[offset+3] << 24) |(buf[offset+2] << 16) 
                                | (buf[offset+1] << 8) | buf[offset];
    offset += 4;
    char* ret = new char[nameLength+1];
    for (int i = 0; i < nameLength; ++i) {
        ret[i] = buf[offset++];
    }
    ret[nameLength] = '\0';
    return ret;
}
inline void skipName(int &offset, vector<unsigned char> &buf) {
    int nameLength = (buf[offset+3] << 24) |(buf[offset+2] << 16) 
                                | (buf[offset+1] << 8) | buf[offset]; 
    offset += 4 + nameLength;
}

inline int getInt(int &offset, vector<unsigned char> &buf) {
    int count = (buf[offset+3] << 24) |(buf[offset+2] << 16) 
                                | (buf[offset+1] << 8) | buf[offset]; 
    offset += 4;
    return count;
}

inline int getShort(int &offset, vector<unsigned char> &buf) {
    int count = (buf[offset+1] << 8) | buf[offset]; 
    offset += 2;
    return count;
}

inline unsigned char getChar(int &offset, vector<unsigned char> &buf) {
    return buf[offset++];
}

inline void skipAtrifact(int &offset, vector<unsigned char> &buf) {
    if (buf[offset++] != 0x00) { //text
        skipName(offset, buf);
    
        if (buf[offset++] != 0x00) { //guards
            offset += (2+2)*7; //GuardTag: id,count, seven units
        } 
        offset += 4; //junk
    }
}

inline void skipPandora(int &offset, vector<unsigned char> &buf) {
    skipAtrifact(offset, buf);
    offset += 4 + 4 + 1 + 1 + 4 * 7 + 4 * 1;
    unsigned char secSkills = getChar(offset, buf);
    if (secSkills > 0) offset += secSkills * 2;
    unsigned char artifacts = getChar(offset, buf);
    if (artifacts > 0) offset += artifacts * 2;
    unsigned char spells = getChar(offset, buf);
    if (spells > 0) offset += spells;
    unsigned char monsters = getChar(offset, buf);
    if (monsters > 0) offset += monsters * (2+2);
    offset += 8; //junk
}

inline void skipTown(int &offset, vector<unsigned char> &buf) {
    offset += 4+1;
    int isName = getChar(offset, buf);
    if (isName ==1) skipName(offset, buf);
    int isGuard = getChar(offset, buf);
    if (isGuard ==1) offset += 7*4;
    offset += 1;
    int isBuildings = getChar(offset, buf);
    if (isBuildings == 1) {
        offset += 6+6;
    } else {
        offset += 1;
    }
    offset += 9+9;
    int events = getInt(offset, buf);
    if (events > 0) {
        for (int i = 0; i < events; ++i) {
            skipName(offset, buf);skipName(offset, buf);
            offset += 7*4;
            offset += 3 + 2 * 2 + 4 * 4 + 6 * 1 + 7 * 2 + 4;
        }
    }
    offset += 4;
}
inline void skipQuestGuard(int &offset, vector<unsigned char> &buf) {
    int quest = getChar(offset, buf);
    switch(quest) {
        case 0:
            break;
        case 1:
            offset += 4;
            break;
        case 2:
           offset += 4;
           break;
        case 3:
           offset += 4;
           break;
        case 4:
           offset += 4;
           break;
        case 5:{
           int arts = getChar(offset, buf);
           offset += arts * 2;
           break;}
        case 6:{
           int creatures_quantity = getChar(offset, buf);
           if ( creatures_quantity > 0 ) {
               offset += creatures_quantity * 4;
           }
           break;}
        case 7:
           offset += 4*7;
           break;
        case 8:
           offset += 1;
           break;
        case 9:
           offset += 1;
           break;
    };
    offset += 4;
    skipName(offset, buf);
    skipName(offset, buf);
    skipName(offset, buf);
}
inline void skipHero(int &offset, vector<unsigned char> &buf) {
    offset += 4;
    offset += 1+1;
    unsigned char name = getChar(offset, buf);
    if (name == 1) skipName(offset, buf);
    unsigned char exp = getChar(offset, buf);
    if (exp == 1) offset += 4;
    unsigned char portrait = getChar(offset, buf);
    if (portrait == 1) offset += 1;
    unsigned char sec = getChar(offset, buf);
    if (sec == 1) {
        int skills = getInt(offset, buf);
        if (skills > 0) offset += skills*2;
    }
    unsigned char isCreature = getChar(offset, buf);
    if (isCreature == 1) offset += 4*7;
    offset++;
    unsigned char artefacts = getChar(offset, buf);

    if (artefacts == 1) {
        offset += 19*2; //equipped
        int backPackCount = getShort(offset,buf);
        if (backPackCount != 0) offset += 2 * backPackCount;
    }
    offset++;
    unsigned char bio = getChar(offset, buf);
    if (bio == 1) skipName(offset, buf);
    offset++;
    unsigned char spells = getChar(offset, buf);
    if (spells == 1) offset += 9;
    unsigned char primary = getChar(offset, buf);
    if (primary == 1) offset += 4;
    offset += 4 * 4;
}

inline void skipMonster(int &offset, vector<unsigned char> &buf) {
    offset += 4+2+1;
    int treasure = getChar(offset, buf);
    if (treasure == 1) {
        skipName(offset, buf);
        offset += 4*7+2;
    }
    offset += 2+2;
}

inline void skipSeer(int &offset, vector<unsigned char> &buf) {
    int quest = getChar(offset, buf);
    switch(quest) {
        case 0:
            goto reward;
            break;
        case 1:
            offset += 4;
            break;
        case 2:
            offset += 4;
            break;
        case 3:
            offset += 4;
            break;
        case 4:
            offset += 4;
            break;
        case 5:{
            int art_quantity = getChar(offset, buf);
            offset += art_quantity * 2;
            break;}
        case 6:{
            int creatures_quantity = getChar(offset, buf);
            if (creatures_quantity > 0) offset += creatures_quantity * (2+2);
            break;}
        case 7:
            offset += 4*7;
            break;
        case 8:
            offset += 1;
            break;
        case 9:
            offset += 1;
            break;
    };
    offset += 4;
    skipName(offset, buf);
    skipName(offset, buf);
    skipName(offset, buf);
reward:
    int reward = getChar(offset, buf);
    switch(reward) {
       case 0:
           break;
       case 1:
           offset += 4;
           break;
       case 2:
           offset += 4;
           break;
       case 3:
           offset += 1;
           break;
       case 4:
           offset += 1;
           break;
       case 5:
           offset += 1;
           offset += 4;
           break;
       case 6:
           offset += 1+1;
           break;
       case 7:
           offset += 1+1;
           break;
       case 8:
           offset += 2;
           break;
       case 9:
           offset += 1;
           break;
       case 10:
           offset += 2+2;
           break;
    };
    offset += 2;//junk
}
