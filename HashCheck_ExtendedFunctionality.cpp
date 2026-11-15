#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

const long long BASE = 257LL;
const long long MOD = 1000000007LL;
const string HISTORY_FILE = "audit_history.txt";
const string DEMO_FOLDER = "demo_submissions";

string riskLabel(float score);
string riskText(float score);

char toLowerChar(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return (char)(ch + ('a' - 'A'));
    }
    return ch;
}

bool isAlnumChar(char ch) {
    return (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9');
}

bool isIdentStart(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

string trimTo(const string& s, size_t width) {
    if (s.length() <= width) return s;
    return s.substr(0, width - 3) + "...";
}

void flushInputLine() {
    char ch;
    while (cin.get(ch) && ch != '\n') {
        if (cin.eof()) {
            break;
        }
    }
}

bool readFile(const string& path, string& content) {
    ifstream inFile(path.c_str(), ios::binary);
    if (!inFile.is_open()) {
        return false;
    }
    content.assign((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    return true;
}

bool writeFile(const string& path, const string& content) {
    ofstream out(path.c_str(), ios::binary);
    if (!out.is_open()) return false;
    out << content;
    return true;
}

vector<string> splitLines(const string& text) {
    vector<string> lines;
    string cur;
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == '\r') continue;
        if (text[i] == '\n') { lines.push_back(cur); cur.clear(); }
        else cur += text[i];
    }
    if (!cur.empty()) lines.push_back(cur);
    return lines;
}

struct LogNode {
    int runNo;
    string label;
    int kVal;
    float score;
    int collisions;
    int blockLen;
    string status;
    LogNode* next;
};

static LogNode* head = NULL;
static LogNode* tail = NULL;
static int testCount = 1;

string formatLogLine(const LogNode* n) {
    ostringstream oss;
    oss << "Run #" << setw(2) << left << n->runNo
        << " | Label: " << setw(30) << left << trimTo(n->label, 30)
        << " | k: " << setw(2) << right << n->kVal
        << " | Sim: " << setw(6) << fixed << setprecision(2) << right << n->score << "%"
        << " | Block: " << setw(4) << n->blockLen
        << " | Collisions: " << setw(2) << n->collisions
        << " | " << n->status;
    return oss.str();
}

void addHistory(const string& label, int k, float score, int coll, const string& status) {
    LogNode* newNode = new LogNode();
    newNode->runNo = testCount++;
    newNode->label = label;
    newNode->kVal = k;
    newNode->score = score;
    newNode->collisions = coll;
    newNode->blockLen = 0;
    newNode->status = status;
    newNode->next = NULL;

    if (head == NULL) {
        head = tail = newNode;
    } else {
        tail->next = newNode;
        tail = newNode;
    }

    ofstream outFile(HISTORY_FILE.c_str(), ios::app);
    if (outFile.is_open()) {
        outFile << formatLogLine(newNode) << "\n";
        outFile.close();
    }
}

void addHistory(const string& label, int k, float score, int coll, int blockLen) {
    LogNode* newNode = new LogNode();
    newNode->runNo = testCount++;
    newNode->label = label;
    newNode->kVal = k;
    newNode->score = score;
    newNode->collisions = coll;
    newNode->blockLen = blockLen;
    newNode->status = riskText(score);
    newNode->next = NULL;

    if (head == NULL) {
        head = tail = newNode;
    } else {
        tail->next = newNode;
        tail = newNode;
    }

    ofstream outFile(HISTORY_FILE.c_str(), ios::app);
    if (outFile.is_open()) {
        outFile << formatLogLine(newNode) << "\n";
        outFile.close();
    }
}

void showHistory() {
    cout << "\n=========================== AUDIT SESSION HISTORY (ACTIVE MEMORY) ===========================\n";
    if (head == NULL) {
        cout << "No evaluations logged in the active session.\n";
    } else {
        LogNode* curr = head;
        while (curr != NULL) {
            cout << formatLogLine(curr) << "\n";
            curr = curr->next;
        }
    }
    cout << "=============================================================================================\n";

    cout << "\n=========================== PERSISTED AUDIT LOG (" << HISTORY_FILE << ") ===========================\n";
    ifstream inFile(HISTORY_FILE.c_str());
    if (!inFile.is_open()) {
        cout << "No persistent audit log file found on disk yet.\n";
    } else {
        string logLine;
        bool hasEntries = false;
        while (getline(inFile, logLine)) {
            if (!logLine.empty()) {
                cout << logLine << "\n";
                hasEntries = true;
            }
        }
        if (!hasEntries) {
            cout << "Audit log file is empty.\n";
        }
        inFile.close();
    }
    cout << "=============================================================================================\n";
}

void freeHistory() {
    LogNode* curr = head;
    while (curr != NULL) {
        LogNode* nextNode = curr->next;
        delete curr;
        curr = nextNode;
    }
    head = tail = NULL;
}

string cleanCode(const string& sourceCode) {
    string cleaned = "";
    bool inLineComment = false;
    bool inBlockComment = false;
    bool inQuotes = false;
    bool inChar = false;

    for (size_t i = 0; i < sourceCode.length(); i++) {
        char ch = sourceCode[i];

        if (inLineComment) {
            if (ch == '\n') {
                inLineComment = false;
            }
            continue;
        }

        if (inBlockComment) {
            if (ch == '*' && i + 1 < sourceCode.length() && sourceCode[i + 1] == '/') {
                inBlockComment = false;
                i++;
            }
            continue;
        }

        if (inQuotes) {
            if (ch == '\\' && i + 1 < sourceCode.length()) {
                cleaned += '\\';
                cleaned += toLowerChar(sourceCode[i + 1]);
                i++;
                continue;
            }
            if (ch == '"') {
                inQuotes = false;
            }
            if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
                cleaned += toLowerChar(ch);
            }
            continue;
        }

        if (inChar) {
            if (ch == '\\' && i + 1 < sourceCode.length()) {
                cleaned += '\\';
                cleaned += toLowerChar(sourceCode[i + 1]);
                i++;
                continue;
            }
            if (ch == '\'') {
                inChar = false;
            }
            if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
                cleaned += toLowerChar(ch);
            }
            continue;
        }

        if (ch == '/' && i + 1 < sourceCode.length() && sourceCode[i + 1] == '/') {
            inLineComment = true;
            i++;
            continue;
        }
        if (ch == '/' && i + 1 < sourceCode.length() && sourceCode[i + 1] == '*') {
            inBlockComment = true;
            i++;
            continue;
        }

        if (ch == '"') {
            inQuotes = true;
            cleaned += '"';
            continue;
        }
        if (ch == '\'') {
            inChar = true;
            cleaned += '\'';
            continue;
        }

        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            continue;
        }

        cleaned += toLowerChar(ch);
    }
    return cleaned;
}

struct Prepared {
    string name;
    string raw;
    string cleaned;
    vector<int> lineMap;
};

static const char* KEYWORDS[] = {
    "int", "float", "double", "char", "long", "short", "void", "bool", "unsigned", "signed",
    "return", "if", "else", "for", "while", "do", "switch", "case", "break", "continue", "default",
    "struct", "class", "public", "private", "protected", "const", "static", "new", "delete",
    "true", "false", "include", "define", "using", "namespace", "auto", "try", "catch", "throw",
    "typedef", "enum", "sizeof", "this", "virtual", "template", "typename", "import", "package",
    "extends", "implements", "final", "null", "nullptr", "def", "elif", "None", "True", "False"
};

