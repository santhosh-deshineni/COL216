#include <string>
#include <tuple>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

typedef long long longint;

// string of bits to long int
longint binaryToLL(std::string bits)
{
    longint num = 0;
    for (int i = 0; i < bits.length(); i++)
    {
        if (bits[i] == '1')
        {
            num |= 1ll << (bits.length() - i - 1);
        }
    }
    return num;
}

// Function to calculate the number of bits in a number
int countBits(longint num)
{
    int count = 0;
    while (num)
    {
        count++;
        num >>= 1;
    }
    return count-1;
}

// Convert a single hexadecimal character to its binary representation
std::string hexCharToBinary(char c)
{
    switch (c)
    {
        case '0': return "0000";
        case '1': return "0001";
        case '2': return "0010";
        case '3': return "0011";
        case '4': return "0100";
        case '5': return "0101";
        case '6': return "0110";
        case '7': return "0111";
        case '8': return "1000";
        case '9': return "1001";
        case 'a': return "1010";
        case 'b': return "1011";
        case 'c': return "1100";
        case 'd': return "1101";
        case 'e': return "1110";
        case 'f': return "1111";
        default: return "";
    }
}

// Convert a hexadecimal string to a string of binary digits
std::string hexStringToBinary(std::string hexString)
{
    std::string binaryString;
    for (char c : hexString)
    {
        std::string binaryChar = hexCharToBinary(c);
        if (!binaryChar.empty())
        {
            binaryString += binaryChar;
        }
        else
        {
            // Invalid hexadecimal character
            std::cerr << "Error: Invalid hexadecimal character '" << c << "'" << std::endl;
            return "";
        }
    }
    return binaryString;
}

