////////////////////////////////////////////////////////////////////////////////
// Gozde DOGAN 131044019
// Homework 2
// 
// Description:
//      Girilen directory icerisindeki her file'da yine girilen stringi aradim.
//      String sayisini ekrana yazdirdim
//      Her buldugum string'in satir ve sutun numarasini 
//      buldugum file ile birlikte log.log dosyasina yazdim.
//      Dosyaya yazarken de oncelikle aranilan stringi 
//      yazip sonrasinda file adi, satir ve sutun numarasini yazdim.
//      Her file dan sonra o file da kac tane string oldugunu da 
//      log dosyasina yazdim.
//      Yapilan fork girilen directory icindeki her directory ve file 
//      icin process olusturur. Ve icindeki directorylerin icindeki 
//      her sey icinde process olusturur.
//
//
// References:
//      https://gist.github.com/semihozkoroglu/737691
//      DirWalk fonksiyonunun calisma sekli icin.
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <sys/wait.h>  //wait fonksiyonlari icin
#include <stdlib.h>
#include <dirent.h>     //DirWalk fonksiyonu icin
#include <string.h>
#include <errno.h>   
#include <sys/stat.h>
#include <unistd.h>     //fork icin
 
#define MAX_PATH_SIZE 1024
#define DEBUG

//Function prototypes
int searchStringInFile(char *sFileName);
//Genel islemlerimi topladigim bir fonksiyon
int isEmpty(FILE *file);
//Gelen dosyanin bos olup olmadigina bakar
char** readToFile(); 
//Dosyayi okuyup iki boyutlu stringe yazacak
void findLengthLineAndNumOFline();
//Dosyadaki satir sayisini ve en uzun satirin sütün sayisini hesapliyor.
int searchString(char* sFileName, char **sFile);
//string iki boyutlu string icinde arayacak
int copyStr(char **sFile, char* word, int iStartRow, int iStartCol, int *iRow, int *iCol);
//1 return ederse kopyalama yapti, 0 return ederse kopyalama yapamadi
int DirWalk(const char *path);
//fork yaparak her dosyanin icine girer

int iNumOfLine = 0;
int iLengthLine = 0;

FILE *fPtrInFile = NULL;
FILE *fPtrLogFile = NULL;

char *sSearchStr = NULL;
int iSizeOfSearchStr = 0;

int main(int argc, char *argv[]){
    int iWordCount = 0;
    
    if (argc != 3) {
        printf ("Usage>> \n");
        printf("./listdir \"searchString\" <directoryName>\n");
        return 1;
    }
   
    iSizeOfSearchStr = strlen(argv[1]);
    //printf("size:%d\n", iSizeOfSearchStr);
    if(argv[1] != NULL)
        sSearchStr = (char*)calloc(strlen(argv[1]), sizeof(char));
    
    strncpy(sSearchStr, argv[1], (int)strlen(argv[1]));
    //fprintf(stderr, "sSearchStr:%s\tsize:%d\n", sSearchStr, iSizeOfSearchStr);
    
    //Outputlarin yazilacagi dosyayi actim
	fPtrLogFile=fopen("log.log", "a+");
    
    printf("\n****************************************************\n\n");
    iWordCount = DirWalk(argv[2]);
    printf("\n****************************************************\n");
    printf("  %d %s were found in total in %s directory!\n", iWordCount, argv[1], argv[2]);
    printf("****************************************************\n\n"); 
    
    fprintf(fPtrLogFile, "\n****************************************************\n");
    fprintf(fPtrLogFile, "  %d %s were found in total in %s directory!\n", iWordCount, argv[1], argv[2]);
    fprintf(fPtrLogFile, "****************************************************\n\n");
    
    free(sSearchStr);
    fclose(fPtrLogFile);
    return 0;
}

//Function Definitions


