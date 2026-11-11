#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push,1)

typedef struct
{
    unsigned char bfType[2]; //2 bytes
    unsigned int bfSize; //4 bytes dimensiune fisier bmp in bytes
    unsigned short bfReserved1; //2 bytes
    unsigned short bfReserved2; //2 bytes
    unsigned int bfOffBits; //4 bytes
} BITMAPFILEHEADER;

typedef struct 
{
    unsigned int biSize; //4 bytes 
    int biWidth; //4 bytes
    int biHeight; //4 bytes
    unsigned short biPlanes; //2 bytes: color planes - 1
    unsigned short biBitCount; //2 bytes: nr of bits/pixel
    unsigned int biCompression; //4 bytes: metoda compresie
    unsigned int biSizeImage; //4 bytes
    int biXPelsPerMeter; //4 bytes: rezolutie pe orizontala
    int biYPelsPerMeter; //4 bytes: rezolutie pe verticala
    unsigned int biClrUsed; //4 bytes: nr culori utilizate
    unsigned int biClrImportant; //4 bytes: nr culori importante
} BITMAPINFOHEADER;

#pragma pack(pop)

void file_info(FILE *inputFile)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader,sizeof(BITMAPFILEHEADER),1,inputFile);
    fread(&infoHeader,sizeof(BITMAPINFOHEADER),1,inputFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);

    printf("Latime %d, inaltime %d, dimensiune fisier %u, nr biti/pixel %d\n", width, height, fileHeader.bfSize, infoHeader.biBitCount);
}

void to_grayscale(FILE *inputFile, FILE *outputFile)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader,sizeof(BITMAPFILEHEADER),1,inputFile);
    fread(&infoHeader,sizeof(BITMAPINFOHEADER),1,inputFile);

    fwrite(&fileHeader,sizeof(BITMAPFILEHEADER),1,outputFile);
    fwrite(&infoHeader,sizeof(BITMAPINFOHEADER),1,outputFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);

    /*liniile unui BMP sunt multiplii de 4. in binar, un nr multiplu de 4 se termina in 00
    folosind ~3 = 11111100 -> aproximeaza in jos la un nr multiplu de 4, daca adaugam +3 la inceput, aproximeaza in sus la cel mai 
    apropiat multiplu de 4 */
    int rowSize = (width*3+3) & ~3;
    unsigned char* rowBuffer = (unsigned char*)malloc(rowSize);

    // ca imaginile sa nu fie corupte
    fseek(inputFile,fileHeader.bfOffBits,SEEK_SET);
    fseek(outputFile,fileHeader.bfOffBits,SEEK_SET);

    //lungimile sunt in pixeli?
    for(int i=0; i<height; i++) 
    {
        fread(rowBuffer,rowSize,1,inputFile); //paddingul de la final se copiaza
        for(int j=0; j<width; j++) 
        {
            int pos=j*3; //3 bytes pt fiecare pixel

            unsigned char gray = (unsigned char)(0.114 * rowBuffer[pos] + 0.587 * rowBuffer[pos+1] + 0.299 * rowBuffer[pos+2]);
            rowBuffer[pos] = rowBuffer[pos+1] = rowBuffer[pos+2] = gray;
        }

        fwrite(rowBuffer,rowSize,1,outputFile);
    }

    free(rowBuffer);
    printf("Imaginea a fost convertita la grayscale\n");
}

void invert_colours(FILE *inputFile, FILE *outputFile)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader,sizeof(BITMAPFILEHEADER),1,inputFile);
    fread(&infoHeader,sizeof(BITMAPINFOHEADER),1,inputFile);

    fwrite(&fileHeader,sizeof(BITMAPFILEHEADER),1,outputFile);
    fwrite(&infoHeader,sizeof(BITMAPINFOHEADER),1,outputFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);

    int rowSize = (width*3+3) & ~3;
    unsigned char* rowBuffer = (unsigned char*)malloc(rowSize);

    fseek(inputFile,fileHeader.bfOffBits,SEEK_SET);
    fseek(outputFile,fileHeader.bfOffBits,SEEK_SET);

    for(int i=0; i<height; i++) 
    {
        fread(rowBuffer,rowSize,1,inputFile);
        for(int j=0; j<width; j++) 
        {
            int pos=j*3;

            // 255 = intensitatea maxima a culorii 
            rowBuffer[pos] = 255 - rowBuffer[pos];
            rowBuffer[pos+1] = 255 - rowBuffer[pos+1];
            rowBuffer[pos+2] = 255 - rowBuffer[pos+2];
        }

        fwrite(rowBuffer,rowSize,1,outputFile);
    }

    free(rowBuffer);
    printf("Imaginea a fost inversata\n");
}

