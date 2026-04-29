#include <iostream>
#include <cstdint>
#include <cstring>
#include <string>
#include "mmu.h"
#include "pagetable.h"
#include <algorithm>

// 64 MB (64 * 1024 * 1024)
#define PHYSICAL_MEMORY 67108864

void printStartMessage(int page_size);
void createProcess(int text_size, int data_size, Mmu *mmu, PageTable *page_table);
void allocateVariable(uint32_t pid, std::string var_name, DataType type, uint32_t num_elements, Mmu *mmu, PageTable *page_table);
void setVariable(uint32_t pid, std::string var_name, uint32_t offset, void *value, Mmu *mmu, PageTable *page_table, uint8_t *memory);
void freeVariable(uint32_t pid, std::string var_name, Mmu *mmu, PageTable *page_table);
void terminateProcess(uint32_t pid, Mmu *mmu, PageTable *page_table);

int main(int argc, char **argv)
{
    // Ensure user specified page size as a command line parameter
    if (argc < 2)
    {
        std::cerr << "Error: you must specify the page size" << std::endl;
        return 1;
    }

    // Print opening instuction message
    int page_size = std::stoi(argv[1]);
    printStartMessage(page_size);

    // Create physical 'memory' (raw array of bytes)
    uint8_t *memory = new uint8_t[PHYSICAL_MEMORY];

    // Create MMU and Page Table
    Mmu *mmu = new Mmu(PHYSICAL_MEMORY);
    PageTable *page_table = new PageTable(page_size);

    // Prompt loop
    std::string command;
    std::cout << "> ";
    std::getline(std::cin, command);
    while (command != "exit")
    {
        // Handle command
        // TODO: implement this!

        // Get next command
        std::cout << "> ";
        std::getline(std::cin, command);
    }

    // Clean up
    delete[] memory;
    delete mmu;
    delete page_table;

    return 0;
}

void printStartMessage(int page_size)
{
    std::cout << "Welcome to the Memory Allocation Simulator! Using a page size of " << page_size << " bytes." << std:: endl;
    std::cout << "Commands:" << std:: endl;
    std::cout << "  * create <text_size> <data_size> (initializes a new process)" << std:: endl;
    std::cout << "  * allocate <PID> <var_name> <data_type> <number_of_elements> (allocated memory on the heap)" << std:: endl;
    std::cout << "  * set <PID> <var_name> <offset> <value_0> <value_1> <value_2> ... <value_N> (set the value for a variable)" << std:: endl;
    std::cout << "  * free <PID> <var_name> (deallocate memory on the heap that is associated with <var_name>)" << std:: endl;
    std::cout << "  * terminate <PID> (kill the specified process)" << std:: endl;
    std::cout << "  * print <object> (prints data)" << std:: endl;
    std::cout << "    * If <object> is \"mmu\", print the MMU memory table" << std:: endl;
    std::cout << "    * if <object> is \"page\", print the page table" << std:: endl;
    std::cout << "    * if <object> is \"processes\", print a list of PIDs for processes that are still running" << std:: endl;
    std::cout << "    * if <object> is a \"<PID>:<var_name>\", print the value of the variable for that process" << std:: endl;
    std::cout << std::endl;
}

void createProcess(int text_size, int data_size, Mmu *mmu, PageTable *page_table)
{
    //   - create new process in the MMU
    int pid = mmu->createProcess();
    //   - allocate new variables for the <TEXT>, <GLOBALS>, and <STACK>
    mmu->addVariableToProcess(pid, "<TEXT>", DataType::Char, text_size, 0);
    mmu->addVariableToProcess(pid, "<GLOBALS>", DataType::Char, data_size, text_size);
    mmu->addVariableToProcess(pid, "<STACK>", DataType::Char, 65536, text_size + data_size);
    //   - print pid
    std::cout << pid << std::endl;

}

