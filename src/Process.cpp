#include <unistd.h>
#include <algorithm>
#include <chrono>
#include "Process.hpp"
#include "Scrapper.hpp"
#include "Exception.hpp"

Process::Process(int nbThread, std::unique_ptr<ICommunication> const& com)
  : _nbThread(nbThread), _com(com), _running(true) {
}

Process::~Process() {
}

void Process::run() {
  for (int i = 0; i < _nbThread; ++i) {
    createThread(i);
  }

  std::chrono::time_point<std::chrono::system_clock> start;
  start = std::chrono::system_clock::now();

  while (1) {
    std::chrono::time_point<std::chrono::system_clock> now;
    now = std::chrono::system_clock::now();

    Option<Task> task = receiveTask();

    // process task here
    if (task) {
      _tasks.push(*task);
      start = now;
    }

    // timeout if it hasn't been working for 5 seconds
    if (!_tasks.empty()) { // TODO ADD CONDITION VAR ?
      start = now;
    } else if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= 5) {
      stop();
    }

  }
}

void Process::createThread(int id) {
  _thEmpty.push_back(false);
  _threads.push_back(std::thread([this, id] {
	Scrapper scrapper;

	while (_running) {
	  Option<Task> task = _tasks.timedPop(1000);
	  if (task) {
	    _thEmpty[id] = false;

	    try {
	      std::vector<std::string> result = scrapper.parseDocument(task->file, task->info);
	      for (std::string const& str : result) {
		// TODO DEBUG
		std::cout << "found: " << str << std::endl;
	      }
	    } catch (FileException const& e) {
	      std::cerr << e.what() << std::endl;
	    }

	  } else {
	    _thEmpty[id] = true;
	  }
	}

      }));
}

bool Process::isFull() const {
  return _tasks.size() >= (_nbThread * 2ul);
}

// TODO RECEIVE TASK
Option<Task> Process::receiveTask() const {
  return {};
}

void Process::stop() {
  _running = false;

  for (std::thread& th : _threads) {
    th.join();
  }

  std::cout << "stopped process" << std::endl;

  exit(0);
}

bool Process::isWorking() const {
  return !_tasks.empty() ||
    std::find(_thEmpty.begin(), _thEmpty.end(), false) != _thEmpty.end();
}
