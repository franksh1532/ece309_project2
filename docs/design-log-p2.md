# Design Log — Project 2

## Growth factor and amortized cost
A doubling growth strategy was used for the dynamic array for the `Conversation` class. The conversation begins with `capacity_ = 0`, becoming `capacity_ = 1` when the first message is appended. Whenever `size_ == capacity_` the capacity doubles, which gives the sequence 0 -> 1 -> 2 -> 4 -> 8 -> 16 -> etc.
When the array is full, `append()` allocates a new `Message` array with the larger capacity, copying the existing messages and deleting the old array. It then updates `data_` and `capacity_`. If there's already unused capacity, the new message is placed directly at `data_[size_]`. Afterwards, `size_` is incremented.
The benefit of doubling is that resizing doesn't happen on every insertion. A resize at capacity 1 copies 1 element, then 2, then 4, and so on. Over `n` appends, the total number of copied elements is approximately `< 2n`. An individual append that causes a resize will take `O(n)` time, with the total resizing across `n` appends being `O(n)`. Dividing this across all `n` append operations would give an amortized cost of `O(1)` per append. As such, I went with multiplicative growth rather than increasing the capacity by only one each time.



## Rule of Five evidence
`Conversation` owns a dynamically allocated array of `Message` objects, so implementing the Rule of Five to manage memory is required.

The destructor calls `delete[] data_` to release the owned array. Since deleting `nullptr` is safe, the destructor also works for empty or moved from conversations.

The copy constructor performs a deep copy, allocating a new array using the other conversation's capacity and copying each valid message. The copied conversation has the same contents but a different `data_` pointer.

The copy assignment operator also creates a separate array and copies the messages before deleting the destination object's old array. A self assignment check (such as `conv = conv`) is used to prevent unnecessary work.

The move constructor avoids copying messages, instead taking the other conversation's `data_`, `size_`, and `capacity_`. The moved from object is then reset to:
`data_ = nullptr;`
`size_ = 0;`
`capacity_ = 0;`

The move assignment operator works similarly, except it first deletes the destination object's current array before taking ownership of the source object's resources. These move operations transfer ownership of the existing allocation rather than duplicating it.


## Sentinel scanner: bounded pending_ proof
`SentinelScanner` must detect the sentinel even when it is divided across chunks. `feed()` combines the previous `pending_` string with the newly received chunk and searches the combined string for the complete sentinel. If the sentinel is found, all text before the sentinel is returned (as safe text), `sentinel_found` is set to true, and the pending buffer is cleared.

If the sentinel is not found, the scanner keeps at most the final `sentinel_.size() - 1` characters in `pending_`. Everything before those characters is returned as safe text. This guarantees that `pending_` does not grow with the total streamed response size and remains bounded by the sentinel length.

If the sentinel has length `x`, an incomplete match can contain at most `x - 1` characters. If `x` characters were available, and the full sentinel was not found, then the earliest character cannot still be part of a sentinel completed by a future chunk. As such only the final `x - 1` characters need to remain in `pending_`.


## What I would change differently
The main thing I'd change would be the array resizing logic. I'd separate most of the logic into its own private helper function in an effort to make `append()` shorter. This should make things clearer in terms of code.


