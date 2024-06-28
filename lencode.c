#include <stdlib.h>
#include <stdio.h>
#include <search.h>
#include <string.h>

#define DICT_SIZE  4194304 
#define SIZE_2Byte 16383

void writeVal(unsigned int val, FILE* out) {
    if (val > SIZE_2Byte) {
     
        unsigned char buffer[3];
        buffer[0] = (val >> 16) & 0x3F;
        buffer[1] = (val >> 8) & 0xFF;
        buffer[2] = val & 0xFF;
        buffer[0] |= 0xC0;
        
        fwrite(buffer, sizeof(char), 3, out);

    } else {
   
        unsigned char buffer[2];
        buffer[0] = (val >> 8) & 0x3F;
        buffer[1] = val & 0xFF;
        buffer[0] |= 0x80;
   
        fwrite(buffer, sizeof(char), 2, out);
    }
}

void encode(FILE* input, FILE* output) {

    hcreate(DICT_SIZE);
    ENTRY item;
    ENTRY *result;
    unsigned int index = 0;

    unsigned char temp[1];
    if (fread(temp, 1, 1, input) != 1) {
        hdestroy();
        return;
    }

    unsigned char *p = malloc(1);
    p[0] = temp[0];
    unsigned int p_size = 1;

    while (1) {
        unsigned char c[1];
        size_t bytesRead = fread(c, 1, 1, input);
        if (bytesRead < 1) break;


        unsigned char *concate = malloc(p_size + 1);
        memcpy(concate, p, p_size);
        concate[p_size] = c[0];

        item.key = concate;
        result = hsearch(item, FIND);

        if (result == NULL) {
            unsigned int *ptr = malloc(sizeof(unsigned int));

            if (index < DICT_SIZE) {
                *ptr = index++;
                item.data = (void*) ptr;
                hsearch(item, ENTER);
            }
            
            if (p_size <= 2) {
                fwrite(p, 1, p_size, output);
                
            } else {
                item.key = p;
                result = hsearch(item, FIND);
                if (result != NULL) {
                    unsigned int val = *(unsigned int*)result->data;
                    // printf("key %d outputed\n",val);
                    writeVal(val, output);
                } else {
                    fwrite(p, 1, p_size, output);
                }
            }
            free(p);
            p = malloc(2);
            p[0] = c[0];
            p_size = 1;
        } else {
            free(p);
            p = concate;
            p_size++;
        }
    }
    
    fwrite(p, sizeof(char), p_size, output);
    free(p);
    hdestroy();
}

int main(int argc, char *argv[]) {

    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror("Error opening input file");
        return 1;
    }

    FILE *output = fopen(argv[2], "wb");
    if (output == NULL) {
        perror("Error opening output file");
        fclose(input);
        return 1;
    }

    encode(input, output);
    fclose(input);
    fclose(output);

    return 0;
}
