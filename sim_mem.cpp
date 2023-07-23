#include "sim_mem.h"

/**
 * Constructor:
 */
sim_mem::sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size, int bss_size,
                 int heap_stack_size, int page_size) {
    this->text_size = text_size;
    this->data_size = data_size;
    this->bss_size = bss_size;
    this->heap_stack_size = heap_stack_size;
    this->page_size = page_size;
    this->GLOBAL_TIME = 0;

    // Open the execute file.
    this->program_fd = open(exe_file_name, O_RDONLY);
    if (this->program_fd == -1) {
        perror("ERR");
        delete this;
        exit(EXIT_FAILURE);
    }

    bool flag = false;
    //Open the swap file.
    if (swap_file_name == nullptr) {
        swap_file_name = (char *) malloc(18 * sizeof(char));
        strcpy(swap_file_name, "my_swap_file_name");
        swap_file_name[17] = '\0';
        flag = true;
    }
    this->swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT,
                             S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (this->swapfile_fd == -1) {
        perror("ERR");
        delete this;
        exit(EXIT_FAILURE);
    }
    if (flag)
        free(swap_file_name);

    //Defined the page descriptor.
    setting_page_descriptor();

//    Main memory initialization
    for (char &i: main_memory)
        i = '0';

    //Swap file initialization
    char zero = '0';
    int num_zeros = this->bss_size + this->data_size + this->heap_stack_size;
    for (int i = 0; i < num_zeros; i++)
        write(swapfile_fd, &zero, sizeof(zero));

    //frame_free_in_swap_file initialization (check where there is space in swap file )
    num_zeros /= this->page_size;
    this->frame_free_in_swap_file = new int[num_zeros];
    for (int i = 0; i < num_zeros; i++)
        this->frame_free_in_swap_file[i] = 0;

    //frame_free_in_main_memory initialization (check if there is space in main memory)
    this->frame_free_in_main_memory = new int[MEMORY_SIZE / this->page_size];
    for (int i = 0; i < MEMORY_SIZE / this->page_size; i++)
        this->frame_free_in_main_memory[i] = 0;

    //Initializes an array of zeros in the size of pages to delete pages (below in the code).
    this->eraser_page = (char *) ::malloc((this->page_size + 1) * sizeof(char));
    for (int i = 0; i < this->page_size; i++)
        this->eraser_page[i] = '0';
    this->eraser_page[this->page_size] = '\0';
}
/**
 * Functions:
 */

// The function receives a logical address,
// converts it to a physical address and loads the desired page into the main memory.
// The function returns the character stored at this address.
char sim_mem::load(int address) {
    int *arr = divide_logical_address(address);
    char res;
    if (valid_address(arr[0], arr[1], arr[2])) {
        GLOBAL_TIME++;
        res = diagram(arr, LOAD, '\0');
    } else {
        cout << "ERR" << endl;
        delete[]arr;
        return '\0';
    }
    delete[]arr;
    return res;
}

// The function receives a logical address,
// converts it to a physical address and loads the desired page into the main memory and stores a new character in it.
void sim_mem::store(int address, char value) {
    int *arr = divide_logical_address(address);
    if (valid_address(arr[0], arr[1], arr[2])) {
        GLOBAL_TIME++;
        diagram(arr, STORE, value);
    } else
        cout << "ERR" << endl;
    delete[]arr;
}

// The function receives indexes of the page table and offset.
//The function checks that such a page does exist in the received offset.
bool sim_mem::valid_address(int index_first_table, int index_second_table, int off_set) const {
    if (index_first_table < 4 && off_set <= log2(this->page_size)) {
        switch (index_first_table) {
            case TEXT:
                if (index_second_table < this->text_size / this->page_size)
                    return true;
                break;
            case DATA:
                if (index_second_table < this->data_size / this->page_size)
                    return true;
                break;
            case BSS:
                if (index_second_table < this->bss_size / this->page_size)
                    return true;
                break;
            case HEAP_STACK:
                if (index_second_table < this->heap_stack_size / this->page_size)
                    return true;
                break;
            default:
                break;
        }
    }
    return false;
}

