#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

long long start_time;
long long students_left = 0;
long long time_wasted = 0;

sem_t sem;

struct student
{
    int id;
    int time;
    int wash_time;
    int patience;
    int wash_start_time;
} stu;

int cmpfunc(struct student *stu1, struct student *stu2)
{
    if (stu1->time > stu2->time)
        return 1;
    else
        return 0;
}

void *washing(void *arg)
{
    struct timespec ts;
    struct student *stu = (struct student *)arg;

    // print in white
    printf("\033[0;37m");
    printf("%lld: Student %d arrives\n", time(NULL) - start_time, stu->id);

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    ts.tv_sec += stu->patience;
    ts.tv_nsec += 1000 * 1000 * 10;
    int s;

    while ((s = sem_timedwait(&sem, &ts)) == -1 && errno == EINTR)
        continue;
    if (s == -1)
    {
        if (errno == ETIMEDOUT)
        {
            // print in red
            printf("\033[31m");
            stu->wash_start_time = time(NULL) - start_time;
            printf("%lld: Student %d leaves without washing\n", time(NULL) - start_time, stu->id);
            students_left++;
            return NULL;
        }
        else
        {
            perror("sem_timedwait");
            exit(EXIT_FAILURE);
        }
    }
    // print in green
    printf("\033[32m");
    stu->wash_start_time = time(NULL) - start_time;
    printf("%lld: Student %d starts washing\n", time(NULL) - start_time, stu->id);

    sleep(stu->wash_time);
    // print in yellow
    printf("\033[33m");
    printf("%lld: Student %d leaves after washing\n", time(NULL) - start_time, stu->id);
    // print reset
    printf("\033[0m");
    sem_post(&sem);
    return NULL;
}

int main(void)
{
    int n, m;
    scanf("%d %d", &n, &m);
    struct student stu[n];
    sem_init(&sem, 0, m);

    for (int i = 0; i < n; i++)
    {
        stu[i].id = i + 1;
        scanf("%d %d %d", &stu[i].time, &stu[i].wash_time, &stu[i].patience);
        stu[i].wash_start_time = 0;
    }
    // create threads for students
    pthread_t student_threads[n];

    qsort(stu, n, sizeof(struct student), (void *)cmpfunc);

    start_time = time(NULL);

    for (int i = 0; i < n; i++)
    {
        sleep(stu[i].time - time(NULL) + start_time);
        pthread_create(&student_threads[i], NULL, washing, &stu[i]);
    }

    for (int i = 0; i < n; i++)
        pthread_join(student_threads[i], NULL);

    sem_destroy(&sem);

    printf("%lld\n", students_left);

    for( int i=0;i<n;i++)
        time_wasted += stu[i].wash_start_time - stu[i].time;

    printf("%lld\n", time_wasted);

    if ((float)students_left / n >= 0.25)
        printf("Yes\n");
    else
        printf("No\n");
}