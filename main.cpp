#include<iostream>
#include<fstream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <string>
#include <pthread.h>
using namespace std;

#define SERVER_PORT 1234
#define QUEUE_SIZE 5


#define BUFSIZE 10000
#define PREF_COUNT 2

char *server = "127.0.0.1";	/* adres IP pętli zwrotnej */
char *protocol = "tcp";
short service_port = 1234;	/* port usługi daytime */

char bufor[BUFSIZE];

bool close_server=false;
string preferences[2];

class twitter_post {
public:
    string username;
    string post;
    string date;

};

class user {
public:
    string username;
    user(){};
};

struct remote_client_data {
    int client_socket;
    struct sockaddr_in* stClientAddr;
    socklen_t nTmp;
};

user* my_user;
fstream pref_file,my_tweets_file;

void clear_screen();
void write_tweet();
void read_tweets();
void* remote_client(void* arg);

void* server_func(void* arg)
{
   int nSocket, nClientSocket;
   int nBind, nListen;
   int nFoo = 1;
   socklen_t nTmp;
   struct sockaddr_in stAddr, stClientAddr;


   /* address structure */
   memset(&stAddr, 0, sizeof(struct sockaddr));
   stAddr.sin_family = AF_INET;
   stAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   stAddr.sin_port = htons(SERVER_PORT);

   /* create a socket */
   nSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (nSocket < 0)
   {
       fprintf(stderr, "%s: Can't create a socket.\n", "Twitter");
       exit(1);
   }
   setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&nFoo, sizeof(nFoo));

   /* bind a name to a socket */
   nBind = bind(nSocket, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
   if (nBind < 0)
   {
       fprintf(stderr, "%s: Can't bind a name to a socket.\n", "Twitter");
       exit(1);
   }
   /* specify queue size */
   nListen = listen(nSocket, QUEUE_SIZE);
   if (nListen < 0)
   {
       fprintf(stderr, "%s: Can't set queue size.\n","Twitter");
   }

   while(!close_server)
   {
       /* block for connection request */
       nTmp = sizeof(struct sockaddr);
       nClientSocket = accept(nSocket, (struct sockaddr*)&stClientAddr, &nTmp);
       if (nClientSocket < 0)
       {
           fprintf(stderr, "%s: Can't create a connection's socket.\n", "Twitter");
           exit(1);
       }

        pthread_t tid;
        struct remote_client_data data;
        data.client_socket = nClientSocket;
        data.stClientAddr = &stClientAddr;
        data.nTmp = nTmp;

        pthread_create(&tid,NULL,remote_client,(void*)&data);


       }

   close(nSocket);
    pthread_exit(NULL);
}

void* remote_client(void* arg) {

    cout<<"Remote Client Thread Created!\n";

    struct remote_client_data* client_data = (struct remote_client_data*)arg;

    int nClientSocket = client_data->client_socket;
    struct sockaddr_in stClientAddr = *client_data->stClientAddr;
    socklen_t nTmp = client_data->nTmp;

    printf("%s: [connection from %s]\n",
    "Twitter", inet_ntoa((struct in_addr)stClientAddr.sin_addr));
    time_t now;
    struct tm *local;
    time (&now);
    local = localtime(&now);
    my_tweets_file.open("my_tweets",ios_base::in);
    if(my_tweets_file.is_open()) cout<<"Opened tweets file\n";


    char read_buffer[512];
    string line;
    int n = read(nClientSocket,read_buffer,512);
    cout<<"User "<<read_buffer<<" connected!\n";
    if(n>0) {
        while(!my_tweets_file.eof()) {
            char buffer[4096];
            getline(my_tweets_file,line);
            line+='\n';

            //my_tweets_file.read(buffer,4096);
            write(nClientSocket, line.c_str(), line.size());
        }
    }
    my_tweets_file.close();
    close(nClientSocket);
}

