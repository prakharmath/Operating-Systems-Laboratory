#include<bits/stdc++.h>
#include<sstream>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#define endl '\n'
using namespace std;
const int MAXN=200;
void execute(string input, int in_fd, int out_fd, int pre_out, int next_in) 
{
    istringstream iss(input);
    pid_t child_pid;
    bool wait_flag = 1;
    int l =0, r=0, bg=0,end=0;
    string l_file, r_file;
    vector<string> tokens;
    copy(istream_iterator<string>(iss),istream_iterator<string>(),back_inserter(tokens));
    for(int i=0;i<tokens.size();i++)
    {
            if(tokens[i]=="<")
            {
                l =1;
                l_file = tokens[i+1];
            }
            if(tokens[i]==">")
            {
                r=1;
                r_file = tokens[i+1];
            }
            if(tokens[i]=="&")
            {
                bg=1;
            }
            if(!l&&!r&&!bg)
                end++;
    }
        const char **argv = new const char *[end+1];
        for (int i = 0; i < end; i++)
        {
            argv[i] = tokens[i].c_str();
        }
        argv[end] = NULL;
        wait_flag = !bg;
        if ((child_pid = fork()) == 0)
        {
            if(r)
                out_fd = open(r_file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
            if(out_fd!=1) 
                dup2(out_fd, 1);
            if(l) 
                in_fd = open(l_file.c_str(), O_RDONLY);
            if(in_fd!=0)
                dup2(in_fd, 0);
            execvp(tokens[0].c_str(), (char **)argv);
            exit(0);
        }
        else
        {
            if(wait_flag)   //Wait for the child process to get completed
            {
                wait(NULL);
                if(in_fd!=0)
                    close(in_fd);
                if(out_fd!=1)
                    close(out_fd);
            }
            else
                cout<< "[ Process ] Background"<<endl;
        }
    return;
}
signed main()
{
       printf("-----------------This is the shell of Group 36----------\n\n\n");
       string input;
       while(1)    //Infinite loop for taking input from the user
       {
            printf("[user]>>>>");
            getline(cin,input);  //Taking the whole line as input using the inbuilt getline function of C++
            if(input=="exit")   //If you give "exit" as input to the shell it will break out of the shell
                break;
            vector<string>commands;
            int command_begin=0;
            int command_end=0;
            for(int i=0;input[i]!='\0';++i)
            {
                 if(input[i]=='|')
                 {
                      string current;
                      for(int j=command_begin;j<command_end;j++)
                        current+=input[j];
                      commands.emplace_back(current);
                      command_begin=i+1;
                      command_end=i+1;
                 }
                 else
                    ++command_end;
            }
            string current;
            for(int j=command_begin;j<command_end;j++)
                    current+=input[j];
            commands.emplace_back(current);
            int number_of_pipes=(int)commands.size()-1;
            int **pipes = new int *[number_of_pipes];
            for(int i=0;i<number_of_pipes;i++)
            {
                pipes[i]=new int[2];
                pipe(pipes[i]);
            }
            int i=0;
            for(auto &x:commands)
            {
                  int in_fd = 0, out_fd = 1;
                  int pre_out = -1, next_in = -1;
                  if(i>0) 
                  {
                     in_fd = pipes[i-1][0];
                     pre_out = pipes[i-1][1];
                  }
                  if(i<(int)commands.size()-1) 
                  {
                     out_fd = pipes[i][1];
                     next_in = pipes[i][0];
                  }
                  execute(commands[i], in_fd, out_fd, pre_out, next_in);
                  ++i;
            }  
            fflush(stdin);
            fflush(stdout);   
       }
       exit(0);
}