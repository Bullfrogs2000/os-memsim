#include <algorithm>
#include "pagetable.h"

PageTable::PageTable(int page_size)
{
    _page_size = page_size;
}

PageTable::~PageTable()
{
}

std::vector<std::string> PageTable::sortedKeys()
{
    std::vector<std::string> keys;

    std::map<std::string, int>::iterator it;
    for (it = _table.begin(); it != _table.end(); it++)
    {
        keys.push_back(it->first);
    }

    std::sort(keys.begin(), keys.end(), PageTableKeyComparator());

    return keys;
}

int PageTable::getPageSize() 
{
    return _page_size;
}


std::map<std::string, int> PageTable::getTable()
{
    return _table;
}

void PageTable::addEntry(uint32_t pid, int page_number)
{
    // Combination of pid and page number act as the key to look up frame number
    std::string entry = std::to_string(pid) + "|" + std::to_string(page_number);

    if (_table.count(entry) > 0) 
    {
        return;
    }

    int frame = 0; 
    // Find free frame
    bool found = false;
    while(!found) 
    {
        found = true;
        std::map<std::string, int>::iterator it;
        for (it = _table.begin(); it != _table.end(); it++)
        {
            if (it->second == frame) 
            {
                found = false;
                frame++;
                break;
            }
        }
    }
    _table[entry] = frame;
}

void PageTable::removeEntry(uint32_t pid, int page_number)
{
    // Combination of pid and page number act as the key to look up frame number
    std::string entry = std::to_string(pid) + "|" + std::to_string(page_number);

    if (_table.count(entry) == 0) 
    {
        return;
    }
    _table.erase(entry);
}

void PageTable::removeAll(uint32_t pid)
{
    std::vector<std::string> keys = sortedKeys();

    for (i = 0; i < keys.size(); i++)
    {
        std::string key = keys[i];
        size_t sep = key.find("|");
        if (pid == std::stoi(key.substr(0, sep)))
        {
            _table.erase(key);
        }
    }
}

int PageTable::getPhysicalAddress(uint32_t pid, uint32_t virtual_address)
{
    // Convert virtual address to page_number and page_offset
    int page_number = virtual_address/_page_size;
    int page_offset = virtual_address & (_page_size-1);

    // Combination of pid and page number act as the key to look up frame number
    std::string entry = std::to_string(pid) + "|" + std::to_string(page_number);
    
    // If entry exists, look up frame number and convert virtual to physical address
    int address = -1;
    if (_table.count(entry) > 0)
    {
        address = (_table[entry] * _page_size) + page_offset;
    }

    return address;
}

void PageTable::print()
{
    int i;

    std::cout << " PID  | Page Number | Frame Number" << std::endl;
    std::cout << "------+-------------+--------------" << std::endl;

    std::vector<std::string> keys = sortedKeys();

    for (i = 0; i < keys.size(); i++)
    {
        std::string key = keys[i];
        size_t sep = key.find("|");
        uint32_t pid = std::stoi(key.substr(0, sep));
        int page = std::stoi(key.substr(sep + 1));
        int frame = _table[key];

        std::printf(" %4u | %11d | %12d \n", pid, page, frame);
    }
}
