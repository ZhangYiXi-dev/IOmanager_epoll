#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <vector>
#include <algorithm>
using namespace std;

#define MAXLINE 10
#define SERV_PORT 12160
//#define SERV_PORT 80
//#define SERV_PORT 8081
//默认参数
int C = 1;
int N = 1;
int ERR = 0;
//string server_addr = "81.69.220.3";
//string server_addr = "202.108.22.5";
string server_addr = "119.23.249.11";
string msgs = "hello_tcp\n";
int n_count = 0;//发消息的数量
vector<float> time_usr;
pthread_mutex_t mutex;
void parameter_deal(int argc, char* argv[]) //参数处理函数
{
    for (int i = 1; i < argc; i++)
    {
        string strs = argv[i];
        char s = strs[1];
        string str;
        switch (s)
        {
            case 'c':
                str.assign(strs.begin() + 2, strs.end());
                C = atoi(str.c_str());
                break;
            case 'n':
                str.assign(strs.begin() + 2, strs.end());
                N = atoi(str.c_str());
                break;
            case 's':
                str.assign(strs.begin() + 2, strs.end());
                msgs = str;
                break;
            case 'h':
                str.assign(strs.begin() + 2, strs.end());
                server_addr = str;
                break;
        }
           
    }
}
string get_time(string ss)
{
    struct timeval us;
    gettimeofday(&us, NULL);
  
    struct tm t;
    char date_time[1024];
    strftime(date_time, sizeof(date_time), "%Y-%m-%d %H:%M:%S", localtime_r(&us.tv_sec, &t));
    printf("%s: date_time=%s, tv_msec=%ld\n",ss.c_str(),date_time, us.tv_usec/1000);
    string re(date_time,date_time+strlen(date_time));
    string mm;
    if(us.tv_usec/1000<100)
    {
    	 mm='0'+to_string(us.tv_usec/1000);
    }
    else
    {
    	 mm=to_string(us.tv_usec/1000);
    }
    re=re+":"+mm;
    return re;
}
void* tfn(void* arg)
{
    pthread_detach(pthread_self());
    struct timeval t1,t2;
    while(true)
    {
        pthread_mutex_lock(&mutex);
	char buf[1024];
        if (n_count >=N )
        {
            break;
        }
        pthread_mutex_unlock(&mutex);
        gettimeofday(&t1, NULL);
        struct sockaddr_in servaddr;
        int sockfd;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        inet_pton(AF_INET, server_addr.c_str(), &servaddr.sin_addr);
        servaddr.sin_port = htons(SERV_PORT);
        int ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if (ret != 0)
        {
            perror("connect error");
            pthread_mutex_lock(&mutex);
            n_count++;
            ERR++;
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        write(sockfd, msgs.c_str(), msgs.size());
	read(sockfd,buf,sizeof(buf));
        gettimeofday(&t2, NULL);
        float deltaT = ((t2.tv_sec - t1.tv_sec) * 1000 + (t2.tv_usec - t1.tv_usec) / 1000.0);
        pthread_mutex_lock(&mutex);
        time_usr.push_back(deltaT);
        n_count++;
        pthread_mutex_unlock(&mutex);
        close(sockfd);
    }
    return NULL;
}
void Percent()
{
    int t_size = time_usr.size();
    cout<<t_size<<endl;
    sort(time_usr.begin(), time_usr.end());
    cout << "50%    " << time_usr[int(t_size / 2)] << endl;
    cout << "66%    " << time_usr[int(t_size * 0.66)] << endl;
    cout << "75%    " << time_usr[int(t_size * 0.75)] << endl;
    cout << "80%    " << time_usr[int(t_size * 0.8)] << endl;
    cout << "90%    " << time_usr[int(t_size * 0.9)] << endl;
    cout << "95%    " << time_usr[int(t_size * 0.95)] << endl;
    cout << "98%    " << time_usr[int(t_size * 0.98)] << endl;
    cout << "99%    " << time_usr[int(t_size * 0.99)] << endl;
    cout << "100%    " << time_usr[t_size - 1] << endl;
}
void QPS(string start,string end)
{
    int start_t[4];
    int end_t[4];
    int temp=0;
    for(int i=11;i<20;i+=3)
    {
    	string s(start.begin()+i,start.begin()+i+2);
    	string e(end.begin()+i,end.begin()+i+2);
	start_t[temp]=(s[0]-'0')*10+(s[1]-'0');
	end_t[temp]=(e[0]-'0')*10+(e[1]-'0');
	temp++;
    }
    	string s(start.begin()+20,start.begin()+23);
    	string e(end.begin()+20,end.begin()+23);
	start_t[3]=(s[0]-'0')*100+(s[1]-'0')*10+(s[2]-'0');
	end_t[3]=(e[0]-'0')*100+(e[1]-'0')*10+(e[2]-'0');
    int result=(end_t[0]-start_t[0])*3600000+(end_t[1]-start_t[1])*60000+(end_t[2]-start_t[2])*1000+(end_t[3]-start_t[3]);
    cout<<"QPS: "<<(double)N/result*1000<<endl;
}
int main(int argc, char *argv[])
{ 
    string t_start,t_end;
    int mu = pthread_mutex_init(&mutex, NULL);
    parameter_deal(argc,argv); //提取参数
    pthread_t* tid = new pthread_t[C];
    int* re[C];
    t_start= get_time("test start time");//获取程序开始时间
    for (int i = 0; i < C; i++)
    {
        int ret = pthread_create(&tid[i], NULL, tfn, NULL);
        if (ret != 0)
        {
            perror("pthread create error");
            exit(1);
        }
    }
    while(n_count<N)
    {
    	;
    }
    t_end = get_time("test start time");
    cout << "ERROR NUM " << ERR << endl;
    QPS(t_start,t_end);
 //   cout << "QPS: " << ((float)N/((t_end.tv_sec - t_Start.tv_sec) * 1000 + (t_end.tv_usec - t_Start.tv_usec) / 1000))*1000;
    Percent();
    return 0;
}

