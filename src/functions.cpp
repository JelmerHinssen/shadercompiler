#include "parser.h"

#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

string Function::getPrototype() const{
    stringstream prototype;
    if(line) {
        prototype << "inline ";
    }
    if(virt) {
        prototype << "virtual ";
    }
    prototype << type << " ";
    prototype << nameAndArguments;
    if(constant){
        prototype << " const";
    }
    if(!line){
        prototype << ";";
    }else{
        prototype << "{";
        for(unsigned int i = 0; i < content.size(); i++){
            prototype << content[i] << " ";
        }
        prototype << "}";
    }
    return prototype.str();
}

string Function::getImplementation(const std::string& parent) const{
    stringstream implementation;
    implementation << type << " ";
    if(parent.size() > 0){
        implementation << parent << "::";
    }
    implementation << nameAndArguments;
    if(constant){
        implementation << " const";
    }
    implementation << " {" << endl;
    for(unsigned int i = 0; i < content.size(); i++){
        implementation << "    " << content[i] << endl;
    }
    implementation << "}";
    return implementation.str();
}

string ParsedFile::getHeader() const{
    stringstream header;
    //header << "#ifndef " << headerShield << endl;
    //header << "#define " << headerShield << endl;
    header << "#pragma once" << endl;
    for(unsigned int i = 0; i < headerIncludes.size(); i++){
        header << "#include " << headerIncludes[i] << endl;
        cout << headerIncludes[i] << endl;
    }
    header << endl;
    header << "class " << name;
    if(parents.size() > 0){
        header << ": ";
        for(unsigned int i = 0; i < parents.size(); i++){
            header << parents[i];
            if(i < parents.size() - 1){
                header << ", ";
            }
        }
    }
    header << " {" << endl;
    header << "public:" << endl;
    for(unsigned int i = 0; i < constructors.size(); i++){
        header << "    " << constructors[i].getPrototype() << endl;
    }
    for(unsigned int i = 0; i < publicFunctions.size(); i++){
        header << "    " << publicFunctions[i].getPrototype() << endl;
    }
    for(unsigned int i = 0; i < staticFunctions.size(); i++){
        header << "    static " << staticFunctions[i].getPrototype() << endl;
    }
    for(unsigned int i = 0; i < publicMembers.size(); i++){
        header << "    " << publicMembers[i].type << " " << publicMembers[i].name;
        if(publicMembers[i].defaultValue != ""){
            header << " = " << publicMembers[i].defaultValue;
        }
        header << ";" << endl;
    }

    header << "protected:" << endl;
    for(unsigned int i = 0; i < protectedFunctions.size(); i++){
        header << "    " << protectedFunctions[i].getPrototype() << endl;
    }
    for(unsigned int i = 0; i < protectedMembers.size(); i++){
        header << "    " << protectedMembers[i].type << " " << protectedMembers[i].name;
        if(protectedMembers[i].defaultValue != ""){
            header << " = " << protectedMembers[i].defaultValue;
        }
        header << ";" << endl;
    }

    header << "private:" << endl;
    for(unsigned int i = 0; i < privateFunctions.size(); i++){
        header << "    " << privateFunctions[i].getPrototype() << endl;
    }
    for(unsigned int i = 0; i < privateMembers.size(); i++){
        header << "    " << privateMembers[i].type << " " << privateMembers[i].name;
        if(privateMembers[i].defaultValue != ""){
            header << " = " << privateMembers[i].defaultValue;
        }
        header << ";" << endl;
    }
    for(unsigned int i = 0; i < staticMembers.size(); i++){
        header << "    static " << staticMembers[i].type << " " << staticMembers[i].name;
        header << ";" << endl;
    }
    header << "};" << endl;
    //header << "#endif";
    return header.str();
}

string ParsedFile::getSource() const{
    stringstream source;
    for(unsigned int i = 0; i < sourceIncludes.size(); i++){
        source << "#include " << sourceIncludes[i] << endl;
    }
    source << endl;

    for(unsigned int i = 0; i < staticMembers.size(); i++){
        if(staticMembers[i].defaultValue != "") {
            source << staticMembers[i].type << " " << name << "::" << staticMembers[i].name
                   << " = " << staticMembers[i].defaultValue << ";" << endl;
        }
    }
    for(unsigned int i = 0; i < constructors.size(); i++){
        if(!constructors[i].line){
            source << constructors[i].getImplementation(name) << endl << endl;
        }
    }
    for(unsigned int i = 0; i < publicFunctions.size(); i++){
        if(!publicFunctions[i].line){
            source << publicFunctions[i].getImplementation(name) << endl << endl;
        }
    }
    for(unsigned int i = 0; i < staticFunctions.size(); i++){
        if(!staticFunctions[i].line){
            source << staticFunctions[i].getImplementation(name) << endl << endl;
        }
    }
    for(unsigned int i = 0; i < protectedFunctions.size(); i++){
        if(!protectedFunctions[i].line){
            source << protectedFunctions[i].getImplementation(name) << endl << endl;
        }
    }
    for(unsigned int i = 0; i < privateFunctions.size(); i++){
        if(!privateFunctions[i].line){
            source << privateFunctions[i].getImplementation(name) << endl << endl;
        }
    }
    return source.str();
}



string Constructor::getPrototype() const{
    stringstream prototype;
    prototype << nameAndArguments;
    if(constant){
        prototype << " const";
    }
    if(!line){
        prototype << ";";
    }else{
        if(supers.size() > 0){
            prototype << ": " << supers;
        }
        prototype << "{";
        for(unsigned int i = 0; i < content.size(); i++){
            prototype << content[i] << " ";
        }
        prototype << "}";
    }
    return prototype.str();
}

string Constructor::getImplementation(const std::string& parent) const{
    stringstream implementation;
    if(parent.size() > 0){
        implementation << parent << "::";
    }
    implementation << nameAndArguments;
    if(supers.size() > 0){
        implementation << ": " << supers;
    }
    implementation << "{" << endl;
    for(unsigned int i = 0; i < content.size(); i++){
        implementation << "    " << content[i] << endl;
    }
    implementation << "}" << endl;
    return implementation.str();
}
