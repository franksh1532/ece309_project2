// tests/p2/test_p2.cpp

#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// for capacity_ and pending_ testing
#define private public
#include "core/conversation.h"
#include "core/sentinel_scanner.h"
#undef private

#include "harness/harness.h"
#include "model/replay_client.h"
#include "model/scripted_client.h"



class MockInput : public InputSource{

    public:
        explicit MockInput(std::vector<std::string> lines): lines_(std::move(lines)){}

        std::string read_line() override{
            if (index_ >= lines_.size()){
                eof_ = true;
                return "";
            }
            return lines_[index_++];
        }

        bool is_eof() const override{
            return eof_;
        }

    private:
        std::vector<std::string> lines_;
        std::size_t index_ = 0;
        bool eof_ = false;
};



class MockOutput : public OutputSink{

    public:
        void write(std::string_view text) override{
            output_ += text;
        }

        std::string output_;
};




// transcript testing
const char* role_name(Role role){

    switch (role){
        case Role::System:
            return "system";

        case Role::User:
            return "user";

        case Role::Assistant:
            return "assistant";
    }

    return "assistant";
}




void write_transcript(const Conversation& conv, const std::string& path){

    std::ofstream file(path);

    bool first = true;

    for (const Message* m = conv.begin(); m != conv.end(); ++m){

        if (!first){
            file << "---\n";
        }

        first = false;

        file << "role: " << role_name(m->role()) << "\n";
        file << m->content() << "\n";
    }
}



// test 1: empty conversation
// verify an empty conversation has no messages and rejects invalid access
void test_empty_conversation(){

    Conversation conv;

    assert(conv.size() == 0);
    assert(conv.begin() == conv.end());

    bool threw = false;

    try{
        conv.at(0);
    }
    catch (const std::out_of_range&){
        threw = true;
    }

    assert(threw);
}



// test 2: system message ordering
// verify messages remain in system, user, assistant order
void test_system_message_ordering(){

    Conversation conv;

    conv.append(Message(Role::System, "system"));
    conv.append(Message(Role::User, "hello"));
    conv.append(Message(Role::Assistant, "hi"));

    assert(conv.size() == 3);

    assert(conv.at(0).role() == Role::System);
    assert(conv.at(1).role() == Role::User);
    assert(conv.at(2).role() == Role::Assistant);
}



// test 3: copy semantics
// verify copy constructor and copy assignment perform deep copies
void test_copy_semantics(){

    Conversation original;

    original.append(Message(Role::User, "hello"));
    original.append(Message(Role::Assistant, "hi"));

    // copy constructor should use a separate array
    Conversation copy(original);

    assert(copy.size() == original.size());

    // deep copy should use a diff backing array
    assert(copy.begin() != original.begin());

    assert(copy.at(0).content() == "hello");
    assert(copy.at(1).content() == "hi");

    Conversation assigned;
    assigned.append(Message(Role::User, "old message"));

    // copy assignment should also make a deep copy
    assigned = original;

    assert(assigned.size() == original.size());
    assert(assigned.begin() != original.begin());
    assert(assigned.at(0).content() == "hello");
    assert(assigned.at(1).content() == "hi");

    assigned = assigned;
    assert(assigned.size() == 2);
}



// test 4: move semantics
// verify move constructor and move assignment correctly transfer ownership
void test_move_semantics(){

    Conversation original;
    original.append(Message(Role::User, "hello"));

    const Message* original_pointer = original.begin();

    // move constructor should take the existing ptr
    Conversation moved(std::move(original));

    // move should steal the existing array instead of copying it
    assert(moved.begin() == original_pointer);
    assert(original.begin() == nullptr);
    assert(original.size() == 0);
    assert(original.capacity_ == 0);
    assert(moved.at(0).content() == "hello");

    Conversation source;
    source.append(Message(Role::User, "new message"));

    Conversation destination;
    destination.append(Message(Role::User, "old message"));

    const Message* source_pointer = source.begin();

    // move assignment should leave the source empty
    destination = std::move(source);

    assert(destination.begin() == source_pointer);
    assert(source.begin() == nullptr);
    assert(source.size() == 0);
    assert(source.capacity_ == 0);
    assert(destination.at(0).content() == "new message");
}



// test 5: array growth
// verify capacity follows the doubling strategy and that messages survive resizing
void test_growth(){

    Conversation conv;

    assert(conv.capacity_ == 0);

    conv.append(Message(Role::User, "0"));
    assert(conv.capacity_ == 1);

    conv.append(Message(Role::User, "1"));
    assert(conv.capacity_ == 2);

    conv.append(Message(Role::User, "2"));
    assert(conv.capacity_ == 4);

    conv.append(Message(Role::User, "3"));
    assert(conv.capacity_ == 4);

    conv.append(Message(Role::User, "4"));
    assert(conv.capacity_ == 8);

    assert(conv.size() == 5);

    // make sure messages survive each resize
    assert(conv.at(0).content() == "0");
    assert(conv.at(1).content() == "1");
    assert(conv.at(2).content() == "2");
    assert(conv.at(3).content() == "3");
    assert(conv.at(4).content() == "4");
}



