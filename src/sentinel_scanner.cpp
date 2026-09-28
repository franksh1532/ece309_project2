#include "core/sentinel_scanner.h"

SentinelScanner::SentinelScanner(std::string sentinel): sentinel_(sentinel), pending_(""){}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk){


    // combine the pending data with the new chunk
    std::string combined = pending_ + std::string(chunk);

    // look for the sentinel in the combined data
    std::size_t pos = combined.find(sentinel_);

    // if the sentinel is found, return the safe data and clear the pending data
    if (pos != std::string::npos){
        std::string safe = combined.substr(0, pos); // everything before sentinel is safe to print

        pending_.clear();

        return {safe, true};
    }

    // if the sentinel is not found, determine how much data is safe to return
    std::size_t hold_count = sentinel_.size() - 1;

    // if combined data is short enough, keep it all in pending_
    if (combined.size() <= hold_count){
        pending_ = combined;
        return {"", false};
    }

    std::size_t safe_count = combined.size() - hold_count;

    std::string safe = combined.substr(0, safe_count);

    // keep trailing chars
    pending_ = combined.substr(safe_count);

    return {safe, false};
}


SentinelScanner::Out SentinelScanner::flush(){
    
    std::string safe = pending_;
    pending_.clear();
    return {safe, false};
}
