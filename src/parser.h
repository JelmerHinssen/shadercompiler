#ifndef SHILM_SHADERCOMPILER_PARSER_H
#define SHILM_SHADERCOMPILER_PARSER_H

#include <string>
#include <vector>


inline char toLower(char c) {
    return (char) :: tolower(c);
}

inline char toUpper(char c) {
    return (char) :: toupper(c);
}

class Function{
    public:
    Function(const std::string& type, const std::string& nameAndArguments, bool constant, bool line, bool virt):
            type(type), nameAndArguments(nameAndArguments), constant(constant), line(line), virt(virt){}
    std::vector<std::string> content;
    std::string type;
    std::string nameAndArguments;
    bool constant = false;
    bool line = false;
    bool virt = false;
    std::string getPrototype() const;
    std::string getImplementation(const std::string& parent) const;
};

class Constructor : public Function{
    public:
    Constructor(const std::string& nameAndArguments, bool line, const std::string& supers) :
        Function("", nameAndArguments, false, line, false), supers(supers){}
    std::string supers;
    std::string getPrototype() const;
    std::string getImplementation(const std::string& parent) const;
};

struct memberVariable{
    std::string type;
    std::string name;
    std::string defaultValue;
};

class ParsedFile{
public:
    std::string getHeader() const;
    std::string getSource() const;
    std::vector<Constructor> constructors;
    std::vector<Function> publicFunctions, protectedFunctions, privateFunctions, staticFunctions;
    std::vector<memberVariable> publicMembers, protectedMembers, privateMembers, staticMembers;
    std::vector<std::string> headerIncludes;
    std::vector<std::string> sourceIncludes;
    std::vector<std::string> parents;
    std::string name;
    std::string headerShield;
private:

};

ParsedFile parse(const std::string& filename, const std::string& headerShield,
           const std::string& srcOutput, const std::string& headerOutput,
           const std::string& subDir);

#endif // SHILM_SHADERCOMPILER_PARSER_H