bool isKeyword(const string& w) {
    for (size_t i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); i++)
        if (w == KEYWORDS[i]) return true;
    return false;
}

class CleanedCodeList {
private:
    struct Node {
        char data;
        int line;
        Node* next;
    };
    Node* head;
    Node* tail;

public:
    CleanedCodeList() : head(NULL), tail(NULL) {}

    void append(char ch, int lineNo) {
        Node* newNode = new Node();
        newNode->data = ch;
        newNode->line = lineNo;
        newNode->next = NULL;
        if (head == NULL) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
    }

    void copyTo(string& cleaned, vector<int>& lineMap) {
        Node* current = head;
        while (current != NULL) {
            cleaned += current->data;
            lineMap.push_back(current->line);
            current = current->next;
        }
    }

    ~CleanedCodeList() {
        Node* current = head;
        while (current != NULL) {
            Node* nextNode = current->next;
            delete current;
            current = nextNode;
        }
        head = tail = NULL;
    }
};

void cleanCodeMapped(const string& src, bool normalize, string& cleaned, vector<int>& lineMap) {
    cleaned.clear();
    lineMap.clear();
    int line = 1;
    bool inLine = false, inBlock = false, inStr = false, inChr = false;
    bool lineStart = true;
    size_t n = src.length();

    CleanedCodeList result;
    auto emit = [&](char c) { result.append(c, line); };

    for (size_t i = 0; i < n; i++) {
        char ch = src[i];
        if (ch == '\r') continue;
        if (ch == '\n') { line++; inLine = false; lineStart = true; continue; }
        if (inLine) continue;
        if (inBlock) {
            if (ch == '*' && i + 1 < n && src[i + 1] == '/') { inBlock = false; i++; }
            continue;
        }
        if (inStr || inChr) {
            char quote = inStr ? '"' : '\'';
            if (ch == '\\' && i + 1 < n) {
                if (!normalize) { emit('\\'); emit(toLowerChar(src[i + 1])); }
                if (src[i + 1] == '\n') line++;
                i++;
                continue;
            }
            if (ch == quote) { inStr = inChr = false; emit(quote); continue; }
            if (!normalize && ch != ' ' && ch != '\t') emit(toLowerChar(ch));
            continue;
        }
        if (ch == '/' && i + 1 < n && src[i + 1] == '/') { inLine = true; i++; continue; }
        if (ch == '/' && i + 1 < n && src[i + 1] == '*') { inBlock = true; i++; continue; }
        if (ch == '"')  { inStr = true; emit('"'); if (normalize) emit('s'); continue; }
        if (ch == '\'') { inChr = true; emit('\''); if (normalize) emit('s'); continue; }
        if (ch == ' ' || ch == '\t') continue;

        bool atStart = lineStart;
        lineStart = false;
        if (ch == '#' && atStart) {
            while (i + 1 < n && src[i + 1] != '\n') i++;
            continue;
        }

        if (isIdentStart(ch)) {
            size_t j = i;
            while (j < n && (isAlnumChar(src[j]) || src[j] == '_')) j++;
            string word = src.substr(i, j - i);
            i = j - 1;
            if (word == "using") {
                while (i + 1 < n && src[i + 1] != ';') {
                    i++;
                    if (src[i] == '\n') line++;
                }
                if (i + 1 < n) i++;
                continue;
            }
            if (normalize && !isKeyword(word)) emit('v');
            else for (size_t c = 0; c < word.length(); c++) emit(toLowerChar(word[c]));
            continue;
        }
        if (normalize && ch >= '0' && ch <= '9') {
            size_t j = i;
            while (j < n && (isAlnumChar(src[j]) || src[j] == '.')) j++;
            i = j - 1;
            emit('n');
            continue;
        }
        emit(toLowerChar(ch));
    }

    result.copyTo(cleaned, lineMap);
}

Prepared prepare(const string& name, const string& raw, bool normalize) {
    Prepared p;
    p.name = name;
    p.raw = raw;
    cleanCodeMapped(raw, normalize, p.cleaned, p.lineMap);
    return p;
}

void replaceWord(string& text, const string& oldWord, const string& newWord) {
    if (oldWord.empty()) {
        return;
    }
    size_t pos = 0;
    int count = 0;
    while ((pos = text.find(oldWord, pos)) != string::npos) {
        bool leftBoundary = (pos == 0) ||
                            (!isAlnumChar(text[pos - 1]) && text[pos - 1] != '_');
        bool rightBoundary = (pos + oldWord.length() >= text.length()) ||
                             (!isAlnumChar(text[pos + oldWord.length()]) && text[pos + oldWord.length()] != '_');

        if (leftBoundary && rightBoundary) {
            text.replace(pos, oldWord.length(), newWord);
            pos += newWord.length();
            count++;
        } else {
            pos += oldWord.length();
        }
    }
    cout << "[Renamer] Replaced " << count << " occurrence(s) of '" << oldWord << "' with '" << newWord << "'.\n";
}

bool extractFunction(const string& code, const string& funcName, string& output) {
    size_t searchPos = 0;
    size_t funcPos = string::npos;
    size_t braceStart = string::npos;

    while ((searchPos = code.find(funcName, searchPos)) != string::npos) {
        bool leftBoundary = (searchPos == 0) ||
                            (!isAlnumChar(code[searchPos - 1]) && code[searchPos - 1] != '_');
        bool rightBoundary = (searchPos + funcName.length() >= code.length()) ||
                             (!isAlnumChar(code[searchPos + funcName.length()]) && code[searchPos + funcName.length()] != '_');

        if (leftBoundary && rightBoundary) {
            size_t afterName = searchPos + funcName.length();
            while (afterName < code.length() && (code[afterName] == ' ' || code[afterName] == '\t' ||
                                                 code[afterName] == '\n' || code[afterName] == '\r')) {
                afterName++;
            }
            if (afterName < code.length() && code[afterName] == '(') {
                size_t closeParen = code.find(')', afterName);
                if (closeParen != string::npos) {
                    size_t candidateBrace = code.find('{', closeParen);
                    size_t candidateSemi = code.find(';', closeParen);
                    if (candidateBrace != string::npos && (candidateSemi == string::npos || candidateBrace < candidateSemi)) {
                        funcPos = searchPos;
                        braceStart = candidateBrace;
                        break;
                    }
                }
            }
        }
        searchPos += funcName.length();
    }

    if (funcPos == string::npos || braceStart == string::npos) {
        cout << "[Error] Function definition for '" << funcName << "' not found.\n";
        return false;
    }

    int braceCount = 0;
    output.clear();
    bool inLineComment = false;
    bool inBlockComment = false;
    bool inQuotes = false;
    bool inChar = false;

    for (size_t i = braceStart; i < code.length(); i++) {
        char ch = code[i];

        if (inLineComment) {
            if (ch == '\n') inLineComment = false;
            output += ch;
            continue;
        }

        if (inBlockComment) {
            if (ch == '*' && i + 1 < code.length() && code[i + 1] == '/') {
                inBlockComment = false;
                output += ch;
                output += code[i + 1];
                i++;
                continue;
            }
            output += ch;
            continue;
        }

        if (inQuotes) {
            if (ch == '\\' && i + 1 < code.length()) {
                output += ch;
                output += code[i + 1];
                i++;
                continue;
            }
            if (ch == '"') inQuotes = false;
            output += ch;
            continue;
        }

        if (inChar) {
            if (ch == '\\' && i + 1 < code.length()) {
                output += ch;
                output += code[i + 1];
                i++;
                continue;
            }
            if (ch == '\'') inChar = false;
            output += ch;
            continue;
        }

        if (ch == '/' && i + 1 < code.length() && code[i + 1] == '/') {
            inLineComment = true;
            output += ch;
            output += code[i + 1];
            i++;
            continue;
        }
        if (ch == '/' && i + 1 < code.length() && code[i + 1] == '*') {
            inBlockComment = true;
            output += ch;
            output += code[i + 1];
            i++;
            continue;
        }

        if (ch == '"') {
            inQuotes = true;
            output += ch;
            continue;
        }
        if (ch == '\'') {
            inChar = true;
            output += ch;
            continue;
        }

        if (ch == '{') {
            braceCount++;
        } else if (ch == '}') {
            braceCount--;
        }
        output += ch;

        if (braceCount == 0) {
            cout << "[Extractor] Function '" << funcName << "' extracted (" << output.length() << " characters).\n";
            return true;
        }
    }

    cout << "[Error] Unclosed function: Mismatched braces encountered.\n";
    return false;
}

