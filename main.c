#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MSB 24
#define LSB 0x001FFFFF
#define N 256
#define PAGE_ELEM 2097152
#define MAX_ROW 1024*64
#define MAX_OP 1024*1024
#define MAX_BLOCK 64*1024*1024
#define CHANGE 13
#define DEL 15
#define FALSE 0
#define TRUE 1


typedef struct op{
   int code;
   int a;
   int b;
   int last;
   int * point;
}op;

void print_row(int a,int b);
void delete_row(int a, int b);
void change_row(int a, int b);

void undo(int n);
void redo(int n);

void add_row(int a);

int min(int x, int y){
    if(x>y)
        return y;
    return x;
}


char* mem[N];
int * text;
int * zero;
int * undo_text;
int * redo_text;
op* undo_queue;
op* redo_queue;


int last=1;
int last_char=1;
int redo_last_text=0;
int undo_last_text=0;
int redo_last=0;
int undo_last=0;
int op_sum=0;
int  redo_full;


int main() {
    int count=0;

    char temp[64];
    int a=0, b=0, i=0;

    for(i=0; i<N; i++){
        mem[i]=NULL;
    }

    redo_full=FALSE;
    undo_queue= malloc (MAX_OP * sizeof(op));
    redo_queue = malloc( MAX_OP * sizeof(op));
    zero = malloc(MAX_BLOCK * sizeof(int));
    text = zero;
    undo_text= malloc(4 * MAX_OP * sizeof(int));
    redo_text= malloc(4 * MAX_OP * sizeof(int));
    mem[0]=(char*)malloc(sizeof(char)*PAGE_ELEM);

    fgets(temp, 64, stdin);
    while(temp[0]!='q'){
        /*if(undo_last> MAX_ROW - 10 || last> MAX_ROW -10 || redo_last > MAX_ROW -10 || undo_last_text > MAX_ROW -10 || redo_last_text > MAX_ROW -10)
            return 1;*/
       b=0; i=0;
        a=temp[i]-48;
        i++;
        while(temp[i]>47 && temp[i]<58){
            a=a*10+temp[i]-48;
            i++;
        }
       if(temp[i]==',') {
            if(op_sum!=0){
                if(op_sum>0){
                    undo(op_sum);
                }
                else{
                    redo(- op_sum);
                }
                op_sum=0;
            }

            i++;
            while (temp[i] > 47 && temp[i] < 58) {
                b = b * 10 + temp[i] - 48;
                i++;
            }
            if (temp[i] == 'p')
                print_row(a, b);
            if (temp[i] == 'c')
                change_row(a, b);
            if (temp[i] == 'd')
                delete_row(a, b);
       }
       else{
           if(temp[i]=='u'){
               op_sum+=a;
               if(op_sum>undo_last){
                   op_sum=undo_last;
               }
           }
           if(temp[i]=='r'){
               op_sum-=a;
               if((-op_sum)>redo_last){
                   op_sum=(-redo_last);
               }
           }
       }
        if((temp[i]=='c' || temp[i]=='d') && redo_full==TRUE) {
            redo_full = FALSE;
            redo_last_text=0;
            redo_last=0;
        }
        fgets(temp, 64, stdin);
    }
    return 0;
}


void print_row(int a, int b){
    int i;

    while(a<1){
        printf(".\n");
        a++;
    }

    i=a;
    while(i<=min(b, last-1)) {
        int addr=text[i];
        printf("%s\n", &mem[addr/ PAGE_ELEM][addr & LSB]);
        i++;
    }
    while(i<=b && i>=last){
        printf(".\n");
        i++;
    }

}


void change_row(int a, int b){

    undo_queue[undo_last].last=last;
    undo_queue[undo_last].a=a;
    undo_queue[undo_last].b=b;
    undo_queue[undo_last].code=CHANGE;
    undo_queue[undo_last].point=NULL;
    undo_last++;

    for(; a<=b; a++) {
        add_row(a);

    }
    scanf(".\n");
}

