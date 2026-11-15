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
