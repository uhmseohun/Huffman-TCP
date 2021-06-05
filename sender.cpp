//
//  main.cpp
//  Huffman-TCP-Sender
//
//  Created by 엄서훈 on 2021/06/05.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <queue>
#include <utility>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
using namespace std;

const int BUFFER_SIZE = 1024;
const int CHAR_RANGE = 300000;
const int HUFFMAN_LENGTH = 15;
typedef pair<char, int> ipair; // Char with Frequency

struct Node { // Huffman Tree Node
    int freq;
    char value;
    Node *left_child, *right_child;
};

typedef pair<Node*, int> npair; // Frequency with Node Pointer

struct min_heap_comparator {
public:
    bool operator() (const npair &p, const npair &q) {
        return p.second > q.second;
    }
};

typedef priority_queue<npair, vector<npair>, min_heap_comparator> pq;

map <char, int> frequency;
pq min_heap;

npair pop_and_return (pq *seq) {
    auto result = seq -> top();
    seq -> pop();
    return result;
}

Node* create_node (int freq, char value, Node *left_child, Node *right_child) {
    struct Node *node = (Node *)malloc(sizeof(struct Node));
    node -> freq = freq;
    node -> value = value;
    node -> left_child = left_child;
    node -> right_child = right_child;
    return node;
}

bool current_code[HUFFMAN_LENGTH];
char huffman_codes[CHAR_RANGE][HUFFMAN_LENGTH];
int huffman_lengths[CHAR_RANGE];
bool coded[CHAR_RANGE];

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void copy_current_code (char value, int length) {
    for (int i=0; i<length; ++i) {
        huffman_codes[value][i] = current_code[i] + '0';
    }
    huffman_codes[value][length] = 0;
    coded[value] = true;
}

void recursive_traversal (Node* node, int level) {
    if (!node -> left_child && node -> value) {
        copy_current_code(node -> value, level);
        return;
    }
    
    // to Left node
    current_code[level] = 0;
    recursive_traversal(node -> left_child, level + 1);
    
    // to Right node
    current_code[level] = 1;
    recursive_traversal(node -> right_child, level + 1);
}

void huffman_encode (char user_input[], char huffman_result[]) {
    int user_length = int(strlen(user_input));
    
    // HashMap 이용 문자 빈도수 추출
    for (int i=0; i<user_length; ++i) {
        frequency[user_input[i]]++;
    }
    
    // 문자별 Node 구성, 노드 정보 MinHeap 구성
    for (auto i=frequency.begin(); i!=frequency.end(); ++i) {
        auto node = create_node(i -> second, i -> first, NULL, NULL);
        min_heap.push(make_pair(node, node -> freq));
    }
    
    // 허프만 트리 구성
    while (min_heap.size() != 1) {
        auto top1 = pop_and_return(&min_heap);
        auto top2 = pop_and_return(&min_heap);
        auto node = create_node(top1.second + top2.second, NULL, top1.first, top2.first);
        min_heap.push(make_pair(node, node -> freq));
    }
    
    auto root = pop_and_return(&min_heap).first;
    
    // 모든 노드 탐색 및 허프만 코드 생성
    recursive_traversal(root, 0);
    
    // 생성된 허프만 부호 출력
    
    for (int i=0; i<CHAR_RANGE; ++i) {
        if (!coded[i]) continue;
        printf("%c : ", i);
        int j;
        for (j=0; huffman_codes[i][j]; ++j) {
            printf("%c", huffman_codes[i][j]);
        }
        
        printf("\n");
    }
    
    
    huffman_result[0] = 0;
    
    for (int i=0; i<user_length; ++i) {
        strcat(huffman_result, huffman_codes[user_input[i]]);
    }
}

void tcp_send(const char receiver_ip[], const char receiver_port[], char message[]) {
    int serv_sock;
    int clnt_sock;
    int port = atoi(receiver_port);
    int on=1;
    
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;
    
    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
        error_handling("socket() error");
    
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(port);
    
    if(::bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))== -1)
        error_handling("bind() error");
    
    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");
    
    clnt_addr_size=sizeof(clnt_addr);
    clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_addr,&clnt_addr_size);
    if(clnt_sock==-1)
        error_handling("accept() error");
    
    write(clnt_sock, message, sizeof(message));
    close(clnt_sock);
    close(serv_sock);
}

void export_huffman_codes() {
    FILE *fp = fopen("huffman.codes", "w");
    for (int i=0; i<CHAR_RANGE; ++i) {
        if (!coded[i]) continue;
        fprintf(fp, "%c %s\n", i, huffman_codes[i]);
    }
    fclose(fp);
}

int main(int argc, const char * argv[]) {
    // 인자 검증
    if (argc != 3) {
        puts("Usage: %s <Receiver IP> <Receiver Port>");
        exit(0);
    }
    
    // 문자열 입력
    printf("전송할 문자열을 입력하세요 (한글 제외) >> ");
    char user_input[BUFFER_SIZE];
    scanf("%s", user_input);
    
    // 허프만 압축
    char huffman_result[BUFFER_SIZE * HUFFMAN_LENGTH];
    huffman_encode(user_input, huffman_result);
    
    printf("허프만 부호화 결과 >> %s\n", huffman_result);
    
    export_huffman_codes();
    
    printf("전송 중 ...\n");
    tcp_send(argv[1], argv[2], huffman_result);
    
    int ascii_length = int(strlen(user_input));
    int huffman_length = int(strlen(huffman_result));
    double encode_rate = 1 - double(ascii_length) / huffman_length;
    printf("ASCII 인코딩 대비 %.2lf%% 압축하여 TCP 프로토콜로 전송했습니다.\n", encode_rate * 100);
    return 0;
}
