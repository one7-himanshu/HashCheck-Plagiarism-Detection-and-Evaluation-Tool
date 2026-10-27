#include <iostream>
#include <string>
#include <iomanip>
#include <vector>
#include <stack>
#include <fstream>

using namespace std;

const long long BASE = 257LL;
const long long MOD = 1000000007LL;
const string HISTORY_FILE = "audit_history.txt";

static LogNode* head = NULL;
static int TestCount = 1;

long long GetHighestPower(int k)
{
  long long power = 1;
  for (int i=0; i<k-1; i++)
    {
      power = (power*BASE)%MOD;
    }
  return power;
}

long long GetInitialHash(const string &text, int k)
{
  long long hashval = 0;
  for (int i=0; i<k; i++)
    {
      hashval = (hashval*BASE + (unsigned char)text[i])%MOD;
    }
  return hashval;
}

long long RollHash(long long oldhash, char oldc, char newc, long long power)
{
  long long outval = ((long long)(unsigned char)oldc * power)%MOD;
  long long rem = (oldhash-outval)%MOD;
  if (rem < 0) rem += MOD;
  long long newhash = (rem*BASE + (unsigned char)newc)%MOD;
  return newhash;
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
    cout << "\n=========================== AUDIT SESSION HISTORY (Current Session) ===========================\n";
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

    cout << "\n========================== AUDIT LOG (" << HISTORY_FILE << ") ===========================\n";
    ifstream inFile(HISTORY_FILE.c_str());
    if (!inFile.is_open()) {
        cout << "No stored audit log file found on disk yet.\n";
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