int DirWalk(const char *path)
{
    DIR *dir;
	pid_t pid;
    struct dirent *dosya;																	
	struct stat status;  //Degisen fname ile birlikte status durumunu alir ve S_ISDIR fonksiyonunda kullanir

	//Variables
	int index=0, status1=0;
	char fname[MAX_PATH_SIZE]; //dosyanin adini tutar
	int iWordCount=0; //toplam string sayisi
	int iCount=0; //bir file daki string sayisi
	int iWords=0;
   
    //Directory açilabiliyor mu kontrolu yaptim
    if ((dir = opendir(path)) == NULL) {
		perror("opendir");
		exit(1);
	}
	while ((dosya = readdir(dir)) != NULL) 
	{	
	    //Dosya adi "." veya ".." olmadiginda islem yapilacak.
		if ((strcmp(dosya->d_name, ".") != 0) && (strcmp(dosya->d_name, "..") != 0 )) 
		{          
					
			pid=fork();   //Directory icindeki her file ve directory icin fork yapildi
			
			if(pid==0)    //fork kontrolu (fork olustuysa child gerekli islemi yapar)
			{		      //yani directory ise fonksiyon tekrar cagrilir(recursive)
			              //file ise icine girip stringi arar    
				sprintf(fname, "%s/%s", path, dosya->d_name); //dosya ismini fname'e attim

				index=strlen(fname);
				if( fname[index-1] != '~'  )
				{   
				    #ifndef DEBUG
					    puts(fname);
				    #endif
					if (stat(fname, &status) == -1) 
					{                            
						perror("stat");                                  
						break;
					}
					if (S_ISDIR(status.st_mode))   //File OR Directory diye baktim
					{			
				        #ifndef DEBUG
				            printf("pid:%d\n", getpid());
			            #endif				
						iWordCount+=DirWalk(fname);	//DirWalk fonksiyonu directory de tekrar cagrildi
					}
					else
					{
                        iCount = searchStringInFile(fname);
                        #ifndef DEBUG
				            printf("pid:%d\n", getpid()); 
				        #endif
                        printf("  %s > %d \n", fname, iCount);   
                   
					    iWordCount+=iCount; //filedaki string sayisini toplam string sayisina ekledim
					    iCount=0; //file daki string sayisini sifirladim
					}																									
				}	
				//fork yapildiginda olusan process parant'in sahip oldugu her seye sahip olur
		        free(sSearchStr); //isi biten processte kopyasi olusan bu string bosaltimak zorunda
		        fclose(fPtrLogFile); //isi biten processte kopyasi olusan bu dosya kapatilmak zorunda
				exit(iWordCount);					
			}
			else {
				wait(&status1);
				iWords+=WEXITSTATUS(status1);
			}
		}
	}
	closedir(dir);
	return iWords;
}


/**
    Yapilan islemler main de kafa karistirmasin diye hepsini bu fonksiyonda 
    topladim.

    sFileName : String, input, icinde arama yapilacak dosyanin adini tutuyor
*/
int searchStringInFile(char* sFileName){
    char **sStr=NULL;
    int i=0, j=0;
    int iWordCount = 0;
    
    //Burada adi verilen dosyanin acilip acilmadigina baktim
    //Acilamadiysa programi sonlandirdim.
    fPtrInFile = fopen (sFileName, "r");
    if (fPtrInFile == NULL) {
        perror (sFileName);
        exit(1);
    }

    if(isEmpty(fPtrInFile) == 1){
        rewind(fPtrInFile);
        //Dosyanin satir sayisini ve en uzun satirin 
        //column sayisini bulan fonksiyonu cagirdim.
        findLengthLineAndNumOFline();
        //Dosyayi tekrar kapatip acmak yerine dosyanin nerede oldugunu 
        //gosteren pointeri dosyanin basina aldim
        rewind(fPtrInFile);

        //Dosyayi string arrayine okudum ve bu string'i return ettim
        sStr=readToFile();

        #ifndef DEBUG //Dosyayi dogru okuyup okumadigimin kontrolü
            printf("File>>>>>>>\n");
            for(i=0; i<iNumOfLine; i++)
                printf("%s\n", sStr[i]);
        #endif
        
        
        //String arrayi icinde stringi aradim ve sayisini iWordCount'a yazdim
        iWordCount=searchString(sFileName, sStr);
        
	    fprintf(fPtrLogFile, "\n*********************************\n");
	    fprintf(fPtrLogFile, "        TotalWordCount:%d\n", iWordCount);
	    fprintf(fPtrLogFile, "*********************************\n\n");

        //Strin icin ayirdigim yeri bosalttim
        for(i=0; i<iNumOfLine; i++)
            free(sStr[i]);
        free(sStr);
    }
   /* else{
        printf("%s is empty!\n", sFileName); 
        fprintf(fPtrLogFile, "%s is empty!\n", sFileName); 
        //exit(1);
    }*/
    fclose(fPtrInFile);
    return iWordCount;
}