long long getHighestPower(int k) {
    long long power = 1;
    for (int i = 0; i < k - 1; i++) {
        power = (power * BASE) % MOD;
    }
    return power;
}

long long getInitialHash(const string& text, int k) {
    long long hashVal = 0;
    for (int i = 0; i < k; i++) {
        hashVal = (hashVal * BASE + (unsigned char)text[i]) % MOD;
    }
    return hashVal;
}

long long rollHash(long long oldHash, char oldChar, char newChar, long long power) {
    long long outgoingVal = ((long long)(unsigned char)oldChar * power) % MOD;
    long long remaining = (oldHash - outgoingVal) % MOD;
    if (remaining < 0) {
        remaining += MOD;
    }
    long long newHash = (remaining * BASE + (unsigned char)newChar) % MOD;
    return newHash;
}

float calculateSimilarityScore(int matches, int totalKgrams) {
    if (totalKgrams <= 0) {
        return 0.0f;
    }
    return ((float)matches / (float)totalKgrams) * 100.0f;
}

string getRiskAssessment(float score) {
    if (score < 15.0f) {
        return "Low Risk : The programs are not similar!";
    } else if (score <= 40.0f) {
        return "Moderate Risk : Possible copy";
    } else {
        return "High Risk : The programs are very similar!";
    }
}

struct Block {
    int length = 0;
    int posA = 0, posB = 0;
    int lineA1 = 0, lineA2 = 0, lineB1 = 0, lineB2 = 0;
};

bool commonOfLength(const string& a, const string& b, int L, int& pa, int& pb) {
    int la = (int)a.length(), lb = (int)b.length();
    if (L <= 0 || L > la || L > lb) return false;
    long long power = getHighestPower(L);
    unordered_map<long long, vector<int> > table;
    table.reserve((size_t)(la - L + 1));
    long long h = getInitialHash(a, L);
    table[h].push_back(0);
    for (int i = 1; i + L <= la; i++) {
        h = rollHash(h, a[i - 1], a[i + L - 1], power);
        table[h].push_back(i);
    }
    long long g = getInitialHash(b, L);
    for (int j = 0; j + L <= lb; j++) {
        if (j > 0) g = rollHash(g, b[j - 1], b[j + L - 1], power);
        unordered_map<long long, vector<int> >::iterator it = table.find(g);
        if (it == table.end()) continue;
        for (size_t q = 0; q < it->second.size(); q++) {
            int p = it->second[q];
            if (a.compare((size_t)p, (size_t)L, b, (size_t)j, (size_t)L) == 0) {
                pa = p;
                pb = j;
                return true;
            }
        }
    }
    return false;
}

Block findLongestBlock(const Prepared& A, const Prepared& B, int minLen) {
    Block best;
    const string& a = A.cleaned;
    const string& b = B.cleaned;
    int lo = 0, hi = (int)min(a.length(), b.length());
    while (lo < hi) {
        int mid = (lo + hi + 1) / 2;
        int pa = -1, pb = -1;
        if (commonOfLength(a, b, mid, pa, pb)) {
            lo = mid;
            best.length = mid;
            best.posA = pa;
            best.posB = pb;
        } else {
            hi = mid - 1;
        }
    }
    if (best.length < minLen) return Block();
    best.lineA1 = A.lineMap[best.posA];
    best.lineA2 = A.lineMap[best.posA + best.length - 1];
    best.lineB1 = B.lineMap[best.posB];
    best.lineB2 = B.lineMap[best.posB + best.length - 1];
    return best;
}

struct HashBucketItem {
    long long hashVal;
    vector<string> strings;
};

class SimpleHashTable {
private:
    static const int BUCKET_COUNT = 10007;
    vector<vector<HashBucketItem> > buckets;

    int getBucketIndex(long long hashVal) const {
        long long index = hashVal % BUCKET_COUNT;
        if (index < 0) {
            index += BUCKET_COUNT;
        }
        return (int)index;
    }

public:
    SimpleHashTable() : buckets(BUCKET_COUNT) {}

    void insert(long long hashVal, const string& chunk) {
        int idx = getBucketIndex(hashVal);
        for (size_t i = 0; i < buckets[idx].size(); i++) {
            if (buckets[idx][i].hashVal == hashVal) {
                for (size_t s = 0; s < buckets[idx][i].strings.size(); s++) {
                    if (buckets[idx][i].strings[s] == chunk) {
                        return;
                    }
                }
                buckets[idx][i].strings.push_back(chunk);
                return;
            }
        }
        HashBucketItem newItem;
        newItem.hashVal = hashVal;
        newItem.strings.push_back(chunk);
        buckets[idx].push_back(newItem);
    }

    const HashBucketItem* find(long long hashVal) const {
        int idx = getBucketIndex(hashVal);
        for (size_t i = 0; i < buckets[idx].size(); i++) {
            if (buckets[idx][i].hashVal == hashVal) {
                return &buckets[idx][i];
            }
        }
        return NULL;
    }
};

struct CompareResult {
    float score;
    int totalKgrams;
    int matches;
    int collisions;
    string status;
};

CompareResult compareCodes(const string& code1, const string& code2, int k) {
    CompareResult res;
    res.score = 0.0f;
    res.totalKgrams = 0;
    res.matches = 0;
    res.collisions = 0;
    res.status = getRiskAssessment(0.0f);

    int len1 = (int)code1.length();
    int len2 = (int)code2.length();
    if (len1 < k || len2 < k) {
        cout << "[Warning] Text length is smaller than window size (k = " << k << ").\n";
        return res;
    }

    long long power = getHighestPower(k);
    int count1 = len1 - k + 1;
    SimpleHashTable hashTable1;

    long long hash1 = getInitialHash(code1, k);
    hashTable1.insert(hash1, code1.substr(0, k));

    for (int i = 1; i < count1; i++) {
        hash1 = rollHash(hash1, code1[i - 1], code1[i + k - 1], power);
        hashTable1.insert(hash1, code1.substr(i, k));
    }

    int count2 = len2 - k + 1;
    res.totalKgrams = count2;
    long long hash2 = getInitialHash(code2, k);

    for (int j = 0; j < count2; j++) {
        if (j > 0) {
            hash2 = rollHash(hash2, code2[j - 1], code2[j + k - 1], power);
        }
        string currentChunk = code2.substr(j, k);
        const HashBucketItem* matchItem = hashTable1.find(hash2);
        if (matchItem != NULL) {
            bool exactMatchFound = false;
            for (size_t m = 0; m < matchItem->strings.size(); m++) {
                if (matchItem->strings[m] == currentChunk) {
                    exactMatchFound = true;
                    break;
                }
            }
            if (exactMatchFound) {
                res.matches++;
            } else {
                res.collisions++;
            }
        }
    }

    res.score = calculateSimilarityScore(res.matches, res.totalKgrams);
    res.status = getRiskAssessment(res.score);
    return res;
}

