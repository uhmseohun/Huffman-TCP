//
//  main.cpp
//  Huffman-TCP-Receiver
//
//  Created by 엄서훈 on 2021/06/05.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <map>
#include <string>
using namespace std;

const int MAX_LENGTH = 1024;
const int CHAR_RANGE = 300;
const int HUFFMAN_LENGTH = 15;
char huffman_codes[CHAR_RANGE][HUFFMAN_LENGTH];
map <string, char> table;

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void import_huffman_codes() {
    FILE* file = fopen("./huffman.codes", "r");
    if (file == NULL) {
        error_handling("허프만 부호 코드를 불러오지 못했습니다.");
    }
    
    char line[HUFFMAN_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strlen(line) - 1] = 0; // remove NL
        table[string(line + 2)] = line[0];
    }
    
    fclose(file);
}

void tcp_receive(char server_ip[], char server_port[], char message[]) {
    int sock;
    struct sockaddr_in serv_addr;
    int str_len;
    int port = atoi(server_port);
    
    sock=socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
        error_handling("socket() error");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(server_ip);
    serv_addr.sin_port=htons(port);
    
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        error_handling("connect() error!");
    }
    
    str_len=read(sock, message, sizeof(char) * MAX_LENGTH * HUFFMAN_LENGTH);
    if(str_len==-1)
        error_handling("read() error!");
}

void huffman_decode(char encoded[], char result[]) {
    int encoded_len = strlen(encoded);
    string key = string("");
    int res_pointer = 0;
    for (int i=0; i<encoded_len; ++i) {
        key += encoded[i];
        if (table.find(key) != table.end()) {
            result[res_pointer++] = table[key];
            key = string("");
        }
    }
    encoded[res_pointer] = 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        error_handling("Usage: %s <Server IP> <Server Port>");
    }
    
    import_huffman_codes();
    printf("서버로부터 허프만 부호화 된 문자열 수신 대기 중 ... ");
    char huffman_encoded[MAX_LENGTH * HUFFMAN_LENGTH];
    tcp_receive(argv[1], argv[2], huffman_encoded);
    printf("수신 완료!\n");
    char huffman_decoded[MAX_LENGTH * HUFFMAN_LENGTH];
    huffman_decode(huffman_encoded, huffman_decoded);
    printf("복호화 된 문자열 >> %s\n", huffman_decoded);
    return 0;
}