/**
    dosyanin bos olup olmadigina bakar
    1->bos degil
    0->bos
*/
int isEmpty(FILE *file){
    long savedOffset = ftell(file);
    fseek(file, 0, SEEK_END);

    if (ftell(file) == 0){
        return 0;
    }

    fseek(file, savedOffset, SEEK_SET);
    return 1;
}


/**
    Dosyadaki satir sayisini ve en uzun sutundaki karakter sayisini bulur.
    Burdan gelen sonuclara gore dynamic allocation yapilir.
*/
void findLengthLineAndNumOFline(){
	int iLenghtLine=0;
	int iMaxSize=0;
	char ch=' ';

		while(!feof(fPtrInFile)){
			fscanf(fPtrInFile, "%c", &ch);
			iMaxSize++;
				if(ch == '\n'){
					iNumOfLine=iNumOfLine+1;
					if(iMaxSize >=(iLengthLine))
						iLengthLine=iMaxSize;
					iMaxSize=0;
				}
		}
		iNumOfLine-=1; //bir azalttim cunku dongu bir defa fazla donuyor ve iNumOfLine
                        //bir fazla bulunuyor.
        iLengthLine+=1;
        #ifndef DEBUG
            printf("iLengthLine:%d\tiNumOfLine:%d\n", iLengthLine, iNumOfLine);
        #endif
}

/**
    Dosya okunur ve iki boyutlu bir karakter arrayyine atilir.
    Karakter arrayi return edilir.    

    output: char**, okunan dosyayi iki boyutlu stringe aktardim ve 
            bu string arrayini return ettim.
*/
char** readToFile(){
    char **sFile=NULL;
    int i=0;

    //Ikı boyutlu string(string array'i) icin yer ayirdim
    sFile=(char **)calloc(iNumOfLine*iLengthLine, sizeof(char*));
    if( sFile == NULL ){ //Yer yoksa hata verdim
        #ifndef DEBUG
            printf("INSUFFICIENT MEMORY!!!\n");
        #endif
        exit(1);
    }
    //Ikı boyutlu oldugu ıcın her satir icinde yer ayirdim
    for(i=0; i<iNumOfLine; i++){
        sFile[i]=(char *)calloc(iLengthLine, sizeof(char));
        if( sFile[i] == NULL ){ //Yer yoksa hata verdim
            #ifndef DEBUG
                printf("INSUFFICIENT MEMORY!!!\n");
            #endif
            exit(1);
        }
    }

    i=0;
    do{ //Dosyayi okuyup string arrayine yazdim
    
        fgets(sFile[i], iLengthLine, fPtrInFile);
        #ifndef DEBUG
            printf("*-%s-*\n", sFile[i]);
        #endif
        i++;
    }while(!feof(fPtrInFile));

    return sFile;
}