struct PairResult {
    int a = 0, b = 0;
    float simAinB = 0.0f;
    float simBinA = 0.0f;
    float score = 0.0f;
    int collisions = 0;
    int totalA = 0, totalB = 0;
    Block block;
    string status;
};

string riskLabel(float score) {
    if (score < 15.0f) return "LOW";
    if (score <= 40.0f) return "MODERATE";
    return "HIGH";
}

string riskText(float score) {
    if (score < 15.0f) return "Minimal Overlap (Normal syntax/boilerplate)";
    if (score <= 40.0f) return "Moderate Similarity (Possible logic reuse)";
    return "High-Risk Match (Strong evidence of plagiarism)";
}

void buildTable(const string& s, int k, SimpleHashTable& t) {
    int len = (int)s.length();
    if (len < k) return;
    long long power = getHighestPower(k);
    long long h = getInitialHash(s, k);
    t.insert(h, s.substr(0, k));
    for (int i = 1; i + k <= len; i++) {
        h = rollHash(h, s[i - 1], s[i + k - 1], power);
        t.insert(h, s.substr(i, k));
    }
}

int countAgainst(const SimpleHashTable& t, const string& s, int k, int& matches, int& collisions) {
    matches = 0;
    collisions = 0;
    int len = (int)s.length();
    if (len < k) return 0;
    long long power = getHighestPower(k);
    long long h = getInitialHash(s, k);
    int total = len - k + 1;
    for (int j = 0; j < total; j++) {
        if (j > 0) h = rollHash(h, s[j - 1], s[j + k - 1], power);
        const HashBucketItem* item = t.find(h);
        if (item == NULL) continue;
        string chunk = s.substr(j, k);
        bool exact = false;
        for (size_t m = 0; m < item->strings.size(); m++)
            if (item->strings[m] == chunk) { exact = true; break; }
        if (exact) matches++;
        else collisions++;
    }
    return total;
}

PairResult comparePair(const Prepared& A, const Prepared& B, int ia, int ib, int k,
                       const SimpleHashTable& tA, const SimpleHashTable& tB) {
    PairResult r;
    r.a = ia;
    r.b = ib;
    r.status = riskText(0.0f);
    if ((int)A.cleaned.length() < k || (int)B.cleaned.length() < k) return r;

    int mB, cB, mA, cA;
    r.totalB = countAgainst(tA, B.cleaned, k, mB, cB);
    r.totalA = countAgainst(tB, A.cleaned, k, mA, cA);
    r.simBinA = r.totalB > 0 ? 100.0f * (float)mB / (float)r.totalB : 0.0f;
    r.simAinB = r.totalA > 0 ? 100.0f * (float)mA / (float)r.totalA : 0.0f;
    r.score = max(r.simAinB, r.simBinA);
    r.collisions = cA + cB;
    r.block = findLongestBlock(A, B, k);
    r.status = riskText(r.score);
    return r;
}

string lineRange(int l1, int l2) {
    if (l1 == 0) return "-";
    if (l1 == l2) return "L" + to_string(l1);
    return "L" + to_string(l1) + "-" + to_string(l2);
}

void printPairDetail(const Prepared& A, const Prepared& B, const PairResult& r) {
    cout << "\n------------------- Evaluation Result -------------------\n";
    cout << " File A                  : " << A.name << "\n";
    cout << " File B                  : " << B.name << "\n";
    cout << " A's k-grams found in B  : " << fixed << setprecision(2) << r.simAinB << "%\n";
    cout << " B's k-grams found in A  : " << fixed << setprecision(2) << r.simBinA << "%\n";
    cout << " Similarity (max of both): " << fixed << setprecision(2) << r.score << "%\n";
    cout << " Hash collisions         : " << r.collisions << "\n";
    cout << " Risk assessment         : " << r.status << "\n";
    cout << " Longest matching block  : " << r.block.length << " cleaned chars";
    if (r.block.length > 0)
        cout << "  (A " << lineRange(r.block.lineA1, r.block.lineA2)
             << " <-> B " << lineRange(r.block.lineB1, r.block.lineB2) << ")";
    cout << "\n";
    if (r.block.length > 0) {
        vector<string> la = splitLines(A.raw);
        cout << " Matched region in A:\n";
        int shown = 0;
        for (int ln = r.block.lineA1; ln <= r.block.lineA2 && ln <= (int)la.size(); ln++) {
            if (shown++ >= 8) { cout << "     ...\n"; break; }
            cout << "   " << setw(3) << right << ln << " | " << la[ln - 1] << "\n";
        }
    }
    cout << "---------------------------------------------------------\n";
}

void printMatrix(const vector<Prepared>& f, const vector<PairResult>& res) {
    int n = (int)f.size();
    if (n > 12) {
        cout << "\n[Info] Matrix skipped (more than 12 files). See the ranked list below.\n";
        return;
    }
    vector<vector<float> > m(n, vector<float>(n, -1.0f));
    for (size_t i = 0; i < res.size(); i++) {
        m[res[i].a][res[i].b] = res[i].score;
        m[res[i].b][res[i].a] = res[i].score;
    }
    cout << "\n==================== PAIRWISE SIMILARITY MATRIX (%) ====================\n";
    for (int i = 0; i < n; i++) cout << " F" << (i + 1) << " = " << f[i].name << "\n";
    cout << "\n       ";
    for (int j = 0; j < n; j++) cout << setw(8) << ("F" + to_string(j + 1));
    cout << "\n";
    for (int i = 0; i < n; i++) {
        cout << "  " << setw(4) << left << ("F" + to_string(i + 1)) << right;
        for (int j = 0; j < n; j++) {
            string cell;
            if (i == j) cell = "--";
            else {
                ostringstream oss;
                oss << fixed << setprecision(1) << m[i][j];
                cell = oss.str();
                if (m[i][j] > 40.0f) cell += "*";
            }
            cout << setw(8) << cell;
        }
        cout << "\n";
    }
    cout << "  (* = high risk, above 40%)\n";
}

bool byScoreDesc(const PairResult& x, const PairResult& y) {
    return x.score > y.score;
}

