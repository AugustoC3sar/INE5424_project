#include <iostream>
#include <vector>
#include <algorithm>

enum Mode {
    ENERGY_SAVING,
    PERFORMANCE
};

/**
 * Simulates a Real Time Thread with 3 parameters
 * 
 * @param priority: thread's priority {0: low, 1: normal, 2: high}
 * @param deadline: thread's threshold to finish all its actions
 * @param exec_time: thread's execution time when processor is on 100% frequency
 */
class SimThread
{
    public:
        SimThread(int id, int priority, int deadline, int activation_time, int exec_time) : _id(id), _priority(priority), _deadline(deadline), _activation_time(activation_time), _exec_time(exec_time){};
    
        int id() {return _id; };
        int priority() { return _priority; };
        int deadline() { return _deadline; };
        int activation_time() {return _activation_time; };
        int absolute_deadline() { return _activation_time + _deadline; };
        int exec_time() { return _exec_time; };
        int time_executed() {return _time_executed; };
        void increase_time_executed() { _time_executed++; };
        bool finished() {return _time_executed == _exec_time; };


    private:
        int _id;
        int _priority;           // 0 = LOW, 1 = NORMAL, 2 = HIGH
        int _deadline;
        int _activation_time;
        int _exec_time;
        int _time_executed;
};

// Global Variables
const int INF = 9999999;
const double MAX_FREQ = 100.0;
const double MIN_FREQ = 50;
const double FREQ_SCALE = 12.5;
SimThread* RUNNING = nullptr;
double CPU_FREQ;
int IPC = 100;
Mode MODE;
double PERF_THRESHOLD;

// Functions

/**
 * Increases the CPU frequency if the frequency is below the maximum frequency allowed
 */
void increaseFrequency()
{
    if (CPU_FREQ < MAX_FREQ){
        CPU_FREQ += FREQ_SCALE;
    }
}

/**
 * Decreases the CPU frequency if the frequency is above the minimum frequency allowed
 */
void decreaseFrequency(){
    if (CPU_FREQ > MIN_FREQ) {
        CPU_FREQ -= FREQ_SCALE;
    }
}

/**
 * Initialize system mode and performance threshold.
 * Also sets inital CPU frequency based on mode selected.
 */
void init(Mode mode, double perf_threshold) {
    MODE = mode;
    PERF_THRESHOLD = perf_threshold;

    if (MODE == Mode::ENERGY_SAVING) {
        CPU_FREQ = MIN_FREQ;
    } else {
        CPU_FREQ = MAX_FREQ;
    }
}

/**
 * Calculates the slack time, which is the time between the current time and the nearest deadline.
 */
int calculate_slack(std::vector<SimThread*> &queue, int cur_time) {
    if (RUNNING != nullptr) {
        return RUNNING->absolute_deadline() - cur_time;
    }
    if (queue.empty()){
        return INF;
    }
    int nearest_deadline = queue[0]->absolute_deadline();
    return nearest_deadline - cur_time;
}

/**
 * (Should) Calculate the IPS based on current CPU frequency and IPC.
 * IPC is measured by task profile (CPU-Bound, I/O Bound, Memory Bound).
 * Did not found a good way to simulate this.
 */
double calculate_IPS(){
    // Question: is IPS  good metric?
    return CPU_FREQ * IPC;
}

/**
 * Defines the new frequency based on CPU usage (workload) and threads deadlines.
 * Not the final implementation, which means values used to compare statistics may change, as we do more tests.
 */
