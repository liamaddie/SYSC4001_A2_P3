/**
 *
 * @file interrupts_101315124_101302106.cpp
 * @author Sasisekhar Govind
 * @author Akshavi Baskaran
 * @author Liam Addie
 */

#include <interrupts_101315124_101302106.hpp>

static unsigned int next_pid = 1; // PID counter to assign PIDs to new processes

std::tuple<std::string, std::string, int> simulate_trace(std::vector<std::string> trace_file, int time, std::vector<std::string> vectors, std::vector<int> delays, std::vector<external_file> external_files, PCB current, std::vector<PCB> wait_queue) {

    std::string trace;      //!< string to store single line of trace file
    std::string execution = "";  //!< string to accumulate the execution output
    std::string system_status = "";  //!< string to accumulate the system status output
    int current_time = time;
    int cntxt_time = 10; // default 10ms for saving context
    int isr_activity_time = 40; // default 40ms for interrupt activity


    //parse each line of the input trace file. 'for' loop to keep track of indices.
    for(size_t i = 0; i < trace_file.size(); i++) {
        auto trace = trace_file[i];

        auto [activity, duration_intr, program_name] = parse_trace(trace);

        if(activity == "CPU"){
            // CPU activity - string format: <time>, <duration>, CPU activity
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", CPU burst\n";
            current_time += duration_intr;
        
        } else if (activity == "SYSCALL"){
            // boilerplate for syscall interrupt
            // boilerplate wants the current tume, corrolated interrupt time, context save time, vector table
            auto [logging, upd_time] = intr_boilerplate(current_time, duration_intr, cntxt_time, vectors);
            execution += logging;
            current_time = upd_time;

            // ISR body (wait time for that device)
            int total_isr = delays[duration_intr];

            // Execution statements based on total_isr time, taken from old assignment 1 solution
            if (total_isr <= isr_activity_time){ // if total_isr fits within the isr_activity_time, just one statement
                execution += std::to_string(current_time) + ", " + std::to_string(total_isr) + ", SYSCALL: run ISR for device " + std::to_string(duration_intr) + "\n";
                current_time += total_isr;
            } else if (total_isr <= 2 * isr_activity_time){ // if total_isr fits within two isr_activity_time, two statements
                execution += std::to_string(current_time) + ", " + std::to_string(isr_activity_time) + ", SYSCALL: run ISR for device " + std::to_string(duration_intr) + "\n";
                current_time += isr_activity_time;

                int remaining = total_isr - isr_activity_time;
                execution += std::to_string(current_time) + ", " + std::to_string(remaining) + ", transfer device data to memory\n";
                current_time += remaining;
            } else { // else, three statements
                execution += std::to_string(current_time) + ", " + std::to_string(isr_activity_time) + ", SYSCALL: run ISR for device " + std::to_string(duration_intr) + "\n";
                current_time += isr_activity_time;

                execution += std::to_string(current_time) + ", " + std::to_string(isr_activity_time) + ", transfer device data to memory\n";
                current_time += isr_activity_time;

                int remaining = total_isr - 2 * isr_activity_time;
                execution += std::to_string(current_time) + ", " + std::to_string(remaining) + ", SYSCALL: finalize ISR for device, check errors\n";
                current_time += remaining;
            }

            // conclude interrupt, return to user mode
            execution += std::to_string(current_time) + ", " + std::to_string(1) + ", return from interrupt (IRET)\n";
            current_time += 1;


        } else if(activity == "END_IO") {
            // boilerplate for endio
            auto[logging, upd_time] = intr_boilerplate(current_time, duration_intr, cntxt_time, vectors);

            execution += logging;
            current_time = upd_time;

            // ISR body (wait time for that device)
            int total_isr = delays[duration_intr];

            if(total_isr <= isr_activity_time){
                execution += std::to_string(current_time) + ", " + std::to_string(total_isr) + ", ENDIO: run the ISR (device driver)\n";
                current_time += total_isr;
            } else{
                execution += std::to_string(current_time) + ", " + std::to_string(isr_activity_time) + ", ENDIO: run the ISR (device driver)\n";
                current_time += isr_activity_time;

                int diff = total_isr - isr_activity_time;
                execution += std::to_string(current_time) + ", " + std::to_string(diff) + ", check device status\n";

                current_time += diff;
            }

            // conclude interrupt, return to user mode
            execution += std::to_string(current_time) + ", " + std::to_string(1) + ", return from interrupt (IRET)\n";
            current_time += 1;

        } else if (activity == "UNKNOWN") {
            // unknown activity
            execution += std::to_string(current_time) + ", " + std::to_string(1) + ", UNKNOWN activity\n";
            current_time += 1;
        }
        else if(activity == "FORK") {
            auto [intr, time] = intr_boilerplate(current_time, 2, 10, vectors);
            execution += intr;
            current_time = time;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your FORK output here
            
            // Cloning PCB for the child process
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", cloning the PCB\n";
            current_time += duration_intr;
            
            // Creating child PCB with new PID with parent's information
            PCB child(next_pid++, current.PID, current.program_name, current.size, -1);
            
            // Allocating memory for the child process
            if(!allocate_memory(&child)) {
                std::cerr << "ERROR: Memory allocation failed for child process" << std::endl;
            }
            
            // Adding child to the wait queue
            wait_queue.push_back(current);
            
            // Child becomes the current process to run (higher priority)
            current = child;
            
            // Calling scheduler
            execution += std::to_string(current_time) + ", " + std::to_string(0) + ", scheduler called\n";

            ///////////////////////////////////////////////////////////////////////////////////////////

            //The following loop helps you do 2 things:
            // * Collect the trace of the chile (and only the child, skip parent)
            // * Get the index of where the parent is supposed to start executing from
            std::vector<std::string> child_trace;
            bool skip = true;
            bool exec_flag = false;
            int parent_index = 0;

            for(size_t j = i; j < trace_file.size(); j++) {
                auto [_activity, _duration, _pn] = parse_trace(trace_file[j]);
                if(skip && _activity == "IF_CHILD") {
                    skip = false;
                    continue;
                } else if(_activity == "IF_PARENT"){
                    skip = true;
                    parent_index = j;
                    if(exec_flag) {
                        break;
                    }
                } else if(skip && _activity == "ENDIF") {
                    skip = false;
                    continue;
                } else if(!skip && _activity == "EXEC") {
                    skip = true;
                    child_trace.push_back(trace_file[j]);
                    exec_flag = true;
                }

                if(!skip) {
                    child_trace.push_back(trace_file[j]);
                }
            }
            i = parent_index;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the child's trace, run the child (HINT: think recursion)
            
            // Returning from FORK interrupt
            execution += std::to_string(current_time) + ", " + std::to_string(1) + ", return from interrupt (IRET)\n";
            current_time += 1;
            
            // Generating system status info/snapshot AFTER IRET
            system_status += "time: " + std::to_string(current_time) + "; current trace: FORK, " + std::to_string(duration_intr) + "\n";
            system_status += print_PCB(current, wait_queue);
            
            // Executing the child process recursively
            auto [child_exec, child_status, child_time] = simulate_trace(
                child_trace,
                current_time,
                vectors,
                delays,
                external_files,
                current,
                wait_queue
            );
            
            // Appending child's execution and system status to our logs
            execution += child_exec;
            system_status += child_status;
            current_time = child_time;
            
            // Child process done, restore/get parent from wait queue
            if(!wait_queue.empty()) {
                current = wait_queue.back();
                wait_queue.pop_back();
            }

            ///////////////////////////////////////////////////////////////////////////////////////////


        } else if(activity == "EXEC") {
            auto [intr, time] = intr_boilerplate(current_time, 3, 10, vectors);
            current_time = time;
            execution += intr;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your EXEC output here
            
            // Getting the size of the new program from external_files
            unsigned int new_size = get_size(program_name, external_files);
            
            // Logging the program size
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", Program is " + std::to_string(new_size) + " MB large\n";
            current_time += duration_intr;
            
            // Freeing current process's memory
            free_memory(&current);
            
            // Updating PCB with new program information
            current.program_name = program_name;
            current.size = new_size;
            
            // Allocating memory for the new program
            if(!allocate_memory(&current)) {
                std::cerr << "ERROR! Memory allocation failed for EXEC!" << std::endl;
            }
            
            // Simulate loading the program from disk to memory (takes 15ms per Mb)
            int load_time = new_size * 15;
            execution += std::to_string(current_time) + ", " + std::to_string(load_time) + ", loading program into memory\n";
            current_time += load_time;
            
            // Mark partitions as occupied (takes 3ms)
            execution += std::to_string(current_time) + ", " + std::to_string(3) + ", marking partition as occupied\n";
            current_time += 3;
            
            // Updating PCB (takes 6ms)
            execution += std::to_string(current_time) + ", " + std::to_string(6) + ", updating PCB\n";
            current_time += 6;
            
            // Calling scheduler
            execution += std::to_string(current_time) + ", " + std::to_string(0) + ", scheduler called\n";

            ///////////////////////////////////////////////////////////////////////////////////////////


            std::ifstream exec_trace_file(program_name + ".txt");

            std::vector<std::string> exec_traces;
            std::string exec_trace;
            while(std::getline(exec_trace_file, exec_trace)) {
                exec_traces.push_back(exec_trace);
            }

            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the exec's trace (i.e. trace of external program), run the exec (HINT: think recursion)
            
            // Returning from EXEC interrupt
            execution += std::to_string(current_time) + ", " + std::to_string(1) + ", return from interrupt (IRET)\n";
            current_time += 1;
            
            // Generating system status info/snapshot AFTER IRET
            system_status += "time: " + std::to_string(current_time) + "; current trace: EXEC " + program_name + ", " + std::to_string(duration_intr) + "\n";
            system_status += print_PCB(current, wait_queue);
            
            // Executing the new program recursively
            auto [exec_exec, exec_status, exec_time] = simulate_trace(
                exec_traces,
                current_time,
                vectors,
                delays,
                external_files,
                current,
                wait_queue
            );
            
            // Appending exec's execution and system status to our logs
            execution += exec_exec;
            system_status += exec_status;
            current_time = exec_time;

            ///////////////////////////////////////////////////////////////////////////////////////////

            break; //Why is this important? (answer in report)

        }
     }
 
     return {execution, system_status, current_time};
 }
 
 int main(int argc, char** argv) {
 
     //vectors is a C++ std::vector of strings that contain the address of the ISR
     //delays  is a C++ std::vector of ints that contain the delays of each device
     //the index of these elemens is the device number, starting from 0
     //external_files is a C++ std::vector of the struct 'external_file'. Check the struct in 
     //interrupt.hpp to know more.
     auto [vectors, delays, external_files] = parse_args(argc, argv);
     std::ifstream input_file(argv[1]);
 
     //Just a sanity check to know what files you have
     print_external_files(external_files);
 
     //Make initial PCB (notice how partition is not assigned yet)
     PCB current(0, -1, "init", 1, -1);
     //Update memory (partition is assigned here, you must implement this function)
     if(!allocate_memory(&current)) {
         std::cerr << "ERROR! Memory allocation failed!" << std::endl;
     }
 
     std::vector<PCB> wait_queue;
 
     /******************ADD YOUR VARIABLES HERE*************************/
 
 
     /******************************************************************/
 
     //Converting the trace file into a vector of strings.
     std::vector<std::string> trace_file;
     std::string trace;
     while(std::getline(input_file, trace)) {
         trace_file.push_back(trace);
     }
 
     auto [execution, system_status, _] = simulate_trace(   trace_file, 
                                             0, 
                                             vectors, 
                                             delays,
                                             external_files, 
                                             current, 
                                             wait_queue);
 
     input_file.close();
 
     write_output(execution, "execution.txt");
     write_output(system_status, "system_status.txt");
 
     return 0;
 }
