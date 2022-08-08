#include "uthreads.h"
#include <queue>
#include <unordered_map>
#include <map>
#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <list>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
#include <setjmp.h>

using namespace std;

typedef void (*timer_handler_func)(int);

#define MAIN_ID 0
#define SECOND 1000000
#define MIN_QUANTUM 0
#define MIN_THREAD_NUM 0

/* ////////////////////////////////////////////////////////////////////////////
 * THE FOLLOWING CODE IS TAKEN FROM THE demo_jmp.c PROVIDED FILE
 * //////////////////////////////////////////////////////////////////////////*/
#ifdef __x86_64__
/* code for 64 bit Intel arch */
typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr) {
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else

/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */

address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

/*////////////////////////////////////////////////////////////////////////////
 * Error handling
 ///////////////////////////////////////////////////////////////////////////*/

#define SYSTEM_CALL_ERR "system error: "
#define THREAD_LIBRARY_ERR "thread library error: "
#define SUCCESS 0
#define FAILURE -1

int raise_error(int error_type) {
    switch (error_type) {
        case 0:
            std::cerr << THREAD_LIBRARY_ERR << "invalid input (thread doesn't exist)\n";
            break;

        case 1:
            std::cerr << THREAD_LIBRARY_ERR << "main thread can't be blocked\n";
            break;

        case 2:
            std::cerr << THREAD_LIBRARY_ERR << "main thread can't sleep\n";
            break;

        case 3:
            std::cerr << THREAD_LIBRARY_ERR << "maximum threads limit achieved\n";
            break;

        case 4:
            std::cerr << SYSTEM_CALL_ERR << "error in setitimer\n";
            exit(1);

        case 5:
            std::cerr << THREAD_LIBRARY_ERR << "invalid length of quantum\n";
            break;

        case 6:
            std::cerr << SYSTEM_CALL_ERR << "interrupts enabling failure\n";
            exit(1);

        case 7:
            std::cerr << SYSTEM_CALL_ERR << "interrupts disabling failure\n";
            exit(1);

        case 8:
            std::cerr << SYSTEM_CALL_ERR << "error in sigaction\n";
            exit(1);

        case 9:
            std::cerr << SYSTEM_CALL_ERR << "error in sigaddset\n";
            exit(1);

        case 10:
            std::cerr << THREAD_LIBRARY_ERR << "thread id is empty\n";
            break;
    }
    return FAILURE;
}


/*////////////////////////////////////////////////////////////////////////////
 * System based methods
 ///////////////////////////////////////////////////////////////////////////*/

enum thread_state {
    READY, RUNNING, BLOCKED
};

struct comparator {
    bool operator()(int i, int j) { return i > j; }
};

sigset_t interrupt_set{};
struct sigaction sa = {0};
struct itimerval timer;

void set_interupts() {
    sigemptyset(&interrupt_set);
    if (sigaddset(&interrupt_set, SIGINT) < 0 || sigaddset(&interrupt_set, SIGVTALRM) < 0) { raise_error(9); }
}

void enableInterrupts() {
    if (sigprocmask(SIG_UNBLOCK, &interrupt_set, nullptr) == FAILURE) {
        raise_error(6);
    }
}

void disableInterrupts() {
    if (sigprocmask(SIG_BLOCK, &interrupt_set, nullptr) == FAILURE) {
        raise_error(7);
    }
}

/* ///////////////////////////////////////////////////////////////////////////
 * Thread ID provider
////////////////////////////////////////////////////////////////////////////*/

class ID_manager {
    //min heap data structure
    priority_queue<int, std::vector<int>, comparator> *ID_heap;
    int curr_id;
public:
    ID_manager() {
        ID_heap = new priority_queue<int, std::vector<int>, comparator>();
        curr_id = 0;
    }

    int get_next_ID() {
        if (ID_heap->empty()) {
            curr_id++;
            return curr_id - 1;
        }
        else {
            int id = ID_heap->top();
            ID_heap->pop();
            return id;
        }
    }

    void add_ID(int id) { ID_heap->push(id); }

    ~ID_manager() { delete ID_heap; }
};

/* /////////////////////////////////////////////////////////////////////////
 * Thread Control Block
 //////////////////////////////////////////////////////////////////////////*/

class TCB {
    enum thread_state state = READY;
    sigjmp_buf env;
    char *stack = nullptr;
    address_t sp;
    address_t pc;
    int ID;
    int total_quantums = 0;
    bool sleeping = false;
    int wake_up_time;

public:
    //constructor:
    TCB(int thread_id, thread_entry_point entry_point) {
        ID = thread_id;
        if (thread_id == MAIN_ID) {
            sigsetjmp(env, 1);
            return;
        }
        stack = new char[STACK_SIZE];
        sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
        pc = (address_t) entry_point;
        sigsetjmp(env, 1);
        (env->__jmpbuf)[JB_SP] = translate_address(sp);
        (env->__jmpbuf)[JB_PC] = translate_address(pc);
        sigemptyset(&env->__saved_mask);
    }

    ~TCB() { if (ID != MAIN_ID) { delete[] stack; }}

    //getters/////////////////////////////////////////////////////////////////////////////////////////////////////////
    int get_quantums() const { return total_quantums; }

    int get_id() const { return ID; }

    sigjmp_buf *get_env() { return &env; }

    thread_state get_state() { return state; }

    bool is_sleeping() const { return sleeping; }

    int get_wake_up_time() const { return wake_up_time; }

    //setters/////////////////////////////////////////////////////////////////////////////////////////////////////////
    void set_state(thread_state new_state) { state = new_state; }

    void increase_quantums() { total_quantums++; }

    int save_env() { return sigsetjmp(env, 1); }

    void set_sleeping(int time) {
        sleeping = !sleeping;
        wake_up_time = time;
    }
};

/* /////////////////////////////////////////////////////////////////////////
 * Data structures & fields of the uthreads library implementation
 ////////////////////////////////////////////////////////////////////////////*/

class ThreadsHandler {
    unordered_map<int, TCB *> *threads;
    list<TCB *> *ready_que;
    unordered_map<int, TCB *> *blocked_que;
    ID_manager *id_manager;
    int total_num_threads;

public:
    ThreadsHandler() {
        threads = new unordered_map<int, TCB *>();
        ready_que = new list<TCB *>();
        blocked_que = new unordered_map<int, TCB *>();
        id_manager = new ID_manager();
        total_num_threads = 0;
    }

    ~ThreadsHandler() {

        delete blocked_que;
        delete ready_que;
        for (auto &i : *threads) {
            delete i.second;
        }
        delete threads;

    }

    void remove_ready_thread(TCB *thread) {
        for (auto it = ready_que->begin(); it != ready_que->end(); ++it) {
            if ((*it)->get_id() == thread->get_id()) {
                ready_que->erase(it);
                return;
            }
        }
    }

    //general thread methods ///////////////////////////////////////////////////////////////////////////////////////
    unordered_map<int, TCB *> *get_threads() { return threads; }

    TCB *get_thread(int tid) {
        auto thread = threads->find(tid);
        if (thread == threads->end()) //no thread with ID tid exists
        {
            return nullptr;
        }
        return thread->second;
    }

    int create_thread(thread_entry_point entry_point) {
        if (total_num_threads >= MAX_THREAD_NUM) { return raise_error(3); }
        total_num_threads++;
        int tid = id_manager->get_next_ID();
        TCB *new_thread = new TCB(tid, entry_point);
        threads->insert({tid, new_thread});
        ready_que->push_back(new_thread);
        return tid;
    }


    TCB *ready_running() {
        if (ready_que->empty()) { return nullptr; }
        TCB *next_thread = ready_que->front();
        ready_que->pop_front();
        next_thread->set_state(RUNNING);
        return next_thread;
    }

    void sleep_ready(TCB *thread) {
        thread->set_sleeping(0);
        if (thread->get_state() != BLOCKED) {
            ready_que->push_back(thread);
            thread->set_state(READY);
        }

    }

    void running_sleep(TCB *thread, int wake_up_time) {
        thread->set_sleeping(wake_up_time);
    }

    //blocks running thread
    void running_block(TCB *thread) {
        thread->set_state(BLOCKED);
        blocked_que->insert({thread->get_id(), thread});
    }

    void terminate_thread(TCB *thread) {
        if(thread!= nullptr)
        {
        total_num_threads--;
        threads->erase(thread->get_id());
        id_manager->add_ID(thread->get_id());
        delete thread;}
    }

    //if its running - only present in threads ds
    void running_terminate(TCB *thread) {
        terminate_thread(thread);
    }

    void running_ready(TCB *thread) {
        thread->set_state(READY);
        ready_que->push_back(thread);
    }

    void ready_terminate(TCB *thread) {
        remove_ready_thread(thread);
        terminate_thread(thread);
    }

    //blocks received ready thread
    void ready_block(TCB *thread) {
        thread->set_state(BLOCKED);
        blocked_que->insert({thread->get_id(), thread});
        remove_ready_thread(thread);
    }

    void blocked_ready(TCB *thread) {
        blocked_que->erase(thread->get_id());
        thread->set_state(READY);
        if (!thread->is_sleeping()) {
            ready_que->push_back(thread);
        }

    }

    void blocked_terminate(TCB *thread) {
        blocked_que->erase(thread->get_id());
        terminate_thread(thread);
    }
};


/* /////////////////////////////////////////////////////////////////////////
 *
 ////////////////////////////////////////////////////////////////////////////*/

class Schedular {
    int quantom_duration;
    int total_num_quantoms = 0;
    TCB *running_thread{};
    ThreadsHandler *threads_handler;
    bool running_terminated = false;

public:

    Schedular(int qd, ThreadsHandler *th) {
        quantom_duration = qd;
        threads_handler = th;
        Schedular::threads_handler->create_thread(nullptr);// creates main thread
    }

    //getters
    TCB *s_get_running_thread() { return running_thread; }

    int s_get_total_num_quantoms() const { return total_num_quantoms; }

    //setters
    void s_increase_quantum() { total_num_quantoms++; }


    //moving between threads methods
    void jump_to_thread(TCB *next) {
        running_thread = next;
        running_thread->increase_quantums();
        enableInterrupts();
        if (setitimer(ITIMER_VIRTUAL, &timer, nullptr)) {
            raise_error(4);

        }


        siglongjmp(*running_thread->get_env(), 1);
    }


    void switchThreads() {
        disableInterrupts();
        if (running_thread != nullptr) // run started;
        {
            if (!running_terminated) {
                if (running_thread->get_state() != BLOCKED &&
                    !running_thread->is_sleeping()) { threads_handler->running_ready(running_thread); }
                int return_value = running_thread->save_env();
                if (return_value == 1) {
                    enableInterrupts();
                    return;
                }
            } else {
                running_terminated = false;
            }


        }
        TCB *next_thread = threads_handler->ready_running();
//        if (next_thread == nullptr) {jump_to_thread(running_thread);}
//        else{ jump_to_thread(next_thread);}

        running_thread = next_thread;
        running_thread->increase_quantums();
        enableInterrupts();
        if (setitimer(ITIMER_VIRTUAL, &timer, nullptr)) {
            raise_error(4);

        }


        siglongjmp(*running_thread->get_env(), 1);

    }

    void wake_up_call() {
        for (auto &thread : *threads_handler->get_threads()) {
            if (thread.second->is_sleeping() && thread.second->get_wake_up_time() == total_num_quantoms) {
                threads_handler->sleep_ready(thread.second);
            }
        }
    }


    void terminate(TCB *thread) {
        if (thread == nullptr) {
            raise_error(10);
            return;
        }
        switch (thread->get_state()) {
            case READY:
                threads_handler->ready_terminate(thread);
                break;
            case BLOCKED:
                threads_handler->blocked_terminate(thread);
                break;
            case RUNNING:
                running_terminated = true;
                threads_handler->running_terminate(thread);
                break;
        }
    }


    void resume(TCB *thread) {

        if (thread->get_state() == BLOCKED) { Schedular::threads_handler->blocked_ready(thread); }
    }

    void block_thread(TCB *thread) {

        switch (thread->get_state()) {
            case READY:
                threads_handler->ready_block(thread);
                break;

            case RUNNING:
                threads_handler->running_block(thread);
                break;

            case BLOCKED:
                break;

        }
    }

    //this function puts a running thread to sleep
    void sleep(int quanta) {
        int wake_up_time = total_num_quantoms + quanta;
        threads_handler->running_sleep(running_thread, wake_up_time);
    }


};


/* /////////////////////////////////////////////////////////////////////////*/


ThreadsHandler *threads_handler;
Schedular *schedular;


void free_all() {
    for(int tid=0; tid<MAX_THREAD_NUM;tid++)
    {
        TCB *thread = threads_handler->get_thread(tid);
        if(thread != nullptr) {schedular->terminate(thread);}
    }
    delete threads_handler;
    delete schedular;
}

void timer_handler(int sig) {

    schedular->wake_up_call();
    schedular->s_increase_quantum();
    schedular->switchThreads();
}

int set_timer(int quantom_duration) {
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa, nullptr) < 0) { return raise_error(8); }
    int val_sec = quantom_duration / SECOND;
    int val_msec = quantom_duration % SECOND;
    timer.it_value.tv_sec = val_sec;
    timer.it_value.tv_usec = val_msec;
    timer.it_interval.tv_sec = val_sec;
    timer.it_interval.tv_usec = val_msec;
    if (setitimer(ITIMER_VIRTUAL, &timer, nullptr)) {
        return raise_error(4);

    }

    return SUCCESS;
}

