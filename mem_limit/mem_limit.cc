#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>
#include <string.h>
#include <sched.h>
#include <sstream>
#include <unistd.h>
#ifndef NO_MPI
#include <mpi.h>
#endif
#ifdef _OPENMP
#include <omp.h>
#endif

// exit codes for application
const int EXIT_OPT_ERROR {1};
const int EXIT_CONFIG_ERROR {2};
const int EXIT_MEM_ERROR {3};

// maximum length for a hostname
const int MAX_PROCESSOR_NAME {1024};

size_t convert_size(const char *size_spec);
long convert_time(const char *time_spec);
char* allocate_memory(size_t size);
void fill_memory(char *buffer, size_t size);
void fill_memory_threaded(char *buffer, size_t size);
void parse_config(const std::string& file_name, int target_line_nr,
                  size_t& max_size, size_t& increment, long& sleeptime,
                  int& nr_threads, size_t** max_sizes,
                  size_t** increments, long** sleeptimes);
void print_help();
std::string get_now();

int main(int argc, char *argv[]) {
    const int root {0};
#ifndef NO_MPI
    int thread_level;
    MPI_Init_thread(NULL, NULL, MPI_THREAD_FUNNELED, &thread_level);
    if (thread_level != MPI_THREAD_FUNNELED) {
        std::cerr << "#error: unexpected thread level" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
#endif
    int rank {0};
    int size {1};
#ifndef NO_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif
    char *conf_file_name {nullptr};
    size_t proc_max_size {0};
    size_t proc_increment {0};
    long  proc_sleeptime {0};
    int nr_threads {1};
    size_t max_size {0};
    size_t *max_sizes {nullptr};
    size_t increment {0};
    size_t *increments {nullptr};
    long sleeptime {0};
    long *sleeptimes {nullptr};
    long lifetime {0};
    int is_verbose {0};
    int name_length {0};
    int is_done = 0;
    if (rank == root) {
        bool opt_sufficient {false};
        char opt {'\0'};
        while ((opt = getopt(argc, argv, "f:t:m:i:s:l:vh")) != -1) {
            try {
                switch (opt) {
                    case 'f':
                        conf_file_name = optarg;
                        name_length = strlen(conf_file_name);
                        opt_sufficient = true;
                        break;
                    case 't':
                        nr_threads = atoi(optarg);
                        break;
                    case 'm':
                        max_size = convert_size(optarg);
                        opt_sufficient = true;
                        break;
                    case 'i':
                        increment = convert_size(optarg);
                        break;
                    case 's':
                        sleeptime = convert_time(optarg);
                        break;
                    case 'l':
                        lifetime = convert_time(optarg);
                        break;
                    case 'v':
                        is_verbose = 1;
                        break;
                    case 'h':
                        print_help();
                        opt_sufficient = 1;
                        is_done = 1;
                        break;
                    default:
                        std::stringstream msg;
                        msg << "# error: unknown option '-" << opt << "'"
                            << std::endl;
                        std::cerr << msg.str();
                        print_help();
#ifndef NO_MPI
                        MPI_Abort(MPI_COMM_WORLD, EXIT_OPT_ERROR);
#endif
                        std::exit(EXIT_OPT_ERROR);
                }
            } catch (const std::invalid_argument& e) {
                    std::stringstream msg;
                    msg << "# error: invalid option value, " << e.what()
                        << std::endl;
                    std::cerr << msg.str();
                    print_help();
                    is_done = 1;
            }
        }
        if (!opt_sufficient) {
            std::stringstream msg;
            msg << "# error: expecting at least -f or -m option"
                << std::endl;
            std::cerr << msg.str();
            print_help();
            is_done = 1;
        }
        std::stringstream msg;
        msg << get_now() << ": running with " << size << " processes"
            << "\n";
        std::cout << msg.str();
    }
#ifndef NO_MPI
    MPI_Bcast(&is_done, 1, MPI_INT, root, MPI_COMM_WORLD);
#endif
    if (is_done) {
#ifndef NO_MPI
        MPI_Finalize();
#endif
        return 0;
    }
#ifndef NO_MPI
    MPI_Bcast(&is_verbose, 1, MPI_INT, root, MPI_COMM_WORLD);
    MPI_Bcast(&name_length, 1, MPI_INT, root, MPI_COMM_WORLD);
#endif
    if (name_length > 0) {
        if (rank != root) {
            conf_file_name = new char[name_length + 1];
        }
#ifndef NO_MPI
        MPI_Bcast(conf_file_name, name_length + 1, MPI_CHAR,
                  root, MPI_COMM_WORLD);
#endif
        if (is_verbose) {
            std::stringstream msg;
            msg << "rank " << rank << ": reading "
                << "'" << conf_file_name << "'" << std::endl;
            std::cerr << msg.str();
        }
        try {
            parse_config(conf_file_name, rank,
                         proc_max_size, proc_increment, proc_sleeptime,
                         nr_threads,
                         &max_sizes, &increments, &sleeptimes);
        } catch (const std::exception& e) {
            std::stringstream msg;
            msg << "# error: invalid configuration file, " << e.what()
                << std::endl;
            std::cerr << msg.str();
#ifndef NO_MPI
            MPI_Abort(MPI_COMM_WORLD, EXIT_CONFIG_ERROR);
#endif
            std::exit(EXIT_CONFIG_ERROR);
        }
        if (is_verbose) {
            std::stringstream msg;
            msg << "rank " << rank << " running with " << nr_threads << " threads"
                << std::endl;
            for (int thread_nr = 0; thread_nr < nr_threads; thread_nr++) {
                msg << "rank " << rank << ": "
                    << "thread " << thread_nr << ", "
                    << "max. size = " << max_sizes[thread_nr] << ", "
                    << "increment = " << increments[thread_nr] << ", "
                    << "sleep time = " << sleeptimes[thread_nr]
                    << std::endl;
            }
            std::cerr << msg.str();
        }
    } else {
#ifndef NO_MPI
        MPI_Bcast(&nr_threads, 1, MPI_INT, root, MPI_COMM_WORLD);
        MPI_Bcast(&max_size, 1, MPI_UNSIGNED_LONG, root, MPI_COMM_WORLD);
        MPI_Bcast(&increment, 1, MPI_UNSIGNED_LONG, root, MPI_COMM_WORLD);
        MPI_Bcast(&sleeptime, 1, MPI_LONG, root, MPI_COMM_WORLD);
        MPI_Bcast(&lifetime, 1, MPI_LONG, root, MPI_COMM_WORLD);
#endif
        if (is_verbose) {
            std::stringstream msg;
            msg << "rank " << rank << ": "
                << "threads = " << nr_threads << ", "
                << "max. size = " << max_size << ", "
                << "increment = " << increment << ", "
                << "sleep time = " << sleeptime << std::endl;
            std::cerr << msg.str();
        }
        proc_increment = increment;
        proc_max_size = max_size;
        max_sizes = new size_t[nr_threads];
        increments = new size_t[nr_threads];
        sleeptimes = new long[nr_threads];
        for (int i = 0; i < nr_threads; i++) {
            max_sizes[i] = max_size;
            increments[i] = increment;
            sleeptimes[i] = sleeptime;
        }
    }
    int max_threads {1};
#ifdef _OPENMP
    max_threads = omp_get_max_threads();
#endif
    if (max_threads < nr_threads) {
        std::stringstream msg;
        msg << "# warning rank " << rank << ": "
            << "nr. threads " << nr_threads
            << " exceeds max. threads " << max_threads
            << std::endl;
        std::cerr << msg.str();
    }
    int max_processor_length = MAX_PROCESSOR_NAME;
#ifndef NO_MPI
    max_processor_length = MPI_MAX_PROCESSOR_NAME;
#endif
    char processor_name[max_processor_length];
#ifndef NO_MPI
    int processor_name_len;
    MPI_Get_processor_name(processor_name, &processor_name_len);
#else
    gethostname(processor_name, max_processor_length);
#endif
#ifdef _OPENMP
    omp_set_num_threads(nr_threads);
#endif

#pragma omp parallel
    {
        int thread_nr {0};
#ifdef _OPENMP
        thread_nr = omp_get_thread_num();
#endif
        size_t increment = increments[thread_nr] > 0 ?
            increments[thread_nr] : max_sizes[thread_nr];
        for (size_t mem = increment; mem <= max_sizes[thread_nr];
                mem += increment) {
            int cpu_nr = sched_getcpu();
            std::stringstream msg;
            msg << get_now() << ": "
                << "rank " << rank << "#" << thread_nr
                << " on " << cpu_nr << "@" << processor_name << ": "
                << "allocating " << mem << " bytes" << "\n";
            std::cout << msg.str();
            try {
                char *buffer = allocate_memory(mem);
                msg.str("");
                msg << get_now() << ": "
                    << "rank " << rank << "#" << thread_nr
                    << " on " << cpu_nr << "@" << processor_name << ": "
                    << "filling " << mem << " bytes" << "\n";
                std::cout << msg.str();
                fill_memory(buffer, mem);
                std::chrono::microseconds period(sleeptimes[thread_nr]);
                std::this_thread::sleep_for(period);
                delete[] buffer;
            } catch (const std::runtime_error& e) {
                std::stringstream msg;
                msg << "# error: allocation of " << mem << " bytes failed"
                    << std::endl;
                std::cerr << msg.str();
#ifndef NO_MPI
                MPI_Abort(MPI_COMM_WORLD, EXIT_MEM_ERROR);
#endif
                std::exit(EXIT_MEM_ERROR);
            }
        }
    }
    std::chrono::microseconds period(lifetime);
    std::this_thread::sleep_for(period);
    delete[] max_sizes;
    delete[] increments;
    delete[] sleeptimes;
#ifndef NO_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    if (rank == root) {
        std::stringstream msg;
        msg << get_now() << ": successfully done" << "\n";
        std::cout << msg.str();
    }
#ifndef NO_MPI
    MPI_Finalize();
#endif
    return 0;
}

