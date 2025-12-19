#include <iostream>
#include <vector>
#include <iomanip>
#include <limits>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <thread>
#include <locale> 

#define NOMINMAX 

#ifdef _WIN32
// Includes for Windows API functions and definitions
#include <windows.h>
#include <conio.h>

#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

#else
// Linux/Mac includes for getch equivalent
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

// --- Data Structures ---

struct Subject {
    string name;
    double grade;
};

struct Student {
    string id;
    string name;
    int age = 0;
    string contactNumber;
    string email;
    string birthday;
    string bloodType;
    string course;
    string section; 
    double tuitionOverride = -1.0;
    // -- NEW FIELDS --
    int semester = 1;            // 1 or 2
    double accountBalance = 0.0; // The money in their "Wallet"
    double tuitionPaid = 0.0;    // Amount paid towards tuition
    
    // 0=None, 1=Sem1, 2=Sem2
    int scholarshipSemester = 0; 
    // ----------------
    
    vector<Subject> subjects; 
    
    double average = 0.0;
    char letter = 'F';
};

vector<Student> students;

const string DB_FILENAME = "students_data.txt";
const double COST_PER_SUBJECT = 500.0;

// --- Console Coloring (Windows/Other) ---

#ifdef _WIN32
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

const int DEFAULT   = 7;  
const int VIOLET    = 13; 
const int PINK      = 12; 
const int PEACH     = 14; 
const int MINT      = 10; 
const int TEAL      = 11; 
const int WHITE     = 15; 
#else
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_VIOLET  "\x1b[1;35m" 
#define ANSI_COLOR_PINK    "\x1b[1;31m" 
#define ANSI_COLOR_PEACH   "\x1b[1;33m" 
#define ANSI_COLOR_MINT    "\x1b[1;32m" 
#define ANSI_COLOR_TEAL    "\x1b[1;36m" 
#define ANSI_COLOR_WHITE   "\x1b[1;37m" 
#define ANSI_COLOR_DEFAULT "\x1b[0m"

const int DEFAULT = 0; const int VIOLET = 1; const int PINK = 2;
const int PEACH = 3; const int MINT = 4; const int TEAL = 5; const int WHITE = 6;
#endif

void setColor(int color) {
#ifdef _WIN32
    SetConsoleTextAttribute(hConsole, color);
#else
    // Linux/Mac colors simplified
    if(color == VIOLET) cout << ANSI_COLOR_VIOLET;
    else if(color == PINK) cout << ANSI_COLOR_PINK;
    else if(color == PEACH) cout << ANSI_COLOR_PEACH;
    else if(color == MINT) cout << ANSI_COLOR_MINT;
    else if(color == TEAL) cout << ANSI_COLOR_TEAL;
    else if(color == WHITE) cout << ANSI_COLOR_WHITE;
    else cout << ANSI_COLOR_RESET;
#endif
}

// --- Input Helper (Immediate Keypress) ---
char getChar() {
#ifdef _WIN32
    return _getch();
#else
    struct termios oldt, newt;
    char ch;
    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
#endif
}

string getPassword() {
    string password;
    char ch;
    while (true) {
        ch = getChar();
        if (ch == '\r' || ch == '\n') { // Enter key
            cout << endl;
            break;
        } else if (ch == '\b' || ch == 127) { // Backspace (8 or 127)
            if (!password.empty()) {
                password.pop_back();
                cout << "\b \b"; // Move back, print space, move back again
            }
        } else {
            password += ch;
            cout << '*';
        }
    }
    return password;
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

//Press any key to conitunre churva
void pauseScreen() {
    setColor(TEAL);
    cout << "\n                                                                                           ᴘʀᴇꜱꜱ ᴀɴʏ ᴋᴇʏ ᴛᴏ ᴄᴏɴᴛɪɴᴜᴇ...";
    setColor(DEFAULT);
    getChar(); 
}

//Slow Print F
void slowPrint(const string& text, int delay_ms = 30) {
    for (char c : text) {
        cout << c << flush;
        this_thread::sleep_for(chrono::milliseconds(delay_ms / 3)); 
    }
}

void initializeConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"ScholarSphere Student Records Management");

    setlocale(LC_ALL, "en_US.UTF-8");
#else
    setlocale(LC_ALL, "");
#endif
}

// ---------------
//  HELPERS
// ---------------
void printHLine(int width) {
    for(int i=0; i<width; i++) cout << "═";
}


string toLower(string str) {
    transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c){ return tolower(c); });
    return str;
}
    //Letter Grade Condition
char getLetterGrade(double avg) {
    if (avg >= 90) return 'A';
    else if (avg >= 80) return 'B';
    else if (avg >= 70) return 'C';
    else if (avg >= 60) return 'D';
    else return 'F';
}
    //Average Calculator
void computeAverageAndLetter(Student &s) {
    double total = 0;
    for (const auto &sub : s.subjects) total += sub.grade;
    
    s.average = s.subjects.empty() ? 0.0 : total / s.subjects.size();
    s.letter = getLetterGrade(s.average);
}

// ---------------
// FINANCE HELPERS
// ---------------
double calculateTotalTuition(const Student &s) {
    // Tuition is free ONLY if the scholarship matches the current semester
    if (s.scholarshipSemester == s.semester) return 0.0;
    return s.subjects.size() * COST_PER_SUBJECT;
}

double calculateOutstandingBalance(const Student &s) {
    double total = calculateTotalTuition(s);
    double balance = total - s.tuitionPaid;
    return (balance < 0) ? 0.0 : balance;
}

bool checkExit(const string &input) {
    string lower_input = toLower(input);
    return lower_input == "exit" || lower_input == "quit" || lower_input == "c";
}

int findStudentIndexByID(const string &id) {
    for (size_t i = 0; i < students.size(); ++i) {
        if (students[i].id == id) return (int)i;
    }
    return -1;
}

void sortStudentsByName() {
    sort(students.begin(), students.end(), [](const Student& a, const Student& b) {
        string nameA = toLower(a.name);
        string nameB = toLower(b.name);
        return nameA < nameB;
    });
}

// --- File I/O ---

void saveStudentsToFile() {
    ofstream file(DB_FILENAME);
    if (!file.is_open()) {
        setColor(PEACH);
        cout << "ᴇʀʀᴏʀ: ᴄᴏᴜʟᴅ ɴᴏᴛ ꜱᴀᴠᴇ ᴅᴀᴛᴀ ᴛᴏ ꜰɪʟᴇ.\n";
        setColor(DEFAULT);
        return;
    }

    for (const auto &s : students) {
        file << s.id << "|" << s.name << "|" << s.age << "|"
             << s.contactNumber << "|" << s.email << "|" << s.birthday << "|" << s.bloodType << "|"
             << s.course << "|" << s.section << "|" 
             << s.semester << "|" << s.accountBalance << "|" << s.tuitionPaid << "|" << s.scholarshipSemester;
        
        for (const auto &sub : s.subjects) {
            file << "|" << sub.name << "|" << sub.grade;
        }
        file << "\n";
    }
    file.close();
}

void loadStudentsFromFile() {
    ifstream file(DB_FILENAME);
    if (!file.is_open()) return;

    students.clear();
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        stringstream ss(line);
        string segment;
        vector<string> data;
        
        while (getline(ss, segment, '|')) {
            data.push_back(segment);
        }

        if (data.size() < 12) continue; 

        Student s;
        s.id = data[0];
        s.name = data[1];
        try { s.age = stoi(data[2]); } catch (...) { s.age = 0; }
        s.contactNumber = data[3];
        s.email = data[4];
        s.birthday = data[5];
        s.bloodType = data[6];
        s.course = data[7];
        s.section = data[8];
        
        try { s.semester = stoi(data[9]); } catch (...) { s.semester = 1; }
        try { s.accountBalance = stod(data[10]); } catch (...) { s.accountBalance = 0.0; }
        try { s.tuitionPaid = stod(data[11]); } catch (...) { s.tuitionPaid = 0.0; }
        
        try { s.scholarshipSemester = (data.size() > 12) ? stoi(data[12]) : 0; } catch (...) { s.scholarshipSemester = 0; }

        size_t subjectStartIndex = (data.size() > 12) ? 13 : 12;

        for (size_t i = subjectStartIndex; i < data.size(); i += 2) {
            if (i + 1 < data.size()) {
                Subject sub;
                sub.name = data[i];
                try { sub.grade = stod(data[i+1]); } catch (...) { sub.grade = 0; }
                s.subjects.push_back(sub);
            }
        }
        
        computeAverageAndLetter(s); 
        students.push_back(s);
    }
    file.close();
    sortStudentsByName();
}

// --- Features ---
//-------------------------
//REGISTER NEW STUDENT 
//-------------------------
void registerStudentInteractive() {
    clearScreen();
    setColor(VIOLET);
    vector<string> ascii_lines = {
        "\n",
        "                             ██████╗ ███████╗ ██████╗ ██╗███████╗████████╗███████╗██████╗     ███╗   ██╗███████╗██╗    ██╗    ███████╗████████╗██╗   ██╗██████╗ ███████╗███╗   ██╗████████╗",
        "                             ██╔══██╗██╔════╝██╔════╝ ██║██╔════╝╚══██╔══╝██╔════╝██╔══██╗    ████╗  ██║██╔════╝██║    ██║    ██╔════╝╚══██╔══╝██║   ██║██╔══██╗██╔════╝████╗  ██║╚══██╔══╝",
        "                             ██████╔╝█████╗  ██║  ███╗██║███████╗   ██║   █████╗  ██████╔╝    ██╔██╗ ██║█████╗  ██║ █╗ ██║    ███████╗   ██║   ██║   ██║██║  ██║█████╗  ██╔██╗ ██║   ██║   ",
        "                             ██╔══██╗██╔══╝  ██║   ██║██║╚════██║   ██║   ██╔══╝  ██╔══██╗    ██║╚██╗██║██╔══╝  ██║███╗██║    ╚════██║   ██║   ██║   ██║██║  ██║██╔══╝  ██║╚██╗██║   ██║   ",
        "                             ██║  ██║███████╗╚██████╔╝██║███████║   ██║   ███████╗██║  ██║    ██║ ╚████║███████╗╚███╔███╔╝    ███████║   ██║   ╚██████╔╝██████╔╝███████╗██║ ╚████║   ██║   ",
        "                             ╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝    ╚═╝  ╚═══╝╚══════╝ ╚══╝╚══╝     ╚══════╝   ╚═╝    ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝   ╚═╝   ",
        "\n"
        "\n",
        "                                                                                 ┏━╸┏┓╻╺┳╸┏━╸┏━┓    ┏━╸┏━┓┏━╸╺┳┓┏━╸┏┓╻╺┳╸╻┏━┓╻  ┏━┓                            ",
        "                                                                       ╺━╸╺━╸    ┣╸ ┃┗┫ ┃ ┣╸ ┣┳┛    ┃  ┣┳┛┣╸  ┃┃┣╸ ┃┗┫ ┃ ┃┣━┫┃  ┗━┓  ╺━╸╺━╸                    ",
        "                                                                                 ┗━╸╹ ╹ ╹ ┗━╸╹┗╸    ┗━╸╹┗╸┗━╸╺┻┛┗━╸╹ ╹ ╹ ╹╹ ╹┗━╸┗━┛                            ",
        "\n"
    };

    for (const string& line : ascii_lines) { 
        cout << line << endl; 
        this_thread::sleep_for(chrono::milliseconds(20));
    }
    setColor(WHITE);

    cin.clear();
    // Removed the peek/ignore check to fix the pause issue
    
    Student s;
    cout << "                                                                                          (ᴛʏᴘᴇ 'ᴇxɪᴛ' ᴏʀ 'ᴄ' ᴀɴʏᴛɪᴍᴇ ᴛᴏ ᴄᴀɴᴄᴇʟ)\n";
    cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n";

    cout << "\n                                                                                   ᴇɴᴛᴇʀ ꜱᴛᴜᴅᴇɴᴛ ɪᴅ: ";
    getline(cin, s.id);
    if (checkExit(s.id)) return;

    if (findStudentIndexByID(s.id) != -1) {
        setColor(PEACH); 
    cout << "                                                                                    ɪᴅ ᴀʟʀᴇᴀᴅʏ ᴇxɪꜱᴛꜱ! ʀᴇɢɪꜱᴛʀᴀᴛɪᴏɴ ᴄᴀɴᴄᴇʟʟᴇᴅ.\n";
        setColor(WHITE); 
    cout << "                                                                   ╚═══════════════════════════════════════════════════════════════════════════════╝\n"; pauseScreen(); return;

    }

    cout << "                                                                                   ᴇɴᴛᴇʀ ꜰᴜʟʟ ɴᴀᴍᴇ: ";
    getline(cin, s.name);
    if (checkExit(s.name)) return;

    while(true) {
        cout << "                                                                                   ᴇɴᴛᴇʀ ꜱᴇᴍᴇꜱᴛᴇʀ (1 ᴏʀ 2): ";
        string semInput;
        getline(cin, semInput);
        if (checkExit(semInput)) return;
        try {
            s.semester = stoi(semInput);
            if(s.semester == 1 || s.semester == 2) break;
        } catch(...) {}
        setColor(PEACH); cout << "                                                                         ɪɴᴠᴀʟɪᴅ ɪɴᴘᴜᴛ. ᴘʟᴇᴀꜱᴇ ᴇɴᴛᴇʀ 1 ᴏʀ 2.\n"; setColor(WHITE);
    }

    string ageInput;
    cout << "                                                                                   ᴇɴᴛᴇʀ ᴀɢᴇ: ";
    getline(cin, ageInput);
    if (checkExit(ageInput)) return;
    try { s.age = stoi(ageInput); } catch (...) { s.age = 0; }

    cout << "                                                                                   ᴇɴᴛᴇʀ ᴄᴏᴜʀꜱᴇ (ᴇ.ɢ. ʙꜱᴄꜱ): ";
    getline(cin, s.course);
    if (checkExit(s.course)) return;

    cout << "                                                                                   ᴇɴᴛᴇʀ ꜱᴇᴄᴛɪᴏɴ (ᴇ.ɢ. 1-ᴀ): ";
    getline(cin, s.section);
    if (checkExit(s.section)) return;

    cout << "                                                                                   ᴇɴᴛᴇʀ ᴄᴏɴᴛᴀᴄᴛ ɴᴜᴍʙᴇʀ: ";
    getline(cin, s.contactNumber);
    if (checkExit(s.contactNumber)) return;

    cout << "                                                                                   ᴇɴᴛᴇʀ ᴇ-ᴍᴀɪʟ ᴀᴅᴅʀᴇꜱꜱ: ";
    getline(cin, s.email);
    if (checkExit(s.email)) return;

    cout << "                                                                                   ᴇɴᴛᴇʀ ʙɪʀᴛʜᴅᴀʏ (ᴍᴍ/ᴅᴅ/ʏʏʏʏ): ";
    getline(cin, s.birthday);
    if (checkExit(s.birthday)) return;

    cout << "                                                                                   ᴇɴᴛᴇʀ ʙʟᴏᴏᴅ ᴛʏᴘᴇ (ᴇ.ɢ., ᴀ+, ᴏ-): ";
    getline(cin, s.bloodType);
    cout << "\n                                                                   ╠═══════════════════════════════════════════════════════════════════════════════╣\n";
    if (checkExit(s.bloodType)) return;

    while (true) {
        setColor(TEAL); cout << "\n                                                                                   ᴇɴʀᴏʟʟ ɪɴ ᴀ ꜱᴜʙᴊᴇᴄᴛ? (ʏ/ɴ ᴏʀ ᴇxɪᴛ): "; setColor(DEFAULT);
        string choice; getline(cin, choice);
        if (checkExit(choice)) return;
        if (toLower(choice) != "y") break;

        Subject sub;
        cout << "                                                                                   ꜱᴜʙᴊᴇᴄᴛ ɴᴀᴍᴇ: "; getline(cin, sub.name);
        if (checkExit(sub.name)) return;

        sub.grade = 0; 
        s.subjects.push_back(sub);
        cout << "                                                                                   ꜱᴜʙᴊᴇᴄᴛ ᴇɴʀᴏʟʟᴇᴅ.\n";
    }

    computeAverageAndLetter(s);
    students.push_back(s);
    sortStudentsByName();
    saveStudentsToFile();

    setColor(MINT);
    slowPrint("\n                                                                                 ꜱᴛᴜᴅᴇɴᴛ ᴇɴʀᴏʟʟᴇᴅ ꜱᴜᴄᴄᴇꜱꜱꜰᴜʟʟʏ! ʏᴏᴜ ᴄᴀɴ ɴᴏᴡ ʟᴏɢ ɪɴ.\n", 10);
    setColor(WHITE); 
      cout << "                                                                   ╚═══════════════════════════════════════════════════════════════════════════════╝\n";
    pauseScreen();
}

