#include "Communication.hpp"

Communication::Communication(int id)
  : _id(id)
{
}

Communication::~Communication()
{
}

void Communication::openCommunicationMain()
{
  std::string oPath(PFIFO + std::to_string(_id));
  std::string iPath(PFIFO + std::to_string(_id + 1));

  int fifo = mkfifo(oPath.c_str(), 0666);
  if (fifo == -1)
    throw CommunicationException("Parent: Error while creating named pipe");
  fifo = mkfifo(iPath.c_str(), 0666);
  if (fifo == -1)
    throw CommunicationException("Parent: Error while create named pipe");

  _outputPipe.open(oPath, std::ofstream::out | std::ofstream::binary);
  if (!_outputPipe.is_open())
    throw CommunicationException("Parent: Error while opening output named pipe");
  _inputPipe.open(iPath, std::ifstream::in | std::ifstream::binary);
  if (!_inputPipe.is_open())
    throw CommunicationException("Parent: Error while opening input named pipe");
}

void Communication::openCommunicationChild()
{
  std::string oPath(PFIFO + std::to_string(_id + 1));
  std::string iPath(PFIFO + std::to_string(_id));

  _inputPipe.open(iPath, std::ifstream::in | std::ifstream::binary);
  if (!_inputPipe.is_open())
    throw CommunicationException("Child: Error while opening input named pipe");
  _outputPipe.open(oPath, std::ofstream::out | std::ofstream::binary);
  if (!_outputPipe.is_open())
    throw CommunicationException("Child: Error while opening output named pipe");
}

void Communication::sendMsg(Package msg)
{
  _outputPipe.write(reinterpret_cast<char*>(&msg), sizeof(Package));
}

Package Communication::receiveMsg()
{
  Package msg;
  _inputPipe.read(reinterpret_cast<char*>(&msg), sizeof(Package));
  if (_inputPipe.fail())
    {
      msg.type = UNDEFINED;
      msg.content.value = -1;
    }
  return (msg);
}

void Communication::close()
{
  _outputPipe.close();
  _inputPipe.close();
}

void Communication::rmfifo()
{
  std::string oPath(PFIFO + std::to_string(_id + 1));
  std::string iPath(PFIFO + std::to_string(_id));

  unlink(oPath.c_str());
  unlink(iPath.c_str());
}