// test 6: scanner clean text
// verify normal streamed text is emitted correctly when no sentinel appears
void test_scanner_clean_text(){

    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    auto first = scanner.feed("Hello ");
    auto second = scanner.feed("world");
    auto final = scanner.flush();

    assert(!first.sentinel_found);
    assert(!second.sentinel_found);
    assert(!final.sentinel_found);

    std::string result = first.safe_text + second.safe_text + final.safe_text;

    assert(result == "Hello world");
}



// test 7: sentinel split at every possible boundary
// verify the sentinel is detected at every possible chunk boundary
void test_scanner_every_split(){

    const std::string sentinel = "<|end_conversation|>";
    const std::string text = "Goodbye." + sentinel;

    // verify sentinel is found regardless of chunk boundary
    for (std::size_t split = 0; split <= text.size(); ++split){

        SentinelScanner scanner(sentinel);

        auto first = scanner.feed(text.substr(0, split));
        auto second = scanner.feed(text.substr(split));

        assert(first.sentinel_found || second.sentinel_found);
        assert(first.safe_text + second.safe_text == "Goodbye.");
    }
}



// test 8: false sentinel
// verify similar looking text doesn't falsely trigger the sentinel
void test_scanner_false_alarm(){

    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    const std::string text = "Hello <|end_world|> goodbye";

    auto first = scanner.feed(text);
    auto final = scanner.flush();

    assert(!first.sentinel_found);
    assert(!final.sentinel_found);

    assert(first.safe_text + final.safe_text == text);
}



// test 9: scanner bounded memory
// verify pending_ stays bounded during a large streamed input
void test_scanner_bounded_memory(){

    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    const std::string pattern = "<|end_";
    const std::size_t bytes = 4 * 1024 * 1024;

    // pending_ must remain bounded even on a 4 MB stream
    for (std::size_t i = 0; i < bytes; ++i){

        char c = pattern[i % pattern.size()];

        scanner.feed(
            std::string_view(&c, 1)
        );

        // pending_ should never exceed sentinel length - 1
        assert(
            scanner.pending_.size()
            <= sentinel.size() - 1
        );
    }
}



// test 10: harness turn limit
// verify the harness stops after the max number of turns is reached
void test_harness_turn_limit(){

    const std::string path = "test_turn_limit.script";

    {
        std::ofstream file(path);

        file << "role: assistant\n";
        file << "Hello!\n";
    }

    auto model = std::make_unique<ScriptedModelClient>(path);

    HarnessConfig config;
    config.max_turns = 1;

    Harness harness(
        std::move(model), config
    );

    MockInput input({"hello"});
    MockOutput output;

    StopReason reason = harness.run(input, output);

    assert(
        reason.kind == StopReason::Kind::TurnLimit
    );

    std::remove(path.c_str());
}



// test 11: harness stops on sentinel
// verify the harness stops on the sentinel without printing it
void test_harness_sentinel(){

    const std::string path = "test_sentinel.script";

    {
        std::ofstream file(path);

        file << "chunk: 3\n";
        file << "role: assistant\n";
        file << "Goodbye!<|end_conversation|>\n";
    }

    auto model = std::make_unique<ScriptedModelClient>(path);

    HarnessConfig config;
    config.max_turns = 5;

    Harness harness(std::move(model), config);

    MockInput input({"bye"});
    MockOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::Sentinel);
    
    // sentinel should stop harness but never be printed
    assert(output.output_.find("Goodbye!") != std::string::npos);
    assert(output.output_.find("<|end_conversation|>") == std::string::npos);

    std::remove(path.c_str());
}



// test 12: transcript round trip
// verify that a saved transcript can be replayed with the expected contents
void test_transcript_round_trip(){

    const std::string path = "test_transcript.txt";

    Conversation original;

    original.append(
        Message(Role::System, "Be concise.")
    );

    original.append(
        Message(Role::User, "hello")
    );

    original.append(
        Message(Role::Assistant, "Hello!")
    );

    original.append(
        Message(Role::User, "bye")
    );

    original.append(
        Message(
            Role::Assistant,
            "Goodbye!<|end_conversation|>"
        )
    );

    write_transcript(original, path);

    ReplayModelClient replay(path);

    assert(
        replay.system_message()
        == "Be concise."
    );

    Conversation dummy;

    Message first =
        replay.generate(dummy);

    Message second =
        replay.generate(dummy);

    assert(
        first.content()
        == "Hello!"
    );

    assert(
        second.content()
        == "Goodbye!<|end_conversation|>"
    );

    std::remove(path.c_str());
}



// run all tests --------------------------------------------------------------
int main(){

    test_empty_conversation();
    test_system_message_ordering();
    test_copy_semantics();
    test_move_semantics();
    test_growth();

    test_scanner_clean_text();
    test_scanner_every_split();
    test_scanner_false_alarm();
    test_scanner_bounded_memory();

    test_harness_turn_limit();
    test_harness_sentinel();
    test_transcript_round_trip();

    return 0;
}
