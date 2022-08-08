#include <map>
#include <iostream>
#include <set>
#include <atomic>
#include <algorithm>
#include "MapReduceFramework.h"
#include "Barrier.h"

/*////////////////////////////////////////////////////////////////////////////
 * Error handling
 ///////////////////////////////////////////////////////////////////////////*/

#define SYSTEM_CALL_ERR "system error: "

void raise_error(int error_type) {
    switch (error_type) {
        case 0:
            std::cerr << SYSTEM_CALL_ERR << "locking mutex failed\n";
            break;
        case 1:
            std::cerr << SYSTEM_CALL_ERR << "unlocking mutex failed\n";
            break;
        case 2:
            std::cerr << SYSTEM_CALL_ERR << "destroying mutex failed\n";
            break;
        case 3:
            std::cerr << SYSTEM_CALL_ERR << "pthread_create failed\n";
            break;
    }
    exit(1);
}

/*////////////////////////////////////////////////////////////////////////////
STRUCTS
/////////////////////////////////////////////////////////////////////////////*/

struct Thread;
/***
 *  A struct which includes all the parameters which are relevant to the job
 *  (e.g., the threads, state, mutexes...). The pointer to this struct can be casted to JobHandle.
 *  You are encouraged to use C++ static casting.
 */
struct JobContext {
    /*///////////general///////////////////////////////////////////////////*/
    JobState job_state;
    const MapReduceClient *client;
    int numThreads;
    Barrier *barrier;
    pthread_t *threads;
    Thread *threadContexts;
    /*///////////counters///////////////////////////////////////////////////*/
    std::atomic<unsigned long> mapCounter;
    std::atomic<unsigned long> mapWork;
    std::atomic<unsigned long> shuffleCounterVec;
    std::atomic<unsigned long> shuffleCounter;
    std::atomic<unsigned long> emit2Counter;
    std::atomic<unsigned long> reduceCounterVec;
    std::atomic<unsigned long> reduceCounter;
    /*///////////mutex///////////////////////////////////////////////////*/
    pthread_mutex_t emit3Mutex;
    pthread_mutex_t stateMutex;
    pthread_mutex_t getterMutex;
    /*///////////DS/////////////////////////////////////////////////////////*/
    const InputVec *inputVec;
    IntermediateVec **threadsIntermediateVectors;
    std::vector<IntermediateVec *> *queue;
    OutputVec *outputVec;
};

/**
 * holds the thread ID and context of the job it handles
 */
struct Thread {
    int threadID;
    JobContext *job_context;
};

/*////////////////////////////////////////////////////////////////////////////
SUPPLEMENTARY METHODS
/////////////////////////////////////////////////////////////////////////////*/
void lockMutex(pthread_mutex_t *mutex) { if (pthread_mutex_lock(mutex) != 0) { raise_error(0); }}

void unlockMutex(pthread_mutex_t *mutex) { if (pthread_mutex_unlock(mutex) != 0) { raise_error(1); }}

void updateStage(JobContext *context, stage_t stage) {
    lockMutex(&context->stateMutex);
    context->job_state.stage = stage;
    context->job_state.percentage = 0; // work started should be 0
    unlockMutex(&context->stateMutex);
}
/*////////////////////////////////////////////////////////////////////////////
MAP, SORT , SHUFFLE , REDUCE PHASE IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////*/
/**
* In this phase each thread reads pairs of (k1, v1) from the input vector and calls the map function on each of them.
 *
 * The map function in turn will produce (k2, v2) and will call the emit2 function to update the framework databases.
 * In the end of this phase, we have multiThreadLevel vectors of (k2, v2) pairs and all elements in the input vector
 * were processed.
 *
 * SYNCHRONIZATION CHALLENGES:
 * 1. Splitting the input values between the threads - this will be done using an atomic variable shared between
 * the threads
 * The variable will be initialised to 0, then each thread will increment the variable and check its old value.
 * The thread can safely call map on the pair in the index old_value knowing that no other thread will do so.
 * This is repeated until old_value is after the end of the input vector, as that means that all pairs have been
 * processed and the Map phase has ended.
 * 2. Prevent output race conditions - This will be done by separating the outputs.
 * We will create a vector for each thread and then emit2 will just append the new (k2, v2) pair into the calling thread’s vector.
 * Accessing the calling thread’s vector can be done by using the context argument.
 * (The Shuffle thread will combine those vectors into a single data structure.)
 *
 *  //In the map stage the percentage is the number of input vector items processed out of all the input vector
 *  items (input vector size).

 */
