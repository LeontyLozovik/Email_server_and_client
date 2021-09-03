#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#define PORT "3490"
#define checkpoint() write(STDOUT_FILENO, "check point\n", 12)
#define MAILADDR "Maddr.txt"
#define PASSWORD "password.txt"
#define Inbox "inbox.txt"
#define DIRPATH "/home/leontiy/SPOVM/kursach/Inbox"

#define head "head"
#define end "end"

#define HEAD_SZ 4
#define NAME_SZ 32
#define DATA_SZ 1024
#define END_SZ 3
#define PATH_LEN 256


#define LOGIN 1
#define SELECT 2
#define FETCH 3
#define LOGOUT 4
#define LIST 5
#define MAIL 6

int mailcount = 3;
char username[NAME_SZ] = {0};
char password[NAME_SZ] = {0};

struct package
{
    char header[HEAD_SZ];
    int type;
    int num;
    char data[DATA_SZ];
    char ender[END_SZ];
};

int listen_client(int);
void reset_package(struct package* pk);
int check_package(struct package* pk);
void itoa(int num, char *buff, int radix);
int cout(char* str);
int Send(int user_socket, struct package *pk);
struct package* Recive(int user_socket, struct package *pk);
void Login(int user_socket, struct package *pk);
void Open_mailbox(int user_socket, struct package *pk);
void Open_message(int user_socket, struct package *pk);
int Logout(int user_socket, struct package *pk);
void find_msg(char* username, int num, char* mail);
void List(int user_socket, struct package *pk);
void write_msg(int user_socket, struct package *pk, char* username);
char* STMP_write (int user_socket, struct package *pk, char* username);

int main()
{
    cout("hello world!\n");
    
    int status, server_socket, user_socket;
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    status = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if(status)
    {
        cout("getaddrinfo error\n");
        exit(1);
    }

    server_socket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(server_socket < 0)
    {
        cout("socket is not created\n");
        freeaddrinfo(servinfo);
        exit(1);
    }
    status = bind(server_socket, servinfo->ai_addr, servinfo->ai_addrlen);
    if(status < 0)
    {
        cout("can't bind port\n");
        freeaddrinfo(servinfo);
        exit(1);
    }

    int out = 0;

    while(!out)
    {
        sleep(1);
        status = listen(server_socket, 1);
        if(status < 0)
        {
            cout("listen error\n");
            freeaddrinfo(servinfo);
            exit(1);
        }
        addr_size = sizeof client_addr;
        user_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if(user_socket < 0)
        {
            cout("accept error");
            freeaddrinfo(servinfo);
            exit(1);
        }
        out = listen_client((user_socket));
    }
    cout("Goodbye");
    return 0;
}

