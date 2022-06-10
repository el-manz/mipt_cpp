#include <iostream>
#include <vector>

template<typename T>
class Deque {
public:

    Deque() : left ((block_number - 1) / 2, 0, this),
              right ((block_number - 1) / 2, 0, this), buffer(block_number) {
        for (size_t i = 0; i < block_number; ++i) {
            buffer[i] = reinterpret_cast<T*>(new uint8_t[block_size * sizeof(T)]);
        }
    }

    explicit Deque(int length) : block_number((length / block_size + 1) * 2),
                        left((block_number - 1) / 2, 0, this),
                        right((block_number - 1) / 2, 0, this),
                        buffer(block_number) {
        for (size_t i = 0; i < block_number; ++i) {
            buffer[i] = reinterpret_cast<T*>(new uint8_t[block_size * sizeof(T)]);
        }
        right += length;
    }

    Deque(int length, const T& value) : block_number((length / block_size + 1) * 2),
                                        left((block_number - 1) / 2, 0, this),
                                        right((block_number - 1) / 2, 0, this),
                                        buffer(block_number) {
        for (size_t i = 0; i < block_number; ++i) {
            buffer[i] = reinterpret_cast<T*>(new uint8_t[block_size * sizeof(T)]);
        }
        for (size_t i = 0; i < static_cast<size_t>(length); ++i) {
            auto current = right;
            new(buffer[current.block] + current.pos) T(value);
            ++right;
        }
    }

    Deque(const Deque& other) : block_number(other.size()),
                                left((block_number - 1) / 2, 0, this),
                                right((block_number - 1) / 2, 0, this),
                                buffer(block_number) {
        for (size_t i = 0; i < block_number; ++i) {
            buffer[i] = reinterpret_cast<T*>(new uint8_t[block_size * sizeof(T)]);
        }
        for (size_t i = 0; i < static_cast<size_t>(other.size()); ++i) {
            auto current = right;
            new(buffer[current.block] + current.pos) T(other.operator[](i));
            ++right;
        }
    }

    Deque& operator=(Deque other) {
        swap(other);
        return *this;
    }

    void swap(Deque& other) {
        std::swap(block_number, other.block_number);
        std::swap(left, other.left);
        std::swap(right, other.right);
        std::swap(buffer, other.buffer);
    }

    size_t size() const {
        if (right.block == left.block) {
            return right.pos - left.pos;
        }
        return (right.block - left.block - 1) * block_size + block_size - left.pos + right.pos;
    }

    T& operator[](const size_t index) {
        base_iterator<false> get_position = left;
        get_position += index;
        return *(buffer[get_position.block] + get_position.pos);
    }

    const T& operator[](const size_t index) const {
        base_iterator<false> get_position = left;
        get_position += index;
        return *(buffer[get_position.block] + get_position.pos);
    }

    T& at(const size_t index) {
        if (index >= size() || index < 0) {
            throw std::out_of_range("Out of range");
        }
        return Deque<T>::operator[](index);
    }

    const T& at(const size_t index) const {
        if (index >= size() || index < 0) {
            throw std::out_of_range("Out of range");
        }
        return Deque<T>::operator[](index);
    }

    void push_back(const T& value) {
        if (right.pos == 0 && right.block == block_number) { // если блоки заполнены
            size_t add_size = right.block - left.block + 1;
            for (size_t i = 0; i < add_size; ++i) {
                buffer.insert(buffer.end(), 1, reinterpret_cast<T*>(new uint8_t[block_size * sizeof(T)]));
            }
            block_number += add_size;
        }

        new(buffer[right.block] + right.pos) T(value);
        ++right;
    }

    void push_front(const T& value) {
        if (left.pos == 0 && left.block == 0) {
            size_t add_size = right.block - left.block + 1;
            for (size_t i = 0; i < add_size; ++i) {
                buffer.insert(buffer.begin(), 1, reinterpret_cast<T*>(new uint8_t[block_size * sizeof(T)]));
            }
            block_number += add_size;
            left.block += add_size;
            right.block += add_size;
        }

        --left;
        new(buffer[left.block] + left.pos) T(value);
    }

