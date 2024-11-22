#include <sys/stat.h>
#include <sys/types.h>
#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "getopt.h"
#include "parser.h"
using namespace std;
namespace fs = std::filesystem;

void exportFile(ParsedFile& shaderFile, const string& srcDir, const string& subDir, const string& headerDir,
                bool sourceOnly) {
    if (shaderFile.name != "") {
        string lowername = shaderFile.name;
        transform(lowername.begin(), lowername.end(), lowername.begin(), toLower);

        ofstream srcout(srcDir + subDir + lowername + ".cpp");
        srcout << shaderFile.getSource();
        srcout.close();
        if (!sourceOnly) {
            ofstream headerout(headerDir + subDir + lowername + ".h");
            headerout << shaderFile.getHeader();
            headerout.close();
        }
    }
}

int main(int argc, char** args) {
    string file;
    string headerShield;
    string srcDir;
    string headerDir;
    string shaderDir;
    string modelDir;
    bool force = false, sourceOnly = false;
    int c;
    while ((c = getopt(argc, args, "f:s:o:h:d:m:FS")) != -1) {
        switch (c) {
            case 'F':
                force = true;
                break;
            case 'S':
                sourceOnly = true;
                break;
            case 'f':
                file = optarg;
                break;
            case 's':
                headerShield = optarg;
                break;
            case 'o':
                srcDir = optarg;
                break;
            case 'h':
                headerDir = optarg;
                break;
            case 'd':
                shaderDir = optarg;
                break;
            case 'm':
                modelDir = optarg;
                break;
        }
    }
    // cout << "File: " << file << endl;
    // cout << "HeaderShield: " << headerShield << endl;
    // cout << "SrcDir: " << srcDir << endl;
    // cout << "HeaderDir: " << headerDir << endl;

    ifstream modified(file + "shaders.modified");
    string tempfile;
    size_t hashRes;
    cout << "Opened modified" << endl;
    unordered_map<string, size_t> modifiedFiles;
    while (modified >> tempfile >> hashRes, file != "" && modified.good()) {
        modifiedFiles[tempfile] = hashRes;
    }
    modified.close();
    cout << "Closed modified" << endl;
    fs::path directory = file;
    cout << directory << endl;
    for (const auto& f : fs::directory_iterator(directory)) {
        string filename = f.path().filename().string();
        cout << filename << endl;
        if (f.is_regular_file() && filename != "shaders.modified") {
            string filecontent;
            ifstream filereader(file + filename);
            getline(filereader, filecontent, '\0');
            filereader.close();
            size_t fileHash = hash<string>{}(filecontent);
            if (force || modifiedFiles.find(filename) == modifiedFiles.end() || modifiedFiles[filename] != fileHash) {
                cout << "Compiling " << filename << endl;
                cout << fileHash << " vs " << modifiedFiles[filename] << endl;

                auto [shaderFile, modelFile] = parse(file + filename, headerShield, srcDir, headerDir, shaderDir);
                exportFile(shaderFile, srcDir, shaderDir, headerDir, sourceOnly);
                if (!modelDir.empty()) {
                    exportFile(modelFile, srcDir, modelDir, headerDir, sourceOnly);
                }
                modifiedFiles[filename] = fileHash;
            } else {
                cout << filename << " didn't change" << endl;
            }
        }
    }
    cout << "Done" << endl;
    ofstream wmodified(file + "shaders.modified");
    for (const auto& f : fs::directory_iterator(directory)) {
        string filename = f.path().filename().string();
        if (f.is_regular_file() && filename != "shaders.modified") {
            wmodified << filename << " " << modifiedFiles[filename] << endl;
        }
    }
    wmodified.close();
}