// --- ADJUST THIS NUMBER to center it on your specific screen ---
const string CENTER_PADDING = string(60, ' '); 

void drawProfileLine(const string& left_label, const string& right_label, int card_width, int text_color) {
    int content_width = card_width - 4;
    int col_width = (content_width / 2) - 2; 

    auto truncate = [&](string s, int w) -> string {
        if ((int)s.length() > w) return s.substr(0, w - 3) + "...";
        return s;
    };

    string l_text = truncate(left_label, col_width);
    string r_text = truncate(right_label, col_width);
    
    // 1. Padding + Left Border (WHITE)
    setColor(WHITE); 
    cout << CENTER_PADDING << "║ "; 
    
    // 2. Left Text (Custom Color)
    setColor(text_color);
    cout << left << setw(col_width) << l_text;

    // 3. Middle Separator (WHITE)
    setColor(WHITE);
    cout << " ║ ";
    
    // 4. Right Text (Custom Color)
    setColor(text_color);
    cout << left << setw(col_width) << r_text;
    
    // 5. Right Border (WHITE)
    setColor(WHITE);
    int used = 2 + col_width + 3 + col_width; 
    int remaining = card_width - used - 1; 
    cout << setw(remaining) << "" << "║\n";
}

//-------------------------
// Student Record Menu/Student Profile (Student View)
//-------------------------
void viewOneStudent(const Student &s) {
    clearScreen();
    setColor(VIOLET); 
    vector<string> ascii_lines = {
        "\n",
        "                                                    ███████╗████████╗██╗   ██╗██████╗ ███████╗███╗   ██╗████████╗  ██████╗ ██████╗  ██████╗ ███████╗██╗██╗     ███████╗",
        "                                                    ██╔════╝╚══██╔══╝██║   ██║██╔══██╗██╔════╝████╗  ██║╚══██╔══╝  ██╔══██╗██╔══██╗██╔═══██╗██╔════╝██║██║     ██╔════╝",
        "                                                    ███████╗   ██║   ██║   ██║██║  ██║█████╗  ██╔██╗ ██║   ██║     ██████╔╝██████╔╝██║   ██║█████╗  ██║██║     █████╗  ",
        "                                                    ╚════██║   ██║   ██║   ██║██║  ██║██╔══╝  ██║╚██╗██║   ██║     ██╔═══╝ ██╔══██╗██║   ██║██╔══╝  ██║██║     ██╔══╝  ",
        "                                                    ███████║   ██║   ╚██████╔╝██████╔╝███████╗██║ ╚████║   ██║     ██║     ██║  ██║╚██████╔╝██║     ██║███████╗███████ ",
        "                                                    ╚══════╝   ╚═╝    ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝   ╚═╝     ╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝╚══════╝",
        "\n"
    };

    for (const string& line : ascii_lines) { 
        cout << line << endl; 
        this_thread::sleep_for(chrono::milliseconds(20)); 
    }
    
    const int CARD_WIDTH = 100; 

    // --- HEADER TEXT ---
    string headerName = "ꜱᴛᴜᴅᴇɴᴛ ʀᴇᴄᴏʀᴅ ꜰᴏʀ: " + s.name;
    for (char &c : headerName) c = toupper(c);

    int textPadding = (CARD_WIDTH - headerName.length());
    cout << "\n"; 
    cout << CENTER_PADDING << string(textPadding, ' ');
    setColor(VIOLET); cout << headerName << "\n";
    setColor(WHITE); slowPrint("                                                                                     ╔══════════════════════════════════════════════════╗\n\n", 2);
    
    // --- TOP BORDER ---
    setColor(WHITE);
    cout << CENTER_PADDING << "╔"; printHLine(CARD_WIDTH - 2); cout << "╗\n";

    // --- Student Details ---
    drawProfileLine("ID: " + s.id, "Name: " + s.name, CARD_WIDTH, TEAL);
    drawProfileLine("Course: " + s.course, "Section: " + s.section + " (Sem: " + to_string(s.semester) + ")", CARD_WIDTH, TEAL);
    
    // --- SEPARATOR ---
    setColor(WHITE); cout << CENTER_PADDING << "╠"; printHLine(CARD_WIDTH - 2); cout << "╣\n";
    
    drawProfileLine("Age: " + to_string(s.age), "Blood Type: " + s.bloodType, CARD_WIDTH, TEAL);
    drawProfileLine("Birthday: " + s.birthday, "Contact: " + s.contactNumber, CARD_WIDTH, TEAL);
    drawProfileLine("Email: " + s.email, "", CARD_WIDTH, TEAL);
    
    // --- FINANCIAL SECTION ---
    setColor(WHITE); cout << CENTER_PADDING << "╠"; printHLine(CARD_WIDTH - 2); cout << "╣\n";
    
    string finance_title = "FINANCIAL STATUS";
    int padding = (CARD_WIDTH - finance_title.length() - 2) / 2;
    int remainder = CARD_WIDTH - finance_title.length() - 2 - padding;
    
    setColor(WHITE); cout << CENTER_PADDING << "║" << setw(padding) << ""; 
    setColor(PEACH); cout << finance_title;
    setColor(WHITE); cout << setw(remainder) << "" << "║\n";

    setColor(WHITE); cout << CENTER_PADDING << "╠"; printHLine(CARD_WIDTH - 2); cout << "╣\n";

    double totalTuition = calculateTotalTuition(s);
    double outstanding = calculateOutstandingBalance(s);

    stringstream ss_wallet, ss_tuition, ss_paid, ss_due;
    ss_wallet << fixed << setprecision(2) << s.accountBalance;
    ss_tuition << fixed << setprecision(2) << totalTuition;
    ss_paid << fixed << setprecision(2) << s.tuitionPaid;
    ss_due << fixed << setprecision(2) << outstanding;

    string scholStatus = "NONE";
    if (s.scholarshipSemester == 1) scholStatus = "ACTIVE (1st Sem)";
    else if (s.scholarshipSemester == 2) scholStatus = "ACTIVE (2nd Sem)";

    drawProfileLine("Wallet: PHP " + ss_wallet.str(), "Scholarship: " + scholStatus, CARD_WIDTH, WHITE);
    drawProfileLine("Tuition: PHP " + ss_tuition.str(), "Paid: PHP " + ss_paid.str(), CARD_WIDTH, WHITE);
    
    // --- OUTSTANDING BALANCE ---
    setColor(WHITE); cout << CENTER_PADDING << "╠"; printHLine(CARD_WIDTH - 2); cout << "╣\n";
    
    string dueStr = " OUTSTANDING BALANCE: PHP " + ss_due.str();
    setColor(WHITE); cout << CENTER_PADDING << "║ ";
    setColor(PINK);  cout << left << setw(CARD_WIDTH - 4) << dueStr;
    setColor(WHITE); cout << " ║\n";
    
    // --- ACADEMIC SUMMARY ---
    setColor(WHITE);
    cout << CENTER_PADDING << "╠"; printHLine(CARD_WIDTH - 2); cout << "╣\n";
    string academic_title = "ACADEMIC SUMMARY";
    
    padding = (CARD_WIDTH - academic_title.length() - 2) / 2;
    remainder = CARD_WIDTH - academic_title.length() - 2 - padding;
    
    setColor(WHITE); cout << CENTER_PADDING << "║" << setw(padding) << "";
    setColor(PEACH); cout << academic_title;
    setColor(WHITE); cout << setw(remainder) << "" << "║\n";
    
    setColor(WHITE); cout << CENTER_PADDING << "╠"; printHLine(CARD_WIDTH - 2); cout << "╣\n";

    // Logic for Color and Status
    string avg_display = s.average == 0.0 && !s.subjects.empty() ? "0.00" : 
                         (s.average == 0.0 && s.subjects.empty() ? "N/A" : 
                          to_string(s.average).substr(0, to_string(s.average).find('.') + 3));

    int summary_color;
    string summary_status;
    
    if (s.subjects.empty()) {
        summary_color = WHITE; summary_status = "N/A";
    }
    else if (s.letter == 'A' || s.letter == 'B') { 
        summary_color = MINT; summary_status = "EXCELLENT"; 
    }
    else if (s.letter == 'C' || s.letter == 'D') { 
        summary_color = PEACH; summary_status = "PASSING"; 
    }
    else { 
        summary_color = PINK; summary_status = "FAILING"; 
    }

    // --- MANUAL PRINTING FOR MIXED COLORS ---
    // We construct the row manually to have White Labels and Colored Values
    int inner_w = CARD_WIDTH - 2;
    int half_w = inner_w / 2;
    string indent = " "; // small left padding
    
    string lbl1 = "Final Average: ";
    string val1 = avg_display;
    string lbl2 = "Letter Grade: ";
    string val2 = string(1, s.letter) + " (" + summary_status + ")";

    setColor(WHITE); cout << CENTER_PADDING << "║";

    // Left Column
    cout << indent;
    setColor(WHITE); cout << lbl1;         // Label in White
    setColor(summary_color); cout << val1; // Value in Color
    // Fill remaining space of first half
    int used_1 = indent.length() + lbl1.length() + val1.length();
    if(half_w > used_1) setColor(WHITE); cout << string(half_w - used_1, ' ');

    // Right Column
    cout << indent;
    setColor(WHITE); cout << lbl2;         // Label in White
    setColor(summary_color); cout << val2; // Value in Color
    // Fill remaining space of second half
    int used_2 = indent.length() + lbl2.length() + val2.length();
    int right_space = (inner_w - half_w) - used_2;
    if(right_space > 0) setColor(WHITE); cout << string(right_space, ' ');

    setColor(WHITE); cout << "║\n";
    // ----------------------------------------
    
    setColor(WHITE); cout << CENTER_PADDING << "╠"; printHLine(CARD_WIDTH - 2); cout << "╣\n";

    // --- SUBJECT GRADES ---
    if (!s.subjects.empty()) {
        string grades_title = "SUBJECT GRADES";
        int p_local = (CARD_WIDTH - grades_title.length() - 2) / 2;
        int r_local = CARD_WIDTH - grades_title.length() - 2 - p_local;
        
        setColor(WHITE);  cout << CENTER_PADDING << "║" << setw(p_local) << "";
        setColor(PEACH); cout << grades_title;
        setColor(WHITE);  cout << setw(r_local) << "" << "║\n";
        
        const int W_STATUS = 15; 
        const int W_GRADE = 20;
        const int W_NAME = CARD_WIDTH - W_GRADE - W_STATUS - 9; 
        
        // Header
        setColor(WHITE); cout << CENTER_PADDING << "╠"; printHLine(W_NAME + 2); cout << "╦"; printHLine(W_GRADE + 2); cout << "╦"; printHLine(W_STATUS + 1); cout << "╣\n";

        // Columns
        setColor(WHITE); cout << CENTER_PADDING << "║ ";
        setColor(PEACH);  cout << left << setw(W_NAME) << " SUBJECT NAME";
        setColor(WHITE); cout << " ║ ";
        setColor(PEACH);  cout << setw(W_GRADE) << " GRADE";
        setColor(WHITE); cout << " ║ ";
        setColor(PEACH);  cout << setw(W_STATUS) << " STATUS";
        setColor(WHITE); cout << "║\n";

        // Divider
        setColor(WHITE); cout << CENTER_PADDING << "╠"; printHLine(W_NAME + 2); cout << "╬"; printHLine(W_GRADE + 2); cout << "╬"; printHLine(W_STATUS + 1); cout << "╣\n";
        
        for (const auto &sub : s.subjects) {
            string status;
            int row_color;

            if (sub.grade == 0) {
                row_color = WHITE; status = "ENROLLED";
            }
            else if (sub.grade >= 90.0) {
                row_color = MINT; status = "EXCELLENT";
            }
            else if (sub.grade >= 75.0) {
                row_color = PEACH; status = "PASSING";
            }
            else {
                row_color = PINK; status = "FAILED";
            }

            string dName = (int)sub.name.length() > (W_NAME - 1) ? sub.name.substr(0, W_NAME - 4) + "..." : sub.name;

            // Name
            setColor(WHITE); cout << CENTER_PADDING << "║ ";
            setColor(WHITE); cout << left << setw(W_NAME) << (" " + dName);
            // Grade
            setColor(WHITE); cout << " ║ ";
            setColor(row_color); cout << " " << setw(W_GRADE - 1) << fixed << setprecision(2) << sub.grade;
            // Status
            setColor(WHITE); cout << " ║ ";
            setColor(row_color); cout << setw(W_STATUS) << (" " + status) << "║\n";
        }
        
        setColor(WHITE); cout << CENTER_PADDING << "╚"; printHLine(W_NAME + 2); cout << "╩"; printHLine(W_GRADE + 2); cout << "╩"; printHLine(W_STATUS + 1); cout << "╝\n";
    } else {
        setColor(WHITE); cout << CENTER_PADDING << "╚"; printHLine(CARD_WIDTH - 2); cout << "╝\n";
    }

    setColor(DEFAULT);
    pauseScreen();
}