bool legit_id(int tid)
{
    return (tid >= MIN_THREAD_NUM) && (tid <= MAX_THREAD_NUM);
}




/* /////////////////////////////////////////////////////////////////////////
 * External interface
////////////////////////////////////////////////////////////////////////////*/

/**
 * @brief initializes the thread library.
 *
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {
    if (quantum_usecs <= MIN_QUANTUM) { return raise_error(5); }
    if (set_timer(quantum_usecs) == FAILURE) return FAILURE;
    threads_handler = new ThreadsHandler();
    schedular = new Schedular(quantum_usecs, threads_handler);
    set_interupts();
    timer_handler(0);
    return SUCCESS;
}


/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn(thread_entry_point entry_point) {
    disableInterrupts();
    int tid = threads_handler->create_thread(entry_point);
    enableInterrupts();
    if (tid == FAILURE) {
        return FAILURE;
    }
    return tid;
}


/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
    if (!legit_id(tid)) return raise_error(0);

    disableInterrupts();
    if (tid == MAIN_ID) {
        free_all();
        enableInterrupts();
        exit(0);
    }

    int curr_thread_id = uthread_get_tid();
    TCB *thread = threads_handler->get_thread(tid);
    if (thread == nullptr) {
        enableInterrupts();
        return raise_error(10);
    }
    schedular->terminate(thread);
    if (tid == curr_thread_id) { timer_handler(0); }
    enableInterrupts();
    return SUCCESS;
}

/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made. Blocking a thread in
 * BLOCKED state has no effect and is not considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block(int tid) {
    if (tid == MAIN_ID) {
        return raise_error(1);
    }
    if (!legit_id(tid)) return raise_error(0);
    disableInterrupts();
    TCB *thread = threads_handler->get_thread(tid);
    if (thread == nullptr) {
        enableInterrupts();
        return raise_error(10);
    }
    schedular->block_thread(thread);
    if (tid == uthread_get_tid()) { timer_handler(0); }
    enableInterrupts();
    return SUCCESS;
}


/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid) {
    if (!legit_id(tid)) return raise_error(0);
    disableInterrupts();
    TCB *thread = threads_handler->get_thread(tid);
    if (thread == nullptr) {
        enableInterrupts();
        return raise_error(10);
    }
    schedular->resume(thread);
    enableInterrupts();
    return SUCCESS;
}


/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * It is considered an error if the main thread (tid==0) calls this function.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums) {
    if (uthread_get_tid() == MAIN_ID) {
        free_all();
        return raise_error(2);
    }
    disableInterrupts();
    schedular->sleep(num_quantums);
    timer_handler(0);
    enableInterrupts();
    return SUCCESS;
}


/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid() { return schedular->s_get_running_thread()->get_id(); }


/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums() { return schedular->s_get_total_num_quantoms(); }


/**
 * @brief Returns the number of quantums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid) {
    if (!legit_id(tid)) return raise_error(0);
    TCB *thread = threads_handler->get_thread(tid);
    if (thread == nullptr) {
        return raise_error(10);
    }
    return thread->get_quantums();
}