void mapPhase(Thread *thread) {
    JobContext *jobContext = thread->job_context;
    unsigned long old_value = jobContext->mapCounter++;
    while (old_value < jobContext->inputVec->size()) {
        InputPair pair = (*jobContext->inputVec)[old_value];
        jobContext->client->map(pair.first, pair.second, thread);
        jobContext->mapWork++;
        old_value = jobContext->mapCounter++;
    }
}

/**
 * Immediately after the Map phase each thread will sort its intermediate vector according to the keys within.
 * std::sort can be used to implement this phase with relatively little code
 * The Shuffle phase must only start after all threads finished their sort phases.
 * In the end of this phase, we must use a barrier – a synchronisation mechanism that makes sure no thread continues
 * before all threads arrived at the barrier.
 * Once all threads arrive, the waiting threads can continue.
 * A sample C++ implementation of a barrier is provided together with the exercise, You may use code from
 * this example as is.
 * After the barrier, one of the threads (thread 0) will move on to the Shuffle phase while the rest will
 * skip it and move directly to the Reduce phase.
 //The shuffle phase will start only after all threads have completed the map and sort phase.

 */
void sortPhase(Thread *thread) {
    std::sort(thread->job_context->threadsIntermediateVectors[thread->threadID]->begin(),
              thread->job_context->threadsIntermediateVectors[thread->threadID]->end(),
              [](IntermediatePair p1, IntermediatePair p2) { return *(p1.first) < *(p2.first); });}

/**
 * our goal in this phase is to create new sequences of (k2, v2) where in each sequence all keys are identical and all
 * elements with a given key are in a single sequence.
 * Since our intermediary vectors are sorted, we know that all elements with the largest key must be at the
 * back of each vector.
 * Thus, creating the new sequence is simply a matter of:
 * 1. popping these elements from the back of each vector
 * 2.inserting them to a new vector.
 *Now all elements with the second largest key are at the back of the vectors so we can repeat the process until the
 * intermediary vectors are empty.
 *We use a single Shuffle thread (thread 0) while all the other threads will wait until the shuffle phase will over.
 * Whenever we finish creating a new vector for some identical key, we put it in a queue- using a vector for the queue
 * (note that it is a vector of vectors)
 * In addition use an atomic counter for counting the number of vectors in it.
 * Whenever a new vector is inserted to the queue you should update the atomic counter.
 * Once all intermediary vectors are empty, the shuffling thread will move on to the Reduce phase.
 * You are encouraged to use semaphore at this phase to make all the threads to wait until the shuffle thread is complete.
 * You can create semaphore using the function sem_init.
 *
 *  //In the shuffle stage the percentage is the number of intermediate pairs shuffled out of all the intermediate pairs.
 */




void shufflePhase(JobContext *jobContext) {
    while (true) {
        K2 *max = nullptr;
        // find maximal element:
        for (int i = 0; i < jobContext->numThreads; i++) {
            if (!jobContext->threadsIntermediateVectors[i]->empty()) {
                if (!max) { max = jobContext->threadsIntermediateVectors[i]->back().first; }
                else if (!(*jobContext->threadsIntermediateVectors[i]->back().first <
                           *max)) { max = jobContext->threadsIntermediateVectors[i]->back().first; }
            }
        }
        if (max == nullptr) { break;} // all vectors are empty - break
        auto *vec = new IntermediateVec();
        jobContext->shuffleCounterVec++;
        jobContext->queue->push_back(vec);
        for (int i = 0; i < jobContext->numThreads; i++) {
            while (!(jobContext->threadsIntermediateVectors[i]->empty()) &&
                   (!(*jobContext->threadsIntermediateVectors[i]->back().first < *max))) {
                vec->push_back(jobContext->threadsIntermediateVectors[i]->back());
                jobContext->threadsIntermediateVectors[i]->pop_back();
                jobContext->shuffleCounter++;
            }
        }
    }
}


