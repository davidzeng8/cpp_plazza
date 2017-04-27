#include <csignal>
#include <sys/wait.h>
#include <string.h>
#include <sstream>
#include <algorithm>
#include "Plazza.hpp"
#include "Fork.hpp"
#include "Process.hpp"
#include "Utils.hpp"
#include "Exception.hpp"

Plazza::Plazza(int nbThread) : _nbThread(nbThread), _finished(true) { // TODO set finished to false
  // handle child death
  static auto handler = [this] (int) {
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
      deleteProcess(pid);
    }
  };

  struct sigaction sa;

  bzero(&sa, sizeof(struct sigaction));
  sa.sa_handler = [] (int sig) {
    handler(sig);
  };

  sigaction(SIGCHLD, &sa, NULL);
}

Plazza::~Plazza() {
}

pid_t Plazza::createProcess() {
  Fork process;
  std::unique_ptr<ICommunication> com; // TODO open a socket ?

  // fork failed
  if (process.getPid() == -1) {
    throw ProcessException("Process initialization failed");
  }
  // child process here
  else if (process.getPid() == 0) {
    int nb = _nbThread;
    process.run([nb] () {
	Process proc(nb);
	proc.run();
      });
    exit(0);
  }
  // parent process
  else {
    _processes[process.getPid()] = std::move(com);
    return process.getPid();
  }
}

// TODO SEND TASK
void Plazza::sendTask(pid_t process, Task const& task) const {
  (void)process;
  (void)task;
}

void Plazza::deleteProcess(pid_t pid) {
  if (!_processes.count(pid)) {
    return;
  }

  // TODO ADD COMMUNICATION
  // std::unique_ptr<ICommunication> com = std::move(_processes[pid]);

  _processes.erase(_processes.find(pid));
  // com->close();
}

pid_t Plazza::getAvailableProcess() const {
  // TODO GET AVAILABLE PROCESS
  return -1;
}

void Plazza::parseSTDIN() {
  _producer = std::thread([this] () {
      std::string line;
      std::string cmd;

      while (getline(std::cin, line)) {
	std::istringstream ss(line);

	while (getline(ss, cmd, ';')) {
	  cmd = Utils::trim(cmd);

	  std::vector<Task> tasks = readTask(cmd);

	  _mutex.lock();
	  std::for_each(tasks.begin(), tasks.end(), [this] (Task const& task) {
	      _tasks.push(task);
	    });
	  _mutex.unlock();
	}

      }
      // notify main that the thread is done
      _producer.detach();
    });
}

Option<Task> Plazza::getNextTask() {
  if (_tasks.empty()) {
    return {};
  }

  _mutex.lock();
  Option<Task> task(_tasks.front());
  _tasks.pop();
  _mutex.unlock();
  return task;
}

bool Plazza::isRunning() const {
  return !_finished || _producer.joinable();
}

// TODO IS PROCESS FULL
bool Plazza::isProcessFull(pid_t pid) const {
  (void)pid;
  return true;
}

std::vector<Task> Plazza::readTask(std::string const& line) const {
  std::cout << "got: " << line << std::endl;
  std::vector<std::string> tokens;
  std::string str;
  std::stringstream sstr(line);

  while ((sstr >> str)) {
    tokens.push_back(str);
  }

  if (tokens.size() <= 1) {
    return {};
  }

  Information info;
  try {
    info = Info::fromString(tokens.back());
  } catch (InformationException const& e) {
    std::cerr << e.what() << std::endl;
    return {};
  }

  tokens.pop_back();

  std::vector<Task> tasks;
  for (std::string const& file : tokens) {
    tasks.push_back({file, info});
  }

  return tasks;
}