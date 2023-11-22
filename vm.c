#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define mem_size 32
#define deliminter " \n"

struct Memory {
    int address, data;
};

struct PageTable {
    int v_page_num, valid_bit, dirty_bit, page_num;
};

struct Memory main_memory[mem_size];
struct Memory virtual_memory[128];
struct PageTable p_table[16];

int fifo = 0, lru = 0;
int main_memory_pages[4] = {-1, -1, -1, -1};
int fifo_order[4] = {-1, -1, -1, -1};
int indexFIFO = 0;
int lru_order[4] = {0, 1, 2, 3};



char** parseline(char * cmdline){
    char ** tokens = malloc(80);
    cmdline[strlen(cmdline)-1] = '\0';
    char * tok = strtok(cmdline, deliminter);
    int count = 0;
    while (tok){
        tokens[count] = tok;
        count++;
        tok = strtok(NULL, deliminter);
    }
    tokens[count] = NULL;
    return tokens;
}

int virtual_reality(int virtual_addr){
    if (virtual_addr >= 0 && virtual_addr <= 127)
        return virtual_addr / 8;
    return -1;
}

void update_lru(int lru_index) {
  if (lru) {
    int i;
    if (lru_order[lru_index] == 0) {
    } else if (lru_order[lru_index] == 1) {
      
      for (i = 0; i < 4; i++) {
        if (lru_order[i] == 0)
          lru_order[i] = 1;
      }
      lru_order[lru_index] = 0;

    } else if (lru_order[lru_index] == 2) {
      
      for (i = 0; i < 4; i++) {
        if (lru_order[i] == 0)
          lru_order[i] = 1;
        else if (lru_order[i] == 1)
          lru_order[i] = 2;
      }
      lru_order[lru_index] = 0;

    } else {

      for (i = 0; i < 4; i++) {
        if (lru_index == i) {
          lru_order[i] = 0;
        } else {
          lru_order[i] = (lru_order[i] + 1) % 4;
        }
      }
    } 
  }
}


int indexmm(int mm_page_index){
    for (int i = 0; i < 4; ++i){
        if (main_memory_pages[i] == mm_page_index)
            return i;
    }
    return -1;
}

int locate_page(){
    for (int i = 0; i < 4; ++i){
        if (main_memory_pages[i] == -1)
            return i;
    }
    if (lru == 1){
        int evict = 0;
        for (int i = 0; i < 4; ++i){
            if (lru_order[i] == 3)
                evict = i;
        }
        return evict;
    }
    return indexFIFO;
}

void dirty_girl(int index){
    int virtual = main_memory_pages[index];
    p_table[virtual].dirty_bit = 1;
}

void copy_to_vm(int mm_ind, int v_ind){
    for (int i = 0; i < 8; ++i){
        virtual_memory[v_ind*8 + i].address = main_memory[mm_ind*8 + i].address;
        virtual_memory[v_ind*8 + i].data = main_memory[mm_ind*8 + i].data;
    }
    p_table[v_ind].dirty_bit = 0;
}

void copy_to_mm(int mm_ind, int v_ind){
    for (int i = 0; i < 8; ++i){
        main_memory[mm_ind*8 + i].address = virtual_memory[v_ind*8 + i].address;
        main_memory[mm_ind*8 + i].data = virtual_memory[v_ind*8 + i].data;
    }
    p_table[v_ind].valid_bit = 1;
    p_table[v_ind].page_num = mm_ind;
}

int read_line(char ** args){
    int virtual_addy = atoi(args[1]);
    int mm_page_num = virtual_reality(virtual_addy);
    if (mm_page_num >= 0 && mm_page_num <= 15){
        int mm_page_index = indexmm(mm_page_num);
        if (-1 == mm_page_index){
            printf("A Page Fault Has Occured.\n");
            int new_index = locate_page();
            if (main_memory_pages[new_index] == -1){
                //copy page from virtual mem to main mem
                copy_to_mm(new_index, mm_page_num);
                main_memory_pages[new_index] = mm_page_num;
            }else if (!p_table[main_memory_pages[new_index]].dirty_bit){
                indexFIFO = (indexFIFO + 1) % 4;
                copy_to_mm(new_index, mm_page_num);
                p_table[main_memory_pages[new_index]].valid_bit = 0;
                p_table[main_memory_pages[new_index]].page_num = main_memory_pages[new_index];
                main_memory_pages[new_index] = mm_page_num;
            }else{
                //backup to virtual mem then copy from vm to mm
                indexFIFO = (indexFIFO + 1) % 4;
                copy_to_vm(new_index, main_memory_pages[new_index]);
                copy_to_mm(new_index, mm_page_num);
                p_table[main_memory_pages[new_index]].valid_bit = 0;
                p_table[main_memory_pages[new_index]].page_num = main_memory_pages[new_index];
                main_memory_pages[new_index] = mm_page_num;
            }
            int val = main_memory[(new_index * 8)+ (virtual_addy % 8)].data;
            printf("%d\n", val);
            update_lru(new_index);
        }else{
            int val = main_memory[(mm_page_index * 8)+ (virtual_addy % 8)].data;
            printf("%d\n", val);
            update_lru(mm_page_index);
        }
    }
    return 1;
}

