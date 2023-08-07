// 5 Stage MIPS pipeline with bypassing
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
		int reg1val,reg2val,reg3val,regwrite,jtype,ALUval,PC;
    }info;
	info L0,L1,L2,L3,L4;

	int registers[32] = {0}, PCcurr = 0, PCnext, Jumpto;
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

    // checking errors in input of sw/lw
	int lwswerrorcheck(std::string location)
	{
		if (location.back() == ')')
		{
			try
			{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg))
					return -3;
				int address = registers[registerMap[reg]] + offset;
				return 0;
			}
			catch (std::exception &e)
			{
				return -4;
			}
		}
		try
		{
			int address = stoi(location);
			return 0;
		}
		catch (std::exception &e)
		{
			return -4;
		}
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
		return latch;
	}

	// execute the commands sequentially
	void executeCommandsPipelined_Bypass()
	{
        int IF=0, ID=-1, EX=-2, MEM=-3, WB=-4;

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

            int EXnext=ID;
            int IDnext=PCcurr;
            int MEMnext=EX;

            info* L1next=&L0;
            info* L2next=&L1;
            info* L3next=&L2;

			bool WBrun = WB>=0 && WB< size;
			bool MEMrun = MEM>=0 && MEM<size;
			bool EXrun = EX>=0 && EX<size;
			bool IDrun = ID>=0 && ID<size;

			if(WBrun){
                // std::cout<<"WB is doing instruction "<<L4.command<<"\n";
				if(L4.regwrite){
					registers[registerMap[L4.reg1]]=L4.ALUval;
				}
			}

			if(MEMrun){
                // std::cout<<"MEM is doing instruction "<<L3.command<<"\n";
				if(L3.command=="sw"){
                    int op1=registers[registerMap[L3.reg1]];
					data[L3.ALUval]=op1;
				}
				else if (L3.command=="lw"){
					L3.ALUval=data[L3.ALUval];
				}
			}

			exit_code ret1 = (exit_code) 0;
			if(EXrun){
                // std::cout<<"EX is doing instruction "<<L2.command<<"\n";
				if(L2.command == "lw" || L2.command=="sw"){
                    if(L2.reg2.back()==')'){
                        std::string rg2=getreg(L2.reg2);
                        int op1=L2.reg2val;
                        bool op1equal=rg2==L3.reg1;
                        if(L3.regwrite==1 && MEMrun &&(op1equal)){
                                op1=L3.ALUval;
                        }
                        if(L4.regwrite==1 && WBrun && !(L3.reg1 == L4.reg1)){
                            if(L4.reg1==rg2){
                                op1=L4.ALUval;
                            }
                        }
                        int address=locateAddress(op1,L2.reg2);
                        if(address==-3){
                            ret1=(exit_code)3;
                        }
                        else{
                            L2.ALUval=address;
                        }
                    }
                    else{
                        int address=locateAddress(0,L2.reg2);
                        if(address==-3){
                            ret1=(exit_code)3;
                        }
                        else{
                            L2.ALUval=address;
                        }
                    }
				}
				else if (L2.command == "addi"){
                    int op1=L2.reg2val;
                    bool op1equal=L2.reg2==L3.reg1;
                    if(L3.regwrite==1 && MEMrun &&(op1equal)){
                            op1=L3.ALUval;
                    }
                    if(L4.regwrite==1 && WBrun && !(L3.reg1 == L4.reg1)){
                        if(L4.reg1==L2.reg2){
                            op1=L4.ALUval;
                        }
                    }
                    L2.ALUval = op1 + stoi(L2.reg3);
				}
				else if(L2.command=="beq"||L2.command=="bne"){
					int op1=L2.reg1val;
					int op2=L2.reg2val;
					bool op1equal=L2.reg1==L3.reg1;
                    bool op2equal=L2.reg2==L3.reg1;
                    if(L3.regwrite==1 && MEMrun &&(op1equal||op2equal)){
                        if(op1equal){
                            op1=L3.ALUval;
                        }
                        if(op2equal){
                            op2=L3.ALUval;
                        }
                    }
                    if(L4.regwrite==1 && WBrun && !(L4.reg1 == L3.reg1)){
                        if(L4.reg1==L2.reg1){
                            op1=L4.ALUval;
                        }
                        if(L4.reg1==L2.reg2){
                            op2=L4.ALUval;
                        }
                    }
					L2.ALUval =(int)(instructions[L2.command](*this,std::to_string(op1),std::to_string(op2),L2.reg3));
					if(Jumpto==-1){
						PCnext=L2.PC+1;
					}
					else{
						PCnext=Jumpto;
					}
				}
				else if(L2.jtype==0){
                    int op1=L2.reg2val;
                    int op2=L2.reg3val;
                    bool op1equal=L2.reg2==L3.reg1;
                    bool op2equal=L2.reg3==L3.reg1;
                    if(L3.regwrite==1 && MEMrun &&(op1equal||op2equal)){
                        if(op1equal){
                            op1=L3.ALUval;
                        }
                        if(op2equal){
                            op2=L3.ALUval;
                        }
                    }
                    if(L4.regwrite==1 && WBrun && !(L4.reg1 == L3.reg1)){
                        if(L4.reg1==L2.reg2){
                            op1=L4.ALUval;
                        }
                        if(L4.reg1==L2.reg3){
                            op2=L4.ALUval;
                        }
                    }
					L2.ALUval =(int)(instructions[L2.command](*this,L2.reg1,std::to_string(op1),std::to_string(op2)));
				}
				if(ret1 != SUCCESS){
						handleExit(ret1, clockCycles);
						return;
				}

			}

			exit_code ret = (exit_code) 0;
			if(IDrun){
                // std::cout<<"ID IS doing instruction "<<L1.command<<"\n";
				std::string a=L1.command;
				if(a=="add" || a=="sub" || a=="mul"|| a=="slt"){
					L1.regwrite=1;
					L1.jtype=0;
					if (!checkRegisters({L1.reg1, L1.reg2, L1.reg3}) || registerMap[L1.reg1] == 0){
						ret=(exit_code) 1;
					}
					else{
						ret=(exit_code) 0;
						L1.reg1val=registers[registerMap[L1.reg1]];
						L1.reg2val=registers[registerMap[L1.reg2]];
						L1.reg3val=registers[registerMap[L1.reg3]];
					}
				}
				else if (a=="beq" || a=="bne"){
					if (!checkLabel(L1.reg3))
							ret = (exit_code) 4;
					if (address.find(L1.reg3) == address.end() || address[L1.reg3] == -1)
							ret = (exit_code) 2;
					if (!checkRegisters({L1.reg1, L1.reg2}))
							ret = (exit_code) 1;

					L1.regwrite=0;
					L1.jtype=1;
					L1.reg1val=registers[registerMap[L1.reg1]];
					L1.reg2val=registers[registerMap[L1.reg2]];
					IDnext=-1;
					PCnext=-1;
					L1next=NULL;
				}
				else if (a == "j"){
					L1.regwrite=0;
					L1.jtype=1;
					ret = (exit_code) instructions[L1.command](*this,L1.reg1,L1.reg2, L1.reg3);
                    PCnext=Jumpto;
				}
				else if(a=="lw"){
					L1.regwrite=1;
					L1.jtype=0;
					if (!checkRegister(L1.reg1) || registerMap[L1.reg1] == 0)
						ret = (exit_code) 1;
					else{
						ret = (exit_code) lwswerrorcheck(L1.reg2);
                        if(L1.reg2.back()==')'){
                            L1.reg2val=registers[registerMap[getreg(L1.reg2)]];
                        }
					}
				}
				else if(a=="sw"){
					L1.regwrite=0;
					L1.jtype=0;
					if (!checkRegister(L1.reg1))
						ret = (exit_code) 1;
					else
                        ret = (exit_code) lwswerrorcheck(L1.reg2);
						L1.reg1val=registers[registerMap[L1.reg1]];
                        if(L1.reg2.back()==')'){
                            L1.reg2val=registers[registerMap[getreg(L1.reg2)]];
                        }
				}
				else if(a=="addi"){
					L1.regwrite=1;
					L1.jtype=0;
					if (!checkRegisters({L1.reg1, L1.reg2}) || registerMap[L1.reg1] == 0)
					{
						ret=(exit_code) 1;
					}

					else {
                        try
                            {
                                stoi(L1.reg3);
                                ret =(exit_code) 0;
                            }
                            catch (std::exception &e)
                            {
                                ret =(exit_code) 4;
                            }
						L1.reg1val=registers[registerMap[L1.reg1]];
						L1.reg2val=registers[registerMap[L1.reg2]];
					}
				}

				if(ret != SUCCESS){
					handleExit(ret, clockCycles);
					return;
				}
                else{
                    std::string rg2=L1.reg2;
			        std::string rg3=L1.reg3;
                    if(L1.command=="beq" || L1.command=="bne"){
                        rg3=L1.reg1;
						if(L2.command=="lw" && IDrun && EXrun&& (L2.reg1==rg2 || L2.reg1==rg3) ){
                            EXnext=-1;
                            L2next=NULL;
                            IDnext=ID;
                            L1next=&L1;
                            PCnext=PCcurr;
                        }
                    }
                    else if(L1.regwrite==1 && !(L1.command=="lw")){
                        if(L2.command=="lw" && IDrun && EXrun&& (L2.reg1==rg2 || L2.reg1==rg3) ){
                            EXnext=-1;
                            L2next=NULL;
                            IDnext=ID;
                            L1next=&L1;
                            PCnext=PCcurr;
                        }
                    }
                    else if(L1.command=="lw"|| L1.command=="sw"){
                        if(L1.reg2.back()==')'&& L2.command=="lw" && EXrun && IDrun && (L2.reg1==getreg(L1.reg2))){
                            EXnext=-1;
                            L2next=NULL;
                            IDnext=ID;
                            L1next=&L1;
                            PCnext=PCcurr;
                        }
                    }
                }
			}

			bool IFrun = (PCcurr >=0 && PCcurr < size);

            // loop exit condition
			if (!(WBrun || MEMrun || EXrun || IDrun || IFrun)){
				break;
			}

            if(IFrun){
				std::vector<std::string> &command = commands[PCcurr];
                // std::cout<<"IF is doing instruction "<<command[0]<<"\n";
				L0.command=command[0];
				L0.reg1=command[1];
				L0.reg2=command[2];
				L0.reg3=command[3];
                if(command[0]=="j" or command[0]=="beq" or command[0]=="bne"){
					PCnext=-1;
					L0.PC = PCcurr;
				}
				++commandCount[PCcurr];
			}

			printRegisters(clockCycles);
			if (MEMrun && L3.command=="sw"){
				std::cout << 1 << " " << L3.ALUval << " " << data[L3.ALUval] << "\n";
			}
			else{
				std::cout << 0 << "\n";
			}

			PCcurr = PCnext;
			L4=L3;
			if (L3next != NULL){
				L3=*L3next;
			}
			else{
				L3 = clearlatch(L3);
			}
			if (L2next != NULL){
				L2=*L2next;
			}
			else{
				L2 = clearlatch(L2);
			}
			if (L1next != NULL){
				L1=*L1next;
			}
			else{
				L1 = clearlatch(L1);
			}
			WB=MEM;
            MEM=MEMnext;
            EX=EXnext;
            ID=IDnext;
            IF=PCnext;
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