/**
    String arama isleminin ve her yeni bir string bulundugunda bulunan 
    kelime sayisinin arttirildigi fonksiyon

    sFile     :String arrayi, input, ıcınde arama yapiacak string arrayi
    sFileName :String'in aranacagi dosya adi
    output degeri ise integer ve bulunan string sayisini return eder
*/
int searchString(char* sFileName, char **sFile){
    int i=0, j=0;
    int iRow=0, iCol=0;
    char *word=NULL;
    int iWordCount=0;    
    //string arrayinin her satirini sira ile str stringine kopyalayip inceleyecegim
    word=(char *)calloc(100, sizeof(char));

    for(i=0; i<iNumOfLine; i++){ //Satir sayisina gore donen dongu
        for(j=0; j<iLengthLine; j++){ //Sutun sayisina gore donen dongu
                //printf("i:%d\tj:%d\n", i, j);
            if((copyStr(sFile, word, i, j, &iRow, &iCol)) == 1){ //str stringine kopyalama yaptim
                //kopyalama ile sSearchStr esit mi diye baktim
                if(strncmp(word, sSearchStr, (int)strlen(sSearchStr)) == 0){
                    #ifndef DEBUG
                        printf("%s: [%d, %d] %s first character is found.\n", sFileName, iRow, iCol, sSearchStr);
                    #endif
                	//Bulunan kelimenin satir ve sutun sayisi LogFile'a yazdim
                	fprintf(fPtrLogFile, "%s: [%d, %d] %s first character is found.\n", sFileName, iRow, iCol, sSearchStr);
                    iWordCount++; //String sayisini bir arttirdim kelime buldugum icin
                }
            }
        }
    }
    free(word);
    return iWordCount; //Bulunan string sayisini return ettim
}

/**
   Aranmasi gereken stringin karakter sayisi kadar karakteri word stringine kopyalar.
   Kopyalama yaparken kopyalanan karakterin space(' '), enter('\n') ve tab('\t') 
   olmamasina dikkat ederek kopyalar.
   
   sFile    :Dosyadaki karakterlerin tutuldugu iki boyutlu karakter arrayi
   word     :Kopyalanan karakterlerin tutulacagi 1 karakter arrayi
   iStartRow:Aramanin baslayacagi satir indexi
   iStartCol:Aramanin baslayacagi sutun indexi
   iRow     :Bulunan kelimenin ilk karakterinin bulundugu satir numarasi
   iCol     :Bulunan kelimenin ilk karakterinin bulundugu sutun numarasi
*/
int copyStr(char **sFile, char* word, int iStartRow, int iStartCol, int *iRow, int *iCol){

    int k=0, i=0, j=0, jStart = 0;
    //printf("iStartRowIndex:%d\tiStartColIndex:%d\n", iStartRow, iStartCol);
    
    if(sFile[iStartRow][iStartCol] == '\n' || sFile[iStartRow][iStartCol] == '\t' || sFile[iStartRow][iStartCol] == ' '){
        return 0;
    }  
    else{
        *iRow = iStartRow+1;
        *iCol = iStartCol+1;
	    #ifndef DEBUG
    	    printf("iRow:%d\tiCol:%d\n", *iRow, *iCol);
		    printf("iStartRow:%d\tiStartCol:%d\n", iStartRow, iStartCol);
	    #endif
        k=0;
        jStart = *iCol-1;
        for(i=*iRow-1; i<iNumOfLine && k < iSizeOfSearchStr; i++){
            for(j=jStart; j<iLengthLine && k < iSizeOfSearchStr; j++){
		        if(sFile[i][j] != '\n' && sFile[i][j] != '\t' && sFile[i][j] != ' ' && k < iSizeOfSearchStr){
                    word[k] = sFile[i][j];
                    k++;
                }                
		        if(sFile[i][j] == '\n' && k < iSizeOfSearchStr){
                    j=iLengthLine;
                }
            }
	        jStart=0; //jnin bir alt satirda baslangic konumu 0 olarak ayarlandi
        }    
        if(k != iSizeOfSearchStr)
            return 0;
        else
            return 1;
    }
    return -1;
}