void allocateVariable(uint32_t pid, std::string var_name, DataType type, uint32_t num_elements, Mmu *mmu, PageTable *page_table)
{
    Process *process = mmu->getProcess(pid);
    if (process == nullptr) 
    {
        std::cout << "error: process not found" << std::endl;
        return;
    }
    std::vector<Variable*> variables = process->variables;
    for (Variable* var : variables)
    {
        if (var->name == var_name) 
        {
            std::cout << "error: variable already exists" << std::endl;
            return;
        }
    }
    size_t var_size;
    switch(type) 
    {
        case Short: 
            var_size = 2*num_elements;
            break;
        //int and float both 4 bytes
        case Int:
        case Float:
            var_size = 4*num_elements;
            break;
        //long and double both 8 bytes
        case Long:
        case Double:
            var_size = 8*num_elements;
            break;
        //default 1 byte includes Char
        default: 
            var_size = num_elements;
    }

    //   - find first free space within a page already allocated to this process that is large enough to fit the new variable

    //current address of variable
    uint32_t addr = 0;
    bool found = false;
    while (!found)
    {
        found = true;
        for (Variable *var : variables) 
        {
            if (var->type != FreeSpace) 
            {
                //check if current address is inside an existing variable
                if (addr >= var->virtual_address && addr < var->virtual_address + var->size)
                {
                    found = false;
                    //move to end of existing variable
                    addr = var->virtual_address + var->size;
                    break;
                }
            }
        }
    }


    //   - if no hole is large enough, allocate new page(s)
    int page_num = (addr + var_size)/page_table->getPageSize();
    //actually just allocate all pages, addEntry() will short if page already exists
    for (int i = 0; i <= page_num; i++) 
    {
        page_table->addEntry(pid, i);
    }
    //   - insert variable into MMU
    mmu->addVariableToProcess(pid, var_name, type, var_size, addr);
    //   - print virtual memory address
    std::cout << addr << std::endl;
}

void setVariable(uint32_t pid, std::string var_name, uint32_t offset, void *value, Mmu *mmu, PageTable *page_table, uint8_t *memory)
{
    Process *process = mmu->getProcess(pid);
    if (process == nullptr) 
    {
        std::cout << "error: process not found" << std::endl;
        return;
    }
    std::vector<Variable*>::iterator it = std::find_if(process->variables.begin(), process->variables.end(), [var_name](Variable* v) 
    {
        return v != nullptr && v->name == var_name;
    });

    if (it == process->variables.end()) 
    {
        std::cout << "error: variable not found" <<std::endl;
        return;
    }

    Variable *var = *it;
    //get the number of bytes and set exactly that number at the end
    int num_bytes;
    switch(var->type) 
    {
        case Short: 
            num_bytes = 2;
            break;
        //int and float both 4 bytes
        case Int:
        case Float:
            num_bytes = 4;
            break;
        //long and double both 8 bytes
        case Long:
        case Double:
            num_bytes = 8;
            break;
        //default 1 byte includes Char
        default: 
            num_bytes = 1;
    }

    if ((offset + 1) * num_bytes > var->size)
    {
        std::cout << "error: index out of range" << std::endl;
        return;
    }

    int v_addr = var->virtual_address + offset*num_bytes;

    //   - look up physical address for variable based on its virtual address / offset
    //   - insert `value` into `memory` at physical address
    //   * note: this function only handles a single element (i.e. you'll need to call this within a loop when setting
    //           multiple elements of an array)
    for (int i = 0; i < num_bytes; i++)
    {
        //consecutive virtual addresses may not be contiguous in physical memory
        int p_addr = page_table->getPhysicalAddress(pid, v_addr + i);
        memory[p_addr] = *(uint8_t*) value;
    }
}

void freeVariable(uint32_t pid, std::string var_name, Mmu *mmu, PageTable *page_table)
{
    //   - remove entry from MMU
    Process* process = mmu->getProcess(pid);
    if (process == nullptr) 
    {
        std::cout << "error: process not found" << std::endl;
        return;
    }

    std::vector<Variable*> variables = process->variables;


    int page_num;
    bool found = false;
    for (std::vector<Variable*>::iterator it = variables.begin(); it != variables.end(); it++)
    {
        Variable* var = *it;
        if (var->name == var_name) 
        {
            found = true;
            variables.erase(it);
            page_num = var->virtual_address/page_table->getPageSize();
            delete var;
        }
    }

    if (!found) 
    {
        std::cout << "error: variable not found" <<std::endl;
        return;
    }

    //   - free page if this variable was the only one on a given page
    found = false;
    for (std::vector<Variable*>::iterator it = variables.begin(); it != variables.end(); it++)
    {
        Variable* var = *it;
        if (var->virtual_address/page_table->getPageSize() == page_num) 
        {
            found = true;
        }
    }

    if (!found) 
    {
        page_table->removeEntry(pid, page_num);
    }

}

void terminateProcess(uint32_t pid, Mmu *mmu, PageTable *page_table)
{
    //   - remove process from MMU
    Process* process = mmu->getProcess(pid);
    std::vector<Process*> processes = mmu->getProcesses();
    processes.erase(std::remove(processes.begin(), processes.end(), process), processes.end());
    delete process;
    //   - free all pages associated with given process
    page_table->removeAll(pid);
}
