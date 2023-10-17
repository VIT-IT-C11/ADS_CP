#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include"queuell.h"


#pragma pack(1) // Ensure that the structure is packed without any padding

// BMP header structure
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


FILE *image_file,*destination_file;
BMPHeader header;
size_t header_size = sizeof(BMPHeader);
unsigned char * image;
int nNodes = 0,imageRes,bytesPerPixel;
int hist[256] = {0};
unsigned char * charArray;
unsigned int * freq;
struct huffNode * root;
struct codeValuePair *code;
int padd_offset=0,sizeList,padding = 0;
int total_size, output_size;

int intToBinary(int num) {
    int binary = 0;
    int base = 1;

    while (num > 0) {
        int remainder = num % 2;
        binary = binary + (remainder * base);
        num = num / 2;
        base = base * 10; 
    }

    return binary;
}

int binaryArrayToInt(int binaryArray[], int arraySize) {
    int result = 0;

    for (int i = 0; i < arraySize; i++) {
        result = (result << 1) | binaryArray[i];
    }

    return result;
}


void intToBitArray(int num, int *bitArray, int numBits) {
      for (int i = numBits - 1; i >= 0; i--) {
        bitArray[i] = (num >> i) & 1;
    }
}


void readImage(char filename[]){
    image_file = fopen(filename, "rb");
    if (image_file == NULL)
    {
        printf("%s does not exits\nTerminating the process");
        exit(1);
    }
    if (fread(&header, header_size, 1, image_file) != 1) {
        perror("Error reading BMP header");
        fclose(image_file);
        exit(1);
    }
    bytesPerPixel = header.bits_per_pixel/8;
    imageRes = header.height * header.width * bytesPerPixel;
    image = (unsigned char *)malloc(imageRes);
    if(image == NULL){
        printf("Error allocating memory");
        exit(1);
    }
    // printf("%d\n",imageRes);
    fread(image,1,imageRes,image_file);
    // for(int i = 0; i<imageRes;i++){
    //     printf("%d\n",image[i]);
    // }
        total_size = ftell(image_file);
    // fseek(image_file, 0, SEEK_END);
    // int c2 = ftell(image_file);
    // printf("TEST : %d",c1-c2);

}

void histogram(){
    for(int i =0; i<imageRes;i++){
        hist[image[i]]++;
    }
    nNodes=0;
    for(int i=0;i<256;i++){
        if(hist[i]>0){
            nNodes++;
        }
    }
    charArray =(unsigned char *)calloc(nNodes,sizeof(unsigned char));
    freq=(unsigned int *)calloc(nNodes,sizeof(unsigned int));
    int top = 0;
    for(int i = 0; i<256;i++){
        if(hist[i]!=0){
            charArray[top] = (unsigned char)i;
            freq[top] = (unsigned int)hist[i];
            top++;
        }
        // printf("%d ",hist[i]);
    }

}

struct huffNode * createNode(unsigned char item, unsigned int freq){
    struct huffNode * temp = (struct huffNode *)malloc(sizeof(struct huffNode));
    temp->left = temp->right = NULL;
    temp->item = item;
    temp->frequency = freq;
    return temp;
}

struct minHeap * createHeap(unsigned capacity){
    struct minHeap * temp = (struct minHeap *) malloc(sizeof(struct minHeap));
    temp->capacity = capacity;
    temp->size = 0;

    temp->array = (struct huffNode **) malloc(capacity * sizeof(struct huffNode *));
    return temp;
}

void swap(struct huffNode ** a,struct huffNode ** b){
    struct huffNode * temp = *a;
    *a = *b;
    *b = temp;
}

int checkSizeOne(struct minHeap * minHeap){
    return(minHeap->size == 1);
}


void heapify(struct minHeap * minHeap, int idx){
    int smallest = idx;
    int left = (2*idx)+1;
    int right = (2*idx)+2;

    if(left < minHeap->size && minHeap->array[left]->frequency < minHeap->array[smallest]->frequency){
        smallest = left;
    }

    if(right < minHeap->size && minHeap->array[right]->frequency < minHeap->array[smallest]->frequency){
        smallest = right;
    }

    if(smallest != idx){
        swap(&minHeap->array[smallest], &minHeap->array[idx]);
        heapify(minHeap, smallest);
    }
}

struct huffNode * extractMin(struct minHeap * minHeap){
    struct huffNode * temp = minHeap->array[0];
    minHeap->array[0] = minHeap->array[minHeap->size - 1];
    --minHeap->size;
    heapify(minHeap,0);

    return temp;
}

struct minHeap * buildHeap(unsigned char arr[],unsigned int freq[], int size){
    struct minHeap * minHeap = createHeap(size);
    for(int i = 0; i< size; i++){
        minHeap->array[i] = createNode(arr[i], freq[i]);
    }
    minHeap->size = size;

    for(int i= (size-2)/2; i>=0; i--){
        heapify(minHeap, i);
    }
    return minHeap;
}

void insertMinHeap(struct minHeap * minHeap, struct huffNode * root){
    ++minHeap->size;
    int i = minHeap->size -1;

    while(i && root->frequency < minHeap->array[(i-1)/2]->frequency){
        minHeap->array[i] = minHeap->array[(i-1)/2];
        i = (i-1)/2;
    }
    minHeap->array[i] = root;
}

