#include <algorithm>
#include <bitset>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>

int instructionCounter{};
int lineCounter{};

struct CInstructionContent {
  std::string comp;
  std::string dest;
  std::string jump;
};

enum class Type {
  COMMENT,
  A_INSTRUCTION,
  C_INSTRUCTION,
  L_INSTRUCTION, // (xxx)
};

class Parser {
public:
  std::string AVal{};
  CInstructionContent CVal{};
  Type instructionType;

  bool isCOperation{};

  Parser(const std::string &line) {
    instructionType = getInstructionType(line);
    std::cout << line << '\n';

    if (instructionType == Type::A_INSTRUCTION) {
      isCOperation = false;
      parseAInstruction(line);
      std::cout << AVal << '\n';
    }

    if (instructionType == Type::C_INSTRUCTION) {
      isCOperation = true;
      parseCInstruction(line);
      std::cout << CVal.dest << ' ' << CVal.comp << ' ' << CVal.jump << '\n';
    }
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
    if (parsed.isCOperation) {
      encodeCOperation(parsed.CVal);
    } else {
      encodeAOperation(parsed.AVal);
    }

    std::cout << binary << '\n';
    std::cout << "\n---------\n\n";
  };

private:
  void encodeCOperation(const CInstructionContent &content) {
    // decode comp field
    int aComp{0};
    if (content.comp.find('M') != std::string::npos) {
      aComp = 1; // fed the ALU with M if M used
    }

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

class SymbolTable {};

int main(int argc, char *argv[]) {

  std::ifstream in(argv[1]);
  std::ofstream out;
  out.open("out.hack");
  if (!in.is_open()) {
    std::cerr << "Unable to open asm file: " << argv[1];
    return 1;
  }

  std::string line;
  while (std::getline(in, line, '\n')) {
    ++lineCounter;
    line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
    if (line.empty())
      continue;
    std::cout << line << '\n';

    Parser parsed(line);
    if (parsed.instructionType == Type::A_INSTRUCTION ||
        parsed.instructionType == Type::C_INSTRUCTION) {
      Code code(parsed);
      out << code.binary << '\n';
    }
  }
}
