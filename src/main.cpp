#include <iostream>
#include <fstream>
#include "getopt.h"
#include <filesystem>
#include <windows.h>
#include <unordered_map>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>

#include "parser.h"
using namespace std;
namespace fs = std::filesystem;

int main(int argc, char **args){
    string file;
    string headerShield;
    string srcDir;
    string headerDir;
    string subDir;
    bool force = false, sourceOnly = false;
    int c;
    while((c = getopt(argc, args, "f:s:o:h:d:FS")) != -1){
        switch(c){
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
            subDir = optarg;
            break;
        }
    }
    //cout << "File: " << file << endl;
    //cout << "HeaderShield: " << headerShield << endl;
    //cout << "SrcDir: " << srcDir << endl;
    //cout << "HeaderDir: " << headerDir << endl;
    cout << "SubDir: " << subDir << endl << endl;

    ifstream modified(file + "shaders.modified");
    string tempfile;
    size_t hashRes;

    unordered_map<string, size_t> modifiedFiles;
    while(modified >> tempfile >> hashRes, file != "" && modified.good()){
        modifiedFiles[tempfile] = hashRes;
    }
    modified.close();
    fs::path directory = file;
    for (const auto& f : fs::directory_iterator(directory)) {
        string filename = f.path().filename().string();
        if (f.is_regular_file() && filename != "shaders.modified") {
            string filecontent;
            ifstream filereader(file + filename);
            getline(filereader, filecontent, '\0');
            filereader.close();
            size_t fileHash = hash<string>{}(filecontent);
            if(force || modifiedFiles.find(filename) == modifiedFiles.end()
               || modifiedFiles[filename] != fileHash){
                cout << "Compiling " << filename << endl;
                cout << fileHash << " vs " << modifiedFiles[filename] << endl;

                ParsedFile pfile = parse(file + filename, headerShield, srcDir, headerDir, subDir);
                if(pfile.name != ""){
                    string lowername = pfile.name;
                    transform(lowername.begin(), lowername.end(), lowername.begin(), toLower);

                    ofstream srcout(srcDir + subDir + lowername + ".cpp");
                    srcout << pfile.getSource();
                    srcout.close();
                    if (!sourceOnly) {
                        ofstream headerout(headerDir + subDir + lowername + ".h");
                        headerout << pfile.getHeader();
                        headerout.close();
                    }
                }
                modifiedFiles[filename] = fileHash;
            }else{
                cout << filename << " didn't change" << endl;
            }
        }

    }
    ofstream wmodified(file + "shaders.modified");
    for (const auto& f : fs::directory_iterator(directory)) {
        string filename = f.path().filename().string();
        if (f.is_regular_file() && filename != "shaders.modified") {
            wmodified << filename << " " << modifiedFiles[filename] << endl;
        }
    }
    wmodified.close();
}