struct huffNode* createHuffmanTree(unsigned char arr[],unsigned int freq[], int size){
    struct huffNode * top, * left, * right;
    struct minHeap * minHeap = buildHeap(arr, freq, size);
    while(!checkSizeOne(minHeap)){
        left = extractMin(minHeap);
        right = extractMin(minHeap);

        top = createNode('*',left->frequency + right->frequency);

        top->left = left;
        top->right = right;

        insertMinHeap(minHeap, top);
    }
    return extractMin(minHeap);
}

void printArr(int arr[], int n){
    for (int i =0; i<n;i++){
        printf("%d",arr[i]);
    }
    printf("\n");
}

void printHCodes(struct huffNode * root, int array[], int top){
    if(root->left){
        array[top] = 0;
        printHCodes(root->left, array, top+1);
    }
    if(root->right){
        array[top] = 1;
        printHCodes(root->right, array, top+1);
    }
    if(!root->left && !root->right){
        printf(" %d | ", root->item);
        printArr(array,top);
    }
}

struct huffNode * HuffmanCodes(unsigned char arr[],unsigned int freq[], int size ){
    struct huffNode * temp = createHuffmanTree(arr, freq, size);
    int array[500], top = 0;
    // printHCodes(temp, array, top);
    return temp;
}

void getCodeValuePairs(struct huffNode * root,struct codeValuePair * pairs,int code, int code_length){
    if (root == NULL) {
        return;
    }

    // If it's a leaf node, store the code and value
    if (!root->left && !root->right) {
        pairs[root->item].value = root->item;
        pairs[root->item].code = code;
        pairs[root->item].code_length = code_length;
    }

    // Traverse left with code '0'
    getCodeValuePairs(root->left, pairs, (code << 1), code_length + 1);

    // Traverse right with code '1'
    getCodeValuePairs(root->right, pairs, (code << 1) | 1, code_length + 1);
}

int getCode(unsigned char pix_val){
    for(int i=0;i<nNodes;i++){
        if(code[i].value == pix_val){
            return code[i].code;
        }
    }
}

int getCodeLength(unsigned char pix_val){
    for(int i=0;i<nNodes;i++){
        if(code[i].value == pix_val){
            return code[i].code_length;
        }
    }
}

void encode(char filename[]){
    destination_file=fopen(filename,"wb");
    if(fwrite(&header, sizeof(BMPHeader), 1, destination_file) != 1) {
        perror("Error writing BMP header");
        fclose(image_file);
        fclose(destination_file);
        exit(1);
    }
    code = (struct codeValuePair *) malloc(sizeof(struct codeValuePair)*nNodes);
    for(int i = 0; i<nNodes; i++){
        code[i].value=0;
        code[i].code=0;
        code[i].code_length=0;
    }
    getCodeValuePairs(root,code,0,0);
    // printf("***********************************\n");
    // for(int i = 0; i<nNodes; i++){
    //     printf("code %d | %d\n",code[i].value,intToBinary(code[i].code));
    // }
    sizeList = nNodes*sizeof(struct codeValuePair);
    fwrite(&sizeList,sizeof(sizeList),1,destination_file);
    padd_offset = ftell(destination_file); //offset for padding will change after encoding
    fwrite(&padding,sizeof(padding),1,destination_file);
    fwrite(code,sizeof(struct codeValuePair)*nNodes,1,destination_file);
    fseek(image_file,header_size,SEEK_SET);
    int buffer;
    createQueue(300);
    for(int i=0;i<imageRes;i++){
        unsigned char pix_val=image[i];
        // fread(&pix_val,sizeof(unsigned char),1,image_file);
        // printf("\n%d",pix_val);
        int code = getCode(pix_val);
        int code_length = getCodeLength(pix_val);
        // printf("val%d code%d len%d",code,pix_val,code_length);
        int *bitArr = (int *)malloc(sizeof(int)*code_length);
        intToBitArray(code,bitArr,code_length);
        // printf("\nBefore qsize:%d",q.size);
        for(int j=0;j<code_length;j++){
            enqueue(bitArr[j]);
        }
        free(bitArr);
        // printf("\nqsize %d",q.size);
        if(q.size>250){
            while(q.size>=8){
                int arr[8];
                for(int l=0;l<8;l++){
                    arr[l]=dequeue();
                }
                unsigned char c=binaryArrayToInt(arr,8);
                fwrite(&c,sizeof(unsigned char),1,destination_file);
            }
        }
    }

        
    
    while(q.size>=8){
                int arr[8];
                for(int l=0;l<8;l++){
                    arr[l]=dequeue();
                }
                unsigned char c=binaryArrayToInt(arr,8);
                fwrite(&c,sizeof(unsigned char),1,destination_file);
            }
    padding=8-q.size;
    if(padding!=0){
    int arr[8];
    int k;
    for(k=0;k<q.size;k++){
        arr[k]=dequeue();
    }
    for(;k<8;k++){
        arr[k]=0;
    }
    unsigned char c=binaryArrayToInt(arr,8);
    fwrite(&c,sizeof(unsigned char),1,destination_file);
    fseek(destination_file,padd_offset,SEEK_SET);
    fwrite(&padding,sizeof(int),1,destination_file);
    }
    output_size = ftell(destination_file);


}

int main(int argc,char argv[]){
    char filename[]=argv[1];
    readImage(filename);
    histogram();
    root = HuffmanCodes(charArray,freq,nNodes);
    char compressed_filename[200];
    strcpy(compressed_filename,filename);
    strcat(compressed_filename,".compressed");
    encode(compressed_filename);
    // printf("%d",nNodes);
    printf("The size of the sum of ORIGINAL files was: %d\n",total_size);
    printf("The size of the COMPRESSED file is: %d\n",output_size);
    printf("Compression is complete");
    return 0;

}