int write_line(char ** args){
    int virtual_addy = atoi(args[1]);
    int value = atoi(args[2]);
    int mm_page_num = virtual_reality(virtual_addy);
    if (mm_page_num >= 0 && mm_page_num <=15){
        int mm_page_index = indexmm(mm_page_num);
        int new_index;
        if (-1 == mm_page_index){
            printf("A Page Fault Has Occurred.\n");
            new_index = locate_page();
            if (main_memory_pages[new_index] == -1){
                copy_to_mm(new_index, mm_page_num);
                main_memory_pages[new_index] = mm_page_num;
            } else if (!p_table[main_memory_pages[new_index]].dirty_bit){
                indexFIFO = (indexFIFO + 1) % 4;
                copy_to_mm(new_index, mm_page_num);
                p_table[main_memory_pages[new_index]].valid_bit = 0;
                p_table[main_memory_pages[new_index]].page_num = main_memory_pages[new_index];
                main_memory_pages[new_index] = mm_page_num;
            }else{
                indexFIFO = (indexFIFO + 1) %4;
                copy_to_vm(new_index, main_memory_pages[new_index]);
                copy_to_mm(new_index, mm_page_num);
                p_table[main_memory_pages[new_index]].valid_bit = 0;
                p_table[main_memory_pages[new_index]].page_num = main_memory_pages[new_index];
                main_memory_pages[new_index] = mm_page_num;
            }
        }else{
            new_index = mm_page_index;
        }
        main_memory[(new_index*8) + (virtual_addy % 8)].data = value;
        //printf("%d\n",main_memory[(new_index*8)+(virtual_addy%8)].data);
        dirty_girl(new_index);
        update_lru(new_index);
    }
    return 1;
}

int showptable(){
    for (int i = 0; i < 16; ++i){
        printf("%d:%d:%d:%d\n", p_table[i].v_page_num, p_table[i].valid_bit, p_table[i].dirty_bit, p_table[i].page_num);
    }
    return 1;
}

int showmain(char ** args){
    int mm_page_index = atoi(args[1]);
    if (mm_page_index >= 0 && mm_page_index <=3){
        int i, addr, value;
        for (i = 0; i < 8; i++){
            addr = (mm_page_index * 8) + i;
            value = main_memory[addr].data;
            printf("%d: %d\n", addr, value);
        }
    }
    return 1;
}

int engine(){
    char **args;
    char *line = NULL;
    int status = 0;
    size_t count_of_char = 0;
    do {
        printf("> ");
        getline(&line, &count_of_char, stdin);
        //if (line == '\n')
        if (strcmp(line, "\n") == 0)
            continue;
        args = parseline(line);
        if (strcmp(args[0], "quit") == 0)
            break;
        else if (strcmp(args[0], "read") == 0)
            status = read_line(args);
        else if (strcmp(args[0], "write") == 0)
            status = write_line(args);
        else if (strcmp(args[0], "showmain") == 0)
            status = showmain(args);
        else if (strcmp(args[0], "showptable") == 0)
            status = showptable();
        else
            status = 1;
    }while (status);
    return 0;
}

void initialize(){
    for (int i = 0; i < mem_size; ++i) {
        main_memory[i].address = i;
        main_memory[i].data = -1;
    }
    for (int i = 0; i < 128; ++i ){
        virtual_memory[i].data = -1;
        virtual_memory[i].address = i;
    }
    for (int i = 0; i < 16; ++i){
        //p_table[i].
        p_table[i].v_page_num = p_table[i].page_num = i;
        p_table[i].valid_bit = p_table[i].dirty_bit = 0;
    }
}


int main(int argc, char ** argv){
    if (argv[1] == NULL || strcmp(argv[1], "FIFO") == 0)
        fifo = 1;
    else if (strcmp(argv[1], "LRU") == 0)
        lru = 1;
    //lru = 1; TEST
    initialize();
    int status = 1;
    do{
        status = engine();
    } while (status);
    return 0;
}
