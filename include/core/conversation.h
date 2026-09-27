#pragma once

#include "core/message.h"       // include for Message objects
#include <cstddef>              // include for std::size_t

class Conversation {
    public:
        Conversation();         // creates an empty conversation
        ~Conversation();        // destroys the conversation and frees allocated memory


        // copy constructor + copy assignment (make new convo from existing one)
        Conversation(const Conversation& other);
        Conversation& operator=(const Conversation& other);


        // move constructor + move assignment (transfer ownership of existing convo's resources)
        Conversation(Conversation&& other) noexcept;
        Conversation& operator=(Conversation&& other) noexcept;

        // add a new message to end of the conversation
        void append(Message m);

        std::size_t size() const noexcept;      // return # of messages currently stored
        const Message& at(std::size_t i) const;   // return message at index i (throws if out of range)

        const Message* begin() const noexcept;   // return pointer to the first message
        const Message* end() const noexcept;     // return pointer to the element past the last message

    private:
        Message* data_ = nullptr;       // pointer to (dynamically) allocated array of messages; nullptr = no array allocated yet
        std::size_t size_ = 0;          // # of messages stored in the array
        std::size_t capacity_ = 0;      // message slots currently allocated
};