void change_brightness(FILE *inputFile, FILE *outputFile, char *fact)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader,sizeof(BITMAPFILEHEADER),1,inputFile);
    fread(&infoHeader,sizeof(BITMAPINFOHEADER),1,inputFile);

    fwrite(&fileHeader,sizeof(BITMAPFILEHEADER),1,outputFile);
    fwrite(&infoHeader,sizeof(BITMAPINFOHEADER),1,outputFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    
    int rowSize = (width*3+3) & ~3;
    unsigned char* rowBuffer = (unsigned char*)malloc(rowSize);

    float factor = atoi(fact+1);
    factor/=100;

    fseek(inputFile,fileHeader.bfOffBits,SEEK_SET);
    fseek(outputFile,fileHeader.bfOffBits,SEEK_SET);

    for(int i=0; i<height; i++) 
    {
        fread(rowBuffer,rowSize,1,inputFile);
        for(int j=0; j<width; j++) 
        {
            for(int k=0; k<=2; k++)
            {
                int pos=j*3+k;
                int val=0;
                if(fact[0]=='-')
                {
                    val = rowBuffer[pos] - (int)(factor*rowBuffer[pos]);
                }
                else if(fact[0]=='+')
                {
                    val = rowBuffer[pos] + (int)(factor*rowBuffer[pos]);   
                }

                //verificam overflow
                if(val>255) val=255;
                else if(val<0) val=0;

                rowBuffer[pos]=val;
            }
        }

        fwrite(rowBuffer,rowSize,1,outputFile);
    }

    free(rowBuffer);
    printf("Imaginii i-a fost schimbata luminozitatea\n");
}

void blur_image(FILE *inputFile, FILE *outputFile)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader,sizeof(BITMAPFILEHEADER),1,inputFile);
    fread(&infoHeader,sizeof(BITMAPINFOHEADER),1,inputFile);

    fwrite(&fileHeader,sizeof(BITMAPFILEHEADER),1,outputFile);
    fwrite(&infoHeader,sizeof(BITMAPINFOHEADER),1,outputFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);

    int rowSize = (width*3+3) & ~3;

    //prelucrez imaginea in totalitate -> buffer pt imaginea initiala si finala
    unsigned char* imgData = (unsigned char*)malloc(rowSize*height);

    fseek(inputFile,fileHeader.bfOffBits,SEEK_SET);
    fread(imgData,1,rowSize*height,inputFile);

    unsigned char* outputData = (unsigned char*)malloc(rowSize*height);

    //copiez headerele și datele inițiale pt a pastra paddingul
    memcpy(outputData,imgData,rowSize*height);

    //blur -> pixel = media pixelilor din jur
    for(int y=1; y<height-1; y++) 
    {
        for(int x=1; x<width-1; x++) 
        {
            //parcurge fiecare pixel B,G,R
            for(int p=0; p<3; p++) 
            {
                int suma=0;
                //pixelii din jur
                for(int ky=-1; ky<=1; ky++) 
                {
                    for(int kx=-1; kx<=1; kx++) 
                    {
                        int pixelPos = (y+ky)*rowSize+(x+kx)*3+p;
                        suma+=imgData[pixelPos];
                    }
                }

                int currentPixelPos = y*rowSize+x*3+p;
                outputData[currentPixelPos] = (unsigned char)(suma/9);
            }
        }
    }

    fseek(outputFile,fileHeader.bfOffBits,SEEK_SET);
    fwrite(outputData,1,rowSize*height,outputFile);

    free(imgData);
    free(outputData);
    printf("Imaginea a fost blurata\n");
}