// The function defines a table of pages according to the sizes that the constructor received.
void sim_mem::setting_page_descriptor() {
    this->page_table = new page_descriptor *[4];
    this->page_table[TEXT] = new page_descriptor[this->text_size / this->page_size];
    this->page_table[DATA] = new page_descriptor[this->data_size / this->page_size];
    this->page_table[BSS] = new page_descriptor[this->bss_size / this->page_size];
    this->page_table[HEAP_STACK] = new page_descriptor[this->heap_stack_size / this->page_size];
    for (int j = 0; j < this->text_size / this->page_size; j++) {
        this->page_table[TEXT][j].valid = false;
        this->page_table[TEXT][j].dirty = false;
        this->page_table[TEXT][j].frame = -1;
        this->page_table[TEXT][j].swap_index = -1;
        this->page_table[TEXT][j].count = 0;
    }
    for (int j = 0; j < this->data_size / this->page_size; j++) {
        this->page_table[DATA][j].valid = false;
        this->page_table[DATA][j].dirty = false;
        this->page_table[DATA][j].frame = -1;
        this->page_table[DATA][j].swap_index = -1;
        this->page_table[DATA][j].count = 0;
    }
    for (int j = 0; j < this->bss_size / this->page_size; j++) {
        this->page_table[BSS][j].valid = false;
        this->page_table[BSS][j].dirty = false;
        this->page_table[BSS][j].frame = -1;
        this->page_table[BSS][j].swap_index = -1;
        this->page_table[BSS][j].count = 0;
    }
    for (int j = 0; j < this->heap_stack_size / this->page_size; j++) {
        this->page_table[HEAP_STACK][j].valid = false;
        this->page_table[HEAP_STACK][j].dirty = false;
        this->page_table[HEAP_STACK][j].frame = -1;
        this->page_table[HEAP_STACK][j].swap_index = -1;
        this->page_table[HEAP_STACK][j].count = 0;
    }
}

// The function receives a logical address in binary and splits it into an array
// according to page table indexes and offset.
int *sim_mem::divide_logical_address(int logical_address) const {
    int off_set_len = ((int) log2(this->page_size));
    int index_page_level_one_len = 10 - off_set_len;
    string binary_logical_address = convert_to_binary(logical_address);
    string index_page_level_one_string = binary_logical_address.substr(0, 2);
    string index_page_level_two_string = binary_logical_address.substr(2, index_page_level_one_len);
    string off_set_string = binary_logical_address.substr(index_page_level_one_len + 2, off_set_len);

    int *arr = new int[3];
    arr[0] = stoi(index_page_level_one_string, nullptr, 2);
    arr[1] = stoi(index_page_level_two_string, nullptr, 2);
    arr[2] = stoi(off_set_string, nullptr, 2);

    return arr;
}

string sim_mem::convert_to_binary(int number) {
    bitset<12> binary(number);
    return binary.to_string().substr(binary.size() - 12);
}

// The function receives indexes of tabulation of pages and offset (in the array), a function to execute
// (store or load), and a character in the vicinity of store.
// The function performs a diagram to know how to load/store the information in the main memory.
char sim_mem::diagram(const int *arr, int func, char char_store) {
    int index_page_level_one = arr[0];
    int index_page_level_two = arr[1];
    int off_set = arr[2];
    if (this->page_table[index_page_level_one][index_page_level_two].valid) {
        this->page_table[index_page_level_one][index_page_level_two].count = GLOBAL_TIME;
        if (func == LOAD) {
            int n = (page_table[index_page_level_one][index_page_level_two].frame * this->page_size) + off_set;
            return main_memory[n];
        } else {// func == STORE
            if (index_page_level_one == TEXT) {
                cout << "STORE_TEXT_ERR" << endl;
                return '\0';
            }
            int n = (page_table[index_page_level_one][index_page_level_two].frame * this->page_size) + off_set;
            main_memory[n] = char_store;
            page_table[index_page_level_one][index_page_level_two].dirty = true;
            return '\0';
        }
    } else { // if not valid.
        if (index_page_level_one == TEXT) {
            if (func == LOAD) {
                load_from_exe_file(index_page_level_one, index_page_level_two);
                int n = (page_table[index_page_level_one][index_page_level_two].frame * this->page_size) + off_set;
                return main_memory[n];
            } else { // func == STORE
                cout << "STORE_TEXT_ERR" << endl;
                return '\0';
            }
        } else { // if bss / data / heap stack
            if (page_table[index_page_level_one][index_page_level_two].dirty) {
                if (func == LOAD) {
                    load_from_swap_file(index_page_level_one, index_page_level_two);
                    int n = (page_table[index_page_level_one][index_page_level_two].frame * this->page_size) +
                            off_set;
                    return main_memory[n];
                } else { // func == STORE
                    load_from_swap_file(index_page_level_one, index_page_level_two);
                    int n = (page_table[index_page_level_one][index_page_level_two].frame * this->page_size) +
                            off_set;
                    main_memory[n] = char_store;
                    return '\0';
                }
            } else { // if not dirty.
                if (index_page_level_one == BSS) {
                    if (func == LOAD) {
                        load_from_exe_file(index_page_level_one, index_page_level_two);
                        int n = (page_table[index_page_level_one][index_page_level_two].frame * this->page_size) +
                                off_set;
                        return main_memory[n];
                    } else { // func == STORE
                        load_from_exe_file(index_page_level_one, index_page_level_two);
                        int n = (page_table[index_page_level_one][index_page_level_two].frame * this->page_size) +
                                off_set;
                        main_memory[n] = char_store;
                        page_table[index_page_level_one][index_page_level_two].dirty = true;
                        return '\0';
                    }
                } else if (index_page_level_one == HEAP_STACK) {
                    if (func == LOAD) {
                        cout << "ERR" << endl;
                        return '\0';
                    } else { // func == STORE
                        int index_free_in_main_memory = check_page_free_in_memory();
                        if (index_free_in_main_memory == -1) // if the main memory is full
                            index_free_in_main_memory = smallest_used_frame_index();
                        this->page_table[index_page_level_one][index_page_level_two].valid = true;
                        this->page_table[index_page_level_one][index_page_level_two].dirty = true;
                        this->page_table[index_page_level_one][index_page_level_two].count = GLOBAL_TIME;
                        this->page_table[index_page_level_one][index_page_level_two].frame = index_free_in_main_memory;
                        int n = (page_table[index_page_level_one][index_page_level_two].frame * this->page_size) +
                                off_set;
                        main_memory[n] = char_store;
                        return '\0';
                    }
                } else { // index_page_level_one == DATA
                    if (func == LOAD) {
                        load_from_exe_file(index_page_level_one, index_page_level_two);
                        int n = (page_table[index_page_level_one][index_page_level_two].frame * this->page_size) +
                                off_set;
                        return main_memory[n];
                    } else { // func == STORE
                        load_from_exe_file(index_page_level_one, index_page_level_two);
                        int n = (page_table[index_page_level_one][index_page_level_two].frame * this->page_size) +
                                off_set;
                        main_memory[n] = char_store;
                        page_table[index_page_level_one][index_page_level_two].dirty = true;
                        return '\0';
                    }
                }
            }
        }
    }
}