// function to break each line of input file into a vector of strings containing 'r/w' and address string
vector<string> breakstr(string str)
{
    std::istringstream iss(str);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

// struct to represent the two-level Cache simulator containing L1, L2 Caches
struct Cache_Simulator
{
    // variables to keep track of the reads and writes
    longint l1write, l1read, l1writemiss, l1readmiss, l1writeback;
    longint l2write, l2read, l2writemiss, l2readmiss, l2writeback;
    longint block_size;
    int address_size;

    // struct for representing each access in the input file
    typedef struct{
    char access_type;
    string hex_address;
    }access;

    // struct for a line containing dirty bit and tag array
    // valid bit is not necessary because we are using a linked list
    typedef struct{
        bool dirty;
        string tag;
    }line;

    // node of the linked list which represents a set
    struct node{
        line* ptr;
        struct node* next;
    };

    // set which contains a pointer to linked list and the size of the linked list
    typedef struct{
        node* ptr;
        longint size;
    }set;

    // struct for cache with appropriate parameters such as size, assoc and number of sets
    typedef struct{
        vector<set> sets;
        longint size;
        longint assoc;
        longint sets_num;
    }cache;

    cache L1,L2;
    vector<access> trace; 

    // builds the vector containing all the accesses
    void constructTrace(std::ifstream &file)
    {
		std::string line;

		while (getline(file, line))
        {
            access* accptr = new access;
            vector<string> broken = breakstr(line);
            accptr->access_type = broken[0].at(0);
            accptr->hex_address = hexStringToBinary(broken[1]);
            int string_length = (accptr->hex_address).length();
            if (address_size < string_length)
            {
                address_size = string_length;
            }
            trace.push_back(*accptr);
        }
		file.close();
    }

    // constructor for the struct initializes the structs
    Cache_Simulator(longint BLOCKSIZE, longint L1_SIZE, longint L1_ASSOC, longint L2_SIZE, longint L2_ASSOC, std::ifstream &file)
    {

        // initialize the L1 and L2 cache parameters
        address_size = 0;
        block_size = BLOCKSIZE;
        L1.size = L1_SIZE;
        L1.assoc = L1_ASSOC;
        L2.size = L2_SIZE;
        L2.assoc = L2_ASSOC;
		L1.sets_num = L1.size / (L1.assoc * block_size);
        L2.sets_num = L2.size / (L2.assoc * block_size);

        // initialize the read-write variables
        l1read = 0;
        l1write = 0;
        l1readmiss = 0;
        l1writemiss = 0;
        l1writeback = 0;
        l2read = 0;
        l2write = 0;
        l2readmiss = 0;
        l2writemiss = 0;
        l2writeback = 0;
        
        // initialize the L1 vector with sets
        set* setptr = NULL;
        for (int i=0;i<L1.sets_num;i++){
            setptr = new set;
            setptr->ptr = NULL;
            setptr->size = 0;
            L1.sets.push_back(*setptr);
        }

        // initialize the L2 vector with sets
        for (int i=0;i<L2.sets_num;i++){
            setptr = new set;
            setptr->ptr = NULL;
            setptr->size = 0;
            L2.sets.push_back(*setptr);
        }

        // construct the traces vector
        constructTrace(file);
    }

    // if the set is full then removes last element then returns head along with removed node
    tuple<struct node*,struct node*> removenode(struct node* head){
        struct node*temp=head;
        struct node*temp2;
		if(head->next==NULL){
			return make_tuple((struct node*)NULL,temp);
		}
        while(temp->next!=NULL){
            temp2=temp;
            temp=temp->next;
        }

        struct node* temp3=temp2->next;
        temp2->next=NULL;
        return make_tuple(head,temp3);
    }
    
    // function to check if a tag is present in a set which is a linked list
    // returns the new head of set and the removed element pointer if removed or NULL otherwise
	tuple<struct node*,struct node*> present(struct node* head,string tag){
		struct node*temp=head;
		struct node*temp2;
		if(head==NULL){
			return make_tuple(head,(struct node*)NULL);
		}
		struct node* temp3;
		if(head->ptr->tag==tag){
			temp3=head->next;
			return make_tuple(temp3,head);
		}
		while(temp->next!=NULL){
			if(temp->next->ptr->tag==tag){
				struct node*temp4=temp->next;
				temp3 = temp->next->next;
				temp->next=temp3;
				return make_tuple(head,temp4);
			}
			temp2=temp;
			temp=temp->next;
		}
		return make_tuple(head,(struct node*)NULL);
	}

    // completes all updates of L1, L2 and DRAM corresponding to 1 access (read or write)
    void executeAccess(string address, char access_type){
        // calculating set number and tag array to be searched and getting the associated set in L1
        int addrlen = address.length();
        int bitsinset = countBits(L1.sets_num);
        string index_bits=address.substr(addrlen - bitsinset, addrlen);
        string tag= address.substr(0, addrlen - bitsinset);
        set currentset =L1.sets[binaryToLL(index_bits)];
        
        // getting the new head of linked list after checking if tag is present in L1 and the truth value
        auto p = present(currentset.ptr, tag);

        // tag found in L1 and removed so add back tag and return
        if (get<1>(p) != NULL){
            currentset.ptr = get<0>(p);

            struct node* newnode = new node;
            newnode->ptr = new line;
            newnode->ptr->tag = tag;
            if (access_type == 'r' && get<1>(p)->ptr->dirty == false)
            {
                newnode->ptr->dirty =false;
            }
            else
            {
                newnode->ptr->dirty =true;
            }
            newnode->next = currentset.ptr;
            currentset.ptr = newnode;

            L1.sets[binaryToLL(index_bits)] = currentset;
            return;
        }

        // update the misses of L1 since tag was not found
        if (access_type == 'r')
        {
            l1readmiss += 1;
        }
        else
        {
            l1writemiss += 1;
        }

        // tag not found in L1 so check if last element has to be removed
        bool setisfull = currentset.size == L1.assoc;
		if(setisfull)
        {
            // set is full so remove LRU line
            auto p1 = removenode(currentset.ptr);
            struct node* removednode = get<1>(p1);

            // set is updated
            currentset.ptr = get<0>(p1);

            // line is dirty so go to L2
            if (removednode->ptr->dirty == true)
            {
                // dirty so writeback from L1 which is same as L2 write
                l1writeback += 1;
                l2write += 1;

                // calculating the index bits and tag bits of evicted in L2
                string l1evicttag = removednode->ptr->tag;
                string evictadd = l1evicttag + index_bits;
                addrlen = evictadd.length();
                bitsinset = countBits(L2.sets_num);
                string evictindex = evictadd.substr(addrlen - bitsinset, addrlen);
                string evicttag = evictadd.substr(0, addrlen - bitsinset);
                set evictset = L2.sets[binaryToLL(evictindex)];

                // checking if the tag is present in L2
                auto p2 = present(evictset.ptr, evicttag);
                evictset.ptr = get<0>(p2);

                // tag is present in L2 so it is removed above and added back in beginning
                if (get<1>(p2) != NULL)
                {
                    struct node* newnode1 = new node;
                    newnode1->ptr = new line;
                    newnode1->ptr->tag = evicttag;
                    newnode1->ptr->dirty = true;
                    newnode1->next = evictset.ptr;
                    evictset.ptr = newnode1;
                }
                // tag is not present in L2 so get from DRAM
                else
                {
                    // this is an L2 write miss because block was not found for writing
                    l2writemiss += 1;

                    // check if the L2 set is full
                    bool supsetisfull = (evictset.size == L2.assoc);
                    if (supsetisfull)
                    {
                        // remove the LRU line since full
                        auto p3 = removenode(evictset.ptr);
                        struct node* supremnode = get<1>(p3);

                        evictset.ptr = get<0>(p3);

                        // if the removed node is dirty then we do a writeback from L2 to DRAM
                        if (supremnode->ptr->dirty == true)
                        {
                            l2writeback += 1;
                        }
                        delete supremnode;
                    }

                    // add back the tag to beginning of set as it is brought from DRAM
                    struct node* newnodesup = new node;
                    newnodesup->ptr = new line;
                    newnodesup->ptr->tag = evicttag;
                    newnodesup->ptr->dirty = true;
                    newnodesup->next = evictset.ptr;
                    evictset.ptr = newnodesup;
                    // increase size only if set is not full
                    if (!supsetisfull)
                    {
                        evictset.size += 1;
                    }

                }
                L2.sets[binaryToLL(evictindex)] = evictset;
            }
			delete removednode;
            // removed node not dirty so no need to go to L2
        }

        // add back the tag associated with access to L1 irrespective of dirty or not and full or not
        struct node* newnode2 = new node;
        newnode2->ptr = new line;
        newnode2->ptr->tag = tag;
        newnode2->ptr->dirty = (access_type == 'r') ? false : true;
        newnode2->next = currentset.ptr;
        currentset.ptr = newnode2;
        // increase size only if set is not full
        if (!setisfull)
        {
            currentset.size += 1;
        }
        L1.sets[binaryToLL(index_bits)] = currentset;

        // ------ moving to L2 ------

        // reached L2 so update the read because it will always be a read to transfer to L1 for read or write
        l2read += 1;
		
        // calculating set number and tag array to be searched and getting the associated set in L2
        addrlen = address.length();
        bitsinset = countBits(L2.sets_num);
        string l2index=address.substr(addrlen - bitsinset, addrlen);
        string l2tag=address.substr(0, addrlen - bitsinset);
        set l2set =L2.sets[binaryToLL(l2index)];

        // getting the new head of linked list after checking if tag is present in L2 and the truth value
        auto p4 = present(l2set.ptr, l2tag);

        // tag found in L2 and removed so add back tag and return
        if (get<1>(p4) != NULL){
            l2set.ptr = get<0>(p4);

            struct node* newnode3 = new node;
            newnode3->ptr = new line;
            newnode3->ptr->tag = l2tag;
            if (access_type == 'r' && get<1>(p4)->ptr->dirty == false)
            {
                newnode3->ptr->dirty = false;
            }
            else
            {
                newnode3->ptr->dirty = true;
            }
            newnode3->next = l2set.ptr;
            l2set.ptr = newnode3;
            L2.sets[binaryToLL(l2index)] = l2set;
            return;
        }

        // tag not found in L2 so update misses
        l2readmiss += 1;

        // tag not found in L2 so check if last element has to be removed
        bool l2setisfull = l2set.size == L2.assoc;
        if (l2setisfull)
        {
            // set is full so remove LRU line
            auto p5 = removenode(l2set.ptr);
            struct node* removednode = get<1>(p5);

            // set is updated
            l2set.ptr = get<0>(p5);

            if (removednode->ptr->dirty == true)
            {
                // go to DRAM
                // update writeback of L2
                l2writeback += 1;
            }
			delete removednode;
            // not dirty so no need to go to DRAM
        }

        // add back the tag associated with access to L2 irrespective of dirty or not and full or not
        struct node* newnode4 = new node;
        newnode4->ptr = new line;
        newnode4->ptr->tag = l2tag;
        newnode4->ptr->dirty = false;
        newnode4->next = l2set.ptr;
        l2set.ptr = newnode4;
        // increase size only if set is not full
        if (!l2setisfull)
        {
            l2set.size += 1;
        }
        L2.sets[binaryToLL(l2index)] = l2set;
    }
	
    // executes the input file completely carrying out all the accesses and printing the associated statistics
    void runTrace(){
        longint size = trace.size();
        for (longint i=0;i<size;i++)
        {
            int currentlen = trace[i].hex_address.length();
            string addrwithoffset = string(address_size-currentlen,'0') + trace[i].hex_address;
            longint addrlen = addrwithoffset.length();
            string address = addrwithoffset.substr(0, addrlen - countBits(block_size));
            char access_type = trace[i].access_type;
			if(access_type=='r'){
				l1read+=1;
			}
			else{
				l1write+=1;
			}
            executeAccess(address,access_type);
        }
		float l1missrate=(float) (l1readmiss+l1writemiss)/(l1read+l1write);
		float l2missrate=(float) (l2readmiss+l2writemiss)/(l2read+l2write);
        longint accesstime=(l1write+l1read)+(l2write+l2read)*20+(l2writeback+l2writemiss+l2readmiss)*200;
		cout << "number of L1 reads: " << l1read << endl;
		cout << "number of L1 read misses: " << l1readmiss << endl;
		cout << "number of L1 writes: " << l1write << endl;
		cout << "number of L1 write misses: " << l1writemiss << endl;
		cout << "L1 miss rate: " << l1missrate << endl;
		cout << "number of writebacks from L1 memory: " << l1writeback << endl;
		cout << "number of L2 reads: " << l2read << endl;
		cout <<"number of L2 read misses: " << l2readmiss << endl;
		cout <<"number of L2 writes: " << l2write << endl;
		cout <<"number of L2 write misses: " << l2writemiss << endl;
		cout <<"L2 miss rate: " << l2missrate << endl;
		cout <<"number of writebacks from L2 memory: " << l2writeback << endl;
        cout << "Total Access Time:" << accesstime << endl;
    }
};

// the main function which takes input and sends to struct for processing followed by actually running the input file
int main(int argc, char *argv[])
{
	if (argc != 7)
	{
		std::cerr << "Required arguments are of the form: 64 1024 2 65536 8 memory_trace_files/trace1.txt \n";
		return 0;
	}
	std::ifstream file(argv[6]);
	Cache_Simulator *cache;
	if (file.is_open())
    {
        cache = new Cache_Simulator(stoi(argv[1]),stoi(argv[2]),stoi(argv[3]),stoi(argv[4]),stoi(argv[5]),file);
    }
	else
	{
		std::cerr << "File could not be opened. Terminating...\n";
		return 0;
	}
	cache->runTrace();
	return 0;
}