size_t convert_size(const char *size_spec) {
    std::stringstream stream;
    stream.str(size_spec);
    size_t number {0};
    stream >> number;
    std::string unit;
    stream >> unit;
    if (unit != "") {
        std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
        if (unit == "kb") {
            number *= 1024;
        } else if (unit == "mb") {
            number *= 1024*1024;
        } else if (unit == "gb") {
            number *= 1024*1024*1024;
        } else if (unit != "b") {
            throw std::invalid_argument("unknown unit");
        }
    }
    return number;
}

long convert_time(const char *time_spec) {
    std::stringstream stream;
    stream.str(time_spec);
    long number {0};
    stream >> number;
    std::string unit;
    stream >> unit;
    if (unit != "") {
        std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
        if (unit == "s") {
            number *= 1000000;
        } else if (unit == "ms") {
            number *= 1000;
        } else if (unit == "m") {
            number *= 60*1000000;
        } else if (unit != "us") {
            throw std::invalid_argument("unknown unit");
        }
    }
    return number;
}

char* allocate_memory(size_t size) {
    char* buffer {NULL};
    if ((buffer = (char*) malloc(size*sizeof(char))) == NULL)
        throw std::runtime_error("can allocate memory");
    return buffer;
}

void fill_memory(char *buffer, size_t size) {
    char fill = 'A';
    for (size_t i = 0; i < size; i++) {
        buffer[i] = fill;
        fill = fill == 'Z' ? 'A' : fill + 1;
    }
}