void printRanking(const vector<Prepared>& f, vector<PairResult> res) {
    sort(res.begin(), res.end(), byScoreDesc);
    cout << "\n=================== RANKED SUSPICIOUS PAIRS ===================\n";
    cout << setw(3) << "#" << "  " << setw(36) << left << "Pair" << right
         << setw(9) << "Sim %" << setw(8) << "Block" << "  " << setw(18) << left << "Lines (A / B)"
         << "Risk\n";
    cout << string(86, '-') << "\n";
    for (size_t i = 0; i < res.size(); i++) {
        const PairResult& r = res[i];
        string pair = trimTo(f[r.a].name, 16) + " <> " + trimTo(f[r.b].name, 16);
        string lines = lineRange(r.block.lineA1, r.block.lineA2) + " / " + lineRange(r.block.lineB1, r.block.lineB2);
        cout << setw(3) << right << (i + 1) << "  " << setw(36) << left << pair << right
             << setw(9) << fixed << setprecision(2) << r.score
             << setw(8) << r.block.length << "  " << setw(18) << left << lines << riskLabel(r.score) << "\n";
    }
    cout << string(86, '-') << "\n";

    int high = 0, mod = 0;
    float sum = 0.0f, mx = 0.0f;
    for (size_t i = 0; i < res.size(); i++) {
        if (res[i].score > 40.0f) high++;
        else if (res[i].score >= 15.0f) mod++;
        sum += res[i].score;
        if (res[i].score > mx) mx = res[i].score;
    }
    cout << " Pairs compared: " << res.size() << " | HIGH: " << high << " | MODERATE: " << mod
         << " | Average: " << fixed << setprecision(2) << (res.empty() ? 0.0f : sum / (float)res.size())
         << "% | Max: " << mx << "%\n";
}

bool checkBrackets(const string& text) {
    stack<char> bracketStack;
    int line = 1;
    int col = 0;
    bool inLineComment = false;
    bool inBlockComment = false;
    bool inQuotes = false;
    bool inChar = false;

    for (size_t i = 0; i < text.length(); i++) {
        char ch = text[i];
        col++;

        if (ch == '\n') {
            line++;
            col = 0;
            inLineComment = false;
            continue;
        }

        if (inLineComment) continue;

        if (inBlockComment) {
            if (ch == '*' && i + 1 < text.length() && text[i + 1] == '/') {
                inBlockComment = false;
                i++;
                col++;
            }
            continue;
        }

        if (inQuotes) {
            if (ch == '\\' && i + 1 < text.length()) {
                i++;
                col++;
                continue;
            }
            if (ch == '"') inQuotes = false;
            continue;
        }

        if (inChar) {
            if (ch == '\\' && i + 1 < text.length()) {
                i++;
                col++;
                continue;
            }
            if (ch == '\'') inChar = false;
            continue;
        }

        if (ch == '/' && i + 1 < text.length() && text[i + 1] == '/') {
            inLineComment = true;
            i++;
            col++;
            continue;
        }
        if (ch == '/' && i + 1 < text.length() && text[i + 1] == '*') {
            inBlockComment = true;
            i++;
            col++;
            continue;
        }

        if (ch == '"') { inQuotes = true; continue; }
        if (ch == '\'') { inChar = true; continue; }

        if (ch == '(' || ch == '{' || ch == '[') {
            bracketStack.push(ch);
        } else if (ch == ')' || ch == '}' || ch == ']') {
            if (bracketStack.empty()) {
                cout << "[Syntax Error] Extra closing '" << ch << "' at Line " << line << ", Col " << col << "\n";
                return false;
            }
            char topBracket = bracketStack.top();
            bracketStack.pop();
            if ((ch == ')' && topBracket != '(') ||
                (ch == '}' && topBracket != '{') ||
                (ch == ']' && topBracket != '[')) {
                cout << "[Syntax Error] Mismatched bracket: expected match for '"
                     << topBracket << "', but found '" << ch
                     << "' at Line " << line << ", Col " << col << "\n";
                return false;
            }
        }
    }

    if (!bracketStack.empty()) {
        cout << "[Syntax Error] Unclosed bracket remaining on stack: '" << bracketStack.top() << "'\n";
        return false;
    }

    cout << "[Success] All brackets are balanced and correct!\n";
    return true;
}

void runEditor(string& fileContent) {
    stack<string> undoStack;
    stack<string> redoStack;
    string currentLine = "";

    for (size_t i = 0; i < fileContent.length(); i++) {
        char ch = fileContent[i];
        if (ch == '\r') continue;
        if (ch == '\n') {
            undoStack.push(currentLine);
            currentLine.clear();
        } else {
            currentLine += ch;
        }
    }
    if (!currentLine.empty()) {
        undoStack.push(currentLine);
    }

    cout << "\n=== Standard Stack-Based Line Editor ===\n";
    cout << "Commands:\n";
    cout << "  :a --> Add a line\n";
    cout << "  :u --> Undo last action\n";
    cout << "  :r --> Redo last action\n";
    cout << "  :p --> Print all lines\n";
    cout << "  :s --> Save and return to menu\n";

    string input;
    while (true) {
        cout << "editor> ";
        if (!getline(cin, input)) break;

        if (input.length() >= 3 && input.substr(0, 3) == ":a ") {
            string newLine = input.substr(3);
            undoStack.push(newLine);
            while (!redoStack.empty()) redoStack.pop();
            cout << "Line added.\n";
        } else if (input == ":u") {
            if (!undoStack.empty()) {
                string removedLine = undoStack.top();
                undoStack.pop();
                redoStack.push(removedLine);
                cout << "Undo successful: Removed \"" << removedLine << "\"\n";
            } else {
                cout << "Nothing to undo.\n";
            }
        } else if (input == ":r") {
            if (!redoStack.empty()) {
                string restoredLine = redoStack.top();
                redoStack.pop();
                undoStack.push(restoredLine);
                cout << "Redo successful: Restored \"" << restoredLine << "\"\n";
            } else {
                cout << "Nothing to redo.\n";
            }
        } else if (input == ":p") {
            stack<string> tempStack = undoStack;
            vector<string> reversedLines;
            while (!tempStack.empty()) {
                reversedLines.push_back(tempStack.top());
                tempStack.pop();
            }
            int lineCount = (int)reversedLines.size();
            cout << "\n--- Current Buffer Content (" << lineCount << " lines) ---\n";
            for (int i = lineCount - 1; i >= 0; i--) {
                int lineNumber = lineCount - i;
                cout << setw(2) << lineNumber << " | " << reversedLines[i] << "\n";
            }
            cout << "-------------------------------------------\n";
        } else if (input == ":s") {
            stack<string> tempStack = undoStack;
            vector<string> reversedLines;
            while (!tempStack.empty()) {
                reversedLines.push_back(tempStack.top());
                tempStack.pop();
            }
            fileContent.clear();
            for (int i = (int)reversedLines.size() - 1; i >= 0; i--) {
                fileContent += reversedLines[i] + "\n";
            }
            cout << "Saved changes to active memory.\n";
            break;
        } else {
            cout << "Invalid command. Options: :a <text>, :u, :r, :p, :s\n";
        }
    }
}

void loadSamples(string& code1, string& code2) {
    code1 =
        "#include <iostream>\n"
        "int calculateSum(int limit) {\n"
        "    int total = 0;\n"
        "    for (int i = 1; i <= limit; i++) {\n"
        "        total += i;\n"
        "    }\n"
        "    return total;\n"
        "}\n"
        "int main() {\n"
        "    std::cout << calculateSum(10);\n"
        "    return 0;\n"
        "}\n";

    code2 =
        "#include <iostream>\n"
        "int calculateSum(int limit) {\n"
        "    int sum = 0;\n"
        "    for (int idx = 1; idx <= limit; idx++) {\n"
        "        sum += idx;\n"
        "    }\n"
        "    return sum;\n"
        "}\n"
        "int main() {\n"
        "    std::cout << calculateSum(10);\n"
        "    return 0;\n"
        "}\n";
}

