#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>

#define COMMAND_SIZE 1024
#define FORMAT "[%s@%s %s]# "

// 下面是shell定义的全局数据

// 1. 命令行参数表
#define MAXARGC 128
char *g_argv[MAXARGC];
int g_argc = 0;

// 2. 环境变量表
#define MAX_ENVS 100
char *g_env[MAX_ENVS];
int g_envs = 0;

// 4. 关于重定向，我们关心的内容
#define NONE_REDIR 0
#define INPUT_REDIR 1
#define OUTPUT_REDIR 2
#define APPEND_REDIR 3

// for test
char cwd[1024];
char cwdenv[1024];

// last exit code
int lastcode = 0;

// last pwd
std::string lastpwd;

int redir = NONE_REDIR;
std::string filename;

const char *GetUserName()
{
    const char *username = getenv("USER");
    return username == NULL ? "None" : username;
}

const char *GetHostName()
{
    const char *hostname = getenv("HOSTNAME");
    return hostname == NULL ? "None" : hostname;
}

const char *GetPwd()
{
    const char *pwd = getcwd(cwd, sizeof(cwd));
    if (pwd != NULL)
    {
        snprintf(cwdenv, sizeof(cwdenv), "PWD=%s", cwd);
        putenv(cwdenv);
    }
    return pwd == NULL ? "None" : pwd;
}

const char *GetHome()
{
    char *home = getenv("HOME");
    return home == NULL ? "Nome" : home;
}

void InitEnv()
{
    extern char **environ;
    memset(g_env, 0, sizeof(g_env));
    g_envs = 0;
    // 本来要从配置文件来
    // 1. 获取环境变量
    for (int i = 0; environ[i]; i++)
    {
        // 1.1 申请空间
        g_env[i] = (char *)malloc(strlen(environ[i] + 1));
        strcpy(g_env[i], environ[i]);
        g_envs++;
    }
    g_env[g_envs] = NULL;

    // 2. 导成环境变量
    for (int i = 0; g_env[i]; i++)
    {
        putenv(g_env[i]);
    }
    environ = g_env;

    lastpwd = GetHome();
}

void cleanup()
{
    for (int i = 0; g_env[i]; i++)
    {
        free(g_env[i]);
    }
    g_envs = 0;
}

// command
bool Cd()
{
    if (g_argc == 1)
    {
        std::string home = GetHome();
        if (home.empty())
            return true;
        lastpwd = home;
        chdir(home.c_str());
    }
    else
    {
        std::string where = g_argv[1];
        if (where == "-")
        {
            // 切换到上一个目录
            std::string pwd = GetPwd();
            chdir(lastpwd.c_str());
            lastpwd = pwd;
        }
        else if (where == "~")
        {
            // 切换到家目录
            std::string pwd = GetPwd();
            chdir(GetHome());
            lastpwd = pwd;
        }
        else
        {
            std::string pwd = GetPwd();
            chdir(where.c_str());
            lastpwd = pwd;
        }
    }
    return true;
}

void Echo()
{
    if (g_argc == 2)
    {
        std::string opt = g_argv[1];

        if (opt == "$?")
        {
            std::cout << lastcode << std::endl;
            lastcode = 0;
        }

        else if (opt[0] == '$')
        {
            std::string env_name = opt.substr(1);
            const char *env_ret = getenv(env_name.c_str());
            if (env_ret)
            {
                std::cout << env_ret << std::endl;
            }
        }
        else
        {
            std::cout << opt << std::endl;
        }
    }
}

void Export()
{
    putenv(g_argv[1]);
}

// / /a/b/c
std::string DirName(const char *pwd)
{
    std::string currentpwd = GetPwd();
    int pos = currentpwd.rfind('/');
    return currentpwd.substr(pos + 1);
}

void PrintCommandPrompt()
{
    char CommandPrompt[COMMAND_SIZE];
    snprintf(CommandPrompt, sizeof(CommandPrompt), FORMAT, GetUserName(), GetHostName(), DirName(GetPwd()).c_str());
    printf("%s", CommandPrompt);
    fflush(stdout);
}

