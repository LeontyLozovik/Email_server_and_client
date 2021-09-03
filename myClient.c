#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#define PORT "3490"
//#define SERVER_IP "192.168.0.111"
#define SERVER_IP "127.0.0.1"
#define checkpoint() write(STDOUT_FILENO, "check point\n", 12)

#define clear() write(STDOUT_FILENO, "\033[1;1H\033[2J", 10)

#define head "head"
#define end "end"

#define HEAD_SZ 4
#define NAME_SZ 32
#define DATA_SZ 1024
#define END_SZ 3

struct package{
    char header[HEAD_SZ];
    int type;
    int num;
    char data[DATA_SZ];
    char ender[END_SZ];
};

int Login(int server_socket, struct package *pk, char* username, char* password);
int Send(int server_socket, struct package *pk);
struct package* Recive(int server_socket, struct package *pk);
char *Open_mailbox(int server_socket, struct package *pk, char* mailbox);
char *Open_message(int server_socket, struct package *pk, int num);
char *Logout(int server_socket, struct package *pk);
void reset_package(struct package* pk);
int check_package(struct package* pk);
void itoa(int num, char *buff, int radix);
int cout(char* str);
char *List(int server_socket, struct package *pk);
char* write_msg(int server_socket, struct package *pk, char* username);
char* STMP_write (int server_socket, struct package *pk, char* username);
void backspace(int n);
int str_in(char *str, int n, int mode);
void echo();


int main(){
    clear();
    puts("hello world!");
    
    int status, server_socket;
    char username[NAME_SZ], password[NAME_SZ];
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(SERVER_IP, PORT, &hints, &servinfo);
    if(status){
        puts("getaddrinfo error");
        exit(1);
    }

    server_socket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(server_socket < 0){
        puts("socket is not created");
        freeaddrinfo(servinfo);
        exit(1);
    }

    status = connect(server_socket, servinfo->ai_addr, servinfo->ai_addrlen);
    if(status < 0){
        puts("connect error");
        freeaddrinfo(servinfo);
        exit(1);
    }

    struct package *pk = (struct package*)malloc(sizeof(struct package));
    do
    {
        cout("Enter your gmail adress: ");
        scanf("%s", username);
        cout("Enter your password: ");
        str_in(password, NAME_SZ, 1);
        //scanf("%s", password);
        if(!(Login(server_socket, pk, username, password)))
            cout("Login error. Wrong adress or password\n");
        else
            cout("OK USER LOGGED IN\n");
    }
    while(!(Login(server_socket, pk, username, password)));

    while(1)
    {
        int choose;
        do
        {
            do
            {
                cout("\nWrite message - 1\nRead message - 2\n");
                scanf("%*[^\n]%*c");       //очистка входного буффера, rewind и fflush не работают
                //rewind(stdin);
                //fflush(stdin);
            }
            while(!scanf("%d", &choose));
        }
        while(choose != 1 && choose !=  2);

        switch(choose)
        {
            case 1: 
            {   
                cout (write_msg(server_socket, pk, username));
                //cout(STMP_write(server_socket, pk, username));
                cout("\n\n");
                do
                {
                    do
                    {
                        cout("\nContinue - 1\nEnd of session - 2\n");
                        scanf("%*[^\n]%*c");
                    }
                    while(!scanf("%d", &choose));
                }
                while(choose != 1 && choose !=  2);	
    
                switch (choose)
                {
                    case 1:
                        break;
                    case 2:
                    {
                        cout(Logout(server_socket, pk));
                        cout("\n");
                        freeaddrinfo(servinfo);
                        free(pk);
                        cout("Goodbye\n");
                        return 0;
                    }
                }
                break;
            }
            case 2:
            {
                do
                {
                    do
                    {
                        cout("\nChoose INBOX mailbox as defalt - 1\nSee all mailboxes and choose by yourself - 2\n");
                        scanf("%*[^\n]%*c");
                    }
                    while(!scanf("%d", &choose));
                }
                while(choose != 1 && choose !=  2);
                int error = 0;
                do
                {
                    if(choose == 1)
                    {
                        cout("Status of mailbox :");
	                    cout(Open_mailbox(server_socket, pk, "null"));
                        cout("\n");
                        error = 1;
                    }
                    else
                    {
                        cout("Choose a mailbox: \n");
                        cout(List(server_socket, pk));
                        cout("\n");
                        cout("Selected mailbox: ");
                        char msg[NAME_SZ];
                        scanf("%s", msg);
                        if(strstr(msg, "/") == NULL && strstr(msg, "INBOX") != NULL)
                            error = 1;
                        cout("\n");
                        cout(Open_mailbox(server_socket, pk, msg));
                        cout("\n\n");
                    }
                }
                while (!error);
    

        	
                int msg_num;	
                do
                {   
                    cout ("Enter number of message to read it : ");
                    scanf("%*[^\n]%*c");
                }
                while(!scanf("%d", &msg_num));
                {
                	cout("\n\n\n");
                	cout(Open_message(server_socket, pk, msg_num));
                }      
                do
                {
                    do
                    {
                        cout("\nContinue - 1\nEnd of session - 2\n");
                        scanf("%*[^\n]%*c");
                    }
                    while(!scanf("%d", &choose));
                }
                while(choose != 1 && choose !=  2);	
        
                switch (choose)
                {
                    case 1:
                    break;
                    case 2:
                    {
                        cout(Logout(server_socket, pk));
                        cout("\n");
                        freeaddrinfo(servinfo);
                        cout("Goodbye\n");
                        return 0;
                    }

	            }  
                break;
            }
        }
    }
}