struct TestCase {
    string title;
    string description;
    string a, b;
    string expected;
};

vector<Prepared> lastFiles;
vector<PairResult> lastResults;

bool ask(const string& msg, string& out) {
    cout << msg;
    if (!getline(cin, out)) return false;
    return true;
}

string sampleOriginal() {
    return R"CPP(#include <iostream>
using namespace std;

int calculateSum(int limit) {
    int total = 0;
    for (int i = 1; i <= limit; i++) {
        total += i;
    }
    return total;
}

int findMax(int arr[], int n) {
    int best = arr[0];
    for (int i = 1; i < n; i++) {
        if (arr[i] > best) {
            best = arr[i];
        }
    }
    return best;
}

int main() {
    int data[5] = {4, 9, 2, 7, 5};
    cout << calculateSum(10) << endl;
    cout << findMax(data, 5) << endl;
    return 0;
}
)CPP";
}

string sampleRenamed() {
    return R"CPP(#include <iostream>
using namespace std;

int addUpTo(int n) {
    int result = 0;
    for (int idx = 1; idx <= n; idx++) {
        result += idx;
    }
    return result;
}

int biggest(int values[], int size) {
    int top = values[0];
    for (int p = 1; p < size; p++) {
        if (values[p] > top) {
            top = values[p];
        }
    }
    return top;
}

int main() {
    int numbers[5] = {4, 9, 2, 7, 5};
    cout << addUpTo(10) << endl;
    cout << biggest(numbers, 5) << endl;
    return 0;
}
)CPP";
}

string sampleReordered() {
    return R"CPP(#include <iostream>
using namespace std;

int findMax(int arr[], int n) {
    int best = arr[0];
    for (int i = 1; i < n; i++) {
        if (arr[i] > best) {
            best = arr[i];
        }
    }
    return best;
}

int calculateSum(int limit) {
    int total = 0;
    for (int i = 1; i <= limit; i++) {
        total += i;
    }
    return total;
}

int main() {
    int data[5] = {4, 9, 2, 7, 5};
    cout << findMax(data, 5) << endl;
    cout << calculateSum(10) << endl;
    return 0;
}
)CPP";
}

string samplePartial() {
    return R"CPP(#include <iostream>
#include <string>
using namespace std;

bool isPrime(int num) {
    if (num < 2) return false;
    for (int d = 2; d * d <= num; d++) {
        if (num % d == 0) return false;
    }
    return true;
}

int calculateSum(int limit) {
    int total = 0;
    for (int i = 1; i <= limit; i++) {
        total += i;
    }
    return total;
}

string shout(string word) {
    string out = "";
    for (size_t q = 0; q < word.size(); q++) {
        out += (char)toupper(word[q]);
    }
    return out + "!";
}

int main() {
    cout << isPrime(29) << endl;
    cout << calculateSum(100) << endl;
    cout << shout("hello") << endl;
    return 0;
}
)CPP";
}

string sampleUnrelated() {
    return R"CPP(#include <iostream>
#include <string>
using namespace std;

long long factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

bool isPalindrome(string s) {
    int left = 0, right = (int)s.size() - 1;
    while (left < right) {
        if (s[left++] != s[right--]) return false;
    }
    return true;
}

int main() {
    string word;
    cin >> word;
    cout << (isPalindrome(word) ? "yes" : "no") << endl;
    cout << factorial(10) << endl;
    return 0;
}
)CPP";
}

bool hasCodeExtension(const fs::path& p) {
    string ext = p.extension().string();
    for (size_t i = 0; i < ext.length(); i++) ext[i] = toLowerChar(ext[i]);
    return ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx" ||
           ext == ".h" || ext == ".hpp" || ext == ".java";
}

bool loadFolder(const string& folder, bool normalize, vector<Prepared>& files) {
    error_code ec;
    if (!fs::is_directory(folder, ec)) {
        cout << "[Error] '" << folder << "' is not a folder.\n";
        return false;
    }
    vector<string> paths;
    for (fs::recursive_directory_iterator it(folder, ec), end; !ec && it != end; it.increment(ec))
        if (it->is_regular_file() && hasCodeExtension(it->path()))
            paths.push_back(it->path().string());
    sort(paths.begin(), paths.end());

    files.clear();
    for (size_t i = 0; i < paths.size(); i++) {
        string content;
        if (!readFile(paths[i], content)) {
            cout << "[Warning] Could not read " << paths[i] << ", skipping.\n";
            continue;
        }
        string rel = fs::relative(fs::path(paths[i]), fs::path(folder), ec).generic_string();
        files.push_back(prepare(rel, content, normalize));
    }
    return true;
}

void scanFolder(const string& folder, int k, bool normalize) {
    vector<Prepared> files;
    if (!loadFolder(folder, normalize, files)) return;
    int n = (int)files.size();
    cout << "[Info] Loaded " << n << " source file(s) from '" << folder << "'.\n";
    if (n < 2) {
        cout << "[Error] Need at least 2 files for a pairwise comparison.\n";
        return;
    }

    vector<SimpleHashTable> tables(n);
    for (int i = 0; i < n; i++) buildTable(files[i].cleaned, k, tables[i]);

    vector<PairResult> results;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            PairResult r = comparePair(files[i], files[j], i, j, k, tables[i], tables[j]);
            results.push_back(r);
            addHistory(files[i].name + " vs " + files[j].name, k, r.score, r.collisions, r.block.length);
        }
    }

    lastFiles = files;
    lastResults = results;
    printMatrix(files, results);
    printRanking(files, results);
}

void exportCsv(const string& path) {
    if (lastResults.empty()) {
        cout << "[Info] Nothing to export yet. Run a folder scan first (Option 11).\n";
        return;
    }
    ofstream out(path.c_str());
    if (!out.is_open()) {
        cout << "[Error] Cannot write " << path << "\n";
        return;
    }
    out << "FileA,FileB,A_in_B_pct,B_in_A_pct,Similarity_pct,LongestBlockChars,LinesA,LinesB,Risk\n";
    for (size_t i = 0; i < lastResults.size(); i++) {
        const PairResult& r = lastResults[i];
        out << lastFiles[r.a].name << "," << lastFiles[r.b].name << ","
            << fixed << setprecision(2) << r.simAinB << "," << r.simBinA << "," << r.score << ","
            << r.block.length << "," << lineRange(r.block.lineA1, r.block.lineA2) << ","
            << lineRange(r.block.lineB1, r.block.lineB2) << "," << riskLabel(r.score) << "\n";
    }
    cout << "[Success] Exported " << lastResults.size() << " rows to " << path << "\n";
}

