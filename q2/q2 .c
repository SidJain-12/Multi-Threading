#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#define debug printf("Reached line %d\n", __LINE__)

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define WHT   "\x1B[37m"
#define BLU   "\x1B[34m"
#define RESET "\x1B[0m"

#define MAX_LEN 100

typedef struct pizza_type {
    int id;
    int prep_time;
    int num_ingredients;
    int ingr[MAX_LEN];
} pizza_type;

typedef struct chef {
    int id;
    int arrival_time;
    int leaving_time;
    int available;
    sem_t lock;
} chef;

typedef struct pizza {
    int id;
    int order_id;
    int possible;
    int done;
} pizza;

typedef struct order {
    int id;
    int arrival_time;
    int num_pizzas;
    pizza pizza_types[MAX_LEN];
    int state;
    sem_t mutex;
} order;

typedef struct conditional {
    int valid;
    pthread_cond_t cond;
    int prep_time;
    int order_id;
    int pizza_id;
    pthread_mutex_t mutex;
    // int was_it_done;
} conditional;


pizza_type pizza_varieties[MAX_LEN];
int ingr_count[MAX_LEN];
chef chefs[MAX_LEN];
order orders[MAX_LEN];
int num_chefs, num_varieties, num_ingr, num_customers, num_ovens, pickup_time;
conditional pending_pizzas[MAX_LEN];

sem_t chef_sem, oven_sem, ingr_lock, pending_pizza_sem;

int closing_time = 0;
int time0;
struct timespec time_start;


int max(int a, int b) {
    return a > b ? a : b;
}

int get_possible_pizzas(int possible_pizzas[], pizza pizzas[]) { // returns index of pizza in pizza_varieties
    int count = 0;
    for (int i = 0; i < MAX_LEN; i++) {
        possible_pizzas[i] = -1;
        if(pizzas[i].id == -1) continue;
        int flag = 1;
        for(int j = 0; j < MAX_LEN; j++) {
            if(ingr_count[j] < pizza_varieties[i].ingr[j]) {
                flag = 0;
                break;
            }
        }
        possible_pizzas[i] = flag;
        count += flag;
    }
    return count;
}

int is_possible(int pizza_id) {
    for(int i = 0; i < MAX_LEN; i++) {
        if(ingr_count[i] < pizza_varieties[pizza_id].ingr[i]) {
            return 0;
        }
    }
    return 1;
}

int prepare_pizza(int pizza_id, int order_id) {
    sem_wait(&ingr_lock);
    int possible = is_possible(pizza_id);
    if (possible) {
        for(int i = 0; i < MAX_LEN; i++) {
            ingr_count[i] -= pizza_varieties[pizza_id].ingr[i];
        }
    }
    sem_post(&ingr_lock);
    return possible;
}

void refund_pizza(int pizza_id, int order_id) {
    sem_wait(&ingr_lock);
    for(int i = 0; i < MAX_LEN; i++) {
        ingr_count[i] += pizza_varieties[pizza_id].ingr[i];
    }
    sem_post(&ingr_lock);
}