// 获取命令行参数
bool GetCommandLine(char *out, int size)
{
    char *ret = fgets(out, size, stdin);
    if (ret == NULL)
        return false;
    out[strlen(out) - 1] = '\0';
    if (strlen(out) == 0)
        return false;
    return true;
}

// 3. 命令行分析 "ls -a -l" -> "ls" "-a" "-l"
bool CommandParse(char *commandline)
{
#define SEP " "
    g_argc = 0;
    g_argv[g_argc++] = strtok(commandline, SEP);
    while ((bool)(g_argv[g_argc++] = strtok(nullptr, SEP)))
        ;
    g_argc--;
    return g_argc > 0 ? true : false;
}

bool CheckAndExecBuiltin()
{
    // 如果内键命令做重定向，更改shell的标准输入，输出，错误
    std::string cmd = g_argv[0];
    if (cmd == "cd")
    {
        Cd();
        return true;
    }
    else if (cmd == "echo")
    {
        Echo();
        return true;
    }
    else if (cmd == "export")
    {
        Export();
        return true;
    }
    return false;
}

int Execute()
{
    pid_t id = fork();
    if (id == 0)
    {
        int fd = -1;
        // 子进程检测重定向情况
        if (redir == INPUT_REDIR)
        {
            fd = open(filename.c_str(), O_RDONLY);
            if (fd < 0)
                exit(1);
            dup2(fd, 0);
            close(fd);
        }
        else if (redir == OUTPUT_REDIR)
        {
            fd = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
            if (fd < 0)
                exit(1);
            dup2(fd, 1);
            close(fd);
        }
        else if (redir == APPEND_REDIR)
        {
            fd = open(filename.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
            if (fd < 0)
                exit(2);
            dup2(fd, 1);
            close(fd);
        }
        else
        {
        }

        execvp(g_argv[0], g_argv);
        exit(1);
    }

    int status = 0;

    pid_t retid = waitpid(id, &status, 0);
    if (retid > 0)
    {
        lastcode = WEXITSTATUS(status);
    }
    return 0;
}

void TrimSpace(char cmd[], int &end)
{
    while (isspace(cmd[end]))
    {
        end++;
    }
}

void RedirCheck(char cmd[])
{
    redir = NONE_REDIR;
    filename.clear();
    int start = 0;
    int end = strlen(cmd) - 1;

    //"ls -a -l >> file.txt" > >> <
    while (end > start)
    {
        if (cmd[end] == '<')
        {
            cmd[end++] = 0;
            TrimSpace(cmd, end);
            redir = INPUT_REDIR;
            filename = cmd + end;
            break;
        }
        else if (cmd[end] == '>')
        {
            if (cmd[end - 1] == '>')
            {
                cmd[end - 1] = 0;
                redir = APPEND_REDIR;
            }
            else
            {
                redir = OUTPUT_REDIR;
            }
            cmd[end++] = 0;
            TrimSpace(cmd, end);
            filename = cmd + end;
            break;
        }
        else
        {
            end--;
        }
    }
}

int main()
{
    // shell 启动的时候，从系统中获取环境变量
    // 我们的环境变量信息应该从父shell统一来
    InitEnv();

    while (true)
    {
        // 1. 输出命令行提示符
        PrintCommandPrompt();

        // 2. 获取用户输入的命令
        char commandline[COMMAND_SIZE];
        if (!GetCommandLine(commandline, sizeof(commandline)))
            continue;

        // 3. 重定向分析 "ls -a -l > file.txt" -> "ls -a -l" "file.txt" -> 判定重定向方式
        RedirCheck(commandline);

        // 4. 命令行分析 "ls -a -l" -> "ls" "-a" "-l"
        if (!CommandParse(commandline))
            continue;
        // PrintArgv();

        // 5. 检测并处理内键命令
        if (CheckAndExecBuiltin())
            continue;

        // 6. 执行命令
        Execute();
    }

    // 清理环境变量表申请的资源
    cleanup();

    return 0;
}
