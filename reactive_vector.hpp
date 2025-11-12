#ifndef REACTIVE_VECTOR_HPP
#define REACTIVE_VECTOR_HPP

#include <vector>
#include <optional>
#include <stdexcept>
#include <utility>

#include "sbpt_generated_includes.hpp"

//
// Signals for Vector modification events
//
template <typename T> struct VectorSignals {
    // Fired when an element is added
    struct Inserted {
        std::size_t index;
        const T &value;
    };

    // Fired when an existing element changes
    struct Updated {
        std::size_t index;
        const T &old_value;
        const T &new_value;
    };

    // Fired when an element is removed
    struct Erased {
        std::size_t index;
        const T &old_value;
    };

    // Fired when the vector is cleared
    struct Cleared {};

    // Fired when vector capacity changes
    struct Reserved {
        std::size_t new_capacity;
    };

    // Fired when vector is resized
    struct Resized {
        std::size_t old_size;
        std::size_t new_size;
    };
};

/**
 * @class ReactiveVector
 * @brief A reactive wrapper around std::vector that emits signals on modification.
 *
 * This class behaves like a standard vector but emits signals when elements are
 * inserted, updated, erased, or cleared.
 *
 * @example
 * ReactiveVector<int> vec;
 * vec.signal_emitter.connect<ReactiveVector<int>::inserted_signal>(
 *     [](const auto &sig){ std::cout << "Inserted at " << sig.index << ": " << sig.value << "\n"; });
 * vec.push_back(42); // emits an Inserted signal
 */
template <typename T> class ReactiveVector {
  public:
    using value_type = T;
    using size_type = std::size_t;
    using vector_type = std::vector<T>;
    using iterator = typename vector_type::iterator;
    using const_iterator = typename vector_type::const_iterator;

    using vector_signals = VectorSignals<T>;
    using inserted_signal = vector_signals::Inserted;
    using updated_signal = vector_signals::Updated;
    using erased_signal = vector_signals::Erased;
    using cleared_signal = vector_signals::Cleared;
    using reserved_signal = vector_signals::Reserved;
    using resized_signal = vector_signals::Resized;

    SignalEmitter signal_emitter;

    ReactiveVector() = default;
    explicit ReactiveVector(std::size_t n, const T &value = T()) : vec_(n, value) {}
    explicit ReactiveVector(std::initializer_list<T> init) : vec_(init) {}

    bool empty() const noexcept { return vec_.empty(); }
    size_type size() const noexcept { return vec_.size(); }
    size_type capacity() const noexcept { return vec_.capacity(); }

    const T &operator[](size_type i) const { return vec_[i]; }
    T &operator[](size_type i) { return vec_[i]; }

    const T &at(size_type i) const { return vec_.at(i); }
    T &at(size_type i) { return vec_.at(i); }

    iterator begin() noexcept { return vec_.begin(); }
    iterator end() noexcept { return vec_.end(); }
    const_iterator begin() const noexcept { return vec_.begin(); }
    const_iterator end() const noexcept { return vec_.end(); }

    void push_back(const T &value) {
        vec_.push_back(value);
        signal_emitter.emit(inserted_signal{vec_.size() - 1, value});
    }

    void push_back(T &&value) {
        vec_.push_back(std::move(value));
        signal_emitter.emit(inserted_signal{vec_.size() - 1, vec_.back()});
    }

    template <typename... Args> T &emplace_back(Args &&...args) {
        vec_.emplace_back(std::forward<Args>(args)...);
        signal_emitter.emit(inserted_signal{vec_.size() - 1, vec_.back()});
        return vec_.back();
    }

    void pop_back() {
        if (vec_.empty())
            return;
        size_type index = vec_.size() - 1;
        T old_value = std::move(vec_.back());
        vec_.pop_back();
        signal_emitter.emit(erased_signal{index, old_value});
    }

    void clear() noexcept {
        if (!vec_.empty()) {
            vec_.clear();
            signal_emitter.emit(cleared_signal{});
        }
    }

    void reserve(size_type new_cap) {
        size_type old_cap = vec_.capacity();
        vec_.reserve(new_cap);
        if (vec_.capacity() != old_cap)
            signal_emitter.emit(reserved_signal{vec_.capacity()});
    }

    void resize(size_type new_size, const T &value = T()) {
        size_type old_size = vec_.size();
        vec_.resize(new_size, value);
        if (new_size != old_size)
            signal_emitter.emit(resized_signal{old_size, new_size});
    }

    bool update_if_exists(size_type index, T new_value) {
        if (index >= vec_.size())
            return false;
        T old_value = vec_[index];
        vec_[index] = std::move(new_value);
        signal_emitter.emit(updated_signal{index, old_value, vec_[index]});
        return true;
    }

    iterator erase(iterator pos) {
        size_type index = std::distance(vec_.begin(), pos);
        T old_value = *pos;
        auto it = vec_.erase(pos);
        signal_emitter.emit(erased_signal{index, old_value});
        return it;
    }

  private:
    vector_type vec_;
};

#endif // REACTIVE_VECTOR_HPP
