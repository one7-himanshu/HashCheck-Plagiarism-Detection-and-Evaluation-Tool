#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <fstream>
#include <iomanip>

using namespace std;

const long long BASE = 257LL;
const long long MOD = 1000000007LL;
const string HISTORY_FILE = "audit_history.txt";

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

void flushInputLine() {
    char ch;
    while (cin.get(ch) && ch != '\n') {
        if (cin.eof()) {
            break;
        }
    }
}

struct LogNode {
    int runNo;
    string label;
    int kVal;
    float score;
    int collisions;
    string status;
    LogNode* next;
};

static LogNode* head = NULL;
static int testCount = 1;

void addHistory(const string& label, int k, float score, int coll, const string& status) {
    LogNode* newNode = new LogNode();
    newNode->runNo = testCount++;
    newNode->label = label;
    newNode->kVal = k;
    newNode->score = score;
    newNode->collisions = coll;
    newNode->status = status;
    newNode->next = NULL;

    if (head == NULL) {
        head = newNode;
    } else {
        LogNode* temp = head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newNode;
    }

    ofstream outFile(HISTORY_FILE.c_str(), ios::app);
    if (outFile.is_open()) {
        outFile << "Run #" << newNode->runNo << " | Label: " << setw(25) << left << newNode->label
                << " | k: " << setw(2) << newNode->kVal
                << " | Sim: " << setw(6) << fixed << setprecision(2) << right << newNode->score << "%"
                << " | Collisions: " << setw(2) << newNode->collisions
                << " | " << newNode->status << "\n";
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
            cout << "Run #" << curr->runNo << " | Label: " << setw(25) << left << curr->label
                 << " | k: " << setw(2) << curr->kVal
                 << " | Sim: " << setw(6) << fixed << setprecision(2) << right << curr->score << "%"
                 << " | Collisions: " << setw(2) << curr->collisions
                 << " | " << curr->status << "\n";
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
    head = NULL;
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

        if (inLineComment) {
            continue;
        }

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
            if (ch == '"') {
                inQuotes = false;
            }
            continue;
        }

        if (inChar) {
            if (ch == '\\' && i + 1 < text.length()) {
                i++;
                col++;
                continue;
            }
            if (ch == '\'') {
                inChar = false;
            }
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

        if (ch == '"') {
            inQuotes = true;
            continue;
        }
        if (ch == '\'') {
            inChar = true;
            continue;
        }

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
        if (ch == '\r') {
            continue;
        }
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
        if (!getline(cin, input)) {
            break;
        }

        if (input.length() >= 3 && input.substr(0, 3) == ":a ") {
            string newLine = input.substr(3);
            undoStack.push(newLine);
            while (!redoStack.empty()) {
                redoStack.pop();
            }
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

bool readFile(const string& path, string& content) {
    ifstream inFile(path.c_str());
    if (!inFile.is_open()) {
        return false;
    }
    content.assign((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    return true;
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

int main() {
    string file1;
    string file2;
    int k = 10;
    loadSamples(file1, file2);

    int choice = 0;
    while (true) {
        cout << "\n============================================================\n";
        cout << "           HashCheck: Complete Evaluation Toolkit          \n";
        cout << "============================================================\n";
        cout << " 1. Run Continuous Similarity Evaluation against Source File\n";
        cout << " 2. Check Balanced Brackets for Syntax Verification\n";
        cout << " 3. Edit File 1 (In-Memory Undo/Redo)\n";
        cout << " 4. Rename Variable / Identifier in File 2\n";
        cout << " 5. Extract Function for Isolated Comparison\n";
        cout << " 6. View Cleaned Code Generated\n";
        cout << " 7. View Session Evaluation History (File Log)\n";
        cout << " 8. Change k-gram Window Size (Current: " << k << ")\n";
        cout << " 9. Load Files from Local Disk\n";
        cout << " 10. Exit\n";
        cout << "============================================================\n";
        cout << "Enter your choice (1-10): ";

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
                if (!getline(cin, sourcePath)) {
                    break;
                }

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
                if (!(cin >> oldVar)) {
                    break;
                }
                cout << "Enter new variable name: ";
                if (!(cin >> newVar)) {
                    break;
                }
                flushInputLine();
                replaceWord(file2, oldVar, newVar);
                break;
            }
            case 5: {
                string funcName;
                cout << "Enter function name to compare (e.g., calculateSum): ";
                if (!(cin >> funcName)) {
                    break;
                }
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
                cout << "\n--- Cleaned File 1 (First 100 characters) ---\n"
                     << (clean1.length() > 100 ? clean1.substr(0, 100) : clean1) << "\n...\n";
                cout << "\n--- Cleaned File 2 (First 100 characters) ---\n"
                     << (clean2.length() > 100 ? clean2.substr(0, 100) : clean2) << "\n...\n";
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
                if (!getline(cin, path1)) {
                    break;
                }
                cout << "Enter path for File 2: ";
                if (!getline(cin, path2)) {
                    break;
                }
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
                cout << "Exiting program. Freeing memory...\n";
                freeHistory();
                return 0;
            }
            default:
                cout << "Invalid choice. Please select 1 to 10.\n";
        }
    }

    freeHistory();
    return 0;
}


