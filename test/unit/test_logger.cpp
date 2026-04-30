#include "logger.hpp"
#include <iostream>
#include <ostream>

int main()
{
  hrd41::Logger* log = hrd41::Singleton<hrd41::Logger>::GetInstance();
  log->Write("Program started", hrd41::Logger::INFO);
  
  log->Write("hello world", hrd41::Logger::INFO);
  log->Write("hello world", hrd41::Logger::DEBUG);
  log->Write("hello world");
  log->SetLoggerLevel(hrd41::Logger::INFO);
  std::cout << "level changed to info" << std::endl;
  log->Write("hello world");
  log->Write("hello world", hrd41::Logger::INFO);
  log->Write("hello world", hrd41::Logger::DEBUG);
  log->Write("hello world");
  log->SetLoggerLevel(hrd41::Logger::ERROR);
  std::cout << "level changed to error" << std::endl;
  log->Write("hello world");
  log->Write("hello world", hrd41::Logger::INFO);
  log->Write("hello world", hrd41::Logger::DEBUG);
  log->Write("hello world");

  log->SetLoggerLevel(hrd41::Logger::DEBUG);

  std::cout << "level changed to debug" << std::endl;
  log->Write("hello world", hrd41::Logger::ERROR, true);
  log->Write("hello world", hrd41::Logger::INFO);
  log->Write("hello world", hrd41::Logger::DEBUG);
  log->Write("hello world");


  return 0;
}