int Login(int server_socket, struct package *pk, char* username, char* password)
	{
        memset(pk->data, 0, DATA_SZ);
        pk->type = 1;
		char tmp[DATA_SZ] = "LOGIN ";
        strcat(tmp, username);
        strcat(tmp, " ");
        strcat(tmp, password);
        strcat(pk->data, tmp);
		Send(server_socket, pk);
		struct package* reply = Recive(server_socket, pk);
		if (strstr(reply->data, "OK USER LOGGED IN") != NULL)
        {
            return 1;
        }
		else
        {
            return 0;
        }
	}

int Send(int server_socket, struct package *pk)
	{
		int n = send(server_socket, (char*)pk, sizeof(struct package), 0);

		if (n < 0) 
			 perror("ERROR writing to socket");

		return n;
	}

struct package* Recive(int server_socket, struct package *pk)
	{
        reset_package(pk);
		int n = (recv(server_socket, (char*)pk, sizeof(struct package), 0) > 0 && check_package(pk));

		if (n < 0)
			 perror("ERROR reading from socket");

		return pk;
	}

char *List(int server_socket, struct package *pk)
{
    memset(pk->data, 0, DATA_SZ);
    pk->type = 5;
	char tmp[DATA_SZ] = "LIST ";
    strcat(pk->data, tmp);
	Send(server_socket, pk);	
	struct package* reply = Recive(server_socket, pk);
	if (strstr(reply->data, "OK") != NULL)
    {
        return reply->data;
    }
	else
    {
        return "Mailboxes not founded";
    }
}

char *Open_mailbox(int server_socket, struct package *pk, char* mailbox)
	{
        memset(pk->data, 0, DATA_SZ);
        pk->type = 2;
        char tmp[DATA_SZ] = {0};
        if(strstr(mailbox, "null") == NULL)
        {
            memset(pk->data, 0, DATA_SZ);
            strcat(tmp, "SELECT ");
            strcat(tmp, mailbox);
        }
        else
        {
            memset(pk->data, 0, DATA_SZ);
            strcat(tmp, "SELECT INBOX ");
        }
        strcat(pk->data, tmp);
		Send(server_socket, pk);	
		struct package* reply = Recive(server_socket, pk);
		if (strstr(reply->data, "error") == NULL)
        {
            return reply->data;
        }
		else
        {
            return "Open mail box error";
        }
	}

char *Open_message(int server_socket, struct package *pk, int num)
{
    memset(pk->data, 0, DATA_SZ);
    pk->type = 3;
    pk->num = num;
	char tmp[DATA_SZ] = "FETCH ";
    char str_num[NAME_SZ];
    itoa(num, str_num, 10);
    strcat(tmp, str_num);
    strcat(tmp, " body[text]");
    strcat(pk->data, tmp);
	Send(server_socket, pk);
	struct package* reply = Recive(server_socket, pk);
    if (strstr(pk->data, "OK COMPLETED\n") != NULL)
    {
        return reply->data;
    }
	else
    {
        return "Reading message error";
    }	
}

char* Logout(int server_socket, struct package *pk)
{
    memset(pk->data, 0, DATA_SZ);
    pk->type = 4;
	char tmp[DATA_SZ] = "LOGOUT ";
    strcat(pk->data, tmp);
    Send(server_socket, pk);	
    struct package* reply = Recive(server_socket, pk);
	if (strstr(reply->data, "* BYE") != NULL)
    {
        shutdown(server_socket, 2);
        return reply->data;
    }
	else
    {
        return "Logout error";
    }	
}

char* write_msg(int server_socket, struct package *pk, char* username)
{
    memset(pk->data, 0, DATA_SZ);
    pk->type = 6;
    char tmp[DATA_SZ] = {0};
    strcat(tmp, "<");
    strcat(tmp, username);
    strcat(tmp, ">\nTo: ");

    cout("To: ");
    char addr[NAME_SZ] = {0};
    scanf("%s", addr);
    strcat(tmp, addr);
    strcat(tmp, "\n");

    cout("Topic: ");
    char topic[NAME_SZ] = {0};
    //scanf("%s", topic);
    str_in(topic, NAME_SZ, 2);
    strcat(tmp, "Topic: ");
    strcat(tmp, topic);
    strcat(tmp, "\n");

    cout("Your message: ");
    char msg[DATA_SZ] = {0};
    //scanf("%s", msg);
    str_in(msg, DATA_SZ, 2);
    strcat(tmp, msg);
    strcat(tmp, "\n");
    
    strcat(pk->data, tmp);
    Send(server_socket, pk);	
    struct package* reply = Recive(server_socket, pk);
	if (strstr(reply->data, "OK") != NULL)
    {
        return reply->data;
    }
	else
    {
        return "Mail sanding error";
    }	    
}