//The function checks using an array defined in the class if there is free space in the main memory.
int sim_mem::check_page_free_in_memory() {
    for (int i = 0; i < MEMORY_SIZE / this->page_size; i++)
        if (this->frame_free_in_main_memory[i] == 0) {
            this->frame_free_in_main_memory[i] = 1;
            return i;
        }
    return -1;
}

void sim_mem::load_from_exe_file(int index_page_level_one, int index_page_level_two) {
    this->page_table[index_page_level_one][index_page_level_two].count = GLOBAL_TIME;
    char buffer[this->page_size + 1];
    int index = 0;
    for (int i = 0; i < index_page_level_one; i++) {
        if (i == TEXT)
            index += this->text_size;
        if (i == DATA)
            index += this->data_size;
        if (i == BSS)
            index += this->bss_size;
        if (i == heap_stack_size)
            index += this->heap_stack_size;
    }
    pread(program_fd, buffer, this->page_size, index + (index_page_level_two * page_size));
    buffer[this->page_size] = '\0';
    int frame_index = check_page_free_in_memory();
    if (frame_index == -1) // No have space in the main memory.
        frame_index = smallest_used_frame_index();
    page_table[index_page_level_one][index_page_level_two].frame = frame_index;
    page_table[index_page_level_one][index_page_level_two].valid = true;
    for (int i = 0; i < this->page_size; i++)
        main_memory[i + (frame_index * this->page_size)] = buffer[i];
}

void sim_mem::load_from_swap_file(int index_page_level_one, int index_page_level_two) {
    this->page_table[index_page_level_one][index_page_level_two].count = GLOBAL_TIME;
    char buffer[this->page_size + 1];
    pread(swapfile_fd, buffer, this->page_size,
          this->page_table[index_page_level_one][index_page_level_two].swap_index * this->page_size);
    buffer[this->page_size] = '\0';
    int frame_index = check_page_free_in_memory();
    if (frame_index == -1) // No have space in the main memory.
        frame_index = smallest_used_frame_index();
    page_table[index_page_level_one][index_page_level_two].frame = frame_index;
    page_table[index_page_level_one][index_page_level_two].valid = true;
    for (int i = 0; i < this->page_size; i++)
        main_memory[i + (frame_index * this->page_size)] = buffer[i];
    pwrite(swapfile_fd, this->eraser_page, this->page_size,
           this->page_table[index_page_level_one][index_page_level_two].swap_index * this->page_size);
    this->frame_free_in_swap_file[this->page_table[index_page_level_one][index_page_level_two].swap_index] = 0;
    this->page_table[index_page_level_one][index_page_level_two].swap_index = -1;
}

