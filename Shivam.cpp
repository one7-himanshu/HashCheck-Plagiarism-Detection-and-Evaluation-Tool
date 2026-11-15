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