void createDemoFolder() {
    error_code ec;
    fs::create_directories(DEMO_FOLDER, ec);
    bool ok = writeFile(DEMO_FOLDER + "/student_A.cpp", sampleOriginal()) &&
              writeFile(DEMO_FOLDER + "/student_B.cpp", sampleRenamed()) &&
              writeFile(DEMO_FOLDER + "/student_C.cpp", sampleReordered()) &&
              writeFile(DEMO_FOLDER + "/student_D.cpp", sampleUnrelated()) &&
              writeFile(DEMO_FOLDER + "/student_E.cpp", samplePartial());
    if (!ok) { cout << "[Error] Could not create demo files.\n"; return; }
    cout << "[Success] Created folder '" << DEMO_FOLDER << "' with 5 sample submissions:\n"
         << "   student_A.cpp  original solution\n"
         << "   student_B.cpp  copy of A with renamed identifiers and new comments\n"
         << "   student_C.cpp  copy of A with the functions reordered\n"
         << "   student_D.cpp  completely different program\n"
         << "   student_E.cpp  partial copy (calculateSum copied, rest different)\n"
         << "Now use Option 11 to scan it.\n";
}

void runTestCases(int k, bool normalize) {
    vector<TestCase> cases;
    TestCase t;
    t.title = "Case 1: Identical copy";
    t.description = "Same file submitted twice";
    t.a = sampleOriginal(); t.b = sampleOriginal(); t.expected = "HIGH";
    cases.push_back(t);
    t.title = "Case 2: Renamed variables";
    t.description = "Identifiers renamed, comments added";
    t.a = sampleOriginal(); t.b = sampleRenamed(); t.expected = normalize ? "HIGH" : "MODERATE";
    cases.push_back(t);
    t.title = "Case 3: Reordered functions";
    t.description = "Same functions, different order";
    t.a = sampleOriginal(); t.b = sampleReordered(); t.expected = "HIGH";
    cases.push_back(t);
    t.title = "Case 4: Half copied";
    t.description = "1 of A's 2 functions copied";
    t.a = sampleOriginal(); t.b = samplePartial(); t.expected = "HIGH";
    cases.push_back(t);
    t.title = "Case 5: Similar style only";
    t.description = "Same loop style, different logic";
    t.a = samplePartial(); t.b = sampleUnrelated(); t.expected = "MODERATE";
    cases.push_back(t);
    t.title = "Case 6: Unrelated programs";
    t.description = "Different logic entirely";
    t.a = sampleOriginal(); t.b = sampleUnrelated(); t.expected = "LOW";
    cases.push_back(t);

    cout << "\n================== BUILT-IN TEST CASES (k = " << k << ", identifier normalization "
         << (normalize ? "ON" : "OFF") << ") ==================\n";
    cout << setw(30) << left << "Case" << setw(38) << "What happens"
         << right << setw(8) << "Sim %" << setw(8) << "Block" << "  " << left << setw(10) << "Expected"
         << setw(10) << "Got" << "Result\n";
    cout << string(112, '-') << "\n";

    int passed = 0;
    vector<PairResult> details;
    for (size_t i = 0; i < cases.size(); i++) {
        Prepared A = prepare("file_A", cases[i].a, normalize);
        Prepared B = prepare("file_B", cases[i].b, normalize);
        SimpleHashTable tA, tB;
        buildTable(A.cleaned, k, tA);
        buildTable(B.cleaned, k, tB);
        PairResult r = comparePair(A, B, 0, 1, k, tA, tB);
        string got = riskLabel(r.score);
        bool ok = (got == cases[i].expected);
        if (ok) passed++;
        addHistory(cases[i].title, k, r.score, r.collisions, r.block.length);
        cout << setw(30) << left << cases[i].title << setw(38) << cases[i].description
             << right << setw(8) << fixed << setprecision(2) << r.score << setw(8) << r.block.length
             << "  " << left << setw(10) << cases[i].expected << setw(10) << got
             << (ok ? "PASS" : "FAIL") << "\n";
        details.push_back(r);
    }
    cout << string(112, '-') << "\n";
    cout << " " << passed << " of " << cases.size() << " cases passed.\n";

    cout << "\n--- Longest matching block per case ---\n";
    for (size_t i = 0; i < cases.size(); i++) {
        cout << " " << setw(30) << left << cases[i].title << right << ": "
             << setw(4) << details[i].block.length << " chars";
        if (details[i].block.length > 0)
            cout << "  A " << lineRange(details[i].block.lineA1, details[i].block.lineA2)
                 << " <-> B " << lineRange(details[i].block.lineB1, details[i].block.lineB2);
        cout << "\n";
    }
}

bool loadSourceByName(const string& path, string& content, string& displayName) {
    if (path == "sample1") { content = sampleOriginal(); displayName = "sample1 (original)"; return true; }
    if (path == "sample2") { content = sampleRenamed(); displayName = "sample2 (renamed copy)"; return true; }
    if (!readFile(path, content)) { cout << "[Error] Unable to read file: " << path << "\n"; return false; }
    displayName = path;
    return true;
}

void compareTwoFiles(int k, bool normalize) {
    string p1, p2, c1, c2, n1, n2;
    cout << "\n--- Compare Two Files (Symmetric + Longest Matching Block) ---\n"
         << "(type 'sample1' / 'sample2' to use built-in samples)\n";
    if (!ask("Path to File A: ", p1)) return;
    if (!ask("Path to File B: ", p2)) return;
    if (!loadSourceByName(p1, c1, n1) || !loadSourceByName(p2, c2, n2)) return;

    Prepared A = prepare(n1, c1, normalize);
    Prepared B = prepare(n2, c2, normalize);
    if ((int)A.cleaned.length() < k || (int)B.cleaned.length() < k) {
        cout << "[Warning] A file is shorter than the window size (k = " << k << ").\n";
        return;
    }
    SimpleHashTable tA, tB;
    buildTable(A.cleaned, k, tA);
    buildTable(B.cleaned, k, tB);
    PairResult r = comparePair(A, B, 0, 1, k, tA, tB);
    printPairDetail(A, B, r);
    addHistory(A.name + " vs " + B.name, k, r.score, r.collisions, r.block.length);
}