// In a situation where there is no space in the main memory, this function takes care
// of clearing space according to the type of content and how often it is used.
// The function returns an index of the freed frame in main memory.
int sim_mem::smallest_used_frame_index() {
    int indexFirstTable;
    int indexSecondTable;
    int minCount = numeric_limits<int>::max();
    for (int j = 0; j < this->text_size / this->page_size; j++) {
        if (this->page_table[TEXT][j].valid && minCount > this->page_table[TEXT][j].count) {
            minCount = this->page_table[TEXT][j].count;
            indexFirstTable = TEXT;
            indexSecondTable = j;
        }
    }
    for (int j = 0; j < this->data_size / this->page_size; j++) {
        if (this->page_table[DATA][j].valid && minCount > this->page_table[DATA][j].count) {
            minCount = this->page_table[DATA][j].count;
            indexFirstTable = DATA;
            indexSecondTable = j;
        }
    }
    for (int j = 0; j < this->bss_size / this->page_size; j++) {
        if (this->page_table[BSS][j].valid && minCount > this->page_table[BSS][j].count) {
            minCount = this->page_table[BSS][j].count;
            indexFirstTable = BSS;
            indexSecondTable = j;
        }
    }
    for (int j = 0; j < this->heap_stack_size / this->page_size; j++) {
        if (this->page_table[HEAP_STACK][j].valid && minCount > this->page_table[HEAP_STACK][j].count) {
            minCount = this->page_table[HEAP_STACK][j].count;
            indexFirstTable = HEAP_STACK;
            indexSecondTable = j;
        }
    }
    int res = this->page_table[indexFirstTable][indexSecondTable].frame;
    this->page_table[indexFirstTable][indexSecondTable].count = 0;
    this->page_table[indexFirstTable][indexSecondTable].frame = -1;
    this->page_table[indexFirstTable][indexSecondTable].valid = false;
    if (indexFirstTable == TEXT || !this->page_table[indexFirstTable][indexSecondTable].dirty) {
        for (int i = 0; i < this->page_size; i++)
            main_memory[(res * this->page_size) + i] = '0';
    } else {
        int index_free_in_swap_file = check_page_free_in_swap();
        this->page_table[indexFirstTable][indexSecondTable].swap_index = index_free_in_swap_file;
        pwrite(swapfile_fd, main_memory + (res * this->page_size), this->page_size,
               index_free_in_swap_file * this->page_size);
        for (int i = 0; i < this->page_size; i++)
            main_memory[(res * this->page_size) + i] = '0';

    }
    return res;
}

// The function checks in the defined array where there
// is free space in the swap file and returns the index of the free space.
int sim_mem::check_page_free_in_swap() {
    int size = (this->data_size + this->bss_size + this->heap_stack_size) / this->page_size;
    for (int i = 0; i < size; i++) {
        if (this->frame_free_in_swap_file[i] == 0) {
            this->frame_free_in_swap_file[i] = 1;
            return i;
        }
    }
    return -1;
}

void sim_mem::print_memory() {
    int i;
    printf("\n Physical memory\n");
    for (i = 0; i < MEMORY_SIZE; i++)
        printf("[%c]\n", main_memory[i]);
}

void sim_mem::print_swap() {
    char *str = (char *) malloc(this->page_size * sizeof(char));
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while (read(swapfile_fd, str, this->page_size) == this->page_size) {
        for (i = 0; i < page_size; i++)
            printf("%d - [%c]\t", i, str[i]);
        printf("\n");
    }
    free(str);
}

void sim_mem::print_page_table() {
    int i;
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < this->text_size / this->page_size; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[TEXT][i].valid,
               page_table[TEXT][i].dirty,
               page_table[TEXT][i].frame,
               page_table[TEXT][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < this->data_size / this->page_size; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[DATA][i].valid,
               page_table[DATA][i].dirty,
               page_table[DATA][i].frame,
               page_table[DATA][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < this->bss_size / this->page_size; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[BSS][i].valid,
               page_table[BSS][i].dirty,
               page_table[BSS][i].frame,
               page_table[BSS][i].swap_index);

    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < this->heap_stack_size / this->page_size; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[HEAP_STACK][i].valid,
               page_table[HEAP_STACK][i].dirty,
               page_table[HEAP_STACK][i].frame,
               page_table[HEAP_STACK][i].swap_index);
    }
}

sim_mem::~sim_mem() {
    close(program_fd);
    close(swapfile_fd);

    for (int i = 0; i < 4; i++) {
        delete[]this->page_table[i];
    }
    delete[] this->page_table;
    delete[] this->frame_free_in_main_memory;
    delete[] this->frame_free_in_swap_file;
    free(this->eraser_page);
}