int listen_client(int args)
{
    int user_socket  = args;
    struct package *pk = (struct package*)malloc(sizeof(struct package));
    int disconect_status = 0;

    while(!disconect_status){
        reset_package(pk);
        cout("\ngetting a package\n");
        if(recv(user_socket, (char*)pk, sizeof(struct package), 0) > 0);// && check_package(pk))
        {
            switch(pk->type)
            {
                case LOGIN:
                {
                    cout("Try to log in\n");
                    Login(user_socket, pk);
                    break;
                }
                case LOGOUT:
                {
                    disconect_status = Logout(user_socket, pk);
                    cout("disconnected\n");
                    break;
                }
                case FETCH:
                {
                    Open_message(user_socket, pk);
                    break;
                }
                case SELECT:
                {
                    Open_mailbox(user_socket, pk);
                    break;
                }
                case LIST:
                {
                    List(user_socket, pk);
                    break;
                }
                case MAIL:
                {
                    puts("try to write\n");
                    write_msg(user_socket, pk, username);
                    //STMP_write(user_socket, pk, username);
                    break;
                }
            }
        }
    }
    return 1;
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

void Login(int user_socket, struct package *pk)
	{
        if(strstr(pk->data, "LOGIN ") == NULL)
        {
            memset(pk->data, 0, DATA_SZ);
            strcat(pk->data, "error");
        }

        char un[NAME_SZ] = {0};
        char pw[NAME_SZ] = {0};

        FILE* fd;

        if ((fd = fopen(MAILADDR, "r")) == NULL)
        {
		    puts("File opening error\n");
	    }
        else
        {
            fgets(un, NAME_SZ, fd);
        }
        fclose(fd);
 
        if ((fd = fopen(PASSWORD, "r")) == NULL)
        {
		    puts("File opening error\n");
	    }
        else
        {
            fgets(pw, NAME_SZ, fd);
        }
        fclose(fd);

        for(int i = 0; i < (strlen(un) - 1); i++)
        {
            username[i] = un[i];
        }

        for(int i = 0; i < (strlen(pw) - 1); i++)
        {
            password[i] = pw[i];
        }

        cout("----------------------------------------\n");
        cout(username);
        cout("\n");
        cout(password);
        cout("\n");
        cout(pk->data);
        cout("\n");
        cout("----------------------------------------\n");
        if(strstr(pk->data, username) != NULL && strstr(pk->data, password) != NULL)
        {
            memset(pk->data, 0, DATA_SZ);
            strcat(pk->data, "OK USER LOGGED IN");
        }
        else
        {
            memset(pk->data, 0, DATA_SZ);
            strcat(pk->data, "error");
        }
        cout("login process has ended\n");
		Send(user_socket, pk);
	}

int Send(int user_socket, struct package *pk)
	{
		int n = send(user_socket, (char*)pk, sizeof(struct package), 0);

		if (n < 0) 
			 perror("ERROR writing to socket");

		return n;
	}

struct package* Recive(int user_socket, struct package *pk)
	{
        reset_package(pk);
		int n = (recv(user_socket, (char*)pk, sizeof(struct package), 0) > 0 && check_package(pk));

		if (n < 0)
			 perror("ERROR reading from socket");

		return pk;
	}

void List(int user_socket, struct package *pk)
{
    cout("Finding boxes\n");
    if(strstr(pk->data, "LIST ") == NULL)
    {
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "error");
    }
    else
    {
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "* LIST INBOX\n* LIST INBOX/Drafts\n* LIST INBOX/Junk\n* LIST INBOX/Sent\n* LIST INBOX/Trash\nOK COMPLETED");
    }
	Send(user_socket, pk);
}

void Open_mailbox(int user_socket, struct package *pk)
{
    cout("Opening a mail box\n");
    if(strstr(pk->data, "SELECT ") == NULL)
    {
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "error");
    }
    else if(strstr(pk->data, "/Drafts") != NULL || strstr(pk->data, "/Junk") != NULL || strstr(pk->data, "/Sent") != NULL || strstr(pk->data, "/Trash") != NULL)
    {
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "Mailbox is empty");
    }
    else if(strstr(pk->data, "INBOX") == NULL)
    {
    	memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "Non such mailbox exists");
    }
    else
    {
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "OK [READ-WRITE] Completed");
    }
	Send(user_socket, pk);
}

void Open_message(int user_socket, struct package *pk)
{
    cout("Opening message\n");
    cout(pk->data);
    if(strstr(pk->data, "FETCH") == NULL)
    {
        cout("err");
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "error");
    }
    else
    {
        cout("ok");
        char str_num[NAME_SZ];
        int num = pk->num;
        /*int i = 0;
        cout("1\n");
        do
        {
            str_num[i] = pk->data[9 + i];
            i++;
        } while (pk->data[10 + i] != ' ');
        cout(str_num);
        num = atoi(str_num);*/
        char mail[DATA_SZ] = {0}; 
        find_msg(username, num, mail);
        //функция поиска сообщеня в директории по номеру
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "OK COMPLETED\n");
        strcat(pk->data, mail);
    }
	Send(user_socket, pk);
}

int Logout(int user_socket, struct package *pk)
{
    if(strstr(pk->data, "LOGOUT ") == NULL)
    {
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "error");
        Send(user_socket, pk);
        return 0;
    }
    else
    {
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "* BYE LOGOUT received");
        Send(user_socket, pk);
        shutdown(user_socket, 2);
        return 1;
    }	
}

