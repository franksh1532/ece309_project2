#include "core/conversation.h"

#include <stdexcept>

Conversation::Conversation(const Conversation& other): data_(nullptr), size_(0), capacity_(other.capacity_){
        
        if (capacity_ == 0){  // if convo has no allocated array, theres nothing to copy
            return;
        }

        // allocate separate message array and copy the messages from the other conversation
        data_ = new Message[capacity_]; // allocate array

        for (std::size_t i = 0; i < other.size_; ++i){  // copying
            data_[i] = other.data_[i];
        }

        // record # of valid messages copied
        size_ = other.size_;
    }


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

    // check if we need to resize the array before adding the new message
    if (size_ == capacity_){

        // first allocate gets capacity 1; doubling each time
        std::size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;

        // allocate new larger array of message objects
        Message* new_data = new Message[new_capacity];

        // copy all existing messages to the new array
        for (std::size_t i = 0; i < size_; ++i){
            new_data[i] = data_[i];
        }

        // free the old array
        delete[] data_;

        // update the pointer and capacity
        data_ = new_data;
        capacity_ = new_capacity;
    }


    data_[size_] = m;   // put new message into first unused slot
    ++size_;            // increment the number of messages stored
}