void fill_memory_threaded(char *buffer, size_t size) {
    char fill = 'A';
#pragma omp for
    for (size_t i = 0; i < size; i++) {
        buffer[i] = fill;
        fill = fill == 'Z' ? 'A' : fill + 1;
    }
}

std::vector<std::string> split(const std::string& str,
                               const std::string& delim) {
    std::vector<std::string> parts;
    size_t pos = 0, old_pos = 0;
    while ((pos = str.find(delim, old_pos)) != std::string::npos) {
        parts.push_back(str.substr(old_pos, pos - old_pos));
        old_pos = pos + delim.length();
    }
    parts.push_back(str.substr(old_pos));
    return parts;
}

void parse_config(const std::string& file_name, int target_line_nr,
                  size_t& max_size, size_t& increment, long& sleeptime,
                  int& nr_threads, size_t** max_sizes,
                  size_t** increments, long** sleeptimes) {
    std::regex comment_re {R"(^\s*#)"};
    std::regex empty_re {R"(^\s*$)"};
    std::ifstream config_file;
    config_file.open(file_name);
    int line_nr {0};
    std::string line;
    while (config_file) {
        std::string curr_line;
        std::getline(config_file, curr_line);
        std::smatch match;
        if (std::regex_search(curr_line, match, empty_re))
            continue;
        if (std::regex_search(curr_line, match, comment_re))
            continue;
        line = curr_line;
        if (line_nr++ == target_line_nr)
            break;
    }
    config_file.close();
    if (line.length() > 0) {
        std::vector<std::string> parts = split(line, ";");
        if (parts.size() == 2) {
            max_size = 0;
            increment = 0;
            sleeptime = 0;
            nr_threads = std::stoi(parts.at(0));
            parts = split(parts.at(1), ":");
        } else if (parts.size() == 3) {
            std::vector<std::string> specs = split(parts.at(0), "+");
            max_size = convert_size(specs.at(0).c_str());
            increment = convert_size(specs.at(1).c_str());
            sleeptime = convert_time(specs.at(2).c_str());
            nr_threads = std::stoi(parts.at(1));
            parts = split(parts.at(2), ":");
        }
        *max_sizes = new size_t[nr_threads];
        *increments = new size_t[nr_threads];
        *sleeptimes = new long[nr_threads];
        size_t i {0};
        for (i = 0; i < parts.size() && ((int) i) < nr_threads; i++) {
            std::vector<std::string> specs = split(parts.at(i), "+");
            (*max_sizes)[i] = convert_size(specs.at(0).c_str());
            (*increments)[i] = convert_size(specs.at(1).c_str());
            (*sleeptimes)[i] = convert_time(specs.at(2).c_str());
        }
        for (int j = i; j < nr_threads; j++) {
            (*max_sizes)[j] = (*max_sizes)[i - 1];
            (*increments)[j] = (*increments)[i - 1];
            (*sleeptimes)[j] = (*sleeptimes)[i - 1];
        }
    } else {
        // TODO: throw exception
    }
}

