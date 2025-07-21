// FixedSizeQueue.h

#ifndef FIXED_SIZE_QUEUE_H
#define FIXED_SIZE_QUEUE_H

#include <stdexcept>
#include <cstddef> // For size_t

template <typename T>
class lpndeque {
public:
    lpndeque(size_t max_size, std::string id)
        : max_size(max_size), data(new T[max_size]), place_id(id),front_index(0), back_index(0), count(0) {}

    void push_back(const T& item) {
        if (count >= max_size) {
            throw std::overflow_error("FixedSizeQueue is full " + place_id);
        }
        data[back_index] = item;
        back_index = (back_index + 1) % max_size;
        ++count;
    }

    void pop_front() {
        if (count == 0) {
            throw std::underflow_error("FixedSizeQueue is empty");
        }
        front_index = (front_index + 1) % max_size;
        --count;
    }

    T& front() {
        if (count == 0) {
            throw std::underflow_error("FixedSizeQueue is empty");
        }
        return data[front_index];
    }

    const T& front() const {
        if (count == 0) {
            throw std::underflow_error("FixedSizeQueue is empty");
        }
        return data[front_index];
    }

    void clear() {
        for (size_t i = 0; i < max_size; ++i) {
            data[i] = nullptr;
        }
        front_index = 0;
        back_index = 0;
        count = 0;
    }

    size_t size() const {
        return count;
    }

    bool empty() const {
        return count == 0;
    }

    T& operator[](size_t idx) {
        if (idx >= count) {
            throw std::out_of_range("Index out of range");
        }
        size_t actual_index = (front_index + idx) % max_size;
        return data[actual_index];
    }

    const T& operator[](size_t idx) const {
        if (idx >= count) {
            throw std::out_of_range("Index out of range");
        }
        size_t actual_index = (front_index + idx) % max_size;
        return data[actual_index];
    }


private:
    std::string place_id;
    size_t max_size;
    T* data;
    size_t front_index;
    size_t back_index;
    size_t count;
};

#endif // FIXED_SIZE_QUEUE_H
