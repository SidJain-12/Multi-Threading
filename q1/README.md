## Implementation

1. A student struct has been created with the following fields:
    1. id
    2. patience_time
    3. washing_time
    4. washing_machine_id
2. Then for each student a thread is created.
3. Each thread will try to get a lock on the washing machine semaphore.
4. First these threads are sorted according to their arrival time and then they are executed.
5. ```sem_timedwait``` is used to wait for a lock on the semaphore and if patience time is over, the thread will return without washing cloths.
6. If the thread is able to get a lock on the semaphore, it will wait for the washing time and then it will release the lock on the semaphore.
7. At the end, statistics are provided. If more than 25% of the students are not able to wash their cloths, then ```Yes``` is printed otherwise ```No``` which signifies if we need a new washing machine or not.