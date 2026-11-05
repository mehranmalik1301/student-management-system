/**
 * ============================================================================
 * Amar Singh College Srinagar - Advanced Campus System
 * Department of Computer Applications - Batch-2024
 * 
 * Features: 4-Subject Attendance, Notice Board, Finance Ledger, Library
 * Architecture: MVC, Binary Crypto-Serialization
 * Target IDE: Dev-C++ 5.11 Compatible (C++98 / C++11)
 * ============================================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <thread>
#include <chrono>
#include <stdexcept>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
    #define CLEAR_CMD "cls"
#else
    #include <termios.h>
    #include <unistd.h>
    #define CLEAR_CMD "clear"
#endif

using namespace std;

// ============================================================================
// [1] CONFIGURATION & UI STYLING
// ============================================================================
namespace Config {
    const string FACULTY_FILE = "sys_faculty.dat";
    const string STUDENT_FILE = "sys_students.dat";
    const string NOTICE_FILE  = "sys_notices.dat";
    const string CRYPTO_KEY   = "ASC_ERP_XOR_VIGENERE_KEY_2024";
    const uint32_t FILE_MAGIC = 0x41534355; 
    const double BASE_FEE     = 45000.0;    // Standard BCA Yearly Fee
}

namespace UI {
    const string RESET   = "\033[0m";
    const string BOLD    = "\033[1m";
    const string RED     = "\033[31m";
    const string GREEN   = "\033[32m";
    const string YELLOW  = "\033[33m";
    const string BLUE    = "\033[34m";
    const string CYAN    = "\033[36m";
    const string MAGENTA = "\033[35m";
}

// ============================================================================
// [2] UTILITIES & SECURITY
// ============================================================================
class Security {
public:
    static string hashPassword(const string& password) noexcept {
        uint64_t hash = 14695981039346656037ull;
        for (size_t i = 0; i < password.length(); i++) {
            hash ^= static_cast<uint64_t>(static_cast<unsigned char>(password[i]));
            hash *= 1099511628211ull;
        }
        stringstream ss;
        ss << hex << setw(16) << setfill('0') << hash;
        return ss.str();
    }

    static string encryptDecrypt(const string& data) noexcept {
        string out = data;
        const string& key = Config::CRYPTO_KEY;
        for (size_t i = 0; i < out.size(); ++i) out[i] ^= key[i % key.length()];
        return out;
    }
};

class Utils {
public:
    static void clearScreen() { system(CLEAR_CMD); }

    static void pause() {
        cout << UI::YELLOW << "\nPress [Enter] to return..." << UI::RESET;
        string dummy; getline(cin, dummy);
    }

    static void simulateLoading(const string& message, int duration_ms = 600) {
        cout << UI::CYAN << message << UI::RESET;
        for (int i = 0; i < 3; ++i) {
            cout << UI::CYAN << "." << UI::RESET << flush;
            this_thread::sleep_for(chrono::milliseconds(duration_ms / 3));
        }
        cout << "\n";
    }

    static string getTimestamp() {
        time_t now = time(NULL);
        tm* ltm = localtime(&now);
        char buf[64];
        strftime(buf, sizeof(buf), "%d-%b-%Y %H:%M", ltm);
        return string(buf);
    }

    static void drawLine(int length = 85, char ch = '=') {
        cout << UI::BLUE << string(length, ch) << UI::RESET << "\n";
    }

    static void printCentered(const string& text, int width = 85) {
        int padLen = width - text.length();
        if (padLen <= 0) { cout << text << "\n"; } 
        else {
            int leftPad = padLen / 2;
            cout << string(leftPad, ' ') << text << "\n";
        }
    }
    
    static void printHeader() {
        clearScreen();
        drawLine();
        cout << UI::BOLD << UI::CYAN;
        printCentered("AMAR SINGH COLLEGE SRINAGAR");
        printCentered("Department of Computer Applications - Batch-2024");
        cout << UI::RESET;
        drawLine();
    }
};

class IO {
public:
    static string readLine(const string& prompt) {
        cout << UI::BOLD << prompt << UI::RESET;
        string input; getline(cin, input);
        return input;
    }

    static string readHidden(const string& prompt) {
        cout << UI::BOLD << prompt << UI::RESET;
        string password;
#ifdef _WIN32
        char ch;
        while ((ch = _getch()) != '\r') {
            if (ch == '\b') { if (!password.empty()) { password.erase(password.length()-1); cout << "\b \b"; } } 
            else { password += ch; cout << '*'; }
        }
        cout << "\n";
#else
        termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt); newt = oldt; newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt); getline(cin, password);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); cout << "\n";
#endif
        return password;
    }

    // New strict DOB validator
    static string readDOB(const string& prompt) {
        while (true) {
            string dob = readLine(prompt);
            // Verify format is exactly XX-XX-XXXX
            if (dob.length() == 10 && dob[2] == '-' && dob[5] == '-') {
                return dob;
            }
            cout << UI::RED << "[!] Invalid format. Must be DD-MM-YYYY (e.g., 12-09-2005).\n" << UI::RESET;
        }
    }

    static double readDouble(const string& prompt, double minVal = 0.0) {
        while (true) {
            string s = readLine(prompt);
            try {
                double val = stod(s);
                if (val >= minVal) return val;
            } catch (...) {}
            cout << UI::RED << "[!] Invalid input. Please enter a valid amount.\n" << UI::RESET;
        }
    }

    static int readInt(const string& prompt, int minVal, int maxVal) {
        while (true) {
            string s = readLine(prompt);
            try {
                int val = stoi(s);
                if (val >= minVal && val <= maxVal) return val;
            } catch (...) {}
            cout << UI::RED << "[!] Invalid input. Please enter a valid number.\n" << UI::RESET;
        }
    }
};

// ============================================================================
// [3] DATA MODELS
// ============================================================================
struct Notice {
    int id;
    string date, author, title, content;

    string serialize() const {
        stringstream ss; ss << id;
        return ss.str() + "|" + date + "|" + author + "|" + title + "|" + content;
    }

    static Notice deserialize(const string& data) {
        Notice n; stringstream ss(data); string t; vector<string> p;
        while (getline(ss, t, '|')) p.push_back(t);
        if (p.size() >= 5) { n.id = stoi(p[0]); n.date = p[1]; n.author = p[2]; n.title = p[3]; n.content = p[4]; }
        return n;
    }
};

struct Faculty {
    string fullName, username, passwordHash, role, createdAt;

    string serialize() const { return fullName + "|" + username + "|" + passwordHash + "|" + role + "|" + createdAt; }

    static Faculty deserialize(const string& data) {
        Faculty f; stringstream ss(data); string t; vector<string> p;
        while (getline(ss, t, '|')) p.push_back(t);
        if (p.size() >= 5) { f.fullName = p[0]; f.username = p[1]; f.passwordHash = p[2]; f.role = p[3]; f.createdAt = p[4]; }
        return f;
    }
};

struct Student {
    // Basic Info
    string rollNo, name, parentage, contact, dob;
    int currentSemester, admissionYear;
    string academicStatus;
    
    // 4-Subject Attendance System [Delivered & Attended]
    int attDbmsDel, attDbmsAtt;
    int attCppDel, attCppAtt;
    int attMathDel, attMathAtt;
    int attIotDel, attIotAtt;
    
    // Finance
    double totalFee;
    double feePaid;
    
    // Library
    vector<string> issuedBooks;
    string createdAt, updatedAt;

    // Constructor to initialize all defaults to 0 safely
    Student() : attDbmsDel(0), attDbmsAtt(0), attCppDel(0), attCppAtt(0),
                attMathDel(0), attMathAtt(0), attIotDel(0), attIotAtt(0),
                totalFee(Config::BASE_FEE), feePaid(0.0) {}

    string joinVec(const vector<string>& v) const {
        string r; for (size_t i=0; i<v.size(); ++i) r += v[i] + (i+1 < v.size() ? "^" : ""); return r;
    }
    vector<string> splitVec(const string& s) {
        vector<string> r; stringstream ss(s); string t;
        while (getline(ss, t, '^')) if (!t.empty()) r.push_back(t); return r;
    }

    string serialize() const {
        ostringstream out;
        out << rollNo << "|" << name << "|" << parentage << "|" << contact << "|" << dob << "|"
            << currentSemester << "|" << admissionYear << "|" << academicStatus << "|"
            << attDbmsDel << "|" << attDbmsAtt << "|"
            << attCppDel << "|" << attCppAtt << "|"
            << attMathDel << "|" << attMathAtt << "|"
            << attIotDel << "|" << attIotAtt << "|"
            << fixed << setprecision(2) << totalFee << "|" << feePaid << "|"
            << joinVec(issuedBooks) << "|" << createdAt << "|" << updatedAt;
        return out.str();
    }

    static Student deserialize(const string& data) {
        Student s; stringstream ss(data); string t; vector<string> p;
        while (getline(ss, t, '|')) p.push_back(t);
        if (p.size() >= 21) {
            s.rollNo = p[0]; s.name = p[1]; s.parentage = p[2]; s.contact = p[3]; s.dob = p[4];
            s.currentSemester = stoi(p[5]); s.admissionYear = stoi(p[6]); s.academicStatus = p[7];
            s.attDbmsDel = stoi(p[8]); s.attDbmsAtt = stoi(p[9]);
            s.attCppDel = stoi(p[10]); s.attCppAtt = stoi(p[11]);
            s.attMathDel = stoi(p[12]); s.attMathAtt = stoi(p[13]);
            s.attIotDel = stoi(p[14]); s.attIotAtt = stoi(p[15]);
            s.totalFee = stod(p[16]); s.feePaid = stod(p[17]);
            s.issuedBooks = s.splitVec(p[18]);
            s.createdAt = p[19]; s.updatedAt = p[20];
        }
        return s;
    }
};

// ============================================================================
// [4] DATABASE MANAGEMENT
// ============================================================================
class Database {
public:
    template <typename T>
    static void save(const string& filename, const vector<T>& data) {
        ofstream file(filename.c_str(), ios::binary | ios::trunc);
        if (!file) return;
        uint32_t magic = Config::FILE_MAGIC;
        file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        for (size_t i = 0; i < data.size(); i++) {
            string enc = Security::encryptDecrypt(data[i].serialize());
            size_t len = enc.size();
            file.write(reinterpret_cast<const char*>(&len), sizeof(len));
            file.write(enc.c_str(), len);
        }
    }

    template <typename T>
    static vector<T> load(const string& filename) {
        vector<T> list; ifstream file(filename.c_str(), ios::binary);
        if (!file) return list;
        uint32_t magic;
        if (!file.read(reinterpret_cast<char*>(&magic), sizeof(magic)) || magic != Config::FILE_MAGIC) return list;
        size_t len;
        while (file.read(reinterpret_cast<char*>(&len), sizeof(len))) {
            string enc(len, '\0');
            if (file.read(&enc[0], len)) list.push_back(T::deserialize(Security::encryptDecrypt(enc)));
        }
        return list;
    }
};

// ============================================================================
// [5] APPLICATION LAYER
// ============================================================================
class Application {
private:
    vector<Faculty> faculties;
    vector<Student> students;
    vector<Notice> notices;
    Faculty loggedInFaculty;

    void loadAll() {
        faculties = Database::load<Faculty>(Config::FACULTY_FILE);
        students = Database::load<Student>(Config::STUDENT_FILE);
        notices = Database::load<Notice>(Config::NOTICE_FILE);
    }
    void saveAll() {
        Database::save(Config::FACULTY_FILE, faculties);
        Database::save(Config::STUDENT_FILE, students);
        Database::save(Config::NOTICE_FILE, notices);
    }

    // --- Helpers ---
    struct MatchRollNo {
        string target; MatchRollNo(const string& t) : target(t) {}
        bool operator()(const Student& st) const { return st.rollNo == target; }
    };
    struct MatchNoticeID {
        int target; MatchNoticeID(int t) : target(t) {}
        bool operator()(const Notice& n) const { return n.id == target; }
    };

    void printNoticeBoard() {
        cout << UI::BOLD << UI::MAGENTA << "\n[ CAMPUS NOTICE BOARD ]\n" << UI::RESET;
        if (notices.empty()) { cout << "  No active notices.\n\n"; return; }
        
        for (vector<Notice>::reverse_iterator it = notices.rbegin(); it != notices.rend(); ++it) {
            Utils::drawLine(85, '-');
            cout << UI::CYAN << " ID: " << it->id << " | Date: " << it->date << " | By: " << it->author << UI::RESET << "\n";
            cout << UI::BOLD << " " << it->title << UI::RESET << "\n\n  " << it->content << "\n";
        }
        Utils::drawLine(85, '-');
    }

    void printSubjectAttRow(const string& name, int att, int del) {
        double perc = (del == 0) ? 100.0 : ((double)att / del) * 100.0;
        string color = (perc >= 75.0) ? UI::GREEN : UI::RED;
        cout << left << setw(35) << ("  " + name) 
             << ": " << right << setw(3) << att << " / " << left << setw(3) << del 
             << " | " << color << fixed << setprecision(1) << setw(5) << perc << "%" << UI::RESET;
        if (perc < 75.0 && del > 0) cout << UI::RED << " (SHORTAGE)" << UI::RESET;
        cout << "\n";
    }

    void printStudentProfile(const Student& s) {
        Utils::drawLine();
        cout << UI::BOLD << "  Student Profile : " << s.name << " (" << s.rollNo << ")\n" << UI::RESET;
        Utils::drawLine();
        
        cout << left << setw(20) << "  Parentage" << ": " << s.parentage << "\n"
             << setw(20) << "  Contact"   << ": " << s.contact << " | DOB: " << s.dob << "\n"
             << setw(20) << "  Batch Year" << ": " << s.admissionYear << " / Semester " << s.currentSemester << "\n"
             << setw(20) << "  Status"    << ": " << s.academicStatus << "\n\n";

        // Advanced 4-Subject Attendance Engine
        cout << UI::BOLD << UI::CYAN << "  [ Subject-wise Attendance ]\n" << UI::RESET;
        printSubjectAttRow("CT 1 (DBMS)", s.attDbmsAtt, s.attDbmsDel);
        printSubjectAttRow("CT 2 (C++)", s.attCppAtt, s.attCppDel);
        printSubjectAttRow("CT 3 (Computing Mathematics)", s.attMathAtt, s.attMathDel);
        printSubjectAttRow("Minor (IOT)", s.attIotAtt, s.attIotDel);
        
        int totalAtt = s.attDbmsAtt + s.attCppAtt + s.attMathAtt + s.attIotAtt;
        int totalDel = s.attDbmsDel + s.attCppDel + s.attMathDel + s.attIotDel;
        cout << "  ----------------------------------------------------------------------\n";
        printSubjectAttRow("OVERALL AGGREGATE", totalAtt, totalDel);
        cout << "\n";

        // Finance Engine
        cout << UI::BOLD << UI::CYAN << "  [ Financial Ledger ]\n" << UI::RESET;
        double dues = s.totalFee - s.feePaid;
        cout << "  Total Fees         : Rs. " << fixed << setprecision(2) << s.totalFee << "\n"
             << "  Amount Paid        : Rs. " << s.feePaid << "\n"
             << "  Outstanding Dues   : " << (dues > 0 ? UI::RED : UI::GREEN) << "Rs. " << dues << UI::RESET << "\n\n";

        // Library Engine
        cout << UI::BOLD << UI::CYAN << "  [ Library Assets Held ]\n" << UI::RESET;
        if (s.issuedBooks.empty()) cout << "  - No books currently issued.\n";
        else { for (size_t i = 0; i < s.issuedBooks.size(); i++) cout << "  - " << s.issuedBooks[i] << "\n"; }
        cout << "\n  Last Sync: " << s.updatedAt << "\n";
        Utils::drawLine();
    }

    // --- Modules ---
    void moduleAdmissions() {
        while(true) {
            Utils::printHeader();
            cout << UI::BOLD << "  [ ADMISSIONS & RECORDS DEPT ]\n" << UI::RESET;
            Utils::drawLine();
            cout << "  1. Enroll New Student\n  2. Update Demographics\n  3. Purge Record\n  0. Back\n";
            int ch = IO::readInt("Action [0-3]: ", 0, 3);
            if (ch == 0) return;

            if (ch == 1) {
                Student s;
                cout << "\n--- Student Enrollment ---\n";
                s.rollNo = IO::readLine("Assign Roll No    : ");
                s.name = IO::readLine("Full Name         : ");
                s.parentage = IO::readLine("Parentage         : ");
                s.contact = IO::readLine("Contact No        : ");
                
                // Using the new robust DOB validator here
                s.dob = IO::readDOB("DOB (DD-MM-YYYY)  : ");
                
                s.currentSemester = IO::readInt("Semester [1-8]    : ", 1, 8);
                s.admissionYear = IO::readInt("Admission Year    : ", 1990, 2100);
                s.academicStatus = "Active";
                s.createdAt = Utils::getTimestamp();
                s.updatedAt = s.createdAt;
                students.push_back(s);
                saveAll();
                cout << UI::GREEN << "\n[+] Student Enrolled Successfully.\n" << UI::RESET;
                Utils::pause();
            }
            else if (ch == 2 || ch == 3) {
                string r = IO::readLine("\nEnter Roll No: ");
                vector<Student>::iterator it = find_if(students.begin(), students.end(), MatchRollNo(r));
                if (it != students.end()) {
                    if (ch == 2) {
                        cout << UI::YELLOW << "\n[ Leave blank to keep existing ]\n" << UI::RESET;
                        string t = IO::readLine("Name [" + it->name + "]: "); if(!t.empty()) it->name = t;
                        t = IO::readLine("Contact [" + it->contact + "]: "); if(!t.empty()) it->contact = t;
                        it->updatedAt = Utils::getTimestamp();
                        saveAll();
                        cout << UI::GREEN << "\n[+] Profile Updated.\n" << UI::RESET;
                    } else {
                        students.erase(it); saveAll();
                        cout << UI::GREEN << "\n[+] Record Purged.\n" << UI::RESET;
                    }
                } else cout << UI::RED << "[!] Student Not Found.\n" << UI::RESET;
                Utils::pause();
            }
        }
    }

    void moduleAcademics() {
        Utils::printHeader();
        cout << UI::BOLD << "  [ ACADEMIC AFFAIRS (ATTENDANCE) ]\n" << UI::RESET;
        string r = IO::readLine("\nEnter Roll No to Mark Attendance: ");
        vector<Student>::iterator it = find_if(students.begin(), students.end(), MatchRollNo(r));
        if (it != students.end()) {
            cout << "\n  Select Subject to Update:\n";
            cout << "  1. CT 1 (DBMS)\n  2. CT 2 (C++)\n  3. CT 3 (Computing Mathematics)\n  4. Minor (IOT)\n";
            int subChoice = IO::readInt("Select [1-4]: ", 1, 4);

            int currentAtt = 0, currentDel = 0;
            string subName = "";
            if (subChoice == 1) { subName = "CT 1 (DBMS)"; currentAtt = it->attDbmsAtt; currentDel = it->attDbmsDel; }
            else if (subChoice == 2) { subName = "CT 2 (C++)"; currentAtt = it->attCppAtt; currentDel = it->attCppDel; }
            else if (subChoice == 3) { subName = "CT 3 (Computing Mathematics)"; currentAtt = it->attMathAtt; currentDel = it->attMathDel; }
            else if (subChoice == 4) { subName = "Minor (IOT)"; currentAtt = it->attIotAtt; currentDel = it->attIotDel; }

            cout << "\nSelected Subject   : " << UI::CYAN << subName << UI::RESET << "\n";
            cout << "Current Attendance : " << currentAtt << " / " << currentDel << "\n";

            int addDel = IO::readInt("Add NEW Lectures Delivered : ", 0, 100);
            int addAtt = IO::readInt("Add NEW Lectures Attended  : ", 0, addDel);

            if (subChoice == 1) { it->attDbmsAtt += addAtt; it->attDbmsDel += addDel; }
            else if (subChoice == 2) { it->attCppAtt += addAtt; it->attCppDel += addDel; }
            else if (subChoice == 3) { it->attMathAtt += addAtt; it->attMathDel += addDel; }
            else if (subChoice == 4) { it->attIotAtt += addAtt; it->attIotDel += addDel; }

            it->updatedAt = Utils::getTimestamp();
            saveAll();
            cout << UI::GREEN << "\n[+] Attendance Ledger Successfully Updated.\n" << UI::RESET;
        } else {
            cout << UI::RED << "[!] Student Not Found.\n" << UI::RESET;
        }
        Utils::pause();
    }

    void moduleFinance() {
        Utils::printHeader();
        cout << UI::BOLD << "  [ FINANCE & LEDGER DEPT ]\n" << UI::RESET;
        string r = IO::readLine("\nEnter Roll No to process fee: ");
        vector<Student>::iterator it = find_if(students.begin(), students.end(), MatchRollNo(r));
        if (it != students.end()) {
            cout << "\nOutstanding Dues: Rs. " << fixed << setprecision(2) << (it->totalFee - it->feePaid) << "\n";
            double payment = IO::readDouble("Enter Deposit Amount: Rs. ");
            it->feePaid += payment;
            it->updatedAt = Utils::getTimestamp();
            saveAll();
            cout << UI::GREEN << "\n[+] Payment Processed and Receipt Generated.\n" << UI::RESET;
        } else {
            cout << UI::RED << "[!] Student Not Found.\n" << UI::RESET;
        }
        Utils::pause();
    }

    void moduleLibrary() {
        Utils::printHeader();
        cout << UI::BOLD << "  [ CENTRAL LIBRARY ADMIN ]\n" << UI::RESET;
        string r = IO::readLine("\nEnter Student Roll No: ");
        vector<Student>::iterator it = find_if(students.begin(), students.end(), MatchRollNo(r));
        if (it != students.end()) {
            cout << "\n1. Issue New Book\n2. Return Book\n";
            int ch = IO::readInt("Action [1-2]: ", 1, 2);
            if (ch == 1) {
                string book = IO::readLine("Enter Book Name/ID: ");
                it->issuedBooks.push_back(book);
                it->updatedAt = Utils::getTimestamp();
                saveAll();
                cout << UI::GREEN << "\n[+] Book Issued.\n" << UI::RESET;
            } else {
                if (it->issuedBooks.empty()) { cout << "[!] No books to return.\n"; }
                else {
                    cout << "Books Held:\n";
                    for(size_t i=0; i<it->issuedBooks.size(); i++) {
                        stringstream sss; sss << i+1;
                        cout << sss.str() << ". " << it->issuedBooks[i] << "\n";
                    }
                    int bIdx = IO::readInt("Select Book to Return: ", 1, it->issuedBooks.size());
                    it->issuedBooks.erase(it->issuedBooks.begin() + (bIdx - 1));
                    it->updatedAt = Utils::getTimestamp();
                    saveAll();
                    cout << UI::GREEN << "\n[+] Book Returned to Library.\n" << UI::RESET;
                }
            }
        } else {
            cout << UI::RED << "[!] Student Not Found.\n" << UI::RESET;
        }
        Utils::pause();
    }

    void moduleNoticeBoard() {
        while(true) {
            Utils::printHeader();
            cout << UI::BOLD << "  [ BROADCAST & NOTICES DEPT ]\n" << UI::RESET;
            Utils::drawLine();
            printNoticeBoard();
            cout << "  1. Publish New Notice\n  2. Delete Notice\n  0. Back\n";
            int ch = IO::readInt("Action [0-2]: ", 0, 2);
            if (ch == 0) return;

            if (ch == 1) {
                Notice n;
                n.id = notices.empty() ? 1001 : notices.back().id + 1;
                n.date = Utils::getTimestamp();
                n.author = loggedInFaculty.fullName + " (" + loggedInFaculty.role + ")";
                cout << "\n";
                n.title = IO::readLine("Notice Title   : ");
                n.content = IO::readLine("Notice Content : ");
                notices.push_back(n);
                saveAll();
                cout << UI::GREEN << "\n[+] Broadcast Published to all Student Portals.\n" << UI::RESET;
            } else if (ch == 2) {
                int id = IO::readInt("Enter Notice ID to Delete: ", 1000, 99999);
                vector<Notice>::iterator it = remove_if(notices.begin(), notices.end(), MatchNoticeID(id));
                if (it != notices.end()) { notices.erase(it, notices.end()); saveAll(); cout << UI::GREEN << "\n[+] Notice Removed.\n" << UI::RESET; }
                else { cout << UI::RED << "\n[!] Notice ID Not Found.\n" << UI::RESET; }
            }
            Utils::pause();
        }
    }

    // --- Main Flows ---
    void facultyDashboard() {
        while (true) {
            Utils::printHeader();
            cout << UI::GREEN << "  Welcome, " << loggedInFaculty.fullName << " | Privilege: " << loggedInFaculty.role << "\n" << UI::RESET;
            Utils::drawLine();
            cout << "  [ MANAGEMENT MODULES ]\n"
                 << "  1. Admissions & Demographics\n"
                 << "  2. Academic Affairs (Attendance)\n"
                 << "  3. Finance & Ledger\n"
                 << "  4. Central Library Admin\n"
                 << "  5. Notice Board Broadcasts\n"
                 << "  6. System Master Search (View Any Profile)\n"
                 << "  0. Secure Logout\n";
            Utils::drawLine();

            int choice = IO::readInt("Access Module [0-6]: ", 0, 6);
            switch (choice) {
                case 1: moduleAdmissions(); break;
                case 2: moduleAcademics(); break;
                case 3: moduleFinance(); break;
                case 4: moduleLibrary(); break;
                case 5: moduleNoticeBoard(); break;
                case 6: {
                    string r = IO::readLine("\nEnter Roll No: ");
                    vector<Student>::iterator it = find_if(students.begin(), students.end(), MatchRollNo(r));
                    if (it != students.end()) printStudentProfile(*it);
                    else cout << UI::RED << "\n[!] Profile Not Found.\n" << UI::RESET;
                    Utils::pause();
                    break;
                }
                case 0: Utils::simulateLoading("Terminating Session"); return;
            }
        }
    }

    void studentPortal() {
        Utils::printHeader();
        cout << UI::BOLD << "\n[ STUDENT CLOUD PORTAL ]\n\n" << UI::RESET;
        string roll = IO::readLine("Roll No          : ");
        string dob = IO::readHidden("DOB (DD-MM-YYYY) : ");
        Utils::simulateLoading("Verifying Credentials");

        for (size_t i = 0; i < students.size(); i++) {
            if (students[i].rollNo == roll && students[i].dob == dob) {
                while(true) {
                    Utils::printHeader();
                    cout << UI::GREEN << "  Portal Active: " << students[i].name << " (" << students[i].rollNo << ")\n" << UI::RESET;
                    Utils::drawLine();
                    cout << "  1. View My Complete Academic Profile\n"
                         << "  2. View Campus Notice Board\n"
                         << "  0. Secure Logout\n";
                    Utils::drawLine();
                    int ch = IO::readInt("Select [0-2]: ", 0, 2);
                    if (ch == 1) { Utils::printHeader(); printStudentProfile(students[i]); Utils::pause(); }
                    else if (ch == 2) { Utils::printHeader(); printNoticeBoard(); Utils::pause(); }
                    else { Utils::simulateLoading("Closing Portal"); return; }
                }
            }
        }
        cout << UI::RED << "\n[!] Authentication Failed.\n" << UI::RESET; Utils::pause();
    }

public:
    void run() {
        // Initial Admin Setup if blank
        ifstream fCheck(Config::FACULTY_FILE.c_str());
        if (!fCheck.good()) {
            Utils::printHeader();
            cout << UI::YELLOW << "\n[ INITIAL DEPLOYMENT SETUP ]\n" << UI::RESET;
            Faculty f;
            f.fullName = IO::readLine("Admin Full Name : ");
            f.username = IO::readLine("Admin Username  : ");
            f.passwordHash = Security::hashPassword(IO::readHidden("Admin Password  : "));
            f.role = "Administrator";
            f.createdAt = Utils::getTimestamp();
            faculties.push_back(f);
            Database::save(Config::FACULTY_FILE, faculties);
            cout << UI::GREEN << "\n[+] System Bootstrapped Successfully!\n" << UI::RESET;
            Utils::pause();
        } else { loadAll(); }

        while (true) {
            Utils::printHeader();
            cout << UI::BOLD << "  [ CENTRAL GATEWAY ]\n" << UI::RESET;
            Utils::drawLine();
            cout << "  1. Student Cloud Portal\n"
                 << "  2. Faculty / Staff Login\n"
                 << "  0. Exit System\n";
            Utils::drawLine();
            int choice = IO::readInt("Select Gateway [0-2]: ", 0, 2);
            
            if (choice == 1) studentPortal();
            else if (choice == 2) {
                Utils::printHeader();
                cout << UI::BOLD << "\n[ FACULTY SECURE LOGIN ]\n\n" << UI::RESET;
                string user = IO::readLine("Username: ");
                string pass = Security::hashPassword(IO::readHidden("Password: "));
                Utils::simulateLoading("Authenticating");
                bool found = false;
                for (size_t i = 0; i < faculties.size(); i++) {
                    if (faculties[i].username == user && faculties[i].passwordHash == pass) {
                        loggedInFaculty = faculties[i]; found = true; facultyDashboard(); break;
                    }
                }
                if (!found) { cout << UI::RED << "\n[!] Access Denied.\n" << UI::RESET; Utils::pause(); }
            }
            else { Utils::simulateLoading("Shutting Down"); return; }
        }
    }
};

int main() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0; GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    try { Application app; app.run(); }
    catch (const exception& e) { cerr << "\n[CRIT] " << e.what() << "\n"; }
    return 0;
}