int main() {
    string file1;
    string file2;
    int k = 8;
    bool normalize = true;
    loadSamples(file1, file2);

    int choice = 0;
    while (true) {
        cout << "\n============================================================\n";
        cout << "           HashCheck: Complete Evaluation Toolkit           \n";
        cout << "============================================================\n";
        cout << "  1. Run Continuous Similarity Evaluation against Source File\n";
        cout << "  2. Check Balanced Brackets for Syntax Verification\n";
        cout << "  3. Edit File 1 (In-Memory Undo/Redo)\n";
        cout << "  4. Rename Variable / Identifier in File 2\n";
        cout << "  5. Extract Function for Isolated Comparison\n";
        cout << "  6. View Cleaned Code Generated\n";
        cout << "  7. View Session Evaluation History (Linked List + File Log)\n";
        cout << "  8. Change k-gram Window Size (Current: " << k << ")\n";
        cout << "  9. Load Files from Local Disk\n";
        cout << " 10. Compare Two Files (Symmetric + Longest Matching Block)\n";
        cout << " 11. Scan a Folder (Pairwise Matrix + Ranked Suspicious Pairs)\n";
        cout << " 12. Run Built-In Test Cases\n";
        cout << " 13. Create Demo Folder with Sample Submissions\n";
        cout << " 14. Toggle Identifier Normalization (Current: " << (normalize ? "ON" : "OFF") << ")\n";
        cout << " 15. Export Last Folder Scan to CSV\n";
        cout << " 16. Exit\n";
        cout << "============================================================\n";
        cout << "Enter your choice (1-16): ";

        if (!(cin >> choice)) {
            if (cin.eof()) {
                cout << "\nEnd of input detected. Exiting the program!\n";
                break;
            }
            cin.clear();
            flushInputLine();
            continue;
        }
        flushInputLine();

        switch (choice) {
            case 1: {
                string sourcePath;
                string sourceContent;
                cout << "\n--- Continuous Batch Evaluation ---\n";
                cout << "Enter path to Base/Source file (or type 'sample' to use loaded Sample 1): ";
                if (!getline(cin, sourcePath)) break;

                if (sourcePath == "sample" || sourcePath.empty()) {
                    sourceContent = file1;
                    sourcePath = "Loaded_Sample_1";
                    cout << "[Info] Using internal Sample 1 as source file.\n";
                } else {
                    if (!readFile(sourcePath, sourceContent)) {
                        cout << "[Error] Unable to read source file from path: " << sourcePath << "\n";
                        break;
                    }
                    cout << "[Success] Source file loaded successfully (" << sourceContent.length() << " bytes).\n";
                }

                string cleanSource = cleanCode(sourceContent);
                int evalCount = 0;

                cout << "\nEnter comparison files one-by-one. Type 'exit' or 'done' to stop.\n";
                while (true) {
                    string targetPath;
                    cout << "\n[Target " << (evalCount + 1) << "] Enter comparison file path: ";
                    if (!getline(cin, targetPath) || targetPath == "exit" || targetPath == "done" || targetPath.empty()) {
                        cout << "[Info] Evaluation loop finished. Total files evaluated: " << evalCount << "\n";
                        break;
                    }

                    string targetContent;
                    if (targetPath == "sample") {
                        targetContent = file2;
                        targetPath = "Loaded_Sample_2";
                        cout << "[Info] Using internal Sample 2 for this comparison.\n";
                    } else if (!readFile(targetPath, targetContent)) {
                        cout << "[Error] Unable to open target file: " << targetPath << ". Skipping.\n";
                        continue;
                    }

                    string cleanTarget = cleanCode(targetContent);
                    CompareResult res = compareCodes(cleanSource, cleanTarget, k);

                    cout << "\n------------------- Evaluation Result -------------------\n";
                    cout << " Source File            : " << sourcePath << "\n";
                    cout << " Target File            : " << targetPath << "\n";
                    cout << " Total k-grams evaluated: " << res.totalKgrams << "\n";
                    cout << " Matching k-grams       : " << res.matches << "\n";
                    cout << " Hash Collisions        : " << res.collisions << "\n";
                    cout << " Similarity Score       : " << fixed << setprecision(2) << res.score << "%\n";
                    cout << " Risk Assessment        : " << res.status << "\n";
                    cout << "---------------------------------------------------------\n";

                    string label = sourcePath + " vs " + targetPath;
                    addHistory(label, k, res.score, res.collisions, res.status);
                    evalCount++;
                }
                break;
            }
            case 2: {
                cout << "\n--- Checking Brackets: File 1 ---\n";
                checkBrackets(file1);
                cout << "\n--- Checking Brackets: File 2 ---\n";
                checkBrackets(file2);
                break;
            }
            case 3: {
                cout << "\nOpening File 1 in standard stack-based editor...\n";
                runEditor(file1);
                break;
            }
            case 4: {
                string oldVar, newVar;
                cout << "Enter variable name to replace in File 2: ";
                if (!(cin >> oldVar)) break;
                cout << "Enter new variable name: ";
                if (!(cin >> newVar)) break;
                flushInputLine();
                replaceWord(file2, oldVar, newVar);
                break;
            }
            case 5: {
                string funcName;
                cout << "Enter function name to compare (e.g., calculateSum): ";
                if (!(cin >> funcName)) break;
                flushInputLine();
                string func1, func2;
                bool ok1 = extractFunction(file1, funcName, func1);
                bool ok2 = extractFunction(file2, funcName, func2);
                if (ok1 && ok2) {
                    string clean1 = cleanCode(func1);
                    string clean2 = cleanCode(func2);
                    CompareResult res = compareCodes(clean1, clean2, k);
                    cout << "\nFunction-Level Similarity ('" << funcName << "'): "
                         << fixed << setprecision(2) << res.score << "% (" << res.status << ")\n";
                    addHistory(funcName, k, res.score, res.collisions, res.status);
                }
                break;
            }
            case 6: {
                string clean1 = cleanCode(file1);
                string clean2 = cleanCode(file2);
                Prepared norm1 = prepare("File 1", file1, normalize);
                Prepared norm2 = prepare("File 2", file2, normalize);
                cout << "\n--- Basic Cleaned File 1 (First 100 chars) ---\n"
                     << (clean1.length() > 100 ? clean1.substr(0, 100) : clean1) << "\n";
                cout << "--- Normalized Cleaned File 1 (First 100 chars) ---\n"
                     << (norm1.cleaned.length() > 100 ? norm1.cleaned.substr(0, 100) : norm1.cleaned) << "\n";
                cout << "\n--- Basic Cleaned File 2 (First 100 chars) ---\n"
                     << (clean2.length() > 100 ? clean2.substr(0, 100) : clean2) << "\n";
                cout << "--- Normalized Cleaned File 2 (First 100 chars) ---\n"
                     << (norm2.cleaned.length() > 100 ? norm2.cleaned.substr(0, 100) : norm2.cleaned) << "\n";
                break;
            }
            case 7: {
                showHistory();
                break;
            }
            case 8: {
                cout << "Enter new k-gram size (4 to 20): ";
                int newK;
                if (cin >> newK && newK >= 4 && newK <= 20) {
                    k = newK;
                    cout << "Window size updated to k = " << k << ".\n";
                } else {
                    cin.clear();
                    cout << "Invalid value. Kept at k = " << k << ".\n";
                }
                flushInputLine();
                break;
            }
            case 9: {
                string path1, path2;
                cout << "Enter path for File 1: ";
                if (!getline(cin, path1)) break;
                cout << "Enter path for File 2: ";
                if (!getline(cin, path2)) break;
                string content1, content2;
                if (readFile(path1, content1) && readFile(path2, content2)) {
                    file1 = content1;
                    file2 = content2;
                    cout << "Files loaded successfully!\n";
                } else {
                    cout << "[Error] Unable to open one or both files. Keeping previous data.\n";
                }
                break;
            }
            case 10: {
                compareTwoFiles(k, normalize);
                break;
            }
            case 11: {
                string folder;
                if (!ask("Folder path (blank = " + DEMO_FOLDER + "): ", folder)) break;
                if (folder.empty()) folder = DEMO_FOLDER;
                scanFolder(folder, k, normalize);
                break;
            }
            case 12: {
                runTestCases(k, normalize);
                break;
            }
            case 13: {
                createDemoFolder();
                break;
            }
            case 14: {
                normalize = !normalize;
                cout << "Identifier normalization is now " << (normalize ? "ON" : "OFF") << ".\n";
                break;
            }
            case 15: {
                string path;
                if (!ask("CSV file name (blank = scan_report.csv): ", path)) break;
                if (path.empty()) path = "scan_report.csv";
                exportCsv(path);
                break;
            }
            case 16: {
                cout << "Exiting program. Freeing memory...\n";
                freeHistory();
                return 0;
            }
            default:
                cout << "Invalid choice. Please select 1 to 16.\n";
        }
    }

    freeHistory();
    return 0;
}