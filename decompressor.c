#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<stdint.h>

#pragma pack(1) // Ensure that the structure is packed without any padding

// BMP header structure

void decode(char compressed_filename[], char output_filename[]);


typedef struct {
    uint16_t type;          // Magic identifier "BM"
    uint32_t size;          // File size in bytes
    uint16_t reserved1;     // Reserved (unused)
    uint16_t reserved2;     // Reserved (unused)
    uint32_t offset;        // Offset to image data in bytes

    // DIB header
    uint32_t dib_size;      // DIB Header size
    int32_t  width;         // Image width
    int32_t  height;        // Image height
    uint16_t planes;        // Number of color planes (must be 1)
    uint16_t bits_per_pixel; // Bits per pixel (e.g., 24 for 24-bit BMP)
    uint32_t compression;   // Compression type (0 for no compression)
    uint32_t image_size;    // Size of the image data in bytes (can be 0 if no compression)
    int32_t  x_resolution;  // Horizontal resolution (pixels per meter, usually 0)
    int32_t  y_resolution;  // Vertical resolution (pixels per meter, usually 0)
    uint32_t colors;        // Number of colors in the color palette (can be 0 for 2^n)
    uint32_t important_colors; // Number of important colors (usually ignored)
} BMPHeader;

struct huffNode{
    unsigned char item;
    unsigned int frequency;
    struct huffNode * left, * right;
};

struct minHeap{
    unsigned capacity;
    unsigned size;
    struct huffNode ** array;
};

struct codeValuePair{
    unsigned char value;
    unsigned int code;
    int code_length;
};


int main(int argc, char argv[]){
    char filename[]=argv[1];

    char * p =strstr(filename,".compressed");
    if(strcmp(p,".compressed")!=0){
        printf("Enter the compressed file");
        exit(1);
    }
    char output[200];
    int dotcount=0;
    char k;
    for(int i=0;i<sizeof(filename);i++){
        k=filename[i];
        if(k=='.'){
            dotcount++;
        }
        if(dotcount==2){
            break;
        }
        output[i]=filename[i];
    }
    strcpy(output,filename);
    // FILE * dec_file = fopen(output,"wb");
    decode(filename,output);
    printf("Decompression is complete");
}


void decode(char compressed_filename[], char output_filename[]) {
    FILE *compressed_file = fopen(compressed_filename, "rb");
    if (compressed_file == NULL) {
        printf("Error opening the compressed file\n");
        exit(1);
    }

    BMPHeader compressed_header;
    if (fread(&compressed_header, sizeof(BMPHeader), 1, compressed_file) != 1) {
        perror("Error reading compressed BMP header");
        fclose(compressed_file);
        exit(1);
    }

    // Read the size of the code value pairs array
    int code_pairs_size;
    if (fread(&code_pairs_size, sizeof(code_pairs_size), 1, compressed_file) != 1) {
        perror("Error reading code value pairs size");
        fclose(compressed_file);
        exit(1);
    }

    // Read the number of padding bits
    int padding;
    if (fread(&padding, sizeof(padding), 1, compressed_file) != 1) {
        perror("Error reading padding bits");
        fclose(compressed_file);
        exit(1);
    }

    // Read the code value pairs array
    struct codeValuePair *code_pairs = (struct codeValuePair *)malloc(code_pairs_size);
    if (fread(code_pairs, code_pairs_size, 1, compressed_file) != 1) {
        perror("Error reading code value pairs");
        fclose(compressed_file);
        free(code_pairs);
        exit(1);
    }

    // Create the output BMP file
    FILE *output_file = fopen(output_filename, "wb");
    if (output_file == NULL) {
        printf("Error creating the output BMP file\n");
        fclose(compressed_file);
        free(code_pairs);
        exit(1);
    }

    // Initialize variables for decoding
    unsigned char current_byte;
    int current_bit;
    int current_code = 0;
    int current_code_length = 0;

    // Read the compressed data and decode it
    while (fread(&current_byte, sizeof(unsigned char), 1, compressed_file) == 1) {
        for (current_bit = 7; current_bit >= 0; current_bit--) {
            current_code = (current_code << 1) | ((current_byte >> current_bit) & 1);
            current_code_length++;

            // Check if the current code matches any code value pair
            for (int i = 0; i < code_pairs_size / sizeof(struct codeValuePair); i++) {
                if (current_code_length == code_pairs[i].code_length && current_code == code_pairs[i].code) {
                    fwrite(&code_pairs[i].value, sizeof(unsigned char), 1, output_file);
                    current_code = 0;
                    current_code_length = 0;
                    break;
                }
            }
        }
    }

    // Close the files
    fclose(compressed_file);
    fclose(output_file);
    free(code_pairs);
}
