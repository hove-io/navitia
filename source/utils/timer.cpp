#include "timer.h"

Timer::Timer() : start(std::chrono::system_clock::now()) {}

Timer::Timer(const std::string &name) : start(std::chrono::system_clock::now()), name(name){}

Timer::~Timer() {
    std::cout << *this << std::endl;
}

std::ostream & operator<<(std::ostream & os, const Timer & timer){
    os << "Timer " << timer.name << " ";
    auto delta = std::chrono::system_clock::now() - timer.start;
    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();

    if(ms < 10000)
        os << ms << " ms";
    else
        os << (ms/1000) << " s";

    return os;
}