void* local_client (void* arg)
{


    char resp;
    bool quit=false;
    while(!quit) {
        //clear_screen();
        cout<<"Witaj i tweetaj "<<my_user->username+"!\n"<<
                "Wybierz opcję z menu:\n"<<
                "1. [T]weetuj!\n"<<
                "2. [C]zytaj tweety Twoich znajomych.\n"<<
                "3. [S]ubskrybuj nowego użytkownika.\n"<<
                "4. [W]yjdź z programu.\n"<<endl;
        cin>>resp;
        if(resp=='1'||resp=='t'||resp=='T') {
            write_tweet();
        }
        else if(resp=='2'||resp=='c'||resp=='C') {
            read_tweets();
        }
        else if(resp=='3'||resp=='s'||resp=='S') {
        }
        else if(resp=='4'||resp=='w'||resp=='W') {
            quit=true;
        }
        else {
            cout<<"Brak takiej opcji. Spróbuj ponownie.\n";
        }


    }



}

void init_user() {

    cout << "Initialising twitter distributed app.";
    pref_file<<0<<endl;

    cout<<"Enter username: ";
    cin>>my_user->username;
    pref_file<<my_user->username<<endl;
}

int main() {

    my_user = new user();

    pref_file.open("twitter_pref",fstream::out);

    if(!pref_file.is_open()) {
        cout<<"Twitter - error opening preferences file. Exiting..."<<endl;
        return 1;
    }

    pref_file.seekp(0,ios::end);
    size_t fsize = pref_file.tellg();

    if( fsize == 0)
    {
        init_user();
    } else {
        pref_file.seekp(0,ios::beg);
        string var;
        int i=0;
        while(pref_file>>var) {
            preferences[i]=var;
            i++;
        }
        if(i!=PREF_COUNT) {
            cout<<"Pref file corrupted. Reinitialising..."<<endl;
            pref_file.close();
            remove("twitter_pref");
            pref_file.open("twitter_pref", fstream::out);
            init_user();
        }
    }


    pref_file.close();

    pthread_t tid;

    pthread_create(&tid,NULL,server_func,0);

    cout<<"main thread"<<endl;
    local_client(0);
    close_server=true;

    return 0;
}

void clear_screen() {
    cout << "\033[2J\033[1;1H";
}

void write_tweet() {

    cout<<"Tweetuj! Zakończ wprowadzanie posta klawiszem ESC i zatwierdz klawiszem ENTER\n";

    my_tweets_file.open("my_tweets",ios_base::out|ios_base::app);

    if(!my_tweets_file.is_open()) {
        cout<<"System error, cannot add tweets.\n"<<endl;
        cin.get();
        return;
    }

    //my_tweets_file.seekp(0,ios::end);

    string tweet="";
    bool reading=true;
    string line="";

    while(reading) {
        getline(cin,line);
        int linesize=line.size();
        for(int i=0;i<linesize;i++)
            if(line[i]==27) {
                reading=false;
                break;
            } else {
                tweet+=line[i];
            }
        tweet+='\n';
    }

    time_t timer;
    timer = time(NULL);

    struct tm *tweet_time = gmtime(&timer);

    my_tweets_file<<"username: {\n"<<my_user->username<<"\n}\n";
    my_tweets_file<<"date: {\n"<<tweet_time<<"\n}\n";

    my_tweets_file<<"content: {"<<tweet<<"}\n";

    my_tweets_file.close();

}


void read_tweets() {

	struct sockaddr_in sck_addr;

	int sck, odp;

	//cout<<"Podaj adres IP\n";
	//scanf("%s",server);

	memset (&sck_addr, 0, sizeof sck_addr);
	sck_addr.sin_family = AF_INET;
	inet_aton (server, &sck_addr.sin_addr);
	sck_addr.sin_port = htons (service_port);

	if ((sck = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror ("Nie można utworzyć gniazdka");
		exit (EXIT_FAILURE);
	}

	if (connect (sck, (struct sockaddr*) &sck_addr, sizeof sck_addr) < 0) {
		perror ("Brak połączenia");
		exit (EXIT_FAILURE);
	}


    write (sck, my_user->username.c_str(), my_user->username.size());


    cout<<"message from server:\n";
	while ((odp = read (sck, bufor, BUFSIZE)) > 0)
		write (1, bufor, odp);
	close (sck);
	cin.get();
}
