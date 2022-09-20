#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

/*
 * Liu Jiapeng 19/9/22
 *
 *
 */

#define MAX_LINE 80 /* 命令所支持的最大长度*/
#define MAX_HIS 10  /* 支持存储的最多命令条数 */

/* 输入函数 */
int get_input(char *input)
{
    char c;
    int len = 0;
    while ((c = getchar()) != '\n' && len < MAX_LINE * 2)
    {
        input[len++] = c;
    }
    if (len > MAX_LINE)
    {
        printf("Error!The maximum length of command is %d\n", MAX_LINE);
        return 0;
    }
    input[len] = '\0';
    return 1;
}

/* args参数赋值函数 */
int get_args(char *args[], char *input, int *run_together)
{
    int cnt = 0; /* args参数个数 */
    int len = strlen(input);
    for (int i = 0; i < len; i++)
    {
        while (input[i] == ' ' || input[i] == '\t')
        { //跳过前导空格&Tap
            i++;
            continue;
        }
        int start = i, end = i;
        while (input[i] != ' ' && input[i] != '\t' && i < len)
        {
            end = ++i;
        }
        if (end > start) //非空才存入args数组，排除后置空格&Tap
        {
            args[cnt] = (char *)malloc(sizeof(end - start));

            for (int i = start; i < end; i++)
            {
                args[cnt][i - start] = input[i];
            }
            cnt++;
        }
    }
    args[cnt] = NULL; //以NULL结尾的数组，方能被execvp()函数调用执行子进程
    if (strcmp(args[cnt - 1], "&") == 0)
    { //有"&"在结尾输入，可后台运行
        *run_together = 1;
        args[--cnt] = NULL;
    }
    return cnt;
}

/* 历史命令记录函数，用类似固定长度的队列方式实现*/
void add_history(char *history[], char *input, int *his_num)
{
    if (*his_num < MAX_HIS)
    {
        history[*his_num] = (char *)malloc(sizeof(input));
        memcpy(history[*his_num], input, sizeof(input));
        (*his_num)++;
    }
    else
    {
        for (int i = 0; i < MAX_HIS - 1; i++)
        {
            history[i] = (char *)malloc(sizeof(history[i + 1]));
            memcpy(history[i], history[i + 1], sizeof(history[i + 1]));
        }
        history[MAX_HIS - 1] = (char *)malloc(sizeof(input));
        memcpy(history[MAX_HIS - 1], input, sizeof(input));
    }
}

/* 历史命令输出函数 */
void print_history(char *history[])
{
    for (int i = MAX_HIS - 1; i >= 0; i--)
    {
        if (history[i] == NULL)
            continue;
        printf("%d %s\n", i + 1, history[i]);
    }
}

int main(void)
{
    char *args[MAX_LINE / 2 + 1]; /* 命令行参数 */
    char *history[MAX_HIS];       /* 记录执行过的命令 */
    for (int i = 0; i < MAX_HIS; i++)
    {
        history[i] = NULL;
    }
    char input[MAX_LINE * 2];
    int his_num = 0;    /* 执行过的命令的个数 */
    int should_run = 1; /* 标记是否继续运行（exit与否） */

    while (should_run)
    {
        wait(NULL); /* 回收之前同步运行但是没有wait的僵尸进程？*/
        printf("osh>");
        fflush(stdout);
        int run_together = 0;
        if (!get_input(input))
        {
            continue;
        }
        int num = get_args(args, input, &run_together);

        /*exit处理*/
        if (strcmp(args[0], "exit") == 0 && num == 1)
        {
            should_run = 0;
            continue;
        }

        /* history处理 */
        if (strcmp(args[0], "history") == 0 && num == 1)
        {
            print_history(history);
            continue;
        }

        /* !!处理 */
        if (strcmp(args[0], "!!") == 0 && num == 1)
        {
            if (his_num == 0)
            {
                puts("No commands in history.");
                continue;
            }
            else if (his_num <= 10)
            {
                memcpy(input, history[his_num - 1], sizeof(history[his_num - 1]));
            }
            else
            {
                memcpy(input, history[MAX_HIS - 1], sizeof(history[MAX_HIS - 1]));
            }
            get_args(args, input, &run_together);
        }

        /* !n处理 */
        if (num == 1)
        {
            if (args[0][0] == '!' && args[0][1] >= '0' && args[0][1] <= '9')
            {
                int N = args[0][1] - '0';
                if (history[N] == NULL)
                {
                    puts("No such command in history.");
                }
                else
                {
                    memcpy(input, history[N - 1], sizeof(history[N]));
                    get_args(args, input, &run_together);
                }
            }
        }
        int pid = fork();
        if (pid < 0)
        {
            puts("Fail to fork!");
            return -1;
        }
        else if (pid == 0) /* pid为0，是子进程 */
        {
            // printf("child %d is running\n",getpid());
            int status = execvp(args[0], args); // execvp函数执行成功，不返回；失败则返回-1
            if (status == -1)
            {
                puts("Please input the command correctly");
                exit(0);
            }
            exit(0);
        }
        else /*pid>0，是父进程 */
        {
            add_history(history, input, &his_num);
            // puts("parent is running");
            if (run_together)
            { /*有&，一起运行*/
                puts("Parent is still running");
            }
            else
            { /*无&，等待子进程运行结束再继续*/
                // puts("waiting for the child");
                int pid = wait(NULL);
                // printf("the child %d is over\n",pid);
            }
        }
    }
    return 0;
}
