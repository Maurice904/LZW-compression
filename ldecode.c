#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#define DICT_SIZE  4194304 


char* concateToString(char* p, char c, int p_size) {
    char* s = malloc(p_size + 2);
    if (s == NULL) {
        perror("Unable to allocate memory");
        exit(1);
    }
    memcpy(s, p, p_size);
    s[p_size] = c;
    s[p_size + 1] = '\0';
    return s;
}

void printDict(char** dict) {
    for (int i = 0; i < DICT_SIZE; i++) {
        if (dict[i] != NULL) {
            printf("{%d: %s}\n", i, dict[i]);
        }
    }
}

char* writeFromDict(FILE* input, FILE* output, int extra_size, unsigned char* c, char* p, int p_size, char** dict, int index) {
    unsigned char buffer[2];
    size_t bytesRead = fread(buffer, 1, extra_size, input);
    if (bytesRead < extra_size) return NULL;

    unsigned int key = c[0];
    for (int i = 0; i < extra_size; i++) {
        key = (key << 8) | (unsigned char)buffer[i];
    }
    // printf("key is %u\n", key);
    // printDict(dict);
    char* s;
    if (key < DICT_SIZE) {
        if (dict[(int)key] == NULL) {
            s = concateToString(p, p[0], p_size);
            dict[(int)key] = s;
            fwrite(s, 1, p_size + 1, output);
        } else {
            s = dict[(int)key];
            fwrite(s, sizeof(char), strlen(s), output);
        }
    } else {
        // printf("index exceed max Size\n");
        return NULL;
    }

    return s;
}

int dictPut(char* s, char **dict, int index) {

    if (index >= DICT_SIZE) return 0;

    ENTRY item;
    ENTRY *result;
    item.key = s;
    result = hsearch(item, FIND);
    if (result == NULL) {
        dict[index] = s;
        item.key = s;
        item.data = NULL;
        hsearch(item, ENTER);
        return 0;
    } 
    return 1;

}

void decode(FILE* input, FILE* output) {
    char** dict = (char**)calloc(DICT_SIZE, sizeof(char*));
    hcreate(DICT_SIZE);
    ENTRY item;
    ENTRY* result;

    if (dict == NULL) {
        perror("Unable to allocate dictionary");
        exit(1);
    }
    int index = 0;

    char temp[1];
    if (fread(temp, 1, 1, input) != 1) {
        free(dict);
        return;
    }

    fwrite(temp, 1, 1, output);

    char* p = malloc(2);
    if (p == NULL) {
        perror("Unable to allocate memory");
        exit(1);
    }
    p[0] = temp[0];
    p[1] = '\0';
    int p_size = 1;

    while (1) {
        unsigned char c[1];
        size_t bytesRead = fread(c, 1, 1, input);
        if (bytesRead < 1) break;
        // printf("p is %s\n", p);
        unsigned int prefix = c[0] >> 7;
        char* s = NULL;
        // printf("prefix is %d\n", prefix);

        if (prefix == 0) {
            fwrite(c, 1, 1, output);
            s = concateToString(p, c[0], p_size);
            int skippedEntry = dictPut(s, dict, index);
            free(p);
            p = malloc(2);
            if (p == NULL) {
                perror("Unable to allocate memory");
                exit(1);
            }
            if (skippedEntry) {
                free(p);
                p = s;
                p_size = strlen(p);
            }
            else {
                p[0] = c[0];
                p[1] = '\0';
                p_size = 1;  
            }
           
        } else {
            // printf("type is %d\n", c[0] >> 6);
            int extra_size = (c[0] >> 6 == 2) ? 1 : 2;
            c[0] &= 0x3F;
            s = writeFromDict(input, output, extra_size, c, p, p_size, dict, index);
            // printf("%s\n", s);
            if (s == NULL) {
                perror("read error");
                free(p);
                for (int i = 0; i < index; i++) {
                    free(dict[i]);
                }
                free(dict);
                return;
            }
            char *dict_val = concateToString(p, s[0], p_size);
            int skippedEntry = dictPut(dict_val, dict, index);
            if (skippedEntry) {
                free(p);
                p = dict_val;
                p_size = strlen(p);
            } else {
                free(p);
                p = strdup(s);
                p_size = strlen(p);
            }
            
        }
        while (index < DICT_SIZE && dict[index] != NULL) index++;
    }

    free(p);
    for (int i = 0; i < index; i++) {
        free(dict[i]);
    }
    free(dict);
}

int main(int argc, char *argv[]) {

    FILE* input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror("Error opening input file");
        return 1;
    }

    FILE* output = fopen(argv[2], "wb");
    if (output == NULL) {
        perror("Error opening output file");
        fclose(input);
        return 1;
    }

    decode(input, output);

    fclose(input);
    fclose(output);

    return 0;
}