    void pop_back() {
        --right;
        buffer[right.block][right.pos].~T();
    }

    void pop_front() {
        buffer[left.block][left.pos].~T();
        ++left;
    }


    template<bool is_const>
    struct base_iterator {

        operator base_iterator<true>() const {
            return base_iterator<true>(block, pos);
        }

        using difference_type = std::ptrdiff_t;
        using value_type = typename std::conditional<is_const, const T, T>::type;
        using pointer = typename std::conditional<is_const, const T*, T*>::type;
        using reference = typename std::conditional<is_const, const T&, T&>::type;
        using iterator_category = std::bidirectional_iterator_tag;

        base_iterator(size_t block, size_t pos) : block(block), pos(pos) {}
        base_iterator(size_t block, size_t pos, Deque<T>* container) : block(block), pos(pos),
                                                                        container(container) {}
        size_t block;
        size_t pos;
        Deque<T>* container;

        base_iterator& operator++() {
            if (pos == block_size - 1) {
                ++block;
                pos = 0;
                return *this;
            }
            ++pos;
            return *this;
        }

        base_iterator& operator--() {
            if (pos == 0) {
                --block;
                pos = block_size - 1;
                return *this;
            }
            --pos;
            return *this;
        }

        base_iterator& operator+=(const size_t delta) {
            block = (block * block_size + pos + delta) / block_size;
            pos = (pos + delta) % block_size;
            return *this;
        }
        base_iterator& operator-=(const size_t delta) {
            *this += (-delta);
            return *this;
        }
        base_iterator operator+(const size_t delta) const {
            base_iterator update = *this;
            update += delta;
            return update;
        }
        base_iterator operator-(const size_t delta) const {
            base_iterator update = *this;
            update -= delta;
            return update;
        }

        bool operator<(const base_iterator& other) const {
            if (block < other.block || (block == other.block && pos < other.pos)) {
                return true;
            } else {
                return false;
            }
        }
        bool operator>(const base_iterator& other) const {
            return other < *this;
        }
        bool operator<=(const base_iterator& other) const {
            return !(*this > other);
        }
        bool operator>=(const base_iterator& other) const {
            return !(*this < other);
        }
        bool operator==(const base_iterator& other) const {
            return *this <= other && *this >= other && container == other.container;
        }
        bool operator!=(const base_iterator& other) const {
            return !(*this == other);
        }

        difference_type operator-(const base_iterator& other) const {
            if (block == other.block) {
                return pos - other.pos;
            }
            return (block - other.block - 1) * block_size + block_size - other.pos + pos;
        }

        reference operator*() {
            return *((container->buffer)[block] + pos);
        }
        pointer operator->() {
            return ((container->buffer)[block] + pos);
        }

    };

    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() {
        return left;
    }

    const_iterator begin() const {
        return left;
    }

    iterator end() {
        return right;
    }

    const_iterator end() const {
        return right;
    }

    const_iterator cbegin() const {
        return left;
    }

    const_iterator cend() const {
        return right;
    }

    reverse_iterator rbegin() {
        return std::make_reverse_iterator(begin());
    }

    const_reverse_iterator rbegin() const {
        return std::make_reverse_iterator(begin());
    }

    reverse_iterator rend() {
        return std::make_reverse_iterator(end());
    }

    const_reverse_iterator rend() const {
        return std::make_reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const {
        return std::make_reverse_iterator(cbegin());
    }

    const_reverse_iterator crend() const {
        return std::make_reverse_iterator(cend());
    }

    void insert(iterator target, const T& value) {
        if (target == begin()) {
            push_front(value);
            return;
        }
        push_back(*(end() - 1));
        for (iterator shift = end() - 2; shift > target; --shift) {
            *shift = *(shift - 1);
        }
        *target = value;
    }

    void erase(iterator target) {
        if (target == begin()) {
            pop_front();
            return;
        }
        for (iterator shift = target + 1; shift < end(); ++shift) {
            *(shift - 1) = *shift;
        }
        pop_back();
    }

private:
    size_t block_number = 10;
    static const size_t block_size = 32;
    base_iterator<false> left;
    base_iterator<false> right;
    std::vector<T*> buffer{block_number};
};