void find_msg(char* username, int num, char* mail)
{
    char path[3 * NAME_SZ] = {0};
    strcat(path, DIRPATH);
    strcat(path, username);
    strcat(path, "/mail");
    char ms_num[HEAD_SZ];
    itoa(num, ms_num, 10);
    strcat(path, ms_num);
    cout(path);

    FILE* fd;
    if ((fd = fopen(path, "r+")) == NULL)
    {
	    puts("File opening error\n");
	}
    else
    {
        char buff[2 * NAME_SZ] = {0};
        while (!feof(fd))
        {
            fgets(buff, 2*NAME_SZ, fd);
            strcat(mail, buff);
        }  
    }
    fclose(fd);
}

void write_msg(int user_socket, struct package *pk, char* username)
{
    cout("\n\n");
    cout(pk->data);
    cout("\n\n");
    char filename[3 * NAME_SZ] = {0};
    strcat(filename, DIRPATH);
    strcat(filename, username);
    strcat(filename, "/mail");
    char str_num[HEAD_SZ];
    mailcount++;
    itoa(mailcount, str_num, 10);
    strcat(filename, str_num);
    FILE* fd;
    if ((fd = fopen(filename, "w+")) == NULL)
    {
	puts("File opening error\n");
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "error");
	}
    else
    {
        if(!fprintf(fd, "%s", pk->data))
        {
            puts("File opening error\n");
            memset(pk->data, 0, DATA_SZ);
            strcat(pk->data, "error");
        }
        else
        {
            memset(pk->data, 0, DATA_SZ);
            strcat(pk->data, "OK");
        }
    }
    fclose(fd);
    Send(user_socket, pk);
}

char* STMP_write (int user_socket, struct package *pk, char* username)
{
    char tmp[DATA_SZ] = {0};
    if(strstr(pk->data, "MAIL FROM") == NULL)
    {
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "error");
        Send(user_socket, pk);
        return 0;
    }
    else
    {
        int len = strlen(pk->data) - 10;
        char buff[NAME_SZ] = {0};
        for(int i = 0; i < len; i++)
        {
            buff[i] = pk->data[10 + i];
        }
        strcat(tmp, "<");
        strcat(tmp, buff);
        strcat(tmp, ">");
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "250 OK");
        Send(user_socket, pk);
    }	

    struct package* reply = Recive(user_socket, pk);
    if(strstr(reply->data, "RCPT TO ") == NULL)
    {
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "error");
        Send(user_socket, pk);
        return 0;
    }
    else
    {
        int len = strlen(pk->data) - 8;
        char buff[NAME_SZ] = {0};
        for(int i = 0; i < len; i++)
        {
            buff[i] = pk->data[8 + i];
        }
        strcat(tmp, "\nTo: ");
        strcat(tmp, buff);
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "250 OK");
        Send(user_socket, pk);
    }
    
    memset(reply->data, 0, DATA_SZ);
    reply = Recive(user_socket, pk);
    if(strstr(reply->data, "DATA ") == NULL)
    {
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "error");
        Send(user_socket, pk);
        return 0;
    }
    else
    {
        int len = strlen(pk->data) - 5;
        char buff[NAME_SZ] = {0};
        for(int i = 0; i < len; i++)
        {
            buff[i] = pk->data[5 + i];
	}
        strcat(tmp, "Topic: \n\n");
        strcat(tmp, buff);
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "354 OK");
        Send(user_socket, pk);
    }		


    char filename[3 * NAME_SZ] = {0};
    strcat(filename, DIRPATH);
    strcat(filename, username);
    strcat(filename, "/mail");
    char str_num[HEAD_SZ];
    mailcount++;
    itoa(mailcount, str_num, 10);
    strcat(filename, str_num);
    FILE* fd;
    if ((fd = fopen(filename, "w+")) == NULL)
    {
	    puts("File opening error\n");
        memset(pk->data, 0, DATA_SZ);
        strcat(pk->data, "error");
	}
    else
    {
        if(!fprintf(fd, "%s", tmp))
        {
            puts("File opening error\n");
            memset(pk->data, 0, DATA_SZ);
            strcat(pk->data, "error");
        }
        else
        {
            memset(pk->data, 0, DATA_SZ);
            strcat(pk->data, "250 OK");
        }
    }
    fclose(fd);
    Send(user_socket, pk);
}

int cout(char* str){
    return write(STDOUT_FILENO, str, strlen(str));
}
