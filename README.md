# RAM-Simulator
Authored by Avihai Mordechay

==Description==
This program implements a simulation of the processor's accesses to memory through the segment page table mechanism that allows programs to be run when only part of it is in memory.
The program memory (also called virtual memory) is divided into pages that are loaded into the main memory as needed.
The main function of the program will consist of load and store commands, these functions simulate the reading/writing of
the processor.

Program memory:
page_table - Simulates the page table of TEXT/DATA/BSS/HEAP_STACK.
sim_mem - Simulates the entire process memory.

Main functions:
divide_logical_address(int logical_address) - The function receives a logical address and divides it into indexes of the page table and an offset.
diagram (const int *arr, int func, char char_store) -  The function receives indexes of tabulation of pages and offset (in the array), a function to execute (store or load), and a character in the vicinity of store.
The function performs a diagram to know how to load/store the information in the main memory.

==Program Files==
sim_mem.h
sim_mem.cpp
main.cpp

==How to compile?==
compile: g++ -o process sim_mem.cpp main.cpp
run: ./process

==Input:==
none (the main)

==Output:==
If you used the load function, you can print the character that returned to the screen.
