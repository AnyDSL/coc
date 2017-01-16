#include "thorin/util/bitset.h"

namespace thorin {

void BitSet::dealloc() const {
    if (num_words_ != 1)
        delete[] words_;
}

size_t BitSet::count() const {
    size_t result = 0;
    auto w = words();
    for (size_t i = 0, e = num_words(); i != e; ++i)
        result += bitcount(w[i]);
    return result;
}

bool BitSet::any() const {
    bool result = false;
    auto w = words();
    for (size_t i = 0, e = num_words(); !result && i != e; ++i)
        result |= w[i] & uint64_t(-1);
    return result;
}

bool BitSet::any_range(const size_t begin, const size_t end) const {
    // TODO optimize
    bool result = false;
    for (size_t i = begin; !result && i != end; ++i)
        result |= test(i);
    return result;
}

bool BitSet::none() const {
    bool result = true;
    auto w = words();
    for (size_t i = 0, e = num_words(); result && i != e; ++i)
        result &= w[i] == uint64_t(0);
    return result;
}

bool BitSet::none_range(const size_t begin, const size_t end) const {
    // TODO optimize
    bool result = true;
    for (size_t i = begin; result && i != end; ++i)
        result &= ~test(i);
    return result;
}

BitSet& BitSet::operator>>=(uint64_t shift) {
    uint64_t div = shift/uint64_t(64);
    uint64_t rem = shift%uint64_t(64);
    auto w = words();

    for (size_t i = 0, e = num_words()-div; i != e; ++i)
        w[i] = w[i+div];
    std::fill(w+num_words()-div, w+num_words(), 0);

    uint64_t carry = 0;
    for (size_t i = num_words()-div; i-- != 0;) {
        uint64_t new_carry = w[i] << (uint64_t(64)-rem);
        w[i] = (w[i] >> rem) | carry;
        carry = new_carry;
    }

    return *this;
}

#define CODE(op)                                        \
BitSet& BitSet::operator op ## =(const BitSet& other) { \
    if (this->num_words() < other.num_words())          \
        this->enlarge(other.num_bits()-1);              \
    else if (other.num_words() < this->num_words())     \
        other.enlarge(this->num_bits()-1);              \
    auto  this_words = this->words();                   \
    auto other_words = other.words();                   \
    for (size_t i = 0, e = num_words(); i != e; ++i)    \
        this_words[i] op ## = other_words[i];           \
    return *this;                                       \
}
THORIN_BITSET_OPS(CODE)
#undef CODE

void BitSet::enlarge(size_t i) const {
    size_t num_new_words = (i+size_t(64)) / size_t(64);
    if (num_new_words > num_words_) {
        num_new_words = round_to_power_of_2(num_new_words);
        assert(num_new_words >= num_words_ * size_t(2)
                && "num_new_words must be a power of two at least twice of num_words_");
        uint64_t* new_words = new uint64_t[num_new_words];

        // copy over and fill rest with zero
        std::fill(std::copy_n(words(), num_words_, new_words), new_words + num_new_words, 0);

        // record new num_words and words_ pointer
        dealloc();
        num_words_ = num_new_words;
        words_ = new_words;
    }
}

}