char* STMP_write (int server_socket, struct package *pk, char* username)
{
    memset(pk->data, 0, DATA_SZ);
    pk->type = 6;
    char tmp[2 * DATA_SZ] = {0};
    cout("MAIL FROM: ");
    cout(username);
    strcat(tmp, "MAIL FROM ");
    strcat(tmp, username);
    strcat(pk->data, tmp);
    Send(server_socket, pk);
    struct package* reply = Recive(server_socket, pk);
	if (strstr(reply->data, "OK") != NULL)
    {
        cout("\n");
        cout (reply->data);
    }
	else
    {
        return "Mail sanding error";
    }

    memset(pk->data, 0, DATA_SZ);
    memset(tmp, 0, DATA_SZ);
    cout("\nRCPT TO: ");
    char addr[NAME_SZ] = {0};
    scanf("%s", addr);
    strcat(tmp, "RCPT TO ");
    strcat(tmp, addr);
    strcat(tmp, "\n");
    strcat(pk->data, tmp);
    Send(server_socket, pk);	
    reply = Recive(server_socket, pk);
	if (strstr(reply->data, "250") != NULL)
    {
        cout (reply->data);
        cout("\n");
    }
	else
    {
        return "Mail sanding error";
    }

    memset(pk->data, 0, DATA_SZ);
    memset(tmp, 0, DATA_SZ);
    cout("DATA: ");
    char msg[NAME_SZ] = {0};
    strcat(tmp, "DATA ");
    //scanf("%s", msg);
    str_in(msg, DATA_SZ, 2);
    strcat(tmp, msg);
    strcat(tmp, "\n\n");
    strcat(pk->data, tmp);
    Send(server_socket, pk);	
    reply = Recive(server_socket, pk);
	if (strstr(reply->data, "354") != NULL)
    {
         cout(reply->data);
         cout("\n");
    }
	else
    {
        return "Mail sanding error";
    }
        
    reply = Recive(server_socket, pk);
	if (strstr(reply->data, "250") != NULL)
    {
        return (reply->data);
    }
	else
    {
        return "Mail sanding error";
    } 
}

void reset_package(struct package* pk)
{
    memcpy(pk->header, head, HEAD_SZ);
    memcpy(pk->ender, end, END_SZ);
    memset(pk->data, 0, DATA_SZ);
}

int check_package(struct package* pk)
{
    if(!memcmp(pk->header, head, HEAD_SZ) && !memcmp(pk->ender, end, END_SZ))
        return 1;
    return 0;
}

void itoa(int num, char *buff, int radix)
{
    static int i;
    i = 0;
    int ost = num % radix;
    num/=radix;
    if(num) itoa(num, buff, radix);
    buff[i] = ost + '0';
    buff[i+1] = '\0';
    i++;
}

int cout(char* str){
    return write(STDOUT_FILENO, str, strlen(str));
}

int str_in(char *str, int n, int mode)
{
    int count = 0; // счётчик символов
    n--;
    if(mode == 1){
        echo();
        while(count < n){
            read(STDIN_FILENO, str + count, 1);
            write(STDOUT_FILENO, "*", 1);

            if(str[count] == 27){ // escape
                clear();
                str[count] = '\0';
                return 0;
            }

            if(str[count] == 127){ // backspase
                if(count == 0){
                    backspace(1);
                    continue;
                }

                if(count > 0){
                    backspace(2);
                    count-=1;
                    continue;
                }
            }

            if(str[count] == '\n'){ // enter
                if(count == 0){
                    backspace(1);
                    continue;
                }

                if(count > 0){
                    str[count] = '\0';
                    echo();
                    backspace(1);
                    write(STDOUT_FILENO, "\n", 1);
                    return 1;
                }
            }

            count++;
            str[count] = '\0';
        }
        echo();
        return 0;
    }
    if(mode == 2){
        while(count < n){
            read(STDIN_FILENO, str + count, 1);

            if(str[count] == 27){ // escape
                clear();
                str[count] = '\0';
                return 0;
            }

            if(str[count] == 127){ // backspase
                if(count == 0){
                    backspace(2);
                    count = -1;
                }

                if(count > 0){
                    backspace(3);
                    count-=2;
                }
            }

            if(str[count] == '\n'){ // enter
                if(count == 0)
                    count = -1;

                if(count > 0){
                    str[count] = '\0';
                    return 1;
                }
            }

            count++;
            str[count] = '\0';
        }
        return 0;
    }
    return 0;
}

void backspace(int n){
    while(n > 0){
        write(STDOUT_FILENO, "\b \b", 3);
        n--;
    }
}

void echo(){
    struct termios temp;
    tcgetattr(STDIN_FILENO, &temp);
    if(temp.c_lflag & ECHO)
        temp.c_lflag &= ~ECHO;
    else
        temp.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &temp);
}