/**
 * The reducing threads will wait for the shuffled vectors to be created by the shuffling thread.
 * Once they wake up, they can pop a vector from the back of the queue and run reduce on it
 * (Remember to lock the mutex when necessary).
 *
 * The reduce function in turn will produce (k3, v3) pairs
 * and will call emit3 to add them to the framework data structures.
 * These can be inserted directly to the output vector (outputVec argument of startMapReduceJob)
 * under the protection of a mutex.
 * The emit3 function can access the output vector through its context argument.
 *
 *  In the reduce stage the percentage is the number of shuffled pairs (key, value) reduced out of
 *  all the shuffled pairs.
 */
void reducePhase(JobContext *jobContext) {
    unsigned long old_value = jobContext->reduceCounterVec++;
    while (old_value < jobContext->shuffleCounterVec) {
        IntermediateVec *vec = (*jobContext->queue)[old_value];
        jobContext->client->reduce(vec, jobContext);
        jobContext->reduceCounter += vec->size();
        old_value = jobContext->reduceCounterVec++;

    }
}

void *init(void *arg) {
    auto *thread = static_cast<Thread *>(arg);
    auto *job = thread->job_context;
    updateStage(job, MAP_STAGE);
    mapPhase(thread);
    sortPhase(thread);
    job->barrier->barrier();
    if (thread->threadID == 0) {
        updateStage(job, SHUFFLE_STAGE);
        shufflePhase(job);
        updateStage(job, REDUCE_STAGE);
    }
    job->barrier->barrier();
    reducePhase(job);
    return nullptr;
}

/*///////////EXTERNAL API///////////////////////////////////////////////////*/
/**
 * This function produces a (K2*, V2*) pair.
 * The function saves the intermediary element in the context data structures.
 * In addition, the function updates the number of intermediary elements using atomic counter.
 * @param key intermediary element K2*
 * @param value intermediary element V2*
 * @param context which contains data structure of the thread that created the intermediary element
 * //Please pay attention that emit2 is called from the client's map
 // function and the context is passed from the framework to the client's map function as parameter.
 */

void emit2(K2 *key, V2 *value, void *context){
    auto *thread = static_cast<Thread *>(context);
    IntermediateVec *interVec = thread->job_context->threadsIntermediateVectors[thread->threadID];
    thread->job_context->emit2Counter++;
    interVec->push_back(IntermediatePair(key, value));
}

/**
 *This function produces a (K3*, V3*) pair.
 * The function saves the output element in the context data structures (output vector).
 * In addition, the function updates the number of output elements using atomic counter.
 * @param key output element K3*
 * @param value output element V3*
 * @param context which contains
 * .data structure of the thread that created the output element
 *  //Please pay attention that emit3 is called from the client's map
 // function and the context is passed from the framework to the client's map function as parameter.
 */

void emit3(K3 *key, V3 *value, void *context) {
    auto *jobContext = (JobContext *) context;
    lockMutex(&(jobContext->emit3Mutex));
    jobContext->outputVec->push_back({key, value});
    unlockMutex(&(jobContext->emit3Mutex));
}

void startJobContext(JobContext *job_context, const MapReduceClient &client,
                     const InputVec &inputVec, OutputVec &outputVec,
                     int multiThreadLevel) {
    /*///////////general///////////////////////////////////////////////////*/
    updateStage(job_context, UNDEFINED_STAGE);
    job_context->client = &client;
    job_context->numThreads = multiThreadLevel;
    job_context->barrier = new Barrier(multiThreadLevel);
    job_context->threads = new pthread_t[multiThreadLevel];
    job_context->threadContexts = new Thread[multiThreadLevel];
    /*///////////counters///////////////////////////////////////////////////*/
    job_context->mapCounter = 0;
    job_context->mapWork = 0;
    job_context->shuffleCounterVec = 0;
    job_context->shuffleCounter = 0;
    job_context->emit2Counter = 0;
    job_context->reduceCounterVec = 0;
    job_context->reduceCounter = 0;
    /*///////////mutex///////////////////////////////////////////////////*/
    pthread_mutex_t emit3Mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t stateMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t getterMutex = PTHREAD_MUTEX_INITIALIZER;
    /*///////////DS/////////////////////////////////////////////////////////*/
    job_context->inputVec = &inputVec;
    job_context->threadsIntermediateVectors = new IntermediateVec *[multiThreadLevel];
    job_context->queue = new std::vector<IntermediateVec *>();
    job_context->outputVec = &outputVec;
    for (int i = 0; i < multiThreadLevel; i++) {
        job_context->threadsIntermediateVectors[i] = new IntermediateVec();
    }
}


