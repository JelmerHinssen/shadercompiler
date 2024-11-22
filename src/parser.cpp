#include "parser.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

struct TypeInfo {
    std::string ctype, gltype, basetype;
    int vecsize;
    bool array, matrix, primitive;
};

TypeInfo parseType(std::string type) {
    TypeInfo result;
    result.array = type[type.size() - 1] == 'v';
    result.matrix = type[0] == 'm';
    result.primitive = true;
    result.vecsize = 1;
    if (result.array) {
        type = type.substr(0, type.size() - 1);
    }
    if (type == "float") {
        result.ctype = result.basetype ="float";
        result.gltype = "f";
    } else if (type == "int") {
        result.ctype = result.basetype = "int";
        result.gltype = "i";
    } else if (type == "uint") {
        result.ctype = result.basetype = "unsigned int";
        result.gltype = "ui";
    } else {
        result.primitive = false;
        result.ctype = "glm::" + type;
        result.basetype = "float";
        result.gltype = (type[0] == 'v' || type[0] == 'm' ? "f" : type.substr(0, 1));
        result.vecsize = type[type.size() - 1] - '0';
    }
    return result;
}

pair<ParsedFile, ParsedFile> parse(const std::string& filename, const std::string& headerShield,
                 [[maybe_unused]] const std::string& srcOutput, [[maybe_unused]] const std::string& headerOutput,
                 const std::string& subDir) {
    ifstream input(filename);
    string line;
    ParsedFile modelFile;
    modelFile.parents.push_back("public linicore::Model");
    modelFile.headerIncludes.push_back("<linicore/model.h>");
    modelFile.sourceIncludes.push_back("<linicore/glinclude.h>");
    Function setIBO("void", "setIBO(const std::vector<unsigned int>& ibov)", false, false, false);
    setIBO.content.push_back("glDeleteBuffers(1, &ibo);");
    setIBO.content.push_back("glGenBuffers(1, &ibo);");
    setIBO.content.push_back("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);");
    setIBO.content.push_back(
        "glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibov.size() * sizeof(unsigned int), &ibov[0], GL_STATIC_DRAW)");
    setIBO.content.push_back("vertexCount = (unsigned int) ibov.size();");

    ParsedFile shaderFile;
    shaderFile.headerIncludes.push_back("<linicore/shaderprogram.h>");
    shaderFile.headerIncludes.push_back("<linicore/inspector.h>");
    shaderFile.sourceIncludes.push_back("<string>");
    shaderFile.sourceIncludes.push_back("<linicore/shaderprogram_impl.h>");
    cout << "\"" << subDir << "shaderprogram.h\"" << endl;
    shaderFile.headerIncludes.push_back("<memory>");
    shaderFile.parents.push_back("public linicore::ShaderProgram");
    shaderFile.parents.push_back("public linicore::Inspector");

    bool vectorInclude = false, glmInclude = false;
    Function uniformFunction("void", "getAllLocations()", false, false, true);
    uniformFunction.content.push_back("start();");
    Function attribFunction("void", "bindAttributes()", false, false, true);
    Function inspectFunction("void", "registerAll()", false, false, true);
    inspectFunction.content.push_back("using namespace std::literals;");
    while (getline(input, line)) {
        if (line.empty()) continue;
        stringstream lineStream(line);
        string opp;
        lineStream >> opp;
        // cout << opp << endl;
        if (opp == "@name") {
            string name, vertexShader, fragmentShader, geometryShader;
            lineStream >> name >> vertexShader >> fragmentShader >> geometryShader;
            string lowername = name, uppername = name;
            transform(lowername.begin(), lowername.end(), lowername.begin(), ::toLower);
            transform(uppername.begin(), uppername.end(), uppername.begin(), ::toUpper);
            modelFile.name = name + "Model";
            Constructor modelCtor(modelFile.name + "()", false, "");
            modelFile.constructors.push_back(modelCtor);
            modelFile.sourceIncludes.push_back("\"" + lowername + "model.h\"");


            shaderFile.name = name + "Shader";
            Constructor ctor(shaderFile.name + "()", true,
                             "ShaderProgram(\"" + vertexShader + "\", \"" + fragmentShader +
                                 (geometryShader == "" ? "\")" : "\", \"" + geometryShader + "\")"));
            shaderFile.constructors.push_back(ctor);

            shaderFile.sourceIncludes.push_back("\"" + subDir + lowername + "shader.h\"");
            shaderFile.headerShield = headerShield + "_" + uppername + "_H";
            shaderFile.staticMembers.push_back({"std::unique_ptr<" + shaderFile.name + ">", "instance", "nullptr"});
            Function getInstance(shaderFile.name + "*", "getInstance()", false, false, false);
            getInstance.content.push_back("if (!instance) {");
            getInstance.content.push_back("    instance = std::unique_ptr<" + shaderFile.name + ">(new " + shaderFile.name + "());");
            getInstance.content.push_back("    instance->create();");
            getInstance.content.push_back("}");
            getInstance.content.push_back("return instance.get();");
            shaderFile.staticFunctions.push_back(getInstance);
            Function refreshFunction("bool", "refresh()", false, false, false);
            refreshFunction.content.push_back("auto old = instance.release();");
            refreshFunction.content.push_back("try {");
            refreshFunction.content.push_back("    instance = nullptr;");
            refreshFunction.content.push_back("    ShaderProgram::loadHeaders(\"shaders\");");
            refreshFunction.content.push_back("    instance = std::make_unique<" + shaderFile.name + ">();");
            refreshFunction.content.push_back("    instance->create();");
            refreshFunction.content.push_back("    delete old;");
            refreshFunction.content.push_back("    return true;");
            refreshFunction.content.push_back("} catch(std::runtime_error&) {");
            refreshFunction.content.push_back("    instance.reset(old);");
            refreshFunction.content.push_back("    return false;");
            refreshFunction.content.push_back("}");
            shaderFile.staticFunctions.push_back(refreshFunction);
        } else if (opp == "@uniform") {
            string name, getterSetter, value, type;
            lineStream >> name >> getterSetter >> type >> ws;
            getline(lineStream, value);
            while (value.size() > 0 && value[0] == ' ') {
                value = value.substr(1);
            }
            string varName = ((char) (toUpper(name[0])) + name.substr(1));
            string locName = "loc" + varName;
            uniformFunction.content.push_back(locName + " = getUniformLocation(\"" + name + "\");");
            
            auto [ctype, gltype, basetype, vecsize, array, matrix, primitive] = parseType(type);
            
            if (array) {
                if (!vectorInclude) {
                    shaderFile.headerIncludes.push_back("<vector>");
                    vectorInclude = true;
                }
                ctype = "std::vector<" + basetype + ">";
            } else if (!primitive && !glmInclude) {
                shaderFile.headerIncludes.push_back("<glm/glm.hpp>");
                glmInclude = true;
            }
            gltype = (matrix ? "Matrix" : "") + to_string(vecsize) + gltype + (array ? "v" : "");

            // cout << "Type: " << type << ", cype: " << ctype << endl;
            // bool array = type.size() > 2 && type[3] == 'v';

            if (value != "") {
                if (!array) {
                    uniformFunction.content.push_back("glUniform" + gltype + "(" + locName + ", " + value + ");");
                } else {
                    uniformFunction.content.push_back(ctype + " start" + varName + " = {" + value + "};");
                    uniformFunction.content.push_back("glUniform" + gltype + "(" + locName + ", start" + varName +
                                                      ".size() / " + to_string(vecsize) + ", &start" + varName +
                                                      "[0]);");
                }
            }
            if (getterSetter == "gs" || getterSetter == "g") {
                Function getter(ctype, "get" + varName + "()", true, true, false);
                getter.content.push_back("return " + name + ";");
                if (primitive && !array) {
                    shaderFile.privateMembers.push_back({ctype, name, value});
                } else {
                    shaderFile.privateMembers.push_back({ctype, name, value == "" ? "" : ("{" + value + "}")});
                }
                shaderFile.publicFunctions.push_back(getter);
            }
            if (getterSetter == "gs" || getterSetter == "s") {
                if (!array) {
                    Function setter("void", "set" + varName + "(" + ctype + " " + name + "_)", false, !matrix && !array,
                                    false);
                    if (primitive) {
                        setter.content.push_back("glUniform" + gltype + "(" + locName + ", " + name + "_);");
                    } else if (!matrix) {
                        string valuestr;
                        for (int i = 0; i < vecsize; i++) {
                            valuestr += name + "_[" + to_string(i) + "]" + (i < vecsize - 1 ? ", " : "");
                        }
                        setter.content.push_back("glUniform" + gltype + "(" + locName + ", " + valuestr + ");");
                    } else {
                        setter.content.push_back("glUniform" + gltype + "v(" + locName + ", 1, false, glm::value_ptr(" +
                                                 name + "_));");
                    }
                    if (getterSetter == "gs") {
                        setter.content.push_back("this->" + name + " = " + name + "_;");
                    }
                    shaderFile.publicFunctions.push_back(setter);
                } else {
                    Function setter("void", "set" + varName + "(" + ctype + " " + name + "_)", false, false, false);
                    setter.content.push_back("glUniform" + gltype + "(" + locName + ", " + name + "_.size() / " +
                                             to_string(vecsize) + ", &" + name + "_[0]);");

                    if (getterSetter == "gs") {
                        setter.content.push_back("this->" + name + " = " + name + "_;");
                    }
                    shaderFile.publicFunctions.push_back(setter);
                }
            }
            if (getterSetter == "gs") {
                if (!array) {
                    if (type == "int" || type == "uint") {
                        inspectFunction.content.push_back("RegisterUniform(" + name + ", " + varName + ", int);");
                    } else if (type == "float") {
                        inspectFunction.content.push_back("RegisterUniform(" + name + ", " + varName + ", float);");
                    } else if (!primitive && type[0] == 'v') {
                        inspectFunction.content.push_back("RegisterUniformv(" + name + ", " + varName + ", " +
                                                          to_string(vecsize) + ");");
                        shaderFile.privateMembers.push_back({"Inspector", name + "_inspector", ""});
                    }
                }
            }

            shaderFile.privateMembers.push_back({"unsigned int", locName, ""});
        } else if (opp == "@attribute") {
            string name, value, type;
            lineStream >> name >> value;
            attribFunction.content.push_back("bindAttribute(" + value + ", \"" + name + "\");");
            if (lineStream >> type) {
                TypeInfo info = parseType(type);
                string varName = ((char) (toUpper(name[0])) + name.substr(1));
                Function setBuffer("void", "set" + varName + "(const std::vector<" + info.basetype + ">& " + name + "Data)", false, false, false);
                setBuffer.content.push_back("setBuffer(" + value + ", &" + name + ", " + to_string(info.vecsize) + ", " + name + "Data, GL_STATIC_DRAW);");
                modelFile.privateMembers.push_back({"uint", name, "0"});
                modelFile.publicFunctions.push_back(setBuffer);
            }
        } else if (opp == "@include") {
            string name;
            lineStream >> ws;
            getline(lineStream, name);
            shaderFile.sourceIncludes.push_back(name);
        } else if (opp == "@hinclude") {
            string name;
            lineStream >> ws;
            getline(lineStream, name);
            shaderFile.headerIncludes.push_back(name);
        } else if (opp == "@function") {
            string scope, modifier, type, nameAndArgs;
            lineStream >> scope;
            bool constant = false, virt = false, iline = false;
            while (true) {
                lineStream >> modifier;
                if (modifier == "const") {
                    constant = true;
                } else if (modifier == "virtual") {
                    virt = true;
                } else if (modifier == "inline") {
                    iline = true;
                } else {
                    type = modifier;
                    break;
                }
            }
            lineStream >> ws;
            getline(lineStream, nameAndArgs);
            Function func(type, nameAndArgs, constant, iline, virt);
            string funcLine;
            while (getline(input, funcLine), funcLine.size() > 0) {
                if (funcLine == "@endfunction") {
                    break;
                } else {
                    func.content.push_back(funcLine);
                }
            }
            if (scope == "public") {
                shaderFile.publicFunctions.push_back(func);
            } else if (scope == "protected") {
                shaderFile.protectedFunctions.push_back(func);
            } else if (scope == "private") {
                shaderFile.privateFunctions.push_back(func);
            }
        } else if (opp == "@member") {
            string scope, type, name, value, gettersetter;
            lineStream >> scope >> gettersetter >> type >> name >> ws;
            getline(lineStream, value);
            vector<memberVariable>* members = nullptr;
            vector<Function>* functions = &shaderFile.publicFunctions;
            if (scope == "public") {
                members = &shaderFile.publicMembers;
            } else if (scope == "protected") {
                members = &shaderFile.protectedMembers;
            } else if (scope == "private") {
                members = &shaderFile.privateMembers;
            }
            if (members && functions) {
                members->push_back({type, name, value});
                string varName = ((char) (toupper(name[0])) + name.substr(1));
                if (gettersetter == "g" || gettersetter == "gs") {
                    Function getter(type, "get" + varName + "()", true, true, false);
                    getter.content.push_back("return " + name + ";");
                    functions->push_back(getter);
                }
                if (gettersetter == "s" || gettersetter == "gs") {
                    Function setter("void", "set" + varName + "(" + type + " " + name + ")", false, true, false);
                    setter.content.push_back("this-> " + name + " = " + name + ";");
                    functions->push_back(setter);
                }
            } else {
                cerr << "Invalid scope: " << scope << endl;
            }
        }
    }
    shaderFile.protectedFunctions.push_back(uniformFunction);
    shaderFile.protectedFunctions.push_back(attribFunction);
    shaderFile.protectedFunctions.push_back(inspectFunction);
    input.close();
    return {shaderFile, modelFile};
}
