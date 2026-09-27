#include "core/conversation.h"

#include <stdexcept>

Conversation::Conversation() = default;

Conversation::~Conversation() {
    delete[] data_;
}

std::size_t Conversation::size() const noexcept {
    return size_;
}

const Message& Conversation::at(std::size_t i) const {
    if (i >= size_) {
        throw std::out_of_range("Index out of range");
    }

    return data_[i];
}
 
const Message* Conversation::begin() const noexcept {
    return data_;
}

const Message* Conversation::end() const noexcept {
    if (data_ == nullptr){
        return nullptr;
    }

    return data_ + size_;
}



void Conversation::append(Message m){
    if (size_ == capacity_){
        std::size_t new_capacity;

        if (capacity_ == 0){
            new_capacity = 1;
        } 
        else{
            new_capacity = capacity_*2;
        }

        Message* new_data = new Message[new_capacity];

        for (std::size_t i = 0; i < size_; ++i){
            new_data[i] = data_[i];
        }

        delete[] data_;

        data_ = new_data;
        capacity_ = new_capacity;
    }
    
    data_[size_] = m;
    ++size_;
}