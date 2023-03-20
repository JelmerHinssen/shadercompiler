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

int main(int argc, char **args){
    string file;
    string headerShield;
    string srcDir;
    string headerDir;
    string subDir;
    char c;
    while((c = getopt(argc, args, "f:s:o:h:d:")) != -1){
        switch(c){
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
    //cout << "closed" << endl;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (file.c_str())) != NULL) {
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {
        if(ent->d_name != string(".") && ent->d_name != string("..") && ent->d_name != string("shaders.modified")){
            string filecontent;
            string filename(ent->d_name);
            ifstream filereader(file + filename);
            getline(filereader, filecontent, '\0');
            filereader.close();
            size_t fileHash = hash<string>{}(filecontent);
            if(modifiedFiles.find(filename) == modifiedFiles.end()
               || modifiedFiles[filename] != fileHash){
                cout << "Compiling " << filename << endl;
                cout << fileHash << " vs " << modifiedFiles[filename] << endl;

                ParsedFile pfile = parse(file + filename, headerShield, srcDir, headerDir, subDir);
                if(pfile.name != ""){
                    string lowername = pfile.name;
                    transform(lowername.begin(), lowername.end(), lowername.begin(), ::tolower);

                    ofstream srcout(srcDir + subDir + lowername + ".cpp");
                    srcout << pfile.getSource();
                    srcout.close();
                    ofstream headerout(headerDir + subDir + lowername + ".h");
                    headerout << pfile.getHeader();
                    headerout.close();
                }
                modifiedFiles[filename] = fileHash;
            }else{
                cout << filename << " didn't change" << endl;
            }
        }
      }
      closedir (dir);
    } else {
      /* could not open directory */
      perror (file.c_str());
      return EXIT_FAILURE;
    }
    ofstream wmodified(file + "shaders.modified");
    if ((dir = opendir (file.c_str())) != NULL) {
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {
        if(ent->d_name != string(".") && ent->d_name != string("..") && ent->d_name != string("shaders.modified")){
            string filename(ent->d_name);
            wmodified << filename << " " << modifiedFiles[filename] << endl;
        }
      }
      closedir (dir);
    } else {
      /* could not open directory */
      perror (file.c_str());
      return EXIT_FAILURE;
    }
    wmodified.close();
}
