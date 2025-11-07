#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h> /* pour htons et inet_aton */
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

int download_files(char msg[256], int status, FILE *fp);
int read_files(FILE *fp, char msg[256], int status) ;
uint64_t get_file_size(const char *filename) ;



int main(){
    //creation socket with specifie argement (tcp, ipv4oripv6...)
    char msg[256], res[256] ;
    int srv = socket(AF_INET, SOCK_STREAM, 0) ;
    if(srv < 0){
        printf("[-]failed creation socket\n") ;
        exit(1);
    }
    printf("[+] socket created \n");
    //create cockaddr and configrate it for accpte spicife client
    struct sockaddr_in prs ;
    prs.sin_family = AF_INET ;
    prs.sin_addr.s_addr = INADDR_ANY ;
    prs.sin_port = htons(8080) ;
    //bind
    bind(srv ,(const struct sockaddr *)&prs , sizeof(prs)) ;
    //lisning the client with to connection with server
    listen(srv, 5);
    socklen_t client_addr_len = sizeof(prs);
    // accepting connection
    int status = accept(srv,(struct sockaddr *)&prs, &prs.sin_addr.s_addr);
    if(status < 0){
        printf("[-]connection failed, try again") ;
        exit(1) ;
    }
    printf("[+] server accepted client connection\n") ;
    FILE *fp ;
    // send and recv msg
    do{
        printf("entre le message : ");
        scanf("%[^\n]%*c", &msg) ;
        send(status, msg,sizeof(msg), 0) ;

        //check if user want to download or upload something
        if(strncmp(msg, "download", 8) == 0){
            if(download_files(msg, status, fp)!=0){
                printf("[-]Failed to download file\n");
            }else{
                printf("[+] file download with succefuly\n") ;
            }
        }else if(strncmp(msg, "upload", 6) == 0){
            read_files(fp, msg, status);
        }else{
            while(strcmp(res, "1") != 0){
                recv(status, res, sizeof(res), 0) ;
                printf("%s", res) ;
            }
        }
        //for empty res variable
        memset(res, 0, sizeof(res));
    } while(strcmp(msg, "exit")!=0) ;
    printf("[-]fin de connection \n") ;

    return 0 ;
}

//download file fonction
int download_files(char msg[256], int status, FILE *fp){
    char buffer[1024] ;
    for(int i=0; i<strlen(msg); i++){
        msg[i] = msg[i+9] ;
    }
    uint64_t filesize_net;
    recv(status, &filesize_net, sizeof(filesize_net), MSG_WAITALL);
    uint64_t filesize = be64toh(filesize_net);
    printf("[+] File size: %lu bytes\n", filesize);
    fp = fopen(msg, "wb");
    if(fp == NULL){
        printf("[-]Failed to open file \n");
        return -1 ;
    }
    printf("[+] file opening succefuly...\n") ;
    size_t total=0 ;
    while(total<filesize){
        int to_read = (filesize - total) < BUFFER_SIZE ? (filesize - total) : BUFFER_SIZE;
        int bytes = recv(status, buffer, to_read, 0) ;
        if (bytes <= 0)
            break;
        fwrite(buffer, 1, bytes, fp);
        total += bytes;
    }
    fclose(fp) ;
    memset(buffer, 0, sizeof(buffer));
    return 0 ;
}



int read_files(FILE *fp, char msg[256], int status){
    for(int i=0; i<strlen(msg); i++){
        msg[i] = msg[i+7] ;
    }

    uint64_t filesize = get_file_size(msg);
    uint64_t filesize_net = htobe64(filesize);
    printf("[+] File size: %lu bytes\n", filesize);
    send(status, &filesize_net, sizeof(filesize_net), 0);
    char buffer[1024] ;
    fp = fopen(msg, "rb") ;
    int bytes ;

    while((bytes = fread(buffer,1, sizeof(buffer), fp))>0){
        send(status, buffer, bytes, 0) ;
    }

    printf("[+] file send with succefuly to client...\n") ;
    return 0 ;

}



uint64_t get_file_size(const char *filename){
    struct stat st;
    if (stat(filename, &st) != 0)
        return 0;
    return (uint64_t)st.st_size;
}