void add_row(int a){
    //TODO recupera elementi cancellati
    int start, size;
    char temp[1024];
    //lettura stringa temporanea

    fgets(temp, 1024, stdin);

    size=strlen(temp)+1;
    temp[size-2]='\0';
    if(last_char%PAGE_ELEM+ size > PAGE_ELEM){
        mem[(last_char+size)/ PAGE_ELEM]=(char*)malloc(sizeof(char)*PAGE_ELEM);
        start=(last_char/PAGE_ELEM + 1)* PAGE_ELEM;
        strcpy(&mem[start/ PAGE_ELEM][start & LSB], temp);
        last_char=start + size;
    }
    else{
        if(last_char%PAGE_ELEM==0){
            mem[last_char/ PAGE_ELEM]=(char*)malloc(sizeof(char)*PAGE_ELEM);
        }
        start=last_char;
        strcpy(&mem[start/ PAGE_ELEM][start & LSB], temp);
        last_char+=size;
    }

    if(a==last){
        text[a]=start;
        last++;
    }
    else{
        undo_text[undo_last_text]=text[a];
        undo_last_text++;
        text[a]=start;
    }


}


void delete_row(int a, int b){
    int lenght;
    int i;

    undo_queue[undo_last].last=last;
    undo_queue[undo_last].a=a;
    undo_queue[undo_last].b=b;
    undo_queue[undo_last].code=DEL;
    undo_queue[undo_last].point=NULL;

    while(a<1) a++;

    if(b!=0 && a<last){
        int * old= text;
        undo_queue[undo_last].point=text;
        text = old + last;
        if(b>=last){
            lenght=last - a;
            for(i=0; i<a; i++){
               text[i]=old[i];
            }
        }
        else{
            lenght= b -a +1;
            for(i=0; i<a; i++){
                text[i]=old[i];
            }
            for(i=a; i<last-lenght; i++){
                text[i]=old[i+lenght];
            }
        }
        last-=lenght;

    }

    undo_last++;
}


void undo(int n){
    op current;
    int a, b;
    redo_full=TRUE;
    for(int i=0; i<n && undo_last>0; i++){
        current=undo_queue[undo_last -1];
        a=current.a;
        b=current.b;
        if(current.code == CHANGE){
            if(current.last==last){
                int lenght = b - a + 1;
                for(int j=0; j<lenght; j++){
                    redo_text[redo_last_text]= text[b-j];
                    redo_last_text++;
                    text[b-j]= undo_text[undo_last_text -j -1];
                    undo_text[undo_last_text -j -1]=0;              //opzionale
                }
                undo_last_text-=lenght;
            }
            else{
                for(int j=last-1; j>=current.last; j--){
                    redo_text[redo_last_text]=text[j];
                    redo_last_text++;
                    text[j]=0;                                      //opzionale
                }
                int lenght = current.last - a;
                for(int j=0; j<lenght; j++){
                    redo_text[redo_last_text]=text[current.last -j -1];
                    redo_last_text++;
                    text[current.last -j -1]= undo_text[undo_last_text -j -1];
                    undo_text[undo_last_text -j -1]=0;                  //opzionale
                }
                undo_last_text-=lenght;
                last=current.last;
            }
        }
        if(current.code == DEL){
            if(current.point!=NULL){
                int * app=text;
                text=current.point;
                current.point=app;
                int temp=last;
                last=current.last;
                current.last=temp;
            }
        }
        redo_queue[redo_last]=current;
        redo_last++;
        undo_last--;
    }
}

void redo(int n){

    op current;
    int a, b;
    for(int i=0; i<n && redo_last>0; i++){
        current=redo_queue[redo_last -1];
        a=current.a;
        b=current.b;
        if(current.code == CHANGE){
            for(int j=a; j<=b; j++){
                if(j==last) {
                    text[j] = redo_text[redo_last_text - 1];
                    last++;
                }
                else{
                    undo_text[undo_last_text]=text[j];
                    undo_last_text++;
                    text[j]=redo_text[redo_last_text -1];
                }
                redo_text[redo_last_text -1]=0;     //opzionale
                redo_last_text--;
            }

        }
        if(current.code == DEL){
            if(current.point!=NULL){
                int * app=text;
                text=current.point;
                current.point=app;
                int temp=last;
                last=current.last;
                current.last=temp;
            }
        }
        undo_queue[undo_last]=current;
        undo_last++;
        redo_last--;
    }
}