void DVFS(std::vector<SimThread*> &queue, int cur_time, int idle_time){
    // Calculates CPU usage
    double usage = cur_time > 0 ? (double) (cur_time - idle_time) / cur_time : 0;

    // Calculates CPU IPS
    double IPS = calculate_IPS();

    // If MODE == ENERGY SAVING
    if (MODE == Mode::ENERGY_SAVING) {
        // Calculates slack time (time between current time and nearest deadline)
        int slack = calculate_slack(queue, cur_time);
        // If slack time is bigger than 5 seconds
        if (usage > 0.8 || slack < 5) {
            // Decrease CPU frequency
            increaseFrequency();
        // Else, if CPU usage is above 80% or slack time is lower then 5 seconds
        } else if (usage < 0.8 && slack > 5) {
            // Increase CPU Frequency
            decreaseFrequency();
        }
        std::cout << "CPU Usage = " << usage << std::endl;
    // Else, if MODE == PERFORMANCE
    } else {
        // If current CPU IPS (Instructions Per Second) is lower then the Performance Threshold
        if (IPS < PERF_THRESHOLD) {
            // Increase CPU Frequency
            increaseFrequency();
        // Else, if CPU usage is lower then 40% and IPS is above Performance Threshold
        } else if (usage < 0.4 && IPS > PERF_THRESHOLD) {
            // Decrease CPU Frequency
            decreaseFrequency();
        }
    }
}

/**
 * Sorts the scheduler's queue based on deadline (since we are using EDF)
 */
void sort_queue(std::vector<SimThread*> &queue) {
    std::sort(queue.begin(), queue.end(), [](SimThread* a, SimThread* b) { return a->absolute_deadline() < b->absolute_deadline();});
}

/**
 * Simulates the EDF algorithm
 */
void EDFSim(std::vector<SimThread*> &queue, int cur_time) {
    if (RUNNING != nullptr) {
        if (RUNNING->finished()) {
            std::cout << "Thread " << RUNNING->id() << " finished its execution" << std::endl;
            delete RUNNING;
            RUNNING = nullptr;
        } else if (RUNNING->absolute_deadline() <= cur_time){
            std::cout << "Thread " << RUNNING->id() << " lost its deadline" << std::endl << std::endl;
            delete RUNNING;
            RUNNING = nullptr;
        } else {
            SimThread* next = queue[0];
            if (next->absolute_deadline() < RUNNING->absolute_deadline() && next->activation_time() <= cur_time) {
                queue.erase(queue.begin());
                queue.push_back(RUNNING);
                sort_queue(queue);
                RUNNING = next;
            }
        }
    } else {
        if (!queue.empty()) {
            SimThread* next = queue[0];
            if (next->activation_time() <= cur_time) {
                RUNNING = next;
                queue.erase(queue.begin());
            }
        }
    }

    if (RUNNING != nullptr){
        std::cout << "Thread " << RUNNING->id() << " running; current time = " << cur_time << std::endl;
    } else {
        std::cout << "Thread idle running; current time = " << cur_time << std::endl;
    }
    
}


int main()
{
    // Initialization
    init(Mode::ENERGY_SAVING, 4000); // Performance mode don't work as it should
    int id_counter = 1;

    // Scheduler queue
    std::vector<SimThread*> queue;

    // With these threads in ENERGY_SAVING mode, frequency will start at 50, then go at 100, then back at 50
    queue.push_back(new SimThread(id_counter++, 0, 100, 10, 90));
    queue.push_back(new SimThread(id_counter++, 1, 100, 100, 50));
    queue.push_back(new SimThread(id_counter++, 2, 200, 150, 40));
    queue.push_back(new SimThread(id_counter++, 1, 30, 200, 15));
    queue.push_back(new SimThread(id_counter++, 2, 30, 250, 25));
    sort_queue(queue);

    for (auto thread : queue){
        std::cout << thread->id() << std::endl;
    }

    int cur_time = 0;
    int idle_time = 0;

    while (cur_time < 300) {
        EDFSim(queue, cur_time);
        DVFS(queue, cur_time, idle_time);
        if (RUNNING == nullptr) {
            idle_time++;
        } else {
            RUNNING->increase_time_executed();
        }
        cur_time++;
        std::cout << "Current CPU Frequency: " << CPU_FREQ << std::endl << std::endl;
    }


    return 0;
}