/**
 * This function starts running the MapReduce algorithm (with several threads)
 * and returns a JobHandle.
 * @param client The implementation of MapReduceClient or in other words the task that the framework should run.
 * @param inputVec a vector of type std::vector<std::pair<K1*, V1*>>, the input elements.
 * @param outputVec a vector of type std::vector<std::pair<K3*, V3*>> to which the output elements will
 * be added before returning.
 * @param multiThreadLevel the number of worker threads to be used for running the algorithm.
 * @return The function returns JobHandle that will be used for monitoring the job.
 *  //You will have to create threads using c function pthread_create.

 */
JobHandle startMapReduceJob(const MapReduceClient &client,
                            const InputVec &inputVec, OutputVec &outputVec,
                            int multiThreadLevel) {
    auto *job_context = new JobContext();
    startJobContext(job_context, client, inputVec, outputVec, multiThreadLevel);
    for (int i = 0; i < multiThreadLevel; ++i) {
        job_context->threadContexts[i] = {i, job_context};
        if (pthread_create(job_context->threads + i, NULL, init, job_context->threadContexts + i) != 0) {
            raise_error(3);
        }
    }
    return static_cast<JobHandle>(job_context);
}

/**
 * a function gets JobHandle returned by startMapReduceFramework and waits until it is finished.
 * @param job
 *  //Hint – you should use the c function pthread_join.
 *
 // It is legal to call the function more than once and you should handle it.
 // Pay attention that calling pthread_join twice from the same process has undefined behavior and you must avoid that.
 */

void waitForJob(JobHandle job) {
    static std::set<JobHandle> jobs_queue;
    if (jobs_queue.find(job) != jobs_queue.end())
        return;
    jobs_queue.insert(job);

    for (int i = 0; i < static_cast<JobContext *>(job)->numThreads; ++i)
        pthread_join(*(static_cast<JobContext *>(job)->threads + i), NULL);

    jobs_queue.erase(jobs_queue.find(job));
}

/**
 *  this function gets a JobHandle and updates the state of the job into the given JobState struct.
 * @param job
 * @param state
 */
void getJobState(JobHandle job, JobState *state) {
    auto *jobContext = static_cast<JobContext *>(job);
    lockMutex(&jobContext->getterMutex);
    switch (jobContext->job_state.stage) {
        case (MAP_STAGE):
            state->stage = MAP_STAGE;
            state->percentage =((float) jobContext->mapWork / (float) jobContext->inputVec->size()) * 100;
            break;
        case (SHUFFLE_STAGE):
            state->stage = SHUFFLE_STAGE;
            state->percentage= ((float)  jobContext->shuffleCounter / (float) jobContext->emit2Counter) * 100;
            break;
        case (REDUCE_STAGE):
            state->stage = REDUCE_STAGE;
            state->percentage =
                    ((float) jobContext->reduceCounter / (float) jobContext->shuffleCounter) * 100;
            break;
        case (UNDEFINED_STAGE):
            state->stage = UNDEFINED_STAGE;
            state->percentage = 0;
            break;
    }
    unlockMutex(&jobContext->getterMutex);
}

/**
 * Releasing all resources of a job.
 * @param job
 *  In case that the function is called and the job is not finished
 // yet wait until the job is finished to close it.
 // In order to release mutexes and semaphores (pthread_mutex, sem_t)
 // you should use the functions pthread_mutex_destroy, sem_destroy.
 */


void closeJobHandle(JobHandle job) {
    auto *jobContext = static_cast<JobContext *>(job);
    waitForJob(jobContext);
    if (pthread_mutex_destroy(&jobContext->stateMutex)) { raise_error(2); }
    if (pthread_mutex_destroy(&jobContext->emit3Mutex)) { raise_error(2); }
    if (pthread_mutex_destroy(&jobContext->getterMutex)) { raise_error(2); }
    for (IntermediateVec *vec : *(jobContext->queue)) { delete vec; }
    for (int i = 0; i < jobContext->numThreads; i++) { delete jobContext->threadsIntermediateVectors[i]; }
    delete[] jobContext->threads;
    delete[] jobContext->threadContexts;
    delete[] jobContext->threadsIntermediateVectors;
    delete jobContext->queue;
    delete jobContext->barrier;
    delete jobContext;
}