void *chef_handler(void* i) {
    chef *c = (chef *)i;
    sleep(c->arrival_time);
    struct timespec time_leave = time_start;
    time_leave.tv_sec += c->leaving_time;
    printf(BLU "Chef %d arrives at time %d\n" RESET, c->id, time(NULL) - time0);
    int s;
    int val;
    while(1) {
        // s = sem_getvalue(&pending_pizza_sem, &val);
        while((s = sem_timedwait(&pending_pizza_sem, &time_leave)) == -1 && errno == EINTR) {
            continue;
        }
        if (s == -1 && errno == ETIMEDOUT) {
            printf(BLU "Chef %d exits at time %d\n" RESET, c->id, time(NULL) - time0);
            return NULL;
        } else if (s == -1) {
            perror("sem_timedwait");
            exit(1);
        }
        for(int i = 0; i < MAX_LEN; i++) {
            if(pending_pizzas[i].valid == 1) {
                if(pending_pizzas[i].prep_time + time(NULL) <= time0 + c->leaving_time) {
                    pthread_mutex_lock(&pending_pizzas[i].mutex);
                    pending_pizzas[i].valid = -1;
                    pthread_mutex_unlock(&pending_pizzas[i].mutex);
                    printf(BLU "Chef %d starts preparing pizza %d of order %d at time %d\n" RESET, c->id, pending_pizzas[i].pizza_id, pending_pizzas[i].order_id, time(NULL) - time0);
                    if(prepare_pizza(pending_pizzas[i].order_id, pending_pizzas[i].pizza_id) == -1) { // todo
                        printf(BLU "Chef %d could not complete pizza %d of order %d due to ingredient shortage at time %d\n" RESET, c->id, pending_pizzas[i].pizza_id, pending_pizzas[i].order_id, time(NULL) - time0);
                        orders[pending_pizzas[i].order_id].pizza_types[pending_pizzas[i].pizza_id].done = 0;
                        pthread_mutex_lock(&pending_pizzas[i].mutex);
                        pending_pizzas[i].valid = 0;
                        pthread_mutex_unlock(&pending_pizzas[i].mutex);
                        break;
                    }
                    sleep(3);
                    while((s = sem_timedwait(&oven_sem, &time_leave)) == -1 && errno == EINTR) {
                        continue;
                    }
                    if (s == -1 && errno == ETIMEDOUT) {
                        printf(BLU "Chef %d exits at time %d\n" RESET, c->id, time(NULL) - time0);
                        refund_pizza(pending_pizzas[i].order_id, pending_pizzas[i].pizza_id); // todo
                        orders[pending_pizzas[i].order_id].pizza_types[pending_pizzas[i].pizza_id].done = 0;
                        pthread_mutex_lock(&pending_pizzas[i].mutex);
                        pending_pizzas[i].valid = 1;
                        pthread_mutex_unlock(&pending_pizzas[i].mutex);
                        sem_post(&pending_pizza_sem);
                        return NULL;
                    } 
                    printf(BLU "Chef %d puts pizza %d of order %d in oven at time %d\n" RESET, c->id, pending_pizzas[i].pizza_id, pending_pizzas[i].order_id, time(NULL) - time0);
                    sleep(pending_pizzas[i].prep_time - 3);
                    printf(BLU "Chef %d takes pizza %d of order %d out of oven at time %d\n" RESET, c->id, pending_pizzas[i].pizza_id, pending_pizzas[i].order_id, time(NULL) - time0);
                    orders[pending_pizzas[i].order_id].pizza_types[pending_pizzas[i].pizza_id].done = 1;
                    sem_post(&oven_sem);
                    pthread_cond_signal(&pending_pizzas[i].cond);
                    pthread_mutex_lock(&pending_pizzas[i].mutex);
                    pending_pizzas[i].valid = 0;
                    pthread_mutex_unlock(&pending_pizzas[i].mutex);
                    break;
                }
            }
            // pthread_mutex_unlock(&pending_pizzas[i].mutex);
        }
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void *pizza_handler(void *i) {
    // int pizza_id = *(int *)i;
    pizza *pz = (pizza *)i;
    
    int pending_id;
    for(int j = 0; j < MAX_LEN; j++) {
        pthread_mutex_lock(&pending_pizzas[j].mutex);
        if(pending_pizzas[j].valid == 0) {
            pending_id = j;
            pending_pizzas[j].valid = 1;
            pending_pizzas[j].order_id = pz->order_id;
            pending_pizzas[j].pizza_id = pz->id;
            pending_pizzas[j].prep_time = pizza_varieties[pz->id].prep_time;
            pthread_mutex_unlock(&pending_pizzas[j].mutex);
            sem_post(&pending_pizza_sem);
            // int val;
            // sem_getvalue(&pending_pizza_sem, &val);
            break;
        }
        pthread_mutex_unlock(&pending_pizzas[j].mutex);
    }

    pthread_mutex_lock(&pending_pizzas[pending_id].mutex);
    while(pending_pizzas[pending_id].valid) {
        pthread_cond_wait(&pending_pizzas[pending_id].cond, &pending_pizzas[pending_id].mutex);
    }
    if(orders[pz->order_id].pizza_types[pz->id].done == 1) {
        if(time0 + orders[pz->order_id].arrival_time + pickup_time > time(NULL)) {
            sleep(time0 + orders[pz->order_id].arrival_time + pickup_time - time(NULL));
        }
        printf(YEL "Customer %d picks up their pizza %d at time %d\n" RESET, pz->order_id, pz->id, time(NULL) - time0);
    }
    pthread_mutex_unlock(&pending_pizzas[pending_id].mutex);
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void *order_handler(void* i) {
    order *o = (order *)i;
    sleep(o->arrival_time);
    if(o->arrival_time >= closing_time) {
        printf(YEL "Customer %d rejected\n" RESET, o->id);
        printf(YEL "Customer %d exits the drive-thru zone\n" RESET, o->id);
        return NULL;
    }
    printf(YEL "Customer %d arrives at time %d\n" RESET, o->id, time(NULL) - time0);
    printf(RED "Order %d placed by customer %d has pizzas {", o->id, o->id);
    int c = 0;
    for(int i = 0; i < MAX_LEN; i++) {
        if(o->pizza_types[i].id != -1) {
            printf("%d", o->pizza_types[i].id);
            c++;
            if(c < o->num_pizzas) {
                printf(", ");
            } else {
                printf("}\n" RESET);
            }
        }
    }
    int possible_pizzas[MAX_LEN];

    sem_wait(&ingr_lock);
    int possible = get_possible_pizzas(possible_pizzas, o->pizza_types);
    sem_post(&ingr_lock);
    if (possible == 0) {
        printf(YEL "Customer %d rejected\n" RESET, o->id);
        printf(YEL "Customer %d exits the drive-thru zone\n" RESET, o->id);
        return NULL;
    } else {
        printf(RED "Order %d placed by customer %d awaits processing\n" RESET, o->id, o->id);
        pthread_t pid[possible];
        int c = 0;
        for(int i = 0; i < MAX_LEN && c < o->num_pizzas; i++) {
            if(possible_pizzas[i] == -1) continue;
            if(possible_pizzas[i] == 0) {
                printf(RED "Pizza %d for order %d is rejected\n" RESET, i, o->id);
                o->pizza_types[i].done = 0;
                continue;
            }
            // for(int j = 0; j < possible_pizzas[i]; j++) {
            //     pthread_create(&pid[c], NULL, pizza_handler, &o->pizza_types[i]);
            //     c++;
            // }
            pthread_create(&pid[c++], NULL, pizza_handler, &o->pizza_types[i]);
        }
        for(int i = 0; i < o->num_pizzas; i++) {
            pthread_join(pid[i], NULL);
        }
        int done = 1;
        for(int i = 0; i < MAX_LEN; i++) {
            if(o->pizza_types[i].done == 0 && o->pizza_types[i].id != -1) {
                done = 0;
                break;
            }
        }
        if(done) {
            printf(RED "Order %d placed by customer %d has been processed\n" RESET, o->id, o->id);
        } else {
            printf(RED "Order %d placed by customer %d has been partially processed\n" RESET, o->id, o->id);
        }
        printf(YEL "Customer %d exits the drive-thru zone\n" RESET, o->id);
    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void init(void) {
    for(int i = 0; i < MAX_LEN; i++) {
        pizza_varieties[i].id = -1;
        for(int j = 0; j < MAX_LEN; j++) {
            pizza_varieties[i].ingr[j] = 0;
        }
    }
    
    for(int i = 0; i < MAX_LEN; i++) {
        ingr_count[i] = 0;
    }

    for(int i = 0; i < MAX_LEN; i++) {
        chefs[i].id = -1;
    }

    for(int i = 0; i < MAX_LEN; i++) {
        orders[i].id = -1;
        for(int j = 0; j < MAX_LEN; j++) {
            orders[i].pizza_types[j].id = -1;
        }
    }
    
    for(int i = 0; i < MAX_LEN; i++) {
        pending_pizzas[i].valid = 0;
        pthread_mutex_init(&pending_pizzas[i].mutex, NULL);
        pthread_cond_init(&pending_pizzas[i].cond, NULL);
    }
} 

int main() {
    init();
    scanf("%d %d %d %d %d %d", &num_chefs, &num_varieties, &num_ingr, &num_customers, &num_ovens, &pickup_time);

    for(int i = 0; i < num_varieties; i++) {
        int id;
        scanf("%d", &id);
        pizza_varieties[id].id = id;
        scanf("%d %d", &pizza_varieties[id].prep_time, &pizza_varieties[id].num_ingredients);
        for(int j = 0; j < pizza_varieties[id].num_ingredients; j++) {
            int igr;
            scanf("%d", &igr);
            pizza_varieties[id].ingr[igr] = 1;
        }
    }
    
    for(int i = 0; i < num_ingr; i++) {
        scanf("%d", &ingr_count[i+1]);
    }

    for(int i = 0; i < num_chefs; i++) {
        int id = i+1;
        chefs[id].id = id;
        scanf("%d %d", &chefs[id].arrival_time, &chefs[id].leaving_time);
        sem_init(&chefs[id].lock, 0, 1);
        closing_time = max(closing_time, chefs[id].leaving_time);
    }

    for(int i = 0; i < num_customers; i++) {
        int id = i+1;
        orders[id].id = id;
        scanf("%d %d", &orders[id].arrival_time, &orders[id].num_pizzas);
        for(int j = 0; j < orders[id].num_pizzas; j++) {
            int pid;
            scanf("%d", &pid);
            orders[id].pizza_types[pid].id = pid;
            orders[id].pizza_types[pid].order_id = id;
            orders[id].pizza_types[pid].done = 0;
        }
        sem_init(&orders[id].mutex, 0, 1);
    }

    sem_init(&chef_sem, 0, 0);
    sem_init(&oven_sem, 0, num_ovens);
    sem_init(&ingr_lock, 0, 1);
    sem_init(&pending_pizza_sem, 0, 0);

    pthread_t oid[num_customers];
    pthread_t cid[num_chefs];
    time0 = time(NULL);
    if (clock_gettime(CLOCK_REALTIME, &time_start) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    int c = 0;
    for(int i = 0; i < MAX_LEN; i++) {
        if(chefs[i].id != -1) {
            pthread_create(&cid[c++], NULL, chef_handler, (void *)(&chefs[i]));
        }
    }
    printf("Simulation started\n");
    c = 0;
    for(int i = 0; i < MAX_LEN; i++) {
        if(orders[i].id != -1) {
            pthread_create(&oid[c++], NULL, order_handler, (void *)(&orders[i]));
        }
    }
    for(int i = 0; i < num_chefs; i++) {
        pthread_join(cid[i], NULL);
    }
    
    for(int i = 0; i < MAX_LEN; i++) {
        pthread_mutex_lock(&pending_pizzas[i].mutex);
        if(pending_pizzas[i].valid == 1) {
            printf(RED "Pizza %d of order %d rejected at time %ld\n" RESET, pending_pizzas[i].pizza_id, pending_pizzas[i].order_id, time(NULL) - time0);
            pending_pizzas[i].valid = 0;
            pthread_cond_broadcast(&pending_pizzas[i].cond);
        }
        pthread_mutex_unlock(&pending_pizzas[i].mutex);
    }
    for(int i = 0; i < num_customers; i++) {
        pthread_join(oid[i], NULL);
    }
    printf("Simulation ended\n");
    return 0;
}