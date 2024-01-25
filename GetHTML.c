#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define port 80
#define mem_size 4096 //リスポンスヘッダーサイズ

//URL解析
void parse_url(const char *url, char *domain, char *dir){
    //Delete Protocol
    const char *temp1 = strstr(url, "//");
    temp1 = temp1 ? temp1 + 2 : url;
    
    //Get Domain & Directory
    const char *temp2 = strstr(temp1,"/");
    if(temp2 != NULL){
        strncpy(domain, temp1, temp2 - temp1);
        strcpy(dir, temp2);
    } else {
        strcpy(domain, temp1);
        strcpy(dir, "/");
    }
}

//サーバーに接続する
int connect_server(char *domain, char *dir){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int ret;

    if(sockfd == -1){
        perror("Create socket fail");
        exit(0);
    }

    //Get IP Address
    struct hostent *ip = gethostbyname(domain);
    
    if (ip == NULL){
        perror("Cannot get IP address");
        exit(0);
    }
    
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, ip->h_addr, ip->h_length);
    //printf("IP address: %s\n", inet_ntoa(addr.sin_addr));

    ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret == -1){
        perror("Connect failed");
        exit(0);
    }

    return sockfd;
}

//コンテンツをダウンロードする
void get_html(char *domain, char *dir){
    int sockfd = connect_server(domain, dir);
    int ret;
    
    //コンテンツをリクエスト
    char header[128];
    sprintf(header,"GET %s HTTP/1.1\r\n", dir);
    sprintf(header + strlen(header), "Host:%s\r\n", domain);
    strcat(header, "Connection:Close\r\n");
    strcat(header,"\r\n");

    ret = send(sockfd, header, sizeof(header), 0);
    if(ret == -1){ 
        perror("Request failed");
        exit(0);
    }

    //ヘッダーを受け取る
    char *rcvbuff = (char *)malloc(2*sizeof(char));
    char *response = (char *)malloc(mem_size*sizeof(char));

    while(recv(sockfd, rcvbuff, sizeof(char), 0)){
        rcvbuff[1] = '\0';
        strcat(response, rcvbuff);
        int flag = 0;
        int i = strlen(response) - 1;
        for (; response[i] == '\n' || response[i] == '\r';
            i--, flag++);
        {
            //ヘッダーを受け取ればブレイク
            if (flag == 4)
                break;
        }
    }
    //puts(response);

    //コンテンツを受け取ってファイルに書き込む
    FILE *fp = fopen("http.html", "w+");
    if(fp == NULL){
        perror("Fail to create a file");
        exit(0);
    }

    while(recv(sockfd, rcvbuff, sizeof(char), 0)){
        fwrite(rcvbuff, sizeof(char), sizeof(char), fp);
    }

    free(response);
    free(rcvbuff);

    fclose(fp);
    close(sockfd);
}

int main(int argc, const char *argv[]){
    char domain[128];
    char dir[256];

    if(argc == 1){
        printf("Please input a valid URL.\n");
        return 0;
    } else {
        parse_url(argv[1], domain, dir);
    }

    //printf("Domain: %s\nDirectory: %s\n", domain, dir);
    get_html(domain, dir);
    puts("Download Success.");

    return 0;
}