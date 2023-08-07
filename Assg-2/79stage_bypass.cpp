// 7-9 Stage MIPS pipeline with bypassing
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <iostream>
#include <boost/tokenizer.hpp>

// A struct to represent the latches, register, commands and memory addresses
struct MIPS_Architecture
{

    typedef struct{
        std::string command;
        std::string reg1,reg2,reg3;
		int reg1val,reg2val,reg3val,regwrite,jtype,ALUval,PC,type9;
    }info;
	info L0,L1,L2,L3,L4,L5,L6,L7,L8;

	int registers[32] = {0}, PCcurr = 0, PCnext,Jumpto;
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, std::string, std::string)>> instructions;
	std::unordered_map<std::string, int> registerMap, address;

	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};

	std::vector<std::vector<std::string>> commands;
	std::vector<int> commandCount;
	enum exit_code
	{
		SUCCESS = 0,
		INVALID_REGISTER,
		INVALID_LABEL,
		INVALID_ADDRESS,
		SYNTAX_ERROR,
		MEMORY_ERROR
	};

    // constructor to initialise the instruction set
	MIPS_Architecture(std::ifstream &file)
	{
		instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}};
		for (int i = 0; i < 32; ++i)
			registerMap["$" + std::to_string(i)] = i;
		registerMap["$zero"] = 0;
		registerMap["$at"] = 1;
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		for (int i = 0; i < 4; ++i)
			registerMap["$a" + std::to_string(i)] = i + 4;
		for (int i = 0; i < 8; ++i)
			registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16;
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;

		constructCommands(file);
		commandCount.assign(commands.size(), 0);
	}

	// perform add operation
	int add(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a + b; });
	}

	// perform subtraction operation
	int sub(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a - b; });
	}

	// perform multiplication operation
	int mul(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a * b; });
	}

	// perform the binary operation
	int op(std::string r1, std::string r2, std::string r3, std::function<int(int, int)> operation)
	{
		int val= operation(stoi(r2), stoi(r3));
		return val;
	}

	// perform the beq operation
	int beq(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a == b; });
	}

	// perform the bne operation
	int bne(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a != b; });
	}

	// implements beq and bne by taking the comparator
	int bOP(std::string r1, std::string r2, std::string label, std::function<bool(int, int)> comp)
	{
		Jumpto = comp(stoi(r1), stoi(r2)) ? address[label] : PCcurr;
		return 0;
	}

	// implements slt operation
	int slt(std::string r1, std::string r2, std::string r3)
	{
		if (stoi(r2) < stoi(r3)){
			return 1;
		}
		return 0;
	}

	// perform the jump operation
	int j(std::string label, std::string unused1 = "", std::string unused2 = "")
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		Jumpto = address[label];
		return 0;
	}

	// find the 2nd register say $t2 in sw and lw from say 8($t2)
    std::string getreg(std::string location){
        int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
        std::string reg = location.substr(lparen + 1);
        reg.pop_back();
        return reg;
    }

	// return memory address of sw and lw by either adding inpint and offset or direct address number
	int locateAddress(int inpint,std::string location)
	{
		if (location.back() == ')')
		{
			int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
			std::string reg = location.substr(lparen + 1);
			reg.pop_back();
			int address = inpint + offset;
			if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
				return -3;
			return address / 4;
		}
		int address = stoi(location);
		if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
			return -3;
		return address / 4;
	}

	// checks if label is valid
	inline bool checkLabel(std::string str)
	{
		return str.size() > 0 && isalpha(str[0]) && all_of(++str.begin(), str.end(), [](char c)
														   { return (bool)isalnum(c); }) &&
			   instructions.find(str) == instructions.end();
	}

	// checks if the register is a valid one
	inline bool checkRegister(std::string r)
	{
		return registerMap.find(r) != registerMap.end();
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(std::vector<std::string> regs)
	{
		return std::all_of(regs.begin(), regs.end(), [&](std::string r)
						   { return checkRegister(r); });
	}

	/*
		handle all exit codes:
		0: correct execution
		1: register provided is incorrect
		2: invalid label
		3: unaligned or invalid address
		4: syntax error
		5: commands exceed memory limit
	*/
	void handleExit(exit_code code, int cycleCount)
	{
		// std::cout << '\n';
		switch (code)
		{
		case 1:
			std::cerr << "Invalid register provided or syntax error in providing register\n";
			break;
		case 2:
			std::cerr << "Label used not defined or defined too many times\n";
			break;
		case 3:
			std::cerr << "Unaligned or invalid memory address specified\n";
			break;
		case 4:
			std::cerr << "Syntax error encountered\n";
			break;
		case 5:
			std::cerr << "Memory limit exceeded\n";
			break;
		default:
			break;
		}
		// if (code != 0)
		// {
		// 	std::cerr << "Error encountered at:\n";
		// 	for (auto &s : commands[PCcurr])
		// 		std::cerr << s << ' ';
		// 	std::cerr << '\n';
		// }
		// std::cout << "\nFollowing are the non-zero data values:\n";
		// for (int i = 0; i < MAX / 4; ++i)
		// 	if (data[i] != 0)
		// 		std::cout << 4 * i << '-' << 4 * i + 3 << std::hex << ": " << data[i] << '\n'
		// 				  << std::dec;
		// std::cout << "\nTotal number of cycles: " << cycleCount << '\n';
		// std::cout << "Count of instructions executed:\n";
		// for (int i = 0; i < (int)commands.size(); ++i)
		// {
		// 	std::cout << commandCount[i] << " times:\t";
		// 	for (auto &s : commands[i])
		// 		std::cout << s << ' ';
		// 	std::cout << '\n';
		// }
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(std::string line)
	{
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		std::vector<std::string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			std::string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			std::string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = std::vector<std::string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != std::string::npos)
		{
			int idx = command[0].find(':');
			std::string label = command[0].substr(0, idx);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command[0] = command[0].substr(idx + 1);
		}
		else if (command[1][0] == ':')
		{
			if (address.find(command[0]) == address.end())
				address[command[0]] = commands.size();
			else
				address[command[0]] = -1;
			command[1] = command[1].substr(1);
			if (command[1] == "")
				command.erase(command.begin(), command.begin() + 2);
			else
				command.erase(command.begin(), command.begin() + 1);
		}
		if (command.empty())
			return;
		if (command.size() > 4)
			for (int i = 4; i < (int)command.size(); ++i)
				command[3] += " " + command[i];
		command.resize(4);
		commands.push_back(command);
	}

	// construct the commands vector from the input file
	void constructCommands(std::ifstream &file)
	{
		std::string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	}

    // returns an empty latch
	info clearlatch(info latch){
		latch.reg1 = "";
		latch.reg2 = "";
		latch.reg3 = "";
		latch.reg1val = 0;
		latch.reg2val = 0;
		latch.reg3val = 0;
		latch.command = "";
		latch.jtype = -1;
		latch.ALUval = 0;
		latch.regwrite = -1;
		latch.PC = -1;
		latch.type9 = -1;
		return latch;
	}

	// execute the commands sequentially
	void executeCommandsPipelined_Bypass()
	{
        int IF1=0, IF2=-1, ID1=-2, ID2=-3, RR=-4, ALU=-5, DM1=-6, DM2=-7, RW=-8;

		if (commands.size() >= MAX / 4)
		{
			handleExit(MEMORY_ERROR, 0);
			return;
		}

		int clockCycles = 0;
        int size=commands.size();

		printRegisters(clockCycles);
		std::cout << 0 << "\n";

        while(true){
			PCnext=PCcurr+1;

			int IF2next = IF1;
			int ID1next = IF2;
			int ID2next = ID1;
			int RRnext = ID2;
			int ALUnext = RR;
			int DM1next = -1;
			int DM2next = -1;
			int RWnext = -1;

            info* L1next=&L0;
            info* L2next=&L1;
            info* L3next=&L2;
			info* L4next=&L3;
			info* L5next=&L4;
			info* L6next=&L5;
			info* L7next=&L6;
			info* L8next=&L7;

			bool IF2run = IF2 >= 0 && IF2 < size;
			bool ID1run = ID1 >= 0 && ID1 < size;
			bool ID2run = ID2 >= 0 && ID2 < size;
			bool RRrun = RR >= 0 && RR < size;
			bool ALUrun = ALU >= 0 && ALU < size;
			bool DM1run = DM1 >=0 && DM1 < size;
			bool DM2run = DM2 >=0 && DM2 < size;
			bool RWrun = RW >= 0 && RW < size;

			if (RWrun){
				// std::cout << "RW is doing instruction " << L8.command << "\n";
				if(L8.regwrite){
					registers[registerMap[L8.reg1]]=L8.ALUval;
				}
			}

			if (DM2run){
				// std::cout << "DM2 is doing instruction " << L7.command << "\n";
				if(L7.command=="sw"){
					data[L7.ALUval]=L7.reg1val;
				}
				else if (L7.command=="lw"){
					L7.ALUval=data[L7.ALUval];
				}
				RWnext = DM2;
				L8next = &L7;
			}

			if(DM1run){
				// std::cout << "DM1 is doing instruction " << L6.command << "\n";
                if(L6.command=="sw"){
                    if(L8.regwrite==1 && RWrun && L8.reg1==L6.reg1){
                        L6.reg1val=L8.ALUval;
                    }
                }
				DM2next = DM1;
				L7next = &L6;
			}

			if(ALUrun){
				// std::cout << "ALU is doing instruction " << L5.command << "\n";
				if (L5.command == "bne" || L5.command == "beq"){
                    int op1=L5.reg1val;
                    int op2=L5.reg2val;
                    if(L8.regwrite==1&& RWrun && (L8.reg1==L5.reg1||L8.reg1==L5.reg2)){
                        if(L8.reg1==L5.reg1){
                            op1=L8.ALUval;
                        }
                        if(L8.reg1==L5.reg2){
                            op2=L8.ALUval;
                        }
                    }
					(instructions[L5.command](*this,std::to_string(op1),std::to_string(op2),L5.reg3));
					if(Jumpto==-1){
						PCnext=L5.PC+1;
						RWnext = ALU;
						L8next = &L5;
					}
					else{
						PCnext=Jumpto;
						RWnext = ALU;
						L8next = &L5;
					}
				}
				else if (L5.type9 == 1){
					int op2=L5.reg2val;
					if (L5.reg2.back()==')'){
                        if(L8.regwrite==1 && RWrun && (L8.reg1==getreg(L5.reg2))){
                            op2=L8.ALUval;
                        }
					}
					int address = locateAddress(op2,L5.reg2);
					L5.ALUval=address;
					DM1next = ALU;
					L6next = &L5;
					if (L5.command=="sw" && L8.regwrite==1 && L8.reg1==L5.reg1){
						L5.reg1val=L8.ALUval;
					}
				}
				else if (L5.command == "addi"){
                    int op2=L5.reg2val;
                    if(L8.regwrite==1&& RWrun && (L8.reg1==L5.reg2)){
                        op2=L8.ALUval;
                    }
					L5.ALUval = op2+ stoi(L5.reg3);
					RWnext = ALU;
					L8next = &L5;
				}
				else{
                    int op1=L5.reg2val;
                    int op2=L5.reg3val;
                    if(L8.regwrite==1&& RWrun && (L8.reg1==L5.reg2||L8.reg1==L5.reg3)){
                        if(L8.reg1==L5.reg2){
                            op1=L8.ALUval;
                        }
                        if(L8.reg1==L5.reg3){
                            op2=L8.ALUval;
                        }
                    }
					L5.ALUval =(int)(instructions[L5.command](*this,L5.reg1,std::to_string(op1),std::to_string(op2)));
					RWnext = ALU;
					L8next = &L5;
				}
			}

			if(RRrun){
				// std::cout << "RR is doing instruction " << L4.command << "\n";
				if (L4.command == "bne" || L4.command == "beq"){
					L4.reg1val = registers[registerMap[L4.reg1]];
					L4.reg2val = registers[registerMap[L4.reg2]];
					PCnext = -1;
					RRnext = -1;
					L4next = NULL;
                    if(L8.regwrite==1 && RWrun && (L8.reg1==L4.reg1||L8.reg1==L4.reg2)){
						if(L8.reg1==L4.reg1){
							L4.reg1val=L8.ALUval;
						}
						if(L8.reg1==L4.reg2){
							L4.reg2val=L8.ALUval;
						}
					}
				}
				else if (L4.type9==1){
					L4.reg1val = registers[registerMap[L4.reg1]];
					L4.reg2val = registers[registerMap[getreg(L4.reg2)]];
                    if(L8.regwrite==1 && RWrun && L8.reg1==getreg(L4.reg2)){
						L4.reg2val=L8.ALUval;
					}
				}
				else{
					L4.reg2val = registers[registerMap[L4.reg2]];
					L4.reg3val = registers[registerMap[L4.reg3]];
                    if(L8.regwrite==1 && RWrun && (L8.reg1==L4.reg2||L8.reg1==L4.reg3)){
						if(L8.reg1==L4.reg2){
							L4.reg2val=L8.ALUval;
						}
						if(L8.reg1==L4.reg3){
							L4.reg3val=L8.ALUval;
						}
					}
				}

				//Data Hazards
                if(L4.command=="lw"){

                    if(L4.reg2.back()==')'){
                        if(L5.command=="lw" && ALUrun && L5.reg1==getreg(L4.reg2)){
                            ALUnext=-1;
                            RRnext=RR;
                            ID2next=ID2;
                            ID1next=ID1;
                            IF2next=IF2;
                            L5next=NULL;
                            L4next=&L4;
                            L3next=&L3;
                            L2next=&L2;
                            L1next=&L1;
                            PCnext=PCcurr;
                        }
                        else if(L6.command=="lw"&&DM1run&&L6.reg1==getreg(L4.reg2)){
                            ALUnext=-1;
                            RRnext=RR;
                            ID2next=ID2;
                            ID1next=ID1;
                            IF2next=IF2;
                            L5next=NULL;
                            L4next=&L4;
                            L3next=&L3;
                            L2next=&L2;
                            L1next=&L1;
                            PCnext=PCcurr;
                        }
                    }
                }
                else if(L4.command=="sw"){
                    if(L5.command=="lw" && ALUrun && L5.reg1==L4.reg1){
                        ALUnext=-1;
						RRnext=RR;
						ID2next=ID2;
						ID1next=ID1;
						IF2next=IF2;
						L5next=NULL;
						L4next=&L4;
						L3next=&L3;
						L2next=&L2;
						L1next=&L1;
						PCnext=PCcurr;
                    }
                    if(L4.reg2.back()==')'){
                        if(L5.command=="lw" && ALUrun && L5.reg1==getreg(L4.reg2)){
                            ALUnext=-1;
                            RRnext=RR;
                            ID2next=ID2;
                            ID1next=ID1;
                            IF2next=IF2;
                            L5next=NULL;
                            L4next=&L4;
                            L3next=&L3;
                            L2next=&L2;
                            L1next=&L1;
                            PCnext=PCcurr;
                        }
                        else if(L6.command=="lw"&&DM1run&&L6.reg1==getreg(L4.reg2)){
                            ALUnext=-1;
                            RRnext=RR;
                            ID2next=ID2;
                            ID1next=ID1;
                            IF2next=IF2;
                            L5next=NULL;
                            L4next=&L4;
                            L3next=&L3;
                            L2next=&L2;
                            L1next=&L1;
                            PCnext=PCcurr;
                        }
                    }
                }
                else if(L4.command=="addi"){
                    if(L5.command=="lw" && ALUrun && L5.reg1==L4.reg2){
                            ALUnext=-1;
                            RRnext=RR;
                            ID2next=ID2;
                            ID1next=ID1;
                            IF2next=IF2;
                            L5next=NULL;
                            L4next=&L4;
                            L3next=&L3;
                            L2next=&L2;
                            L1next=&L1;
                            PCnext=PCcurr;
                        }
                        else if(L6.command=="lw"&& DM1run && L6.reg1==L4.reg2){
                            ALUnext=-1;
                            RRnext=RR;
                            ID2next=ID2;
                            ID1next=ID1;
                            IF2next=IF2;
                            L5next=NULL;
                            L4next=&L4;
                            L3next=&L3;
                            L2next=&L2;
                            L1next=&L1;
                            PCnext=PCcurr;
                        }

                }
                else{
                    std::string rg2;
                    std::string rg3;
                    if(L4.command=="bne"||L4.command=="beq"){
                        rg2=L4.reg1;
                        rg3=L4.reg2;
                    }
                    else{
                        rg2=L4.reg2;
                        rg3=L4.reg3;
                    }
                    if(L5.command=="lw" && ALUrun && (L5.reg1==rg2||L5.reg1==rg3)){
                        ALUnext=-1;
                        RRnext=RR;
                        ID2next=ID2;
                        ID1next=ID1;
                        IF2next=IF2;
                        L5next=NULL;
                        L4next=&L4;
                        L3next=&L3;
                        L2next=&L2;
                        L1next=&L1;
                        PCnext=PCcurr;
                    }
                    else if(L6.command=="lw" && DM1run && (L6.reg1==rg2||L6.reg1==rg3)){
                        ALUnext=-1;
                        RRnext=RR;
                        ID2next=ID2;
                        ID1next=ID1;
                        IF2next=IF2;
                        L5next=NULL;
                        L4next=&L4;
                        L3next=&L3;
                        L2next=&L2;
                        L1next=&L1;
                        PCnext=PCcurr;
                    }
                }

				// Structural Hazards
				if (L5.type9 == 1 && L4.type9 == 0 && ALUrun){
					ALUnext=-1;
					RRnext=RR;
					ID2next=ID2;
					ID1next=ID1;
					IF2next=IF2;
					L5next=NULL;
					L4next=&L4;
					L3next=&L3;
					L2next=&L2;
					L1next=&L1;
					PCnext=PCcurr;
				}
				else if (L6.command=="lw" && L4.type9 == 0 && DM1run){
					ALUnext = -1;
					RRnext=RR;
					ID2next=ID2;
					ID1next=ID1;
					IF2next=IF2;
					L5next=NULL;
					L4next=&L4;
					L3next=&L3;
					L2next=&L2;
					L1next=&L1;
					PCnext=PCcurr;
				}
			}

			if(ID2run){
				// std::cout << "ID2 is doing instruction " << L3.command << "\n";
				if(L3.command=="j"){
					L3.regwrite=0;
					L3.jtype=1;
					instructions[L1.command](*this,L1.reg1,L1.reg2, L1.reg3);
					PCnext=Jumpto;
				}
				else if (L3.command=="beq" || L3.command == "bne"){
                    if(ID2next==ID1){
                        L3.regwrite=0;
                        L3.jtype=1;
                        PCnext = -1;
                        ID2next = -1;
                        L3next = NULL;
                    }
				}
				else if (L3.command=="sw"){
					L3.regwrite=0;
					L3.jtype=0;
				}
				else{
					L3.regwrite=1;
					L3.jtype=0;
				}

			}

			if(ID1run){
				// std::cout << "ID1 is doing instruction " << L2.command << "\n";
				if(L2.command=="j" || L2.command=="beq" || L2.command=="bne"){
					if(ID1next==IF2){
						PCnext=-1;
						ID1next=-1;
						L2next = NULL;
						L2.type9=0;
					}
				}
				else if(L2.command=="lw" || L2.command=="sw"){
					L2.type9=1;
				}
				else{
					L2.type9=0;
				}
			}

            if(IF2run){
				// std::cout << "IF2 is doing instruction " << L1.command << "\n";
				if(L1.command=="j" || L1.command=="beq" || L1.command=="bne"){
					if(IF2next==IF1){
						PCnext=-1;
						IF2next=-1;
						L1next = NULL;
					}
				}
			}

			bool IF1run = (PCcurr >=0 && PCcurr < size);

			if(IF1run){
				std::vector<std::string> &command = commands[PCcurr];
                // std::cout<<"IF1 is doing instruction "<<command[0]<<"\n";
				L0.command=command[0];
				L0.reg1=command[1];
				L0.reg2=command[2];
				L0.reg3=command[3];
				L0.PC=PCcurr;
				if(L0.command=="j" || L0.command=="beq" || L0.command=="bne"){
					PCnext=-1;
				}
				++commandCount[PCcurr];
			}

            // loop exit condition
			if (!(RWrun || DM2run || DM1run || ALUrun || RRrun || ID2run || ID1run || IF2run || IF1run)){
				break;
			}

			printRegisters(clockCycles);
			if (DM2run && L7.command=="sw"){
				std::cout << 1 << " " << L7.ALUval << " " << data[L7.ALUval] << "\n";
			}
			else{
				std::cout << 0 << "\n";
			}

			PCcurr = PCnext;
			if (L8next != NULL){
				L8=*L8next;
			}
			else{
				L8=clearlatch(L8);
			}
			if (L7next != NULL){
				L7=*L7next;
			}
			else{
				L7=clearlatch(L7);
			}
			if (L6next != NULL){
				L6=*L6next;
			}
			else{
				L6=clearlatch(L6);
			}
			if (L5next != NULL){
				L5=*L5next;
			}
			else{
				L5=clearlatch(L5);
			}
			if (L4next != NULL){
				L4=*L4next;
			}
			else{
				L4=clearlatch(L4);
			}
			if (L3next != NULL){
				L3=*L3next;
			}
			else{
				L3=clearlatch(L3);
			}
			if (L2next != NULL){
				L2=*L2next;
			}
			else{
				L2=clearlatch(L2);
			}
			if (L1next != NULL){
				L1=*L1next;
			}
			else{
				L1=clearlatch(L1);
			}

			RW=RWnext;
			DM2=DM2next;
			DM1=DM1next;
			ALU=ALUnext;
			RR=RRnext;
			ID2=ID2next;
			ID1=ID1next;
			IF2=IF2next;
			IF1=PCnext;
			++clockCycles;
        }
		handleExit(SUCCESS, clockCycles);
	}

	// print the register data
	void printRegisters(int clockCycle)
	{
		// std::cout << "Cycle number: " << clockCycle << '\n';
		for (int i = 0; i < 32; ++i)
			std::cout << registers[i] << ' ';
		std::cout << std::dec << '\n';
	}
};

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Required argument: file_name\n./MIPS_interpreter <file name>\n";
		return 0;
	}
	std::ifstream file(argv[1]);
	MIPS_Architecture *mips;
	if (file.is_open())
		mips = new MIPS_Architecture(file);
	else
	{
		std::cerr << "File could not be opened. Terminating...\n";
		return 0;
	}
	mips->executeCommandsPipelined_Bypass();
	return 0;
}