void print_help() {
    std::stringstream msg;
    msg << "Usage: mem_limit [-v] [-h] ( "
        << "-f <conf_file> | \\" << std::endl
        << "\t\t( -m <size> [-i <size>] [-s <time>] [-t <n>] )) "
        << "[-l <time>] "
        << std::endl;
    msg << "\t-f <conf_file>: configuration file to use" << std::endl;
    msg << "\t-m <size>: maximum memory size" << std::endl;
    msg << "\t-i <size>: memory increment per step" << std::endl;
    msg << "\t-s <time>: time to sleep between steps" << std::endl;
    msg << "\t-t <n>: number of threads per processs" << std::endl;
    msg << "\t-l <time>: time to stay alive after last step" << std::endl;
    msg << "\t-v: give verbose output" << std::endl;
    msg << "\t-h: show this help message" << std::endl;
    msg << "<size> takes units b, kb, mb, gb, default is bytes"
        << std::endl;
    msg << "<time> takes units m, s, ms, us, default is microseconds"
        << std::endl;
    std::cerr << msg.str();
}

std::string get_now() {
    auto time {std::chrono::system_clock::now()};
    auto duration = time.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1'000'000;
    auto time_c {std::chrono::system_clock::to_time_t(time)};
    auto tm {std::localtime(&time_c)};
    std::stringstream str;
    str << std::put_time(tm, "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(6) << microseconds;
    return str.str();
}
