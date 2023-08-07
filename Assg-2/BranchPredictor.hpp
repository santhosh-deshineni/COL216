#ifndef __BRANCH_PREDICTOR_HPP__
#define __BRANCH_PREDICTOR_HPP__
#include <vector>
#include <bitset>
#include <cassert>

struct BranchPredictor {
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, bool taken) = 0;
};

struct SaturatingBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> table;
    SaturatingBranchPredictor(int value) : table(1 << 14, value) {}

    bool predict(uint32_t pc) {
        uint32_t address=pc & 0x3FFF;
        std::bitset<2> bit=table[address];
        if(bit==std::bitset<2>("10")||bit==std::bitset<2>("11")){
            return true;
        }
        return false;
    }

    void update(uint32_t pc, bool taken) {
        uint32_t address=pc & 0x3FFF;
        std::bitset<2> bit=table[address];
        int value=bit.to_ulong();
        if(taken){
            if(value<3){
                value++;
            }
        }
        else{
            if(value>0){
                value--;
            }
        }
        std::bitset<2> newvalue(value);
        table[address]=newvalue;
    }
};

struct BHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    BHRBranchPredictor(int value) : bhrTable(1 << 2, value), bhr(value) {}

    bool predict(uint32_t pc) {
        int index=bhr.to_ulong();
        int value=(bhrTable[index]).to_ulong();
        if(value>=2){
            return true;
        }
        return false;
    }

    void update(uint32_t pc, bool taken) {
        int index=bhr.to_ulong();
        int value=(bhrTable[index]).to_ulong();
        if(taken){
            if(value<3){
                value++;
            }
            bhr=(bhr<<1);
            bhr.set(0,1);
        }
        else{
            if(value>=1){
                value--;
            }
             bhr=(bhr<<1);
            bhr.set(0,0);
        }
        std::bitset<2> newvalue(value);
        bhrTable[index]=newvalue;
    }
};

struct SaturatingBHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    std::vector<std::bitset<2>> table;
    std::vector<std::bitset<2>> combination;
    SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
        assert(size <= (1 << 16));
    }

    bool predict(uint32_t pc) {
        uint32_t address=pc & 0x3FFF;
        std::bitset<2> bit=table[address];
        if(bit==std::bitset<2>("11")){
            return true;
        }
        else if(bit==std::bitset<2>("00")){
            return false;
        }

        int bits=bhr.to_ulong();
        bits=bits<<14;
        address=bits^address;
        int value1=(combination[address]).to_ulong();

         if(value1==3){
            return true;
        }
        if(value1==0) return false;

        int index=bhr.to_ulong();
        int value=(bhrTable[index]).to_ulong();
        if(value>=2){
            return true;
        }
        return false;
    }

    void update(uint32_t pc, bool taken) {
        uint32_t address=pc & 0x3FFF;
        std::bitset<2> bit=table[address];
        int value1=bit.to_ulong();
        if(taken){
            if(value1<3){
                value1++;
            }
        }
        else{
            if(value1>0){
                value1--;
            }
        }
        std::bitset<2> newvalue(value1);
        table[address]=newvalue;

        int index=bhr.to_ulong();
        int value=(bhrTable[index]).to_ulong();
        if(taken){
            if(value<3){
                value++;
            }
        }
        else{
            if(value>0){
                value--;
            }
        }
        std::bitset<2> newvalue1(value);
        bhrTable[index]=newvalue1;

        int bits=bhr.to_ulong();
        bits=bits<<14;
        address=bits^address;
        value=(combination[address]).to_ulong();
        if(taken){
            if(value<3){
                value++;
            }
            bhr=(bhr<<1);
            bhr.set(0,1);
        }
        else{
            if(value>0){
                value--;
            }
            bhr=(bhr<<1);
            bhr.set(0,0);
        }
        std::bitset<2> newvalue2(value);
        combination[address]=newvalue2;
    }
};


#endif