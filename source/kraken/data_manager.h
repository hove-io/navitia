#pragma once
#include <memory>
#include <iostream>

template<typename Data>
class DataManager{
    std::shared_ptr<Data> current_data;

    public:
    DataManager() : current_data(std::make_shared<Data>()){}

    inline std::shared_ptr<Data> get_data() const{return current_data;}

    bool load(const std::string& database){
        auto data = std::make_shared<Data>();
        bool success = data->load(database);
        if(success){
            std::swap(current_data, data);
        }
        return success;
    }
};
