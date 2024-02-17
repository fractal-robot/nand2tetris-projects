// the code if working for every test program exept for the largest one: pong,
// and does not include the somewhat advanced features of file manipulation, but
// I consider this project done

#include <algorithm>
#include <bitset>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

long instructionCounter{};

struct CInstructionContent {
  std::string comp;
  std::string dest;
  std::string jump;
};

enum class Type : char {
  COMMENT,
  A_INSTRUCTION,
  C_INSTRUCTION,
  L_INSTRUCTION, // (xxx)
};

class Parser {
public:
  std::string AVal{};
  std::string LVal{};
  CInstructionContent CVal{};
  Type instructionType;

  Parser(const std::string &line) {
    instructionType = getInstructionType(line);
    if (instructionType == Type::A_INSTRUCTION) {
      parseAInstruction(line);
    }

    if (instructionType == Type::C_INSTRUCTION)
      parseCInstruction(line);

    if (instructionType == Type::L_INSTRUCTION)
      parseLInstruction(line);
  }

private:
  Type getInstructionType(const std::string &line) {
    if (line.find("//") != std::string::npos)
      return Type::COMMENT;
    if (line.find('@') != std::string::npos)
      return Type::A_INSTRUCTION;
    if (line.find('(') != std::string::npos)
      return Type::L_INSTRUCTION;

    return Type::C_INSTRUCTION;
  }

  void parseAInstruction(const std::string &line) {
    for (const char c : line) {
      if (c != '@')
        AVal += c;
    }
  }

  void parseLInstruction(const std::string &line) {
    for (const char c : line) {
      if (c != '(' && c != ')')
        LVal += c;
    }
  }

  void parseCInstruction(const std::string &line) {
    // if dest empty, = ommited
    bool isDest{false};
    // if jump empty, ; ommited
    bool isJump{false};

    if (line.find('=') != std::string::npos)
      isDest = true;
    if (line.find(';') != std::string::npos)
      isJump = true;

    std::size_t charIndex{0};
    if (isDest) {
      for (const char c : line) {
        ++charIndex;
        if (c == '=')
          break;
        CVal.dest += c;
      }
    }

    for (std::size_t i{charIndex}; i < std::size(line); i++) {
      ++charIndex;
      if (isJump && line[i] == ';')
        break;
      CVal.comp += line[i];
    }

    if (isJump) {
      for (std::size_t i{charIndex}; i < std::size(line); i++) {
        CVal.jump += line[i];
      }
    }
  }
};

class Code {
public:
  std::string binary;

  Code(const Parser &parsed) {
    if (parsed.instructionType == Type::C_INSTRUCTION)
      encodeCOperation(parsed.CVal);
    else
      encodeAOperation(parsed.AVal);
  };

private:
  void encodeCOperation(const CInstructionContent &content) {
    // decode comp field
    int aComp{0};
    if (content.comp.find('M') != std::string::npos)
      aComp = 1; // fed the ALU with M if M used

    // populate binary instruction

    binary += "111";
    binary += std::to_string(aComp);

    std::unordered_map<std::string, std::string> comp{
        {"0", "101010"},   {"1", "111111"},   {"-1", "111010"},
        {"D", "001100"},   {"A", "110000"},   {"M", "110000"},
        {"!D", "001101"},  {"!A", "110001"},  {"!M", "110001"},
        {"-D", "001111"},  {"-A", "110011"},  {"-M", "110011"},
        {"D+1", "011111"}, {"A+1", "110111"}, {"M+1", "110111"},
        {"D-1", "001110"}, {"A-1", "110010"}, {"M-1", "110010"},
        {"D+A", "000010"}, {"D+M", "000010"}, {"D-A", "010011"},
        {"D-M", "010011"}, {"A-D", "000111"}, {"M-D", "000111"},
        {"D&A", "000000"}, {"D&M", "000000"}, {"D|A", "010101"},
        {"D|M", "010101"},
    };
    binary += comp[content.comp];

    content.dest.find('A') != std::string::npos ? binary += '1' : binary += '0';
    content.dest.find('D') != std::string::npos ? binary += '1' : binary += '0';
    content.dest.find('M') != std::string::npos ? binary += '1' : binary += '0';

    std::unordered_map<std::string, std::string> jump{
        {"JGT", "001"}, {"JEQ", "010"}, {"JGE", "011"}, {"JLT", "100"},
        {"JNE", "101"}, {"JLE", "110"}, {"JMP", "111"},
    };
    content.jump != "" ? binary += jump[content.jump] : binary += "000";
  }

  void encodeAOperation(const std::string &content) {
    binary += '0';
    binary += std::bitset<15>(std::stoi(content)).to_string();
  }
};

class SymbolTable {
public:
  std::unordered_map<std::string, long> symbols{
      {"SP", 0},   {"LCL", 1},        {"ARG", 2},     {"THIS", 3},
      {"THAT", 4}, {"SCREEN", 16384}, {"KBD", 24576},
  };

  long mallocAddr{15}; // start at address 16?

  SymbolTable() {
    for (int i{0}; i <= 15; ++i)
      symbols["R" + std::to_string(i)] = i;
  }

  void getSymbols(Parser &parsed) {
    if (parsed.instructionType == Type::L_INSTRUCTION)
      ISymbols(parsed);
    else if (parsed.instructionType == Type::A_INSTRUCTION &&
             !std::isdigit(parsed.AVal[0])) {
      ASymbols(parsed);
    }
  }

private:
  void ISymbols(Parser &parsed) { symbols[parsed.LVal] = instructionCounter; }

  void ASymbols(Parser &parsed) {
    if (symbols.count(parsed.AVal) == 0) {
      symbols[parsed.AVal] = mallocAddr;
      ++mallocAddr;
    }
  }
};

int main(int argc, char *argv[]) {
  std::ifstream in(argv[1]);
  std::ofstream out;
  out.open("out.hack");
  if (!in.is_open()) {
    std::cerr << "Unable to open asm file: " << argv[1];
    return 1;
  }

  SymbolTable symbols;

  std::string line;
  std::vector<Parser> instructions;
  while (std::getline(in, line, '\n')) {
    line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
    std::cout << line << std::endl;
    if (line.empty())
      continue;

    Parser parsed(line);
    symbols.getSymbols(parsed);

    if (parsed.instructionType == Type::A_INSTRUCTION ||
        parsed.instructionType == Type::C_INSTRUCTION) {
      instructionCounter++;
      instructions.emplace_back(parsed);
    }
  }

  for (Parser &instruction : instructions) {
    if (instruction.instructionType == Type::A_INSTRUCTION &&
        !isdigit(instruction.AVal[0]))
      instruction.AVal = std::to_string(symbols.symbols[instruction.AVal]);
    Code code(instruction);
    out << code.binary << '\n';
    std::cout << code.binary << "\n\n----\n\n";
  }
}
