#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "parser.h"

using namespace std;

ParsedFile parse(const std::string& filename, const std::string& headerShield,
           [[maybe_unused]] const std::string& srcOutput, [[maybe_unused]] const std::string& headerOutput,
           const std::string& subDir){
    ifstream input(filename);
    string line;
    ParsedFile parsedFile;
    parsedFile.headerIncludes.push_back("\"" + subDir + "shaderprogram.h\"");
    parsedFile.sourceIncludes.push_back("\"" + subDir + "shaderprogram_impl.h\"");
    parsedFile.headerIncludes.push_back("\"inspector.h\"");
    parsedFile.sourceIncludes.push_back("<string>");
    cout << "\"" << subDir << "shaderprogram.h\"" << endl;
    parsedFile.headerIncludes.push_back("<memory>");
    parsedFile.parents.push_back("public ShaderProgram");
    parsedFile.parents.push_back("public Inspector");

    bool vectorInclude = false, glmInclude = false;
    Function uniformFunction("void", "getAllLocations()", false, false, true);
    uniformFunction.content.push_back("start();");
    Function attribFunction("void", "bindAttributes()", false, false, true);
    Function inspectFunction("void", "registerAll()", false, false, true);
    inspectFunction.content.push_back("using namespace std::literals;");
    while(getline(input, line)){
        if (line.empty()) continue;
        stringstream lineStream(line);
        string opp;
        lineStream >> opp;
        //cout << opp << endl;
        if(opp == "@name"){
            string name, vertexShader, fragmentShader, geometryShader;
            lineStream >> name >> vertexShader >> fragmentShader >> geometryShader;
            parsedFile.name = name;
            Constructor ctor(name + "()", true, "ShaderProgram(\"" + vertexShader + "\", \""
                                  + fragmentShader + (geometryShader == "" ? "\")" : "\", \"" + geometryShader + "\")"));
            parsedFile.constructors.push_back(ctor);

            string lowername = name, uppername = name;
            transform(lowername.begin(), lowername.end(), lowername.begin(), ::toLower);
            transform(uppername.begin(), uppername.end(), uppername.begin(), ::toUpper);
            parsedFile.sourceIncludes.push_back("\"" + subDir +  lowername + ".h\"");
            parsedFile.headerShield = headerShield + "_" + uppername + "_H";
            parsedFile.staticMembers.push_back({"std::unique_ptr<" + name + ">", "instance", "nullptr"});
            Function getInstance(name + "*", "getInstance()", false, false, false);
            getInstance.content.push_back("if (!instance) {");
            getInstance.content.push_back("    instance = std::unique_ptr<" + name + ">(new " + name + "());");
            getInstance.content.push_back("    instance->create();");
            getInstance.content.push_back("}");
            getInstance.content.push_back("return instance.get();");
            parsedFile.staticFunctions.push_back(getInstance);
            Function refreshFunction("bool", "refresh()", false, false, false);
            refreshFunction.content.push_back("auto old = instance.release();");
            refreshFunction.content.push_back("try {");
            refreshFunction.content.push_back("    instance = nullptr;");
            refreshFunction.content.push_back("    ShaderProgram::loadHeaders(\"shaders\");");
            refreshFunction.content.push_back("    instance = std::make_unique<" + name + ">();");
            refreshFunction.content.push_back("    instance->create();");
            refreshFunction.content.push_back("    delete old;");
            refreshFunction.content.push_back("    return true;");
            refreshFunction.content.push_back("} catch(std::runtime_error&) {");
            refreshFunction.content.push_back("    instance.reset(old);");
            refreshFunction.content.push_back("    return false;");
            refreshFunction.content.push_back("}");
            parsedFile.staticFunctions.push_back(refreshFunction);
        }else if(opp == "@uniform"){
            string name, getterSetter, value, type;
            lineStream >> name >> getterSetter >> type >> ws;
            getline(lineStream, value);
            while(value.size() > 0 && value[0] == ' '){
                value = value.substr(1);
            }
            string varName = ((char)(toUpper(name[0])) + name.substr(1));
            string locName = "loc" + varName;
            uniformFunction.content.push_back(locName + " = getUniformLocation(\"" + name + "\");");
            string ctype, gltype;
            bool array = type[type.size() - 1] == 'v';
            bool matrix = type[0] == 'm';
            bool primitive = true;
            if(array){
                type = type.substr(0, type.size() - 1);
            }
            if(type == "float"){
                ctype = "float";
                gltype = "f";
            }else if(type == "int"){
                ctype = "int";
                gltype = "i";
            }else if(type == "uint"){
                ctype = "unsigned int";
                gltype = "ui";
            }else{
                primitive = false;
                ctype = "glm::" + type;
                gltype = (type[0] == 'v' || type[0] == 'm' ? "f" : type.substr(0, 1));
            }
            int vecsize = !primitive ? type[type.size() - 1] - '0' : 1;
            if(array){
                if(!vectorInclude){
                    parsedFile.headerIncludes.push_back("<vector>");
                    vectorInclude = true;
                }
                ctype = string("std::vector<") + (gltype == "f" ? "float" : (gltype == "i" ? "int" : "unsigned int")) + ">";
            }else if(!primitive && !glmInclude){
                parsedFile.headerIncludes.push_back("<glm/glm.hpp>");
                glmInclude = true;
            }
            gltype = (matrix ? "Matrix" : "") + to_string(vecsize) + gltype + (array ? "v" : "");

            //cout << "Type: " << type << ", cype: " << ctype << endl;
            //bool array = type.size() > 2 && type[3] == 'v';

            if(value != ""){
                if(!array){
                    uniformFunction.content.push_back("glUniform" + gltype + "(" + locName + ", " + value + ");");
                }else{
                    uniformFunction.content.push_back(ctype + " start" + varName + " = {" + value + "};");
                    uniformFunction.content.push_back("glUniform" + gltype + "(" + locName + ", start" + varName + ".size() / "
                                              + to_string(vecsize) + ", &start" + varName + "[0]);");
                }
            }
            if(getterSetter == "gs" || getterSetter == "g"){
                Function getter(ctype, "get" + varName + "()", true, true, false);
                getter.content.push_back("return " + name + ";");
                if(primitive && !array){
                    parsedFile.privateMembers.push_back({ctype, name, value});
                }else{
                    parsedFile.privateMembers.push_back({ctype, name, value == "" ? "" : ("{" + value + "}")});
                }
                parsedFile.publicFunctions.push_back(getter);
            }
            if(getterSetter == "gs" || getterSetter == "s"){
                if(!array){
                    Function setter("void", "set" + varName + "(" + ctype + " " + name + "_)", false, !matrix && !array, false);
                    if(primitive){
                        setter.content.push_back("glUniform" + gltype + "(" + locName + ", " + name + "_);");
                    }else if(!matrix){
                        string valuestr;
                        for(int i = 0; i < vecsize; i++){
                            valuestr += name + "_[" + to_string(i) + "]" + (i < vecsize - 1 ? ", " : "");
                        }
                        setter.content.push_back("glUniform" + gltype + "(" + locName + ", " + valuestr + ");");
                    }else{
                        setter.content.push_back("glUniform" + gltype + "v(" + locName + ", 1, false, glm::value_ptr(" + name + "_));");
                    }
                    if(getterSetter == "gs"){
                        setter.content.push_back("this->" + name + " = " + name + "_;");
                    }
                    parsedFile.publicFunctions.push_back(setter);
                }else{
                    Function setter("void", "set" + varName + "(" + ctype + " " + name + "_)", false, false, false);
                    setter.content.push_back("glUniform" + gltype + "(" + locName + ", " + name + "_.size() / "
                                              + to_string(vecsize) + ", &" + name + "_[0]);");

                    if(getterSetter == "gs"){
                        setter.content.push_back("this->" + name + " = " + name + "_;");
                    }
                    parsedFile.publicFunctions.push_back(setter);
                }
            }
            if (getterSetter == "gs") {
                if (!array) {
                    if (type == "int" || type == "uint") {
                        inspectFunction.content.push_back("RegisterUniform(" + name + ", " + varName  + ", int);");
                    } else if (type == "float") {
                        inspectFunction.content.push_back("RegisterUniform(" + name + ", " + varName  + ", float);");
                    } else if (!primitive && type[0] == 'v') {
                        inspectFunction.content.push_back("RegisterUniformv(" + name + ", " + varName  + ", " + to_string(vecsize) + ");");
                        parsedFile.privateMembers.push_back({"Inspector", name + "_inspector", ""});
                    }
                }
            }

            parsedFile.privateMembers.push_back({"unsigned int", locName, ""});
        }else if(opp == "@attribute"){
            string name, value;
            lineStream >> name>> value;
            attribFunction.content.push_back("bindAttribute(" + value + ", \"" + name + "\");");
        }else if(opp == "@include"){
            string name;
            lineStream >> ws;
            getline(lineStream, name);
            parsedFile.sourceIncludes.push_back(name);
        }else if(opp == "@hinclude"){
            string name;
            lineStream >> ws;
            getline(lineStream, name);
            parsedFile.headerIncludes.push_back(name);
        }else if(opp == "@function"){
            string scope, modifier, type, nameAndArgs;
            lineStream >> scope;
            bool constant = false, virt = false, iline = false;
            while(true){
                lineStream >> modifier;
                if(modifier == "const"){
                    constant = true;
                }else if(modifier == "virtual"){
                    virt = true;
                }else if(modifier == "inline"){
                    iline = true;
                }else{
                    type = modifier;
                    break;
                }
            }
            lineStream >> ws;
            getline(lineStream, nameAndArgs);
            Function func(type, nameAndArgs, constant, iline, virt);
            string funcLine;
            while(getline(input, funcLine), funcLine.size() > 0){
                if(funcLine == "@endfunction"){
                    break;
                }else{
                    func.content.push_back(funcLine);
                }
            }
            if(scope == "public"){
                parsedFile.publicFunctions.push_back(func);
            }else if(scope == "protected"){
                parsedFile.protectedFunctions.push_back(func);
            }else if(scope == "private"){
                parsedFile.privateFunctions.push_back(func);
            }
        }else if(opp == "@member"){
            string scope, type, name, value, gettersetter;
            lineStream >> scope >> gettersetter >> type >> name >> ws;
            getline(lineStream, value);
            vector<memberVariable>* members = nullptr;
            vector<Function>* functions = &parsedFile.publicFunctions;
            if(scope == "public"){
                members = &parsedFile.publicMembers;
            }else if(scope == "protected"){
                members = &parsedFile.protectedMembers;
            }else if(scope == "private"){
                members = &parsedFile.privateMembers;
            }
            if(members && functions){
                members->push_back({type, name, value});
                string varName = ((char)(toupper(name[0])) + name.substr(1));
                if(gettersetter == "g" || gettersetter == "gs"){
                    Function getter(type, "get" + varName + "()", true, true, false);
                    getter.content.push_back("return " + name + ";");
                    functions->push_back(getter);
                }
                if(gettersetter == "s" || gettersetter == "gs"){
                    Function setter("void", "set" + varName + "(" + type + " " + name + ")", false, true, false);
                    setter.content.push_back("this-> " + name + " = " + name + ";");
                    functions->push_back(setter);
                }
            }else{
                cerr << "Invalid scope: " << scope << endl;
            }
        }
    }
    parsedFile.protectedFunctions.push_back(uniformFunction);
    parsedFile.protectedFunctions.push_back(attribFunction);
    parsedFile.protectedFunctions.push_back(inspectFunction);
    input.close();
    return parsedFile;
}


