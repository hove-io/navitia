#include "locker.h"
#include "type/data.h"

namespace navitia{ namespace type {
    Locker::Locker() : locked(false), data(NULL){}

    Locker::Locker(navitia::type::Data& data, bool exclusive) : data(&data), exclusive(exclusive) {
        if(exclusive){
            this->data->load_mutex.lock();
            locked = true;
        }else{
            this->data->load_mutex.lock_shared();
            locked = true;
        }
    }

    Locker::~Locker() {
        if(locked){
            if(exclusive){
                data->load_mutex.unlock();
            }else{
                data->load_mutex.unlock_shared();
            }
        }
    }

    Locker::Locker(Locker&& other) : locked(other.locked), data(other.data), exclusive(other.exclusive){
        other.locked = false;
    }


}}