// --- HELPER: Draw a single row in the finance table ---
// --- HELPER FUNCTION: Draw a single row with correct alignment ---
void drawFinanceRow(string label, string value, int valueColor) {
    // 1. This padding moves the whole row 30 spaces right
    const string CENTER_PADDING = string(30, ' '); 
    
    // 2. Exact Column Widths to match the borders (31 and 48)
    int labelWidth = 30; 
    int valueLimit = 47; 
    // --- LEFT COLUMN ---
    setColor(WHITE); 
    cout << CENTER_PADDING << "║ "; 
    
    // Print Label (White)
    cout << left << setw(labelWidth) << label;
    
    // --- MIDDLE SEPARATOR ---
    cout << "║ "; 
    
    // --- RIGHT COLUMN ---
    // We print the value in COLOR, then switch to WHITE to fill the empty space.
    // This prevents the green "highlight" from stretching across the empty space.
    setColor(valueColor); 
    cout << value;
    
    // Calculate how much empty space is left and print it in WHITE
    int currentLength = (int)value.length();
    if (currentLength < valueLimit) {
        setColor(WHITE);
        cout << string(valueLimit - currentLength, ' ');
    }
    
    // --- RIGHT BORDER ---
    setColor(WHITE); 
    cout << "║\n";
}  
//-------------------
//FINANCE PORTAL MENU (Finance/Financial Management)
//-------------------
void financeManagement(Student* loggedInStudent = nullptr) {
    clearScreen();
    
    // Check if we are in Admin Mode (passed nullptr) or Student Mode
    bool isAdmin = (loggedInStudent == nullptr);

    setColor(VIOLET);
    vector<string> ascii_lines = {
        "\n",
        "                                                        ███████╗██╗███╗   ██╗ █████╗ ███╗   ██╗ ██████╗███████╗    ██████╗  ██████╗ ██████╗ ████████╗ █████╗  ██╗     ",
        "                                                        ██╔════╝██║████╗  ██║██╔══██╗████╗  ██║██╔════╝██╔════╝    ██╔══██╗██╔═══██╗██╔══██╗╚══██╔══╝██╔══██╗ ██║     ",
        "                                                        █████╗  ██║██╔██╗ ██║███████║██╔██╗ ██║██║     █████╗      ██████╔╝██║   ██║██████╔╝   ██║   ███████║ ██║     ",
        "                                                        ██╔══╝  ██║██║╚██╗██║██╔══██║██║╚██╗██║██║     ██╔══╝      ██╔═══╝ ██║   ██║██╔══██╗   ██║   ██╔══██║ ██║     ",
        "                                                        ██║     ██║██║ ╚████║██║  ██║██║ ╚████║╚██████╗███████╗    ██║     ╚██████╔╝██║  ██║   ██║   ██║  ██║ ███████╗",
        "                                                        ╚═╝     ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝╚══════╝    ╚═╝      ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝ ╚══════╝",
        "\n"
    };

    for (const string& line : ascii_lines) { 
        cout << line << endl; 
        this_thread::sleep_for(chrono::milliseconds(20)); 
    }
    setColor(WHITE);

    Student* sPtr = loggedInStudent;

    if (isAdmin) {
        cin.clear();
        cout << "\n                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n\n"; 
        cout << "                                                                                       ᴇɴᴛᴇʀ ꜱᴛᴜᴅᴇɴᴛ ɪᴅ ᴛᴏ ᴀᴄᴄᴇꜱꜱ ꜰɪɴᴀɴᴄᴇ: ";
        string id;
        getline(cin, id);
        if (checkExit(id)) return;

        int index = findStudentIndexByID(id);
        if (index == -1) {
            setColor(PEACH); 
        cout << "                                                                                                  ꜱᴛᴜᴅᴇɴᴛ ɴᴏᴛ ꜰᴏᴜɴᴅ.\n"; pauseScreen(); return;
        }
        sPtr = &students[index];
    } 

    Student &s = *sPtr;
    
    while(true) {
        // --- LOGIC: Calculate Tuition (Handling Override) ---
        double totalTuition;
        bool isCustomTuition = (s.tuitionOverride >= 0); 
        
        if (isCustomTuition) totalTuition = s.tuitionOverride;
        else totalTuition = calculateTotalTuition(s);
        
        double outstanding = (totalTuition - s.tuitionPaid);
        if (outstanding < 0) outstanding = 0; 

        // Refresh Display
        clearScreen(); 
        setColor(VIOLET);
        for (const string& line : ascii_lines) cout << line << endl; 

        // Header
        setColor(VIOLET);
        cout << "                                                                                       == ꜰɪɴᴀɴᴄᴇ ᴘᴏʀᴛᴀʟ: " << s.name << " (" << s.course << ") ==\n";
        setColor(TEAL);
        cout  << "                                                                                                  ᴄᴜʀʀᴇɴᴛ ꜱᴇᴍᴇꜱᴛᴇʀ: " << (s.semester == 1 ? "1ꜱᴛ" : "2nd") << "\n\n";
        
        // --- TOP BORDER ---
        setColor(WHITE);
        cout << CENTER_PADDING << "        ╔"; printHLine(31); cout << "╦"; printHLine(48); cout << "╗\n";

        // --- ROW 1: WALLET ---
        stringstream ss_wallet; ss_wallet << "PHP " << fixed << setprecision(2) << s.accountBalance;
        string val1 = ss_wallet.str();
        
        setColor(WHITE); cout << CENTER_PADDING << "        ║ "; // Start Left
        cout << left << setw(30) << " 1. ᴡᴀʟʟᴇᴛ ʙᴀʟᴀɴᴄᴇ"; // Left Text
        cout << "            ║ "; // Middle
        setColor(MINT); cout << val1; // Right Value (Color)
        setColor(WHITE); cout << string(47 - val1.length(), ' '); // Right Padding (White)
        cout << "║\n"; // End Right

        // --- DIVIDER ---
        setColor(WHITE);
        cout << CENTER_PADDING << "        ╠"; printHLine(31); cout << "╬"; printHLine(48); cout << "╣\n";

        // --- ROW 2: TUITION ---
        stringstream ss_tuition; 
        ss_tuition << "PHP " << fixed << setprecision(2) << totalTuition;
        if (isCustomTuition) ss_tuition << " (ADMIN OVERRIDE)";
        else ss_tuition << " (" << s.subjects.size() << " subjects)";
        string val2 = ss_tuition.str();

        setColor(WHITE); cout << CENTER_PADDING << "        ║ ";
        cout << left << setw(30) << " 2. ᴛᴏᴛᴀʟ ᴛᴜɪᴛɪᴏɴ ꜰᴇᴇ";
        cout << "         ║ ";
        setColor(WHITE); cout << val2; // Value is White here
        cout << string(47 - val2.length(), ' ');
        cout << "║\n";

        // --- DIVIDER ---
        setColor(WHITE);
        cout << CENTER_PADDING << "        ╠"; printHLine(31); cout << "╬"; printHLine(48); cout << "╣\n";

        // --- ROW 3: PAID ---
        stringstream ss_paid; ss_paid << "PHP " << fixed << setprecision(2) << s.tuitionPaid;
        string val3 = ss_paid.str();

        setColor(WHITE); cout << CENTER_PADDING << "        ║ ";
        cout << left << setw(30) << " 3. ᴀᴍᴏᴜɴᴛ ᴘᴀɪᴅ";
        cout << "               ║ ";
        setColor(WHITE); cout << val3;
        cout << string(47 - val3.length(), ' ');
        cout << "║\n";

        // --- DIVIDER ---
        setColor(WHITE);
        cout << CENTER_PADDING << "        ╠"; printHLine(31); cout << "╬"; printHLine(48); cout << "╣\n";

        // --- ROW 4: OUTSTANDING ---
        stringstream ss_out; ss_out << "PHP " << fixed << setprecision(2) << outstanding;
        string val4 = ss_out.str();
        int dueColor = (outstanding > 0) ? PINK : MINT; 

        setColor(WHITE); cout << CENTER_PADDING << "        ║ ";
        cout << left << setw(30) << " 4. ᴏᴜᴛꜱᴛᴀɴᴅɪɴɢ ʙᴀʟᴀɴᴄᴇ";
        cout << "       ║ ";
        setColor(dueColor); cout << val4;
        setColor(WHITE); cout << string(47 - val4.length(), ' ');
        cout << "║\n";

        // --- DIVIDER ---
        setColor(WHITE);
        cout << CENTER_PADDING << "        ╠"; printHLine(31); cout << "╬"; printHLine(48); cout << "╣\n";

        // --- ROW 5: SCHOLARSHIP ---
        string val5 = "None";
        if(s.scholarshipSemester == 1) val5 = "Active (1st Sem)";
        else if(s.scholarshipSemester == 2) val5 = "Active (2nd Sem)";
        int scholColor = (s.scholarshipSemester > 0) ? TEAL : PEACH;

        setColor(WHITE); cout << CENTER_PADDING << "        ║ ";
        cout << left << setw(30) << " 5. ꜱᴄʜᴏʟᴀʀꜱʜɪᴘ ꜱᴛᴀᴛᴜꜱ";
        cout << "        ║ ";
        setColor(scholColor); cout << val5;
        setColor(WHITE); cout << string(47 - val5.length(), ' ');
        cout << "║\n";

        // --- BOTTOM BORDER ---
        setColor(WHITE);
        cout << CENTER_PADDING << "        ╚"; printHLine(31); cout << "╩"; printHLine(48); cout << "╝\n";
        setColor(WHITE);
        cout << "\n\n";
        cout << "                      ╔════════════════════════════════════════════════════════════════════════════════════════════╗       ╔═══════════════════════════════════════════════════════════════╗\n";
        cout << "                      ║ ▄▄        ▄▄                                                                               ║       ║ ▄▄        ▄▄                                                  ║\n";
        cout << "                      ║ █  ████▄   █    ▄▄▄▄  ▄▄▄▄▄ ▄▄▄▄   ▄▄▄   ▄▄▄▄ ▄▄ ▄▄▄▄▄▄   ▄▄   ▄▄  ▄▄▄  ▄▄  ▄▄ ▄▄▄▄▄ ▄▄ ▄▄ ║       ║ █  █████▄  █    █████▄  ▄▄▄  ▄▄ ▄▄   ██████ ▄▄▄▄▄ ▄▄▄▄▄  ▄▄▄▄ ║\n";
        cout << "                      ║ █  ██  ██  █    ██▀██ ██▄▄  ██▄█▀ ██▀██ ███▄▄ ██   ██     ██▀▄▀██ ██▀██ ███▄██ ██▄▄  ▀███▀ ║       ║ █  ██▄▄█▀  █    ██▄▄█▀ ██▀██ ▀███▀   ██▄▄   ██▄▄  ██▄▄  ███▄▄ ║\n";
        cout << "                      ║ █  ████▀   █    ████▀ ██▄▄▄ ██    ▀███▀ ▄▄██▀ ██   ██     ██   ██ ▀███▀ ██ ▀██ ██▄▄▄   █   ║       ║ █  ██      █    ██     ██▀██   █     ██     ██▄▄▄ ██▄▄▄ ▄▄██▀ ║\n";
        cout << "                      ║ ▀▀        ▀▀                                                                               ║       ║ ▀▀        ▀▀                                                  ║\n";
        cout << "                      ╚════════════════════════════════════════════════════════════════════════════════════════════╝       ╚═══════════════════════════════════════════════════════════════╝\n";
        cout << "                                    ╔═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗    \n";
        cout << "                                    ║ ▄▄        ▄▄                                                                                                                      ║    \n";
        cout << "                                    ║ █  ▄█████  █    ▄████▄ ▄▄▄▄  ▄▄▄▄  ▄▄  ▄▄ ▄▄   ▄▄▄▄▄  ▄▄▄  ▄▄▄▄     ▄▄▄▄  ▄▄▄▄ ▄▄ ▄▄  ▄▄▄  ▄▄     ▄▄▄  ▄▄▄▄   ▄▄▄▄ ▄▄ ▄▄ ▄▄ ▄▄▄▄  ║    \n";
        cout << "                                    ║ █  ▀▀▀▄▄▄  █    ██▄▄██ ██▄█▀ ██▄█▀ ██  ▀███▀   ██▄▄  ██▀██ ██▄█▄   ███▄▄ ██▀▀▀ ██▄██ ██▀██ ██    ██▀██ ██▄█▄ ███▄▄ ██▄██ ██ ██▄█▀ ║    \n";
        cout << "                                    ║ █  █████▀  █    ██  ██ ██    ██    ██▄▄▄ █     ██    ▀███▀ ██ ██   ▄▄██▀ ▀████ ██ ██ ▀███▀ ██▄▄▄ ██▀██ ██ ██ ▄▄██▀ ██ ██ ██ ██    ║    \n";
        cout << "                                    ║ ▀▀        ▀▀                                                                                                                      ║    \n";
        cout << "                                    ╚═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝    \n";
        // --- ADMIN ONLY OPTIONS ---
        if (isAdmin) {
            setColor(WHITE);
        cout << "                  ╔════════════════════════════════════════════════════════════════════════════════════════════════════════╗    ╔═══════════════════════════════════════════════════════════════╗\n";
        cout << "                  ║ ▄▄        ▄▄                                                                                           ║    ║ ▄▄        ▄▄                                                  ║\n";
        cout << "                  ║ █  ▄▄ ▄▄   █    ▄▄ ▄▄  ▄▄▄  ▄▄ ▄▄▄▄    ▄█████  ▄▄▄▄ ▄▄ ▄▄  ▄▄▄  ▄▄     ▄▄▄  ▄▄▄▄   ▄▄▄▄ ▄▄ ▄▄ ▄▄ ▄▄▄▄  ║    ║ █  ██████  █    ██████ ▄▄▄▄  ▄▄ ▄▄▄▄▄▄    ██████ ▄▄▄▄▄ ▄▄▄▄▄  ║\n";
        cout << "                  ║ █  ██▄██   █    ██▄██ ██▀██ ██ ██▀██   ▀▀▀▄▄▄ ██▀▀▀ ██▄██ ██▀██ ██    ██▀██ ██▄█▄ ███▄▄ ██▄██ ██ ██▄█▀ ║    ║ █    ██    █    ██▄▄   ██▀██ ██   ██      ██▄▄   ██▄▄  ██▄▄   ║\n";
        cout << "                  ║ █   ▀█▀    █     ▀█▀  ▀███▀ ██ ████▀   █████▀ ▀████ ██ ██ ▀███▀ ██▄▄▄ ██▀██ ██ ██ ▄▄██▀ ██ ██ ██ ██    ║    ║ █    ██    █    ██▄▄▄▄ ████▀ ██   ██      ██     ██▄▄▄ ██▄▄▄  ║\n";
        cout << "                  ║ ▀▀        ▀▀                                                                                           ║    ║ ▀▀        ▀▀                                                  ║\n";
        cout << "                  ╚════════════════════════════════════════════════════════════════════════════════════════════════════════╝    ╚═══════════════════════════════════════════════════════════════╝\n"; 
        }
        
        setColor(PINK);
        cout << "                                                                ╔═══════════════════════════════════════════════════════════════════════════════╗\n"; 
        cout << "                                                                ║ ▄▄        ▄▄                                                                  ║\n";
        cout << "                                                                ║ █  ██████  █    ██████ ▄▄ ▄▄ ▄▄ ▄▄▄▄▄▄   █████▄  ▄▄▄  ▄▄▄▄  ▄▄▄▄▄▄ ▄▄▄  ▄▄    ║\n";
        cout << "                                                                ║ █  ██▄▄    █    ██▄▄   ▀█▄█▀ ██   ██     ██▄▄█▀ ██▀██ ██▄█▄   ██  ██▀██ ██    ║\n";
        cout << "                                                                ║ █  ██▄▄▄▄  █    ██▄▄▄▄ ██ ██ ██   ██     ██     ▀███▀ ██ ██   ██  ██▀██ ██▄▄▄ ║\n";
        cout << "                                                                ║ ▀▀        ▀▀                                                                  ║\n";
        cout << "                                                                ╚═══════════════════════════════════════════════════════════════════════════════╝\n\n";
        setColor(WHITE);
        cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
        cout << "\n                                                                                              ┏━┓┏━┓┏━╸┏━┓┏━┓   ┏━╸╻ ╻┏━┓╻┏━╸┏━╸ ";
        cout << "\n                                                                                              ┣━┛┣┳┛┣╸ ┗━┓┗━┓   ┃  ┣━┫┃ ┃┃┃  ┣╸ ╹";
        cout << "\n                                                                                              ╹  ╹┗╸┗━╸┗━┛┗━┛   ┗━╸╹ ╹┗━┛╹┗━╸┗━╸╹";
        char c = getChar();
        char choice = tolower(c);

        if (choice == 'e') break;

        // --- DEPOSIT ---
        if (choice == 'd') {
            clearScreen();
            setColor(PEACH);
            cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            cout << "                                                                                       == ꜰɪɴᴀɴᴄᴇ ᴘᴏʀᴛᴀʟ: " << s.name << " (" << s.course << ") ==\n";
            cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n\n";
            setColor(WHITE);
            cout << "\n                                                                                            ᴇɴᴛᴇʀ ᴀᴍᴏᴜɴᴛ ᴛᴏ ᴅᴇᴘᴏꜱɪᴛ: ";
            string amountStr; getline(cin, amountStr);
            try {
                double amount = stod(amountStr);
                if (amount > 0) {
                    s.accountBalance += amount;
                    setColor(MINT); 
                    cout << "\n                                                                                        ꜱᴜᴄᴄᴇꜱꜱꜰᴜʟʟʏ ᴅᴇᴘᴏꜱɪᴛᴇᴅ ᴘʜᴘ " << amount << ".\n";
                    cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";
                } else { setColor(PEACH); 
                    cout << "\n                                                                                                    ɪɴᴠᴀʟɪᴅ ᴀᴍᴏᴜɴᴛ.\n"; 
                    cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";
                }
                    
                
            } catch (...) { setColor(PEACH); 
            cout << "\n                                                                                                      ɪɴᴠᴀʟɪᴅ ɪɴᴘᴜᴛ.\n"; 
            cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";
        }
            pauseScreen();
        }
        // --- PAY ---
        else if (choice == 'p') {
            clearScreen();
            if (outstanding <= 0) { setColor(MINT);
            setColor(PEACH);
            cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            cout << "                                                                                       == ꜰɪɴᴀɴᴄᴇ ᴘᴏʀᴛᴀʟ: " << s.name << " (" << s.course << ") ==\n";
            cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n\n"; 
            cout << "\n                                                                                                  ɴᴏ ʙᴀʟᴀɴᴄᴇ ᴅᴜᴇ.\n"; 
            cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";}
            else {
                setColor(PEACH);
                cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
                cout << "                                                                                       == ꜰɪɴᴀɴᴄᴇ ᴘᴏʀᴛᴀʟ: " << s.name << " (" << s.course << ") ==\n";
                cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n\n";
                setColor(WHITE);
                cout << "\n                                                                                           ᴘᴀʏ ᴀᴍᴏᴜɴᴛ (ᴍᴀx: " << s.accountBalance << "): ";
                string payStr; getline(cin, payStr);
                try {
                    double pay = stod(payStr);
                    if (pay <= 0) { setColor(PEACH); 
                    cout << "\n                                                                                                     ɪɴᴠᴀʟɪᴅ ᴀᴍᴏᴜɴᴛ.\n"; 
                    cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";}
                    else if (pay > s.accountBalance) { setColor(PEACH); 
                    cout << "\n                                                                                                    ɪɴꜱᴜꜰꜰɪᴄɪᴇɴᴛ ꜰᴜɴᴅꜱ.\n";
                    cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n"; 
                }
                    else {
                        if (pay > outstanding) pay = outstanding;
                        s.accountBalance -= pay;
                        s.tuitionPaid += pay;
                        setColor(MINT); 
                        cout << "\n                                                                                            ᴘᴀʏᴍᴇɴᴛ ꜱᴜᴄᴄᴇꜱꜱꜰᴜʟ: ᴘʜᴘ " << pay << "\n";
                        cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";
                    }
                } catch (...) { setColor(PEACH); 
                cout << "\n                                                                                                      ɪɴᴠᴀʟɪᴅ ɪɴᴘᴜᴛ.\n"; 
                cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";}
            }
            pauseScreen();
        }
        // --- APPLY SCHOLARSHIP ---
        else if (choice == 's') {
            clearScreen();
            setColor(PEACH);
            cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            cout << "                                                                                       == ꜰɪɴᴀɴᴄᴇ ᴘᴏʀᴛᴀʟ: " << s.name << " (" << s.course << ") ==\n";
            cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n\n";
            setColor(WHITE);
            if (s.scholarshipSemester == s.semester) { 
                setColor(PEACH); 
                cout << "\n                                                                                            ᴀʟʀᴇᴀᴅʏ ᴀᴄᴛɪᴠᴇ ꜰᴏʀ ᴛʜɪꜱ ꜱᴇᴍᴇꜱᴛᴇʀ.\n";
                cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n"; 
            }
            else {
                setColor(TEAL); 
                    cout << "\n                                                                        ᴀᴘᴘʟʏ ꜰᴏʀ ꜰʀᴇᴇ ʜɪɢʜᴇʀ ᴇᴅᴜᴄᴀᴛɪᴏɴ ꜰᴏʀ ꜱᴇᴍᴇꜱᴛᴇʀ " << s.semester << "? (ʏ/ɴ): ";
                string confirm; getline(cin, confirm);
                if (toLower(confirm) == "y") {
                    s.scholarshipSemester = s.semester;
                    setColor(MINT); 
                    cout << "\n                                                                                    ꜱᴄʜᴏʟᴀʀꜱʜɪᴘ ᴀᴘᴘʀᴏᴠᴇᴅ ꜰᴏʀ ꜱᴇᴍᴇꜱᴛᴇʀ: " << s.semester << "!\n";
                    cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n"; 
                }
            }
            pauseScreen();
        }
        // --- ADMIN: REVOKE SCHOLARSHIP ---
        else if (choice == 'v' && isAdmin) {
            clearScreen();
            setColor(PEACH);
            cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            cout << "                                                                                       == ꜰɪɴᴀɴᴄᴇ ᴘᴏʀᴛᴀʟ: " << s.name << " (" << s.course << ") ==\n";
            cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n\n";
            setColor(WHITE);
            if (s.scholarshipSemester == 0) {
                 setColor(PEACH); 
                 cout << "\n                                                                                           ɴᴏ ᴀᴄᴛɪᴠᴇ ꜱᴄʜᴏʟᴀʀꜱʜɪᴘ ᴛᴏ ʀᴇᴠᴏᴋᴇ.\n";
                 cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n"; 
            } else {
                 setColor(PEACH); 
                    cout << "\n                                                                                            ʀᴇᴠᴏᴋᴇ ꜱᴄʜᴏʟᴀʀꜱʜɪᴘ? (ʏ/ɴ): ";
                 string confirm; getline(cin, confirm);
                 if (toLower(confirm) == "y") {
                     s.scholarshipSemester = 0;
                     setColor(MINT); 
                     cout << "\n                                                                                                  ꜱᴄʜᴏʟᴀʀꜱʜɪᴘ ʀᴇᴠᴏᴋᴇᴅ.\n";
                     cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";
                 }
            }
            pauseScreen();
        }
        // --- ADMIN: EDIT TUITION ---
        else if (choice == 't' && isAdmin) {
            clearScreen();
            setColor(PEACH);
            cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            cout << "                                                                                       == ꜰɪɴᴀɴᴄᴇ ᴘᴏʀᴛᴀʟ: " << s.name << " (" << s.course << ") ==\n";
            cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n\n";
            setColor(WHITE);
            setColor(TEAL);
            cout << "\n                                                                                   ᴇɴᴛᴇʀ ɴᴇᴡ ᴛᴏᴛᴀʟ ᴛᴜɪᴛɪᴏɴ (ᴇɴᴛᴇʀ c ᴛᴏ ʀᴇꜱᴇᴛ ᴛᴏ ᴅᴇꜰᴀᴜʟᴛ): ";
            string tStr; getline(cin, tStr);
            try {
                double newTuition = stod(tStr);
                // Ensure the student struct has 'tuitionOverride' variable!
                s.tuitionOverride = newTuition;
                
                setColor(MINT); 
                if (newTuition == c) cout << "                                                                                       ᴛᴜɪᴛɪᴏɴ ʀᴇꜱᴇᴛ ᴛᴏ ᴀᴜᴛᴏ-ᴄᴀʟᴄᴜʟᴀᴛɪᴏɴ.\n";
                else cout << "\n                                                                                        ᴛᴜɪᴛɪᴏɴ ᴜᴘᴅᴀᴛᴇᴅ ᴛᴏ ᴘʜᴘ " << newTuition << ".\n";
                cout << "\n                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n"; 
                
            } catch(...) {
                setColor(PEACH); 
                cout << "\n                                                                                                      ɪɴᴠᴀʟɪᴅ ɪɴᴘᴜᴛ.\n";
                cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";
            }
            pauseScreen();
        }
    }
    saveStudentsToFile();
}
 
//-------------------------
// Student Record Menu/Student Profile (Admin View)
//-------------------------
void viewAllStudents() {
clearScreen();
setColor(VIOLET);
vector<string> ascii_lines = {
"\n",
"                                                 ███████╗████████╗██╗   ██╗██████╗ ███████╗███╗   ██╗████████╗    ██████╗ ███████╗ ██████╗ ██████╗ ██████╗ ██████╗",
"                                                 ██╔════╝╚══██╔══╝██║   ██║██╔══██╗██╔════╝████╗  ██║╚══██╔══╝    ██╔══██╗██╔════╝██╔════╝██╔═══██╗██╔══██╗██╔══██╗",
"                                                 ███████╗   ██║   ██║   ██║██║  ██║█████╗  ██╔██╗ ██║   ██║       ██████╔╝█████╗  ██║     ██║   ██║██████╔╝██║  ██║",
"                                                 ╚════██║   ██║   ██║   ██║██║  ██║██╔══╝  ██║╚██╗██║   ██║       ██╔══██╗██╔══╝  ██║     ██║   ██║██╔══██╗██║  ██║",
"                                                 ███████║   ██║   ╚██████╔╝██████╔╝███████╗██║ ╚████║   ██║       ██║  ██║███████╗╚██████╗╚██████╔╝██║  ██║██████╔╝",
"                                                 ╚══════╝   ╚═╝    ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝   ╚═╝       ╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚═════╝",
"\n"
};

for (const string& line : ascii_lines) {
        cout << line << endl;
        this_thread::sleep_for(chrono::milliseconds(20));
    }

    // --- GRID SETTINGS ---
    const int COLS = 3;             // Number of columns
    const int CARD_WIDTH = 48;      // Reduced width to fit 3 cards on screen
    const string GAP = "   ";       // Space between cards

    // Loop through students in batches of 3
    for (size_t i = 0; i < students.size(); i += COLS) {
        
        // ---------------- LINE 1: TOP BORDER ----------------
        for (int j = 0; j < COLS; j++) {
            if (i + j < students.size()) {
                setColor(VIOLET); cout << "               ╔"; printHLine(CARD_WIDTH - 2); cout << "╗" << GAP;
            }
        }
        cout << endl;

        // ---------------- LINE 2: ID & NAME ----------------
        for (int j = 0; j < COLS; j++) {
            if (i + j < students.size()) {
                const auto& s = students[i + j];
                setColor(VIOLET); cout << "               ║ ";
                setColor(TEAL); cout << "[" << s.id << "] ";
                
                // Calculate remaining space for name
                int usedSpace = 2 + (int)s.id.length() + 3 + 2; // Border+Space + ID + Space + Border
                int nameSpace = CARD_WIDTH - usedSpace;
                
                string dName = s.name;
                if ((int)dName.length() > nameSpace) dName = dName.substr(0, nameSpace - 3) + "...";

                setColor(WHITE); cout << left << setw(nameSpace + 1) << dName; 
                setColor(VIOLET); cout << "║" << GAP;
            }
        }
        cout << endl;

        // ---------------- LINE 3: COURSE & SEMESTER ----------------
        for (int j = 0; j < COLS; j++) {
            if (i + j < students.size()) {
                const auto& s = students[i + j];
                setColor(VIOLET); cout << "               ║ ";
                setColor(PEACH); cout << "Cr: ";
                setColor(WHITE); cout << left << setw(8) << s.course; // Shortened label to fit
                setColor(PEACH); cout << "Sem: ";
                setColor(WHITE); cout << left << setw(CARD_WIDTH - 21) << s.semester;
                setColor(VIOLET); cout << " ║" << GAP;
            }
        }
        cout << endl;

        // ---------------- LINE 4: AVERAGE & GRADE ----------------
        for (int j = 0; j < COLS; j++) {
            if (i + j < students.size()) {
                const auto& s = students[i + j];
                
                // Determine Color
                int grade_color;
                string status;
                if (s.letter == 'A' || s.letter == 'B') { grade_color = MINT; status = "EXCELLENT"; }
                else if (s.letter == 'C' || s.letter == 'D') { grade_color = PEACH; status = "PASSING"; }
                else { grade_color = PINK; status = "FAILING"; }

                setColor(VIOLET); cout << "               ║ ";
                setColor(PEACH); cout << "Avg: ";
                setColor(grade_color); cout << left << setw(7) << fixed << setprecision(2) << s.average;
                
                setColor(PEACH); cout << "Grd: ";
                setColor(grade_color); cout << s.letter << " (" << status << ")";

                // Calculate padding to close the border correctly
                int contentLen = 5 + 7 + 5 + 1 + 2 + (int)status.length(); // Length of text printed above
                int remaining = CARD_WIDTH - 4 - contentLen;
                
                setColor(WHITE); cout << setw(remaining) << "";
                setColor(VIOLET); cout << "║" << GAP;
            }
        }
        cout << endl;

        // ---------------- LINE 5: BOTTOM BORDER ----------------
        for (int j = 0; j < COLS; j++) {
            if (i + j < students.size()) {
                setColor(VIOLET); cout << "               ╚"; printHLine(CARD_WIDTH - 2); cout << "╝" << GAP;
            }
        }
        cout << endl << endl; // Extra line between rows
    }

    setColor(DEFAULT); 
    pauseScreen();
}

//-------------------------
//UPDATE PERSONAL INFO MENU (For Admin Profile)
//-------------------------
void updateStudentDetails() { 
    clearScreen();
    setColor(VIOLET);
    // Keep the Admin Header
    vector<string> ascii_lines = {
        "\n",
        "                             ██╗   ██╗██████╗ ██████╗  █████╗ ████████╗███████╗    ███████╗████████╗██╗   ██╗██████╗ ███████╗███╗   ██╗████████╗    ██████╗ ███████╗████████╗ █████╗ ██╗██╗     ███████╗",
        "                             ██║   ██║██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝██╔════╝    ██╔════╝╚══██╔══╝██║   ██║██╔══██╗██╔════╝████╗  ██║╚══██╔══╝    ██╔══██╗██╔════╝╚══██╔══╝██╔══██╗██║██║     ██╔════╝",
        "                             ██║   ██║██████╔╝██║  ██║███████║   ██║   █████╗      ███████╗   ██║   ██║   ██║██║  ██║█████╗  ██╔██╗ ██║   ██║       ██║  ██║█████╗     ██║   ███████║██║██║     ███████╗",
        "                             ██║   ██║██╔═══╝ ██║  ██║██╔══██║   ██║   ██╔══╝      ╚════██║   ██║   ██║   ██║██║  ██║██╔══╝  ██║╚██╗██║   ██║       ██║  ██║██╔══╝     ██║   ██╔══██║██║██║     ╚════██║",
        "                             ╚██████╔╝██║     ██████╔╝██║  ██║   ██║   ███████╗    ███████║   ██║   ╚██████╔╝██████╔╝███████╗██║ ╚████║   ██║       ██████╔╝███████╗   ██║   ██║  ██║██║███████╗███████║",
        "                              ╚═════╝ ╚═╝     ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚══════╝    ╚══════╝   ╚═╝    ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝   ╚═╝       ╚═════╝ ╚══════╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚══════╝╚══════╝",
        "\n"
    };

    for (const string& line : ascii_lines) { 
        cout << line << endl; 
        this_thread::sleep_for(chrono::milliseconds(20)); 
    }
    setColor(WHITE);

    cin.clear();
    // 1. Ask for ID first (Admin functionality)
    cout << "\n                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
    cout << "\n                                                                                   ᴇɴᴛᴇʀ ꜱᴛᴜᴅᴇɴᴛ ɪᴅ ᴛᴏ ᴜᴘᴅᴀᴛᴇ (ᴏʀ 'ᴇxɪᴛ'): ";
    string id; getline(cin, id);
    cout << "\n                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n\n";
    
    if (checkExit(id)) return;

    int index = findStudentIndexByID(id);
    if (index == -1) { 
        setColor(PEACH); 
    cout << "                                                                                                ꜱᴛᴜᴅᴇɴᴛ ɴᴏᴛ ꜰᴏᴜɴᴅ.\n"; 
        pauseScreen(); return;
    }

    Student &s = students[index];

    // 3. UI and Logic
    setColor(VIOLET);
    cout << "                                                                                           ʟᴇᴀᴠᴇ ʙʟᴀɴᴋ ᴛᴏ ᴋᴇᴇᴘ ᴄᴜʀʀᴇɴᴛ ᴠᴀʟᴜᴇ.\n";
    setColor(WHITE);
    cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";
    
    string temp;
    
    // Update Name
    cout << "                                                                                 Full Name (" << s.name << "): "; getline(cin, temp);
    if (!temp.empty() && !checkExit(temp)) s.name = temp;

    // Update Course
    cout << "                                                                                 Course (" << s.course << "): "; getline(cin, temp);
    if (!temp.empty() && !checkExit(temp)) s.course = temp;

    // Update Section
    cout << "                                                                                 Section (" << s.section << "): "; getline(cin, temp);
    if (!temp.empty() && !checkExit(temp)) s.section = temp;

    // Update Contact
    cout << "                                                                                 Contact # (" << s.contactNumber << "): "; getline(cin, temp);
    if (!temp.empty() && !checkExit(temp)) s.contactNumber = temp;

    // Update Email
    cout << "                                                                                 Email (" << s.email << "): "; getline(cin, temp);
    if (!temp.empty() && !checkExit(temp)) s.email = temp;

    // Update Birthday
    cout << "                                                                                 Birthday (" << s.birthday << "): "; getline(cin, temp);
    if (!temp.empty() && !checkExit(temp)) s.birthday = temp;
    
    // Update Blood Type
    cout << "                                                                                 Blood Type (" << s.bloodType << "): "; getline(cin, temp);
    if (!temp.empty() && !checkExit(temp)) s.bloodType = temp;

    setColor(MINT); 
    slowPrint("\n                                                                                           Information updated successfully!", 10);
    setColor(WHITE); 
    slowPrint("\n                                                                   ╚═══════════════════════════════════════════════════════════════════════════════╝\n", 2);
    
    saveStudentsToFile();
    pauseScreen();
}

//-------------------------
//UPDATE PERSONAL INFO MENU (For Student Profile)
//-------------------------

void updateOwnDetails(Student &s) {
    clearScreen();
    setColor(VIOLET);
    vector<string> ascii_lines = {
        "\n",
            "                             ██╗   ██╗██████╗ ██████╗  █████╗ ████████╗███████╗    ██████╗ ███████╗██████╗ ███████╗ ██████╗ ███╗   ██╗ █████╗ ██╗         ██╗███╗   ██╗███████╗ ██████╗ ",
            "                             ██║   ██║██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝██╔════╝    ██╔══██╗██╔════╝██╔══██╗██╔════╝██╔═══██╗████╗  ██║██╔══██╗██║         ██║████╗  ██║██╔════╝██╔═══██╗",
            "                             ██║   ██║██████╔╝██║  ██║███████║   ██║   █████╗      ██████╔╝█████╗  ██████╔╝███████╗██║   ██║██╔██╗ ██║███████║██║         ██║██╔██╗ ██║█████╗  ██║   ██║",
            "                             ██║   ██║██╔═══╝ ██║  ██║██╔══██║   ██║   ██╔══╝      ██╔═══╝ ██╔══╝  ██╔══██╗╚════██║██║   ██║██║╚██╗██║██╔══██║██║         ██║██║╚██╗██║██╔══╝  ██║   ██║",
            "                             ╚██████╔╝██║     ██████╔╝██║  ██║   ██║   ███████╗    ██║     ███████╗██║  ██║███████║╚██████╔╝██║ ╚████║██║  ██║███████╗    ██║██║ ╚████║██║     ╚██████╔╝",
            "                              ╚═════╝ ╚═╝     ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚══════╝    ╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝    ╚═╝╚═╝  ╚═══╝╚═╝      ╚═════╝ ",
        "\n"
    };

    for (const string& line : ascii_lines) { 
        cout << line << endl; 
        this_thread::sleep_for(chrono::milliseconds(20)); 
    }

    setColor(WHITE); cout << "                                                                                           ʟᴇᴀᴠᴇ ʙʟᴀɴᴋ ᴛᴏ ᴋᴇᴇᴘ ᴄᴜʀʀᴇɴᴛ ᴠᴀʟᴜᴇ.\n";
    setColor(WHITE);
    cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n\n";
    string temp;
    cout << "                                                                                 Full Name (" << s.name << "): "; getline(cin, temp);
    if (!temp.empty()) s.name = temp;

    cout << "                                                                                 Course (" << s.course << "): "; getline(cin, temp);
    if (!temp.empty()) s.course = temp;

    cout << "                                                                                 Section (" << s.section << "): "; getline(cin, temp);
    if (!temp.empty()) s.section = temp;

    cout << "                                                                                 Contact # (" << s.contactNumber << "): "; getline(cin, temp);
    if (!temp.empty()) s.contactNumber = temp;

    cout << "                                                                                 Email (" << s.email << "): "; getline(cin, temp);
    if (!temp.empty()) s.email = temp;

    cout << "                                                                                 Birthday (" << s.birthday << "): "; getline(cin, temp);
    if (!temp.empty()) s.birthday = temp;
    
    cout << "                                                                                 Blood Type (" << s.bloodType << "): "; getline(cin, temp);
    if (!temp.empty()) s.bloodType = temp;

    setColor(MINT); slowPrint("\n                                                                                           Information updated successfully!", 10);
    setColor(WHITE); slowPrint("\n                                                                   ╚═══════════════════════════════════════════════════════════════════════════════╝\n", 2); pauseScreen();
    saveStudentsToFile();
}

//-------------------------
//DELETE STUDENT MENU 
//-------------------------

void deleteStudent() {
    clearScreen();
    setColor(VIOLET);
    vector<string> ascii_lines = {
        "\n",
            "                                                      ██████╗ ███████╗██╗     ███████╗████████╗███████╗    ███████╗████████╗██╗   ██╗██████╗ ███████╗███╗   ██╗████████╗",
            "                                                      ██╔══██╗██╔════╝██║     ██╔════╝╚══██╔══╝██╔════╝    ██╔════╝╚══██╔══╝██║   ██║██╔══██╗██╔════╝████╗  ██║╚══██╔══╝",
            "                                                      ██║  ██║█████╗  ██║     █████╗     ██║   █████╗      ███████╗   ██║   ██║   ██║██║  ██║█████╗  ██╔██╗ ██║   ██║   ",
            "                                                      ██║  ██║██╔══╝  ██║     ██╔══╝     ██║   ██╔══╝      ╚════██║   ██║   ██║   ██║██║  ██║██╔══╝  ██║╚██╗██║   ██║   ",
            "                                                      ██████╔╝███████╗███████╗███████╗   ██║   ███████╗    ███████║   ██║   ╚██████╔╝██████╔╝███████╗██║ ╚████║   ██║   ",
            "                                                      ╚═════╝ ╚══════╝╚══════╝╚══════╝   ╚═╝   ╚══════╝    ╚══════╝   ╚═╝    ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝   ╚═╝   ",
        "\n" 
    };

    for (const string& line : ascii_lines) { 
        cout << line << endl; 
        this_thread::sleep_for(chrono::milliseconds(20)); // Small delay for animation
    } 
    setColor(WHITE);
    
    cin.clear(); 
    // Removed blocking peek
    cout << "                                                                   ╔═════════════════════════════════════════════════════════════════════════════════╗\n";
    cout << "                                                                                                 ᴇɴᴛᴇʀ ꜱᴛᴜᴅᴇɴᴛ ɪᴅ ᴛᴏ ᴅᴇʟᴇᴛᴇ: ";
    string id; getline(cin, id);
    if (checkExit(id)) return;

    int index = findStudentIndexByID(id);
    if (index == -1) { setColor(PEACH); 
        cout << "                                                                                                 ꜱᴛᴜᴅᴇɴᴛ ɴᴏᴛ ꜰᴏᴜɴᴅ.\n"; 
        setColor(WHITE);
        slowPrint("\n                                                                   ╚═══════════════════════════════════════════════════════════════════════════════╝\n", 2);
        pauseScreen(); 
        return; }
        

    setColor(PINK);
    cout << "                                                                                                 ᴅᴇʟᴇᴛᴇ " << students[index].name << "? (ʏ/ɴ): ";
    string confirmation; getline(cin, confirmation);
    setColor(WHITE);
    slowPrint("\n                                                                   ╚═══════════════════════════════════════════════════════════════════════════════╝\n", 2);

    if (toLower(confirmation) == "y") {
        students.erase(students.begin() + index);
        saveStudentsToFile();
        setColor(MINT); cout << "                                                                                                 ʀᴇᴄᴏʀᴅ ᴅᴇʟᴇᴛᴇᴅ.\n";
        setColor(WHITE);
        slowPrint("\n                                                                   ╚═══════════════════════════════════════════════════════════════════════════════╝\n", 2);
    }
    pauseScreen();
}
//-------------------------
//UPDATE STUDENT GRADE MENU
//-------------------------
void updateGradesInteractive() {
    clearScreen();
    // 1. We ask for ID first so we know who we are dealing with
    // You can keep the header here for the initial entry if you like, 
    // but the main loop below will handle the repetitive UI.
    
    // Setup ASCII art vector once
    vector<string> ascii_lines = {
        "\n",
        "                            ██╗   ██╗██████╗ ██████╗  █████╗ ████████╗███████╗    ███████╗████████╗██╗   ██╗██████╗ ███████╗███╗   ██╗████████╗     ██████╗ ██████╗  █████╗ ██████╗ ███████╗███████╗",
        "                            ██║   ██║██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝██╔════╝    ██╔════╝╚══██╔══╝██║   ██║██╔══██╗██╔════╝████╗  ██║╚══██╔══╝    ██╔════╝ ██╔══██╗██╔══██╗██╔══██╗██╔════╝██╔════╝",
        "                            ██║   ██║██████╔╝██║  ██║███████║   ██║   █████╗      ███████╗   ██║   ██║   ██║██║  ██║█████╗  ██╔██╗ ██║   ██║       ██║  ███╗██████╔╝███████║██║  ██║█████╗  ███████╗",
        "                            ██║   ██║██╔═══╝ ██║  ██║██╔══██║   ██║   ██╔══╝      ╚════██║   ██║   ██║   ██║██║  ██║██╔══╝  ██║╚██╗██║   ██║       ██║   ██║██╔══██╗██╔══██║██║  ██║██╔══╝  ╚════██║",
        "                            ╚██████╔╝██║     ██████╔╝██║  ██║   ██║   ███████╗    ███████║   ██║   ╚██████╔╝██████╔╝███████╗██║ ╚████║   ██║       ╚██████╔╝██║  ██║██║  ██║██████╔╝███████╗███████║",
        "                             ╚═════╝ ╚═╝     ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚══════╝    ╚══════╝   ╚═╝    ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝   ╚═╝        ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝╚══════╝",
        "\n"
    };

    // Print header once for the ID entry screen
    setColor(VIOLET);
    for (const string& line : ascii_lines) { cout << line << endl; }
    setColor(WHITE);

    cin.clear();
    cout << "\n                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n\n"; 
    slowPrint("                                                                                       ᴇɴᴛᴇʀ ꜱᴛᴜᴅᴇɴᴛ ɪᴅ: ", 3);
    string id; getline(cin, id);
    if (checkExit(id)) return;

    int index = findStudentIndexByID(id);
    if (index == -1) { setColor(PEACH); slowPrint("                                                                                       ꜱᴛᴜᴅᴇɴᴛ ɴᴏᴛ ꜰᴏᴜɴᴅ.\n", 3); pauseScreen(); return; }

    Student &s = students[index];

    // --- MAIN INTERACTIVE LOOP STARTS HERE ---
    while (true) {
        clearScreen(); // Clear screen at the start of every loop iteration

        // Print the Header again so it looks like the "Main Screen"
        setColor(VIOLET);
        for (const string& line : ascii_lines) { 
            cout << line << endl; 
            // Removed sleep here to make the menu feel snappier when looping back, 
            // but you can add it back if you prefer animation every time.
        } 
        setColor(WHITE);

        // Print the Student Details (Main Display)
        setColor(PEACH);
        cout << "\n\n";
            cout << "                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n"; 
            cout << "                                                                                             ꜱᴜʙᴊᴇᴄᴛꜱ ꜰᴏʀ: " << s.name << "\n";
            cout << "                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            cout << "\n"; 
            for (size_t i = 0; i < s.subjects.size(); ++i) {
                setColor(WHITE);
                cout << "                                                                                    " << i + 1 << ". " << left << setw(30) << s.subjects[i].name << ": " << fixed << setprecision(2) << s.subjects[i].grade;
                cout << "\n";
            }
            cout << "\n                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
        
        // Print the Menu Options
        setColor(WHITE);
        cout << "\n\n";
        cout << "                      ╔═══════════════════════════════════════════════════════════════════════╗            ╔═════════════════════════════════════════════════════════════════════════════╗\n";
        cout << "                      ║ ▄▄        ▄▄                                                          ║            ║ ▄▄        ▄▄                                                                ║\n";
        cout << "                      ║ █  ▄████▄  █  ▄▄▄▄  ▄▄▄▄     ▄▄▄▄ ▄▄ ▄▄ ▄▄▄▄    ▄▄ ▄▄▄▄▄  ▄▄▄▄ ▄▄▄▄▄▄ ║            ║ █  ██  ██  █  ▄▄▄▄  ▄▄▄▄   ▄▄▄ ▄▄▄▄▄▄ ▄▄▄▄▄    ▄▄▄▄ ▄▄▄▄   ▄▄▄  ▄▄▄▄  ▄▄▄▄▄ ║\n";
        cout << "                      ║ █  ██▄▄██  █  ██▀██ ██▀██   ███▄▄ ██ ██ ██▄██   ██ ██▄▄  ██▀▀▀   ██   ║            ║ █  ██  ██  █  ██▄█▀ ██▀██ ██▀██  ██   ██▄▄    ██ ▄▄ ██▄█▄ ██▀██ ██▀██ ██▄▄  ║\n";
        cout << "                      ║ █  ██  ██  █  ████▀ ████▀   ▄▄██▀ ▀███▀ ██▄█▀ ▄▄█▀ ██▄▄▄ ▀████   ██   ║            ║ █  ▀████▀  █  ██    ████▀ ██▀██  ██   ██▄▄▄   ▀███▀ ██ ██ ██▀██ ████▀ ██▄▄▄ ║\n";
        cout << "                      ║ ▀▀        ▀▀                                                          ║            ║ ▀▀        ▀▀                                                                ║\n";
        cout << "                      ╚═══════════════════════════════════════════════════════════════════════╝            ╚═════════════════════════════════════════════════════════════════════════════╝\n";
        setColor(PINK);
        cout << "\n";
        cout << "                                                                                        ╔══════════════════════════════════╗\n"; 
        cout << "                                                                                        ║ ▄▄        ▄▄                     ║\n";
        cout << "                                                                                        ║ █  ████▄   █   ▄▄▄  ▄▄  ▄▄ ▄▄▄▄▄ ║\n";
        cout << "                                                                                        ║ █  ██  ██  █  ██▀██ ███▄██ ██▄▄  ║\n";
        cout << "                                                                                        ║ █  ████▀   █  ▀███▀ ██ ▀██ ██▄▄▄ ║\n";
        cout << "                                                                                        ║ ▀▀        ▀▀                     ║\n";
        cout << "                                                                                        ╚══════════════════════════════════╝\n\n";
        setColor(WHITE);
        cout << "\n                  ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
        cout << "\n                                                                                              ┏━┓┏━┓┏━╸┏━┓┏━┓   ┏━╸╻ ╻┏━┓╻┏━╸┏━╸ ";
        cout << "\n                                                                                              ┣━┛┣┳┛┣╸ ┗━┓┗━┓   ┃  ┣━┫┃ ┃┃┃  ┣╸ ╹";
        cout << "\n                                                                                              ╹  ╹┗╸┗━╸┗━┛┗━┛   ┗━╸╹ ╹┗━┛╹┗━╸┗━╸╹"; 
        
        char c = getChar();
        char choice = tolower(c);

        if (choice == 'd') break;

        if (choice == 'a') {
            clearScreen();
            // We kept the table printing here so the user sees what they are adding to,
            // but simplified logic since the Main Loop handles the full refresh after we are done.
            setColor(PEACH);
            cout << "\n\n";
            cout << "                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n"; 
            cout << "                                                                                             ꜱᴜʙᴊᴇᴄᴛꜱ ꜰᴏʀ: " << s.name << "\n";
            cout << "                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            cout << "\n"; 
            for (size_t i = 0; i < s.subjects.size(); ++i) {
                setColor(WHITE);
                cout << "                                                                                    " << i + 1 << ". " << left << setw(30) << s.subjects[i].name << ": " << fixed << setprecision(2) << s.subjects[i].grade;
                cout << "\n";
            }
            cout << "\n                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            
            cout << "\n                                                                                                 ꜱᴜʙᴊᴇᴄᴛ ɴᴀᴍᴇ: "; 
            Subject sub;
            getline(cin, sub.name);
            if (!sub.name.empty()) {
                sub.grade = 0; // default
                cout << "                                                                                                ɢʀᴀᴅᴇ: "; 
                string g; getline(cin, g);
                try { sub.grade = stod(g); } catch(...) {}
                s.subjects.push_back(sub);
            }
            // No need to print menu here. Loop ends, goes to top, clears screen, reprints Main Menu.
        } 
        else if (choice == 'u') {
            clearScreen();
            setColor(PEACH);
            cout << "\n\n";
            cout << "                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n"; 
            cout << "                                                                                             ꜱᴜʙᴊᴇᴄᴛꜱ ꜰᴏʀ: " << s.name << "\n";
            cout << "                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            cout << "\n"; 
            for (size_t i = 0; i < s.subjects.size(); ++i) {
                setColor(WHITE);
                cout << "                                                                                    " << i + 1 << ". " << left << setw(30) << s.subjects[i].name << ": " << fixed << setprecision(2) << s.subjects[i].grade;
                cout << "\n";
            }
            cout << "\n                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            
            cout << "\n                                                                                                ꜱᴜʙᴊᴇᴄᴛ #: "; string n; getline(cin, n);
            try {
                int i = stoi(n) - 1;
                if (i >= 0 && i < s.subjects.size()) {
                    cout << "                                                                                                ɴᴇᴡ ɢʀᴀᴅᴇ: "; string g; getline(cin, g);
                    s.subjects[i].grade = stod(g);
                }
            } catch (...) {}
            // No need to print menu here. Loop ends, goes to top, clears screen, reprints Main Menu.
        }
        computeAverageAndLetter(s);
        saveStudentsToFile(); // Auto-save after every change
    }
}

void displayTitleScreen() {
    clearScreen();
    setColor(VIOLET);
    vector<string> ascii_lines = {
        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",
        "",
        "                                             █████████           █████               ████                      █████████            █████                                  ",
        "                                            ███░░░░░███         ░░███               ░░███                     ███░░░░░███          ░░███                                   ",
        "                                           ░███    ░░░   ██████  ░███████    ██████  ░███   ██████   ████████ ░███    ░░░  ████████  ░███████    ██████  ████████   ██████ ",
        "                                           ░░█████████  ███░░███ ░███░░███  ███░░███ ░███  ░░░░░███ ░░███░░███░░█████████ ░░███░░███ ░███░░███  ███░░███░░███░░███ ███░░███",
        "                                            ░░░░░░░░███░███ ░░░  ░███ ░███ ░███ ░███ ░███   ███████  ░███ ░░░  ░░░░░░░░███ ░███ ░███ ░███ ░███ ░███████  ░███ ░░░ ░███████ ",
        "                                            ███    ░███░███  ███ ░███ ░███ ░███ ░███ ░███  ███░░███  ░███      ███    ░███ ░███ ░███ ░███ ░███ ░███░░░   ░███     ░███░░░  ",
        "                                           ░░█████████ ░░██████  ████ █████░░██████  █████░░████████ █████    ░░█████████  ░███████  ████ █████░░██████  █████    ░░██████ ",
        "                                            ░░░░░░░░░   ░░░░░░  ░░░░ ░░░░░  ░░░░░░  ░░░░░  ░░░░░░░░ ░░░░░      ░░░░░░░░░   ░███░░░  ░░░░ ░░░░░  ░░░░░░  ░░░░░      ░░░░░░  ",
        "                                                                                                                           █████                                           ",
        "                                                                                                                          ░░░░░                                            ",
        "\n",
    };

    for (const string& line : ascii_lines) { 
        cout << line << endl; 
        this_thread::sleep_for(chrono::milliseconds(30)); // Small delay for animation
    }
    setColor(WHITE);
    cout<< "\n";
       slowPrint( "\n                                                          ┏━┓╺┳╸╻ ╻╺┳┓┏━╸┏┓╻╺┳╸   ┏━┓┏━╸┏━╸┏━┓┏━┓╺┳┓   ┏┳┓┏━┓┏┓╻┏━┓┏━╸┏━╸┏┳┓┏━╸┏┓╻╺┳╸   ┏━┓╻ ╻┏━┓╺┳╸┏━╸┏┳┓  ", 2.5);
       slowPrint( "\n                                                  ╺━╸╺━╸  ┗━┓ ┃ ┃ ┃ ┃┃┣╸ ┃┗┫ ┃    ┣┳┛┣╸ ┃  ┃ ┃┣┳┛ ┃┃   ┃┃┃┣━┫┃┗┫┣━┫┃╺┓┣╸ ┃┃┃┣╸ ┃┗┫ ┃    ┗━┓┗┳┛┗━┓ ┃ ┣╸ ┃┃┃ ╺━╸╺━╸ ", 2.5);
       slowPrint( "\n                                                          ┗━┛ ╹ ┗━┛╺┻┛┗━╸╹ ╹ ╹    ╹┗╸┗━╸┗━╸┗━┛╹┗╸╺┻┛   ╹ ╹╹ ╹╹ ╹╹ ╹┗━┛┗━╸╹ ╹┗━╸╹ ╹ ╹    ┗━┛ ╹ ┗━┛ ╹ ┗━╸╹ ╹  ", 2.5);
    this_thread::sleep_for(chrono::seconds(3)); // Auto-advance after 2 seconds
}

// --- MENUS ---

void displayAdminMenu() {
    clearScreen();
    setColor(VIOLET);
    vector<string> ascii_lines = {
        "\n\n\n\n\n\n\n\n\n",
            "                                                                  █████╗ ██████╗ ███╗   ███╗██╗███╗   ██╗    ███╗   ███╗███████╗███╗   ██╗██╗   ██╗",
            "                                                                 ██╔══██╗██╔══██╗████╗ ████║██║████╗  ██║    ████╗ ████║██╔════╝████╗  ██║██║   ██║",
            "                                                                 ███████║██║  ██║██╔████╔██║██║██╔██╗ ██║    ██╔████╔██║█████╗  ██╔██╗ ██║██║   ██║",
            "                                                                 ██╔══██║██║  ██║██║╚██╔╝██║██║██║╚██╗██║    ██║╚██╔╝██║██╔══╝  ██║╚██╗██║██║   ██║",
            "                                                                 ██║  ██║██████╔╝██║ ╚═╝ ██║██║██║ ╚████║    ██║ ╚═╝ ██║███████╗██║ ╚████║╚██████╔╝",
            "                                                                 ╚═╝  ╚═╝╚═════╝ ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝    ╚═╝     ╚═╝╚══════╝╚═╝  ╚═══╝ ╚═════╝ ",
        "\n"
    };

    for (const string& line : ascii_lines) { 
        cout << line << endl; 
        this_thread::sleep_for(chrono::milliseconds(20)); // Small delay for animation
    } 
    setColor(VIOLET);
    setColor(TEAL);
    cout << "               ╔════════════════════════════════════════════════════════════════════════════════════════════╗     ╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    cout << "               ║▄██     ▄▄▄▄▄ ▄▄  ▄▄ ▄▄▄▄   ▄▄▄  ▄▄    ▄▄       ▄▄▄▄ ▄▄▄▄▄▄ ▄▄ ▄▄ ▄▄▄▄  ▄▄▄▄▄ ▄▄  ▄▄ ▄▄▄▄▄▄ ║     ║ ████▄     ██  ██ ▄▄ ▄▄▄▄▄ ▄▄   ▄▄   ▄▄▄▄  ▄▄▄▄▄  ▄▄▄▄  ▄▄▄  ▄▄▄▄  ▄▄▄▄   ▄▄▄▄ ║\n";
    cout << "               ║ ██     ██▄▄  ███▄██ ██▄█▄ ██▀██ ██    ██      ███▄▄   ██   ██ ██ ██▀██ ██▄▄  ███▄██   ██   ║     ║  ▄██▀     ██▄▄██ ██ ██▄▄  ██ ▄ ██   ██▄█▄ ██▄▄  ██▀▀▀ ██▀██ ██▄█▄ ██▀██ ███▄▄ ║\n";
    cout << "               ║ ██ ▄   ██▄▄▄ ██ ▀██ ██ ██ ▀███▀ ██▄▄▄ ██▄▄▄   ▄▄██▀   ██   ▀███▀ ████▀ ██▄▄▄ ██ ▀██   ██   ║     ║ ███▄▄ ▄    ▀██▀  ██ ██▄▄▄  ▀█▀█▀    ██ ██ ██▄▄▄ ▀████ ▀███▀ ██ ██ ████▀ ▄▄██▀ ║\n";
    cout << "               ╚════════════════════════════════════════════════════════════════════════════════════════════╝     ╚═══════════════════════════════════════════════════════════════════════════════╝\n";
    cout << "               ╔═══════════════════════════════════════════════════════════════════════════════════════════╗           ╔══════════════════════════════════════════════════════════════════════════╗\n";
    cout << "               ║████▄    ▄▄▄▄ ▄▄▄▄▄  ▄▄▄  ▄▄▄▄   ▄▄▄▄ ▄▄ ▄▄    ▄▄▄▄ ▄▄▄▄▄▄ ▄▄ ▄▄ ▄▄▄▄  ▄▄▄▄▄ ▄▄  ▄▄ ▄▄▄▄▄▄╸║           ║██  ██     ▄▄▄▄▄ ▄▄▄▄  ▄▄ ▄▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄ ▄▄▄▄▄▄ ▄▄▄  ▄▄ ▄▄     ▄▄▄▄ ║\n";
    cout << "               ║ ▄▄██   ███▄▄ ██▄▄  ██▀██ ██▄█▄ ██▀▀▀ ██▄██   ███▄▄   ██   ██ ██ ██▀██ ██▄▄  ███▄██   ██   ║           ║▀█████     ██▄▄  ██▀██ ██   ██     ██▀██ ██▄▄    ██  ██▀██ ██ ██    ███▄▄ ║\n";
    cout << "               ║▄▄▄█▀ ▄ ▄▄██▀ ██▄▄▄ ██▀██ ██ ██ ▀████ ██ ██   ▄▄██▀   ██   ▀███▀ ████▀ ██▄▄▄ ██ ▀██   ██   ║           ║    ██ ▄   ██▄▄▄ ████▀ ██   ██     ████▀ ██▄▄▄   ██  ██▀██ ██ ██▄▄▄ ▄▄██▀ ║\n";
    cout << "               ╚═══════════════════════════════════════════════════════════════════════════════════════════╝           ╚══════════════════════════════════════════════════════════════════════════╝\n";
    cout << "               ╔═══════════════════════════════════════════════════════════════════════╗          ╔═══════════════════════════════════════════════════════════════════════════════════════════════╗\n";
    cout << "               ║███▀▀▀     ▄▄▄▄▄ ▄▄▄▄  ▄▄ ▄▄▄▄▄▄    ▄▄▄▄ ▄▄▄▄   ▄▄▄  ▄▄▄▄  ▄▄▄▄▄  ▄▄▄▄ ║          ║▄██▀▀      ▄▄▄▄  ▄▄▄▄▄ ▄▄    ▄▄▄▄▄ ▄▄▄▄▄▄ ▄▄▄▄▄    ▄▄▄▄ ▄▄▄▄▄▄ ▄▄ ▄▄ ▄▄▄▄  ▄▄▄▄▄ ▄▄  ▄▄ ▄▄▄▄▄▄ ║\n";
    cout << "               ║▀▀███▄     ██▄▄  ██▀██ ██   ██     ██ ▄▄ ██▄█▄ ██▀██ ██▀██ ██▄▄  ███▄▄ ║          ║██▄▄▄      ██▀██ ██▄▄  ██    ██▄▄    ██   ██▄▄    ███▄▄   ██   ██ ██ ██▀██ ██▄▄  ███▄██   ██   ║\n";
    cout << "               ║▄▄▄██▀ ▄   ██▄▄▄ ████▀ ██   ██     ▀███▀ ██ ██ ██▀██ ████▀ ██▄▄▄ ▄▄██▀ ║          ║▀█▄▄█▀ ▄   ████▀ ██▄▄▄ ██▄▄▄ ██▄▄▄   ██   ██▄▄▄   ▄▄██▀   ██   ▀███▀ ████▀ ██▄▄▄ ██ ▀██   ██   ║\n";
    cout << "               ╚═══════════════════════════════════════════════════════════════════════╝          ╚═══════════════════════════════════════════════════════════════════════════════════════════════╝\n";
    cout << "                                                            ╔════════════════════════════════════════════════════════════════════════════════════╗\n"; 
    cout << "                                                            ║██████      ▄▄▄   ▄▄▄▄  ▄▄▄▄ ▄▄▄▄▄ ▄▄▄▄▄▄ ▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄  ▄▄▄▄  ▄▄▄  ▄▄▄▄  ▄▄▄▄  ║\n"; 
    cout << "                                                            ║  ▄██▀     ██▀██ ███▄▄ ███▄▄ ██▄▄    ██  ███▄▄  ██▄█▄ ██▄▄  ██▀▀▀ ██▀██ ██▄█▄ ██▀██ ║\n"; 
    cout << "                                                            ║ ██▀   ▄   ██▀██ ▄▄██▀ ▄▄██▀ ██▄▄▄   ██  ▄▄██▀  ██ ██ ██▄▄▄ ▀████ ▀███▀ ██ ██ ████▀ ║\n"; 
    cout << "                                                            ╚════════════════════════════════════════════════════════════════════════════════════╝\n"; 
    setColor(PINK);
    cout << "                                                                            ╔════════════════════════════════════════════════╗\n";
    cout << "                                                                            ║▄████▄     ▄▄     ▄▄▄   ▄▄▄▄  ▄▄▄  ▄▄ ▄▄ ▄▄▄▄▄▄ ║\n";
    cout << "                                                                            ║██▄▄██     ██    ██▀██ ██ ▄▄ ██▀██ ██ ██   ██   ║\n"; 
    cout << "                                                                            ║▀█▄▄█▀ ▄   ██▄▄▄ ▀███▀ ▀███▀ ▀███▀ ▀███▀   ██   ║\n";
    cout << "                                                                            ╚════════════════════════════════════════════════╝\n";
    setColor(DEFAULT);
    cout << "\n                   ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
    cout << "\n                                                                                    ┏━┓┏━┓┏━╸┏━┓┏━┓   ┏━╸╻ ╻┏━┓╻┏━╸┏━╸ ";
    cout << "\n                                                                                    ┣━┛┣┳┛┣╸ ┗━┓┗━┓   ┃  ┣━┫┃ ┃┃┃  ┣╸ ╹";
    cout << "\n                                                                                    ╹  ╹┗╸┗━╸┗━┛┗━┛   ┗━╸╹ ╹┗━┛╹┗━╸┗━╸╹";
} 
//-------------------------
//SEARCH STUDENT MENU (For Admin)
//-------------------------
void adminSession() {
    while(true) {
        displayAdminMenu();
        
        char choice = getChar();

        switch (choice) {
            case '1': registerStudentInteractive(); break;
            case '2': viewAllStudents(); break;
            case '3': {
                clearScreen();
    setColor(VIOLET);
    vector<string> ascii_lines = {
        "\n",
        "                                                            ███████╗███████╗ █████╗ ██████╗  ██████╗██╗  ██╗    ███████╗████████╗██╗   ██╗██████╗ ███████╗███╗   ██╗████████╗",
        "                                                            ██╔════╝██╔════╝██╔══██╗██╔══██╗██╔════╝██║  ██║    ██╔════╝╚══██╔══╝██║   ██║██╔══██╗██╔════╝████╗  ██║╚══██╔══╝",
        "                                                            ███████╗█████╗  ███████║██████╔╝██║     ███████║    ███████╗   ██║   ██║   ██║██║  ██║█████╗  ██╔██╗ ██║   ██║   ",
        "                                                            ╚════██║██╔══╝  ██╔══██║██╔══██╗██║     ██╔══██║    ╚════██║   ██║   ██║   ██║██║  ██║██╔══╝  ██║╚██╗██║   ██║   ",
        "                                                            ███████║███████╗██║  ██║██║  ██║╚██████╗██║  ██║    ███████║   ██║   ╚██████╔╝██████╔╝███████╗██║ ╚████║   ██║   ",
        "                                                            ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝    ╚══════╝   ╚═╝    ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝   ╚═╝   ",
        "\n"
    };

    for (const string& line : ascii_lines) { 
        cout << line << endl; 
        this_thread::sleep_for(chrono::milliseconds(20)); // Small delay for animation
    }
    setColor(WHITE);
    cout << "\n                    ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
                setColor(TEAL); cout << "                                                                                     ᴇɴᴛᴇʀ ꜱᴛᴜᴅᴇɴᴛ ɪᴅ ᴛᴏ ꜱᴇᴀʀᴄʜ: ";
                string id; getline(cin, id); 
                if (checkExit(id));
                int index = findStudentIndexByID(id);
                if (index != -1) viewOneStudent(students[index]);
                else { setColor(PEACH); cout << "ꜱᴛᴜᴅᴇɴᴛ ɴᴏᴛ ꜰᴏᴜɴᴅ.\n"; pauseScreen(); }
                break;
            }
            case '4': updateStudentDetails(); break;
            case '5': updateGradesInteractive(); break;
            case '6': deleteStudent(); break;
            case '7': financeManagement(nullptr); break; // Pass nullptr for Admin mode
            case '8': return; // Logout
            default: break;
        }
    }
}
//-------------------------
// STUDENT MENU (For Student)
//-------------------------
void displayStudentMenu(const string& name) {
    clearScreen();
    setColor(VIOLET);
    vector<string> ascii_lines = {
        "\n\n\n\n\n\n\n\n\n\n\n\n",
            "                                                              ███████╗████████╗██╗   ██╗██████╗ ███████╗███╗   ██╗████████╗    ███╗   ███╗███████╗███╗   ██╗██╗   ██╗",
            "                                                              ██╔════╝╚══██╔══╝██║   ██║██╔══██╗██╔════╝████╗  ██║╚══██╔══╝    ████╗ ████║██╔════╝████╗  ██║██║   ██║",
            "                                                              ███████╗   ██║   ██║   ██║██║  ██║█████╗  ██╔██╗ ██║   ██║       ██╔████╔██║█████╗  ██╔██╗ ██║██║   ██║",
            "                                                              ╚════██║   ██║   ██║   ██║██║  ██║██╔══╝  ██║╚██╗██║   ██║       ██║╚██╔╝██║██╔══╝  ██║╚██╗██║██║   ██║",
            "                                                              ███████║   ██║   ╚██████╔╝██████╔╝███████╗██║ ╚████║   ██║       ██║ ╚═╝ ██║███████╗██║ ╚████║╚██████╔╝",
            "                                                              ╚══════╝   ╚═╝    ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝   ╚═╝       ╚═╝     ╚═╝╚══════╝╚═╝  ╚═══╝ ╚═════╝ ",
        "\n"
    };

    for (const string& line : ascii_lines) { 
        cout << line << endl; 
        this_thread::sleep_for(chrono::milliseconds(20)); 
    } 
    setColor(PEACH); 
    cout << "                                                                                           ===== ᴡᴇʟᴄᴏᴍᴇ " << name << " =====\n\n";
    setColor(WHITE);
    cout << "               ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n\n";
    cout << "         ╔════════════════════════════════════════════════════════════════════════╗    ╔═══════════════════════════════════════════════╗    ╔══════════════════════════════════════════════════════════╗\n";
    cout << "         ║▄██     ▄▄ ▄▄ ▄▄ ▄▄▄▄▄ ▄▄   ▄▄   ▄▄▄▄  ▄▄▄▄   ▄▄▄  ▄▄▄▄▄ ▄▄ ▄▄    ▄▄▄▄▄ ║    ║ ████▄      ▄▄▄   ▄▄▄▄  ▄▄▄▄ ▄▄▄▄▄ ▄▄▄▄▄▄ ▄▄▄▄ ║    ║████▄     ██████ ▄▄▄▄  ▄▄ ▄▄▄▄▄▄   ▄▄▄▄   ▄▄▄ ▄▄▄▄▄▄ ▄▄▄  ║\n";
    cout << "         ║ ██     ██▄██ ██ ██▄▄  ██ ▄ ██   ██▄█▀ ██▄█▄ ██▀██ ██▄▄  ██ ██    ██▄▄  ║    ║  ▄██▀     ██▀██ ███▄▄ ███▄▄ ██▄▄    ██  ███▄▄ ║    ║ ▄▄██     ██▄▄   ██▀██ ██   ██     ██▀██ ██▀██  ██  ██▀██ ║\n";
    cout << "         ║ ██ ▄    ▀█▀  ██ ██▄▄▄  ▀█▀█▀    ██    ██ ██ ▀███▀ ██    ██ ██▄▄▄ ██▄▄▄ ║    ║ ███▄▄ ▄   ██▀██ ▄▄██▀ ▄▄██▀ ██▄▄▄   ██  ▄▄██▀ ║    ║▄▄▄█▀ ▄   ██▄▄▄▄ ████▀ ██   ██     ████▀ ██▀██  ██  ██▀██ ║\n";
    cout << "         ╚════════════════════════════════════════════════════════════════════════╝    ╚═══════════════════════════════════════════════╝    ╚══════════════════════════════════════════════════════════╝\n";
    setColor(PINK);
    cout << "                                                                                             ╔════════════════════════════════╗\n";
    cout << "                                                                                             ║██  ██    ▄▄▄▄▄ ▄▄ ▄▄ ▄▄ ▄▄▄▄▄▄ ║\n";
    cout << "                                                                                             ║▀█████    ██▄▄  ▀█▄█▀ ██   ██   ║\n";
    cout << "                                                                                             ║    ██ ▄  ██▄▄▄ ██ ██ ██   ██   ║\n";
    cout << "                                                                                             ╚════════════════════════════════╝\n";
    setColor(DEFAULT);
    cout << "\n               ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
    cout << "\n                                                                                           ┏━┓┏━┓┏━╸┏━┓┏━┓   ┏━╸╻ ╻┏━┓╻┏━╸┏━╸ ";
    cout << "\n                                                                                           ┣━┛┣┳┛┣╸ ┗━┓┗━┓   ┃  ┣━┫┃ ┃┃┃  ┣╸ ╹";
    cout << "\n                                                                                           ╹  ╹┗╸┗━╸┗━┛┗━┛   ┗━╸╹ ╹┗━┛╹┗━╸┗━╸╹";
}

void studentSession(Student& s) {
    while(true) {
        displayStudentMenu(s.name);
        
        char choice = getChar();

        switch (choice) {
            case '1': viewOneStudent(s); break;
            case '2': financeManagement(&s); break; 
            case '3': updateOwnDetails(s); break;
            case '4': return; // Logout
            default: break;
        }
    }
}

int main() {
    cin.clear();
    initializeConsole();
    loadStudentsFromFile();
    displayTitleScreen();

    while (true) {
        //-------------------------
        // LOGIN PAGE 
        //-------------------------
        clearScreen();
        setColor(TEAL); 
            vector<string> ascii_lines = {
                "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                "                                                                   ██╗      ██████╗  ██████╗       ██╗███╗   ██╗    ██████╗  █████╗  ██████╗ ███████╗",
                "                                                                   ██║     ██╔═══██╗██╔════╝       ██║████╗  ██║    ██╔══██╗██╔══██╗██╔════╝ ██╔════╝",
                "                                                                   ██║     ██║   ██║██║  ███╗█████╗██║██╔██╗ ██║    ██████╔╝███████║██║  ███╗█████╗  ",
                "                                                                   ██║     ██║   ██║██║   ██║╚════╝██║██║╚██╗██║    ██╔═══╝ ██╔══██║██║   ██║██╔══╝  ",
                "                                                                   ██████╗ ╚██████╔╝╚██████╔╝      ██║██║ ╚████║    ██║     ██║  ██║╚██████╔╝███████╗",
                "                                                                   ╚═════╝  ╚═════╝  ╚═════╝       ╚═╝╚═╝  ╚═══╝    ╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚══════╝",
                "\n"
            };
            

            for (const string& line : ascii_lines) { 
                cout << line << endl; 
                this_thread::sleep_for(chrono::milliseconds(20)); // Small delay for animation
            } 
       setColor(WHITE);
    cout << "                                                     ╔═════════════════════════════════════════════╗            ╔══════════════════════════════════════════╗\n";
    cout << "                                                     ║▄██     ▄▄▄▄▄ ▄▄  ▄▄ ▄▄▄▄   ▄▄▄  ▄▄    ▄▄    ║            ║████▄     ▄▄     ▄▄▄   ▄▄▄▄     ▄▄ ▄▄  ▄▄ ║\n";
    cout << "                                                     ║ ██     ██▄▄  ███▄██ ██▄█▄ ██▀██ ██    ██    ║            ║ ▄██▀     ██    ██▀██ ██ ▄▄ ▄▄▄ ██ ███▄██ ║\n";
    cout << "                                                     ║ ██ ▄   ██▄▄▄ ██ ▀██ ██ ██ ▀███▀ ██▄▄▄ ██▄▄▄ ║            ║███▄▄ ▄   ██▄▄▄ ▀███▀ ▀███▀     ██ ██ ▀██ ║\n";
    cout << "                                                     ╚═════════════════════════════════════════════╝            ╚══════════════════════════════════════════╝\n";
    setColor(PINK);
    cout << "                                                                                       ╔════════════════════════════════╗\n";
    cout << "                                                                                       ║████▄     ▄▄▄▄▄ ▄▄ ▄▄ ▄▄ ▄▄▄▄▄▄ ║\n";
    cout << "                                                                                       ║ ▄▄██     ██▄▄  ▀█▄█▀ ██   ██   ║\n";
    cout << "                                                                                       ║▄▄▄█▀ ▄   ██▄▄▄ ██ ██ ██   ██   ║\n";
    cout << "                                                                                       ╚════════════════════════════════╝\n\n";
    setColor(DEFAULT);
    cout << "                    ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
    cout << "\n                                                                                         ┏━┓┏━┓┏━╸┏━┓┏━┓   ┏━╸╻ ╻┏━┓╻┏━╸┏━╸ ";
    cout << "\n                                                                                         ┣━┛┣┳┛┣╸ ┗━┓┗━┓   ┃  ┣━┫┃ ┃┃┃  ┣╸ ╹";
    cout << "\n                                                                                         ╹  ╹┗╸┗━╸┗━┛┗━┛   ┗━╸╹ ╹┗━┛╹┗━╸┗━╸╹";

        char input = getChar();

        if (input == '1') {
            registerStudentInteractive();
        } 
        
        //-------------------------
        // Goodbye Display 
        //-------------------------
        else if (input == '3') {
            clearScreen();
            setColor(TEAL);
                vector<string> ascii_lines = {
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "                                                                               ▄████  ▒█████   ▒█████  ▓█████▄  ▄▄▄▄ ▓██   ██▓▓█████ ",
                    "                                                                              ██▒ ▀█▒▒██▒  ██▒▒██▒  ██▒▒██▀ ██▌▓█████▄▒██  ██▒▓█   ▀ ",
                    "                                                                             ▒██░▄▄▄░▒██░  ██▒▒██░  ██▒░██   █▌▒██▒ ▄██▒██ ██░▒███   ",
                    "                                                                             ░▓█  ██▓▒██   ██░▒██   ██░░▓█▄   ▌▒██░█▀  ░ ▐██▓░▒▓█  ▄ ",
                    "                                                                             ░▒▓███▀▒░ ████▓▒░░ ████▓▒░░▒████▓ ░▓█  ▀█▓░ ██▒▓░░▒████▒",
                    "                                                                              ░▒   ▒ ░ ▒░▒░▒░ ░ ▒░▒░▒░  ▒▒▓  ▒ ░▒▓███▀▒ ██▒▒▒ ░░ ▒░ ░",
                    "                                                                               ░   ░   ░ ▒ ▒░   ░ ▒ ▒░  ░ ▒  ▒ ▒░▒   ░▓██ ░▒░  ░ ░  ░",
                    "                                                                             ░ ░   ░ ░ ░ ░ ▒  ░ ░ ░ ▒   ░ ░  ░  ░    ░▒ ▒ ░░     ░   ",
                    "                                                                                  ░     ░ ░      ░ ░     ░     ░     ░ ░        ░  ░ ",
                    "\n"
                    "\n"
                    "\n"
                    "\n"
                    "\n"
                };
                
    
                for (const string& line : ascii_lines) { 
                    cout << line << endl; 
                    this_thread::sleep_for(chrono::milliseconds(100)); // Small delay for animation
                }
            return 0;
        }
        else if (input == '2') {
            //-------------------------
            // Display Menu (For Login Type(Admin-Student))
            //-------------------------
            clearScreen();
            setColor(TEAL);
                vector<string> ascii_lines = {
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "                                                                   ██╗      ██████╗  ██████╗       ██╗███╗   ██╗    ████████╗██╗   ██╗██████╗ ███████╗",
                    "                                                                   ██║     ██╔═══██╗██╔════╝       ██║████╗  ██║    ╚══██╔══╝╚██╗ ██╔╝██╔══██╗██╔════╝",
                    "                                                                   ██║     ██║   ██║██║  ███╗█████╗██║██╔██╗ ██║       ██║    ╚████╔╝ ██████╔╝█████╗  ",
                    "                                                                   ██║     ██║   ██║██║   ██║╚════╝██║██║╚██╗██║       ██║     ╚██╔╝  ██╔═══╝ ██╔══╝  ",
                    "                                                                   ███████╗╚██████╔╝╚██████╔╝      ██║██║ ╚████║       ██║      ██║   ██║     ███████╗",
                    "                                                                   ╚══════╝ ╚═════╝  ╚═════╝       ╚═╝╚═╝  ╚═══╝       ╚═╝      ╚═╝   ╚═╝     ╚══════╝",
                    "\n"
                };
                
    
                for (const string& line : ascii_lines) { 
                    cout << line << endl; 
                    this_thread::sleep_for(chrono::milliseconds(20)); // Small delay for animation
                }
            setColor(WHITE);
            cout << "                                                   ╔═════════════════════════════════════════════════════╗            ╔═════════════════════════════════════════╗\n";
            cout << "                                                   ║▄██      ▄▄▄▄ ▄▄▄▄▄▄ ▄▄ ▄▄ ▄▄▄▄  ▄▄▄▄▄ ▄▄  ▄▄ ▄▄▄▄▄▄ ║            ║████▄      ▄▄▄  ▄▄▄▄  ▄▄   ▄▄ ▄▄ ▄▄  ▄▄  ║\n";
            cout << "                                                   ║ ██     ███▄▄   ██   ██ ██ ██▀██ ██▄▄  ███▄██   ██   ║            ║ ▄██▀     ██▀██ ██▀██ ██▀▄▀██ ██ ███▄██  ║\n";
            cout << "                                                   ║ ██ ▄   ▄▄██▀   ██   ▀███▀ ████▀ ██▄▄▄ ██ ▀██   ██   ║            ║███▄▄ ▄   ██▀██ ████▀ ██   ██ ██ ██ ▀██  ║\n";
            cout << "                                                   ╚═════════════════════════════════════════════════════╝            ╚═════════════════════════════════════════╝\n";
            setColor(PINK);
            cout << "                                                                                      ╔════════════════════════════════════════════════╗\n";
            cout << "                                                                                      ║████▄      ▄▄▄▄  ▄▄▄    ▄▄▄▄   ▄▄▄   ▄▄▄▄ ▄▄ ▄▄ ║\n";
            cout << "                                                                                      ║ ▄▄██     ██ ▄▄ ██▀██   ██▄██ ██▀██ ██▀▀▀ ██▄█▀ ║\n";
            cout << "                                                                                      ║▄▄▄█▀ ▄   ▀███▀ ▀███▀   ██▄█▀ ██▀██ ▀████ ██ ██ ║\n";
            cout << "                                                                                      ╚════════════════════════════════════════════════╝\n";
            setColor(DEFAULT);
            cout << "                    ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
            cout << "\n                                                                                           ┏━┓┏━┓┏━╸┏━┓┏━┓   ┏━╸╻ ╻┏━┓╻┏━╸┏━╸ ";
            cout << "\n                                                                                           ┣━┛┣┳┛┣╸ ┗━┓┗━┓   ┃  ┣━┫┃ ┃┃┃  ┣╸ ╹";
            cout << "\n                                                                                           ╹  ╹┗╸┗━╸┗━┛┗━┛   ┗━╸╹ ╹┗━┛╹┗━╸┗━╸╹";

            
            char typeChoice = getChar();

            if (typeChoice == '1') {
                //-------------------------
                // Display Menu (For Student Account)
                //-------------------------
                clearScreen();
                setColor(VIOLET);
                vector<string> ascii_lines = {
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "\n",
                    "                                             ███████╗████████╗██╗   ██╗██████╗ ███████╗███╗   ██╗████████╗     █████╗  ██████╗ ██████╗ ██████╗ ██╗   ██╗███╗   ██╗████████╗",
                    "                                             ██╔════╝╚══██╔══╝██║   ██║██╔══██╗██╔════╝████╗  ██║╚══██╔══╝    ██╔══██╗██╔════╝██╔════╝██╔═══██╗██║   ██║████╗  ██║╚══██╔══╝",
                    "                                             ███████╗   ██║   ██║   ██║██║  ██║█████╗  ██╔██╗ ██║   ██║       ███████║██║     ██║     ██║   ██║██║   ██║██╔██╗ ██║   ██║   ",
                    "                                             ╚════██║   ██║   ██║   ██║██║  ██║██╔══╝  ██║╚██╗██║   ██║       ██╔══██║██║     ██║     ██║   ██║██║   ██║██║╚██╗██║   ██║   ",
                    "                                             ███████║   ██║   ╚██████╔╝██████╔╝███████╗██║ ╚████║   ██║       ██║  ██║╚██████╗╚██████╗╚██████╔╝╚██████╔╝██║ ╚████║   ██║   ",
                    "                                             ╚══════╝   ╚═╝    ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝   ╚═╝       ╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═════╝  ╚═════╝ ╚═╝  ╚═══╝   ╚═╝   ",
                    "\n"
                    "                                                                                 ┏━╸┏┓╻╺┳╸┏━╸┏━┓    ┏━╸┏━┓┏━╸╺┳┓┏━╸┏┓╻╺┳╸╻┏━┓╻  ┏━┓                            ",
                    "                                                                       ╺━╸╺━╸    ┣╸ ┃┗┫ ┃ ┣╸ ┣┳┛    ┃  ┣┳┛┣╸  ┃┃┣╸ ┃┗┫ ┃ ┃┣━┫┃  ┗━┓  ╺━╸╺━╸                    ",
                    "                                                                                 ┗━╸╹ ╹ ╹ ┗━╸╹┗╸    ┗━╸╹┗╸┗━╸╺┻┛┗━╸╹ ╹ ╹ ╹╹ ╹┗━╸┗━┛                            ",
                    "\n"
                };
                
    
                for (const string& line : ascii_lines) { 
                    cout << line << endl; 
                    this_thread::sleep_for(chrono::milliseconds(20)); 
                }  
                setColor(WHITE);
                slowPrint("                                                                           ══════════════════════════════════════════════════════════════\n", 2);
                slowPrint("\n                                                                                              ᴇɴᴛᴇʀ ꜱᴛᴜᴅᴇɴᴛ ɪᴅ: ", 3); string id; getline(cin, id);
                int idx = findStudentIndexByID(id);
                
                if (idx == -1) {
                    setColor(PEACH); cout << "\n                                                                                              ᴇʀʀᴏʀ: ɪᴅ ɴᴏᴛ ꜰᴏᴜɴᴅ.\n"; 
                    setColor(WHITE); slowPrint("\n                                                                           ══════════════════════════════════════════════════════════════\n", 2); pauseScreen();
                } else {
                                     cout << "\n                                                                               ᴇɴᴛᴇʀ ɴᴀᴍᴇ (ꜰᴏʀ ᴄᴏɴꜰɪʀᴍᴀᴛɪᴏɴ): "; string name; getline(cin, name);
                    if (toLower(name) == toLower(students[idx].name)) {
                        setColor(MINT); cout << "\n                                                                                                ʟᴏɢɪɴ ꜱᴜᴄᴄᴇꜱꜱꜰᴜʟ!\n";
                        setColor(WHITE); slowPrint("\n                                                                           ══════════════════════════════════════════════════════════════\n", 2); pauseScreen();
                        studentSession(students[idx]);
                    } else {
                        setColor(PEACH); cout << "\n                                                                                        ᴇʀʀᴏʀ: ɴᴀᴍᴇ ᴅᴏᴇꜱ ɴᴏᴛ ᴍᴀᴛᴄʜ ɪᴅ ʀᴇᴄᴏʀᴅ.\n";
                        setColor(WHITE); slowPrint("\n                                                                           ══════════════════════════════════════════════════════════════\n", 2); pauseScreen();
                    }
                }
            } 
            else if (typeChoice == '2') {
                //-------------------------
                // Display Menu (For Admin Account)
                //-------------------------
                clearScreen();
                setColor(VIOLET);
                vector<string> ascii_lines = {
                    "\n\n\n\n\n\n\n\n\n\n\n\n\n",
                    "                                                       █████╗ ██████╗ ███╗   ███╗██╗███╗   ██╗     █████╗  ██████╗ ██████╗ ██████╗ ██╗   ██╗███╗   ██╗████████╗",
                    "                                                      ██╔══██╗██╔══██╗████╗ ████║██║████╗  ██║    ██╔══██╗██╔════╝██╔════╝██╔═══██╗██║   ██║████╗  ██║╚══██╔══╝",
                    "                                                      ███████║██║  ██║██╔████╔██║██║██╔██╗ ██║    ███████║██║     ██║     ██║   ██║██║   ██║██╔██╗ ██║   ██║   ",
                    "                                                      ██╔══██║██║  ██║██║╚██╔╝██║██║██║╚██╗██║    ██╔══██║██║     ██║     ██║   ██║██║   ██║██║╚██╗██║   ██║   ",
                    "                                                      ██║  ██║██████╔╝██║ ╚═╝ ██║██║██║ ╚████║    ██║  ██║╚██████╗╚██████╗╚██████╔╝╚██████╔╝██║ ╚████║   ██║   ",
                    "                                                      ╚═╝  ╚═╝╚═════╝ ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝    ╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═════╝  ╚═════╝ ╚═╝  ╚═══╝   ╚═╝   ",
                    "\n"
                    "                                                                                 ┏━╸┏┓╻╺┳╸┏━╸┏━┓    ┏━╸┏━┓┏━╸╺┳┓┏━╸┏┓╻╺┳╸╻┏━┓╻  ┏━┓                            ",
                    "                                                                       ╺━╸╺━╸    ┣╸ ┃┗┫ ┃ ┣╸ ┣┳┛    ┃  ┣┳┛┣╸  ┃┃┣╸ ┃┗┫ ┃ ┃┣━┫┃  ┗━┓  ╺━╸╺━╸                    ",
                    "                                                                                 ┗━╸╹ ╹ ╹ ┗━╸╹┗╸    ┗━╸╹┗╸┗━╸╺┻┛┗━╸╹ ╹ ╹ ╹╹ ╹┗━╸┗━┛                            ",
                    "\n" 
                };
                
    
                for (const string& line : ascii_lines) { 
                    cout << line << endl; 
                    this_thread::sleep_for(chrono::milliseconds(20)); // Small delay for animation
                } 
                setColor(WHITE);
                cout << "\n";
                slowPrint("                                                                                ════════════════════════════════════════════════════\n", 2);
                slowPrint("\n                                                                                                 ᴜꜱᴇʀɴᴀᴍᴇ: ", 2); string u; getline(cin, u); 
                slowPrint("\n                                                                                                 ᴘᴀꜱꜱᴡᴏʀᴅ: ", 2); 
                string p = getPassword();
                
                if (u == "admin" && p == "admin123") {
                    setColor(MINT); slowPrint("\n                                                                                                 ᴀᴄᴄᴇꜱꜱ ɢʀᴀɴᴛᴇᴅ.\n", 3); 
                    setColor(WHITE); slowPrint("\n                                                                                ════════════════════════════════════════════════════\n", 2); pauseScreen();
                    adminSession();
                } else {
                    setColor(PINK); slowPrint("\n                                                                                                 ᴀᴄᴄᴇꜱꜱ ᴅᴇɴɪᴇᴅ.\n", 10); 
                    setColor(WHITE); slowPrint("\n                                                                                ════════════════════════════════════════════════════\n", 2); pauseScreen();
                }
            }
        }
    }
    return 0; 
}