void crop_image(FILE *inputFile, FILE *outputFile, char* x_st, char* y_st, char* n_w, char* n_h)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader,sizeof(BITMAPFILEHEADER),1,inputFile);
    fread(&infoHeader,sizeof(BITMAPINFOHEADER),1,inputFile);

    fwrite(&fileHeader,sizeof(BITMAPFILEHEADER),1,outputFile);
    fwrite(&infoHeader,sizeof(BITMAPINFOHEADER),1,outputFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);

    int x_start=atoi(x_st);
    int y_start=atoi(y_st);
    int n_width=atoi(n_w);
    int n_height=atoi(n_h);

    if(x_start+n_width>width || y_start+n_height>height) 
    {
        printf("Regiunea de decupare depaseste marginile imaginii\n");
        exit(1);
    }

    int oldRowSize = (width*3+3) & ~3;
    int newRowSize = (n_width*3+3) & ~3;

    //actualizare headers
    infoHeader.biWidth = n_width;
    infoHeader.biHeight = n_height;
    infoHeader.biSizeImage = newRowSize*n_height;
    fileHeader.bfSize = fileHeader.bfOffBits+infoHeader.biSizeImage;
    fseek(outputFile,0,SEEK_SET);
    fwrite(&fileHeader,sizeof(BITMAPFILEHEADER),1,outputFile);
    fwrite(&infoHeader,sizeof(BITMAPINFOHEADER),1,outputFile);

    //adaugare padding
    int currentPos = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
    while(currentPos<fileHeader.bfOffBits) 
    {
        fputc(0,outputFile);
        currentPos++;
    }

    unsigned char* rowBuffer = (unsigned char*)malloc(newRowSize);

    for(int i=0; i<n_height; i++) 
    {
        //BMP-urile sunt "Bottom-Up". Rândul 0 este cel de jos, altfel, trebuie formula source_y = (oldHeight - 1) - (y_start + i)

        int source_y = y_start+i; 

        long sourceOffset = fileHeader.bfOffBits+(source_y*oldRowSize)+(x_start*3);
        fseek(inputFile,sourceOffset,SEEK_SET);

        //citesc liniile 
        memset(rowBuffer,0,newRowSize);
        fread(rowBuffer,1,n_width*3,inputFile);

        //scriu date+padding
        fwrite(rowBuffer,1,newRowSize,outputFile);
    }

    free(rowBuffer);
    printf("Decupare finalizata: %dx%d incepand de la (%d, %d)\n", n_width, n_height, x_start, y_start);
}

void rotate_180(FILE *inputFile, FILE *outputFile)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader,sizeof(BITMAPFILEHEADER),1,inputFile);
    fread(&infoHeader,sizeof(BITMAPINFOHEADER),1,inputFile);

    fwrite(&fileHeader,sizeof(BITMAPFILEHEADER),1,outputFile);
    fwrite(&infoHeader,sizeof(BITMAPINFOHEADER),1,outputFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int rowSize = (width*3+3) & ~3;

    unsigned char* imgData = (unsigned char*)malloc(rowSize*height);

    fseek(inputFile,fileHeader.bfOffBits,SEEK_SET);
    fread(imgData,1,rowSize*height,inputFile);

    unsigned char* outputData = (unsigned char*)malloc(rowSize*height);

    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++) 
        {
            int oldPos = y*rowSize+x*3; //formula calcul index in bmp
            int newPos = (height-1-y)*rowSize+(width-1-x)*3;

            outputData[newPos] = imgData[oldPos];
            outputData[newPos+1] = imgData[oldPos + 1];
            outputData[newPos+2] = imgData[oldPos + 2];
        }
    }

    fseek(outputFile,fileHeader.bfOffBits,SEEK_SET);
    fwrite(outputData,1,rowSize*height,outputFile);

    free(imgData);
    free(outputData);
    printf("Imaginea a fost rotita cu 180 de grade\n");
}

void rotate_90(FILE *inputFile, FILE *outputFile)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader,sizeof(BITMAPFILEHEADER),1,inputFile);
    fread(&infoHeader,sizeof(BITMAPINFOHEADER),1,inputFile);

    int oldWidth = infoHeader.biWidth;
    int oldHeight = abs(infoHeader.biHeight);
    int newWidth = oldHeight;
    int newHeight = oldWidth;

    int oldRowSize = (oldWidth * 3 + 3) & ~3;
    int newRowSize = (newWidth * 3 + 3) & ~3;

    //update la headers
    infoHeader.biWidth = newWidth;
    infoHeader.biHeight = newHeight;
    infoHeader.biSizeImage = newRowSize*newHeight;
    fileHeader.bfSize = fileHeader.bfOffBits+infoHeader.biSizeImage;

    fwrite(&fileHeader,sizeof(BITMAPFILEHEADER),1,outputFile);
    fwrite(&infoHeader,sizeof(BITMAPINFOHEADER),1,outputFile);

    unsigned char* oldData = malloc(oldRowSize*oldHeight);
    unsigned char* newData = calloc(newRowSize*newHeight,1); // calloc pt padding curat la noua imagine

    fseek(inputFile,fileHeader.bfOffBits,SEEK_SET);
    fread(oldData,1,oldRowSize*oldHeight,inputFile);

    for(int y=0; y<oldHeight; y++)
    {
        for(int x=0; x<oldWidth; x++)
        {
            int oldPos = y*oldRowSize+x*3;
            int newPos = ((newHeight-1)-x)*newRowSize+y*3;

            newData[newPos] = oldData[oldPos];
            newData[newPos+1] = oldData[oldPos+1];
            newData[newPos+2] = oldData[oldPos+2];
        }
    }

    fseek(outputFile,fileHeader.bfOffBits,SEEK_SET);
    fwrite(newData,1,newRowSize*newHeight,outputFile);

    free(oldData);
    free(newData);
    printf("Imaginea a fost rotita cu 90 de grade\n");
}


