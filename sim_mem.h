#ifndef UNTITLED_SIM_MEM_H
#define UNTITLED_SIM_MEM_H

#include <fstream>
#include <fcntl.h>
#include <cmath>
#include <iostream>
#include <unistd.h>
#include <bitset>
#include <string>
#include <cstring>
#include <limits>

#define MEMORY_SIZE 200
#define TEXT 0
#define DATA 1
#define BSS 2
#define HEAP_STACK 3
#define LOAD 0
#define STORE 1

using namespace std;

extern char main_memory[MEMORY_SIZE];

typedef struct page_descriptor {
    bool valid;
    int frame;
    bool dirty;
    int swap_index;
    int count;
} page_descriptor;

class sim_mem {
    int swapfile_fd;    //swap file fd
    int program_fd;    //executable file fd
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int page_size;
    int GLOBAL_TIME;
    page_descriptor **page_table; //pointer to page table
    int *frame_free_in_main_memory;
    int *frame_free_in_swap_file;
    char *eraser_page;

public:
    sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size,
            int bss_size, int heap_stack_size, int page_size);

    char load(int address);

    void store(int address, char value);

    void print_memory();

    void print_swap();

    void print_page_table();

    bool valid_address(int index_first_table, int index_second_table, int off_set) const;

    ~sim_mem();

private:
    void setting_page_descriptor();

    int *divide_logical_address(int logical_address) const;

    static string convert_to_binary(int number);

    char diagram(const int *arr, int func, char char_store);

    int check_page_free_in_memory();

    void load_from_exe_file(int index_page_level_one, int index_page_level_two);

    void load_from_swap_file(int index_page_level_one, int index_page_level_two);

    int smallest_used_frame_index();

    int check_page_free_in_swap();

};

#endif //UNTITLED_SIM_MEM_H