int main(int argc, char** argv) {
    FILE *inputFile = fopen(argv[2],"rb");

    if(inputFile==NULL) 
    {
        printf("Fisierul input nu a putut fi deschis\n");
        return 1;
    }

    // printf("%lld ",sizeof(BITMAPFILEHEADER)); //e 16 si trebuie sa fie 14 -> adaug pragma pack pt ca compilatorul sa nu adauge padding suplimentar

    //./prog info imagine_2.bmp   
    if(strcmp(argv[1],"info")==0)
    {
        if(argc!=3)
        {
            printf("Nr incorect de argumente");
            exit(1);
        } 

        file_info(inputFile);
    }

    //./prog grayscale imagine_2.bmp output.bmp   
    else if(strcmp(argv[1],"grayscale")==0)
    {
        FILE *outputFile = fopen(argv[3],"wb");
        if(outputFile==NULL)
        {
            printf("Fisierul output nu a putut fi deschis\n");
            exit(1);
        }

        if(argc!=4)
        {
            printf("Nr incorect de argumente");
            exit(1);
        } 

        to_grayscale(inputFile,outputFile);

        fclose(outputFile);
    }

    //./prog invert imagine_2.bmp output.bmp   
    else if(strcmp(argv[1],"invert")==0)
    {
        FILE *outputFile = fopen(argv[3],"wb");
        if(outputFile==NULL)
        {
            printf("Fisierul output nu a putut fi deschis\n");
            exit(1);
        }

        if(argc!=4)
        {
            printf("Nr incorect de argumente");
            exit(1);
        } 

        invert_colours(inputFile, outputFile);

        fclose(outputFile);
    }

    //./prog brightness imagine_2.bmp output.bmp -50%
    else if(strcmp(argv[1],"brightness")==0)
    {
        FILE *outputFile = fopen(argv[3],"wb");
        if(outputFile==NULL)
        {
            printf("Fisierul output nu a putut fi deschis\n");
            exit(1);
        }

        if(argc!=5)
        {
            printf("Nr incorect de argumente");
            exit(1);
        } 

        change_brightness(inputFile, outputFile, argv[4]);

        fclose(outputFile);
    }
  
    // ./prog blur input.bmp output.bmp
    else if(strcmp(argv[1],"blur")==0)
    {
        FILE *outputFile = fopen(argv[3],"wb");
        if(outputFile==NULL)
        {
            printf("Fisierul output nu a putut fi deschis\n");
            exit(1);
        }

        if(argc!=4)
        {
            printf("Nr incorect de argumente");
            exit(1);
        } 

        blur_image(inputFile, outputFile);

        fclose(outputFile);
    }

    // ./prog crop input.bmp output.bmp 20 20 300 250
    else if(strcmp(argv[1],"crop")==0)
    {
        FILE *outputFile = fopen(argv[3],"wb");
        if(outputFile==NULL)
        {
            printf("Fisierul output nu a putut fi deschis\n");
            exit(1);
        }

        if(argc!=8)
        {
            printf("Nr incorect de argumente");
            exit(1);
        }

        crop_image(inputFile, outputFile, argv[4], argv[5], argv[6], argv[7]);

        fclose(outputFile);
    }

    // ./prog rotate input.bmp output.bmp 180
    else if(strcmp(argv[1],"rotate")==0)
    {
        FILE *outputFile = fopen(argv[3],"wb");
        if(outputFile==NULL)
        {
            printf("Fisierul output nu a putut fi deschis\n");
            exit(1);
        }

        if(argc!=5)
        {
            printf("Nr incorect de argumente");
            exit(1);
        }

        if(strcmp(argv[4],"90")==0)
        {
            rotate_90(inputFile,outputFile);
        }
        else if(strcmp(argv[4],"180")==0)
        {
            rotate_180(inputFile,outputFile);
        }
        else
        {
            printf("Aceasta rotire nu este disponibila");
        }

        fclose(outputFile);
    }

    else
    {
        printf("Nu a fost aleasa o optiune\n");
    }

    fclose(inputFile);

    return 0;
}