#include <iostream>
#include <vector>

template<typename T>
class Deque {
public:

    Deque() : _left ((_block_number - 1) / 2, 0, this),
              _right ((_block_number - 1) / 2, 0, this), _buffer(_block_number) {
        for (size_t i = 0; i < _block_number; ++i) {
            _buffer[i] = reinterpret_cast<T*>(new uint8_t[_block_size * sizeof(T)]);
        }
    }

    explicit Deque(int length) : _block_number((length / _block_size + 1) * 2),
                                 _left((_block_number - 1) / 2, 0, this),
                                 _right((_block_number - 1) / 2, 0, this),
                                 _buffer(_block_number) {
        for (size_t i = 0; i < _block_number; ++i) {
            _buffer[i] = reinterpret_cast<T*>(new uint8_t[_block_size * sizeof(T)]);
        }
        _right += length;
    }

    Deque(int length, const T& value) : _block_number((length / _block_size + 1) * 2),
                                        _left((_block_number - 1) / 2, 0, this),
                                        _right((_block_number - 1) / 2, 0, this),
                                        _buffer(_block_number) {
        for (size_t i = 0; i < _block_number; ++i) {
            _buffer[i] = reinterpret_cast<T*>(new uint8_t[_block_size * sizeof(T)]);
        }
        for (size_t i = 0; i < static_cast<size_t>(length); ++i) {
            auto current = _right;
            new(_buffer[current._block] + current._pos) T(value);
            ++_right;
        }
    }

    Deque(const Deque& other) : _block_number(other.size()),
                                _left((_block_number - 1) / 2, 0, this),
                                _right((_block_number - 1) / 2, 0, this),
                                _buffer(_block_number) {
        for (size_t i = 0; i < _block_number; ++i) {
            _buffer[i] = reinterpret_cast<T*>(new uint8_t[_block_size * sizeof(T)]);
        }
        for (size_t i = 0; i < static_cast<size_t>(other.size()); ++i) {
            auto current = _right;
            new(_buffer[current._block] + current._pos) T(other.operator[](i));
            ++_right;
        }
    }

    Deque& operator=(Deque other) {
        swap(other);
        return *this;
    }

    void swap(Deque& other) {
        std::swap(_block_number, other._block_number);
        std::swap(_left, other._left);
        std::swap(_right, other._right);
        std::swap(_buffer, other._buffer);
    }

    size_t size() const {
        if (_right._block == _left._block) {
            return _right._pos - _left._pos;
        }
        return (_right._block - _left._block - 1) * _block_size + _block_size - _left._pos + _right._pos;
    }

    T& operator[](const size_t index) {
        base_iterator<false> get_position = _left;
        get_position += index;
        return *(_buffer[get_position._block] + get_position._pos);
    }

    const T& operator[](const size_t index) const {
        base_iterator<false> get_position = _left;
        get_position += index;
        return *(_buffer[get_position._block] + get_position._pos);
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
        if (_right._pos == 0 && _right._block == _block_number) { // если блоки заполнены
            size_t add_size = _right._block - _left._block + 1;
            for (size_t i = 0; i < add_size; ++i) {
                _buffer.insert(_buffer.end(), 1, reinterpret_cast<T*>(new uint8_t[_block_size * sizeof(T)]));
            }
            _block_number += add_size;
        }

        new(_buffer[_right._block] + _right._pos) T(value);
        ++_right;
    }

    void push_front(const T& value) {
        if (_left._pos == 0 && _left._block == 0) {
            size_t add_size = _right._block - _left._block + 1;
            for (size_t i = 0; i < add_size; ++i) {
                _buffer.insert(_buffer.begin(), 1, reinterpret_cast<T*>(new uint8_t[_block_size * sizeof(T)]));
            }
            _block_number += add_size;
            _left._block += add_size;
            _right._block += add_size;
        }

        --_left;
        new(_buffer[_left._block] + _left._pos) T(value);
    }

    void pop_back() {
        --_right;
        _buffer[_right._block][_right._pos].~T();
    }

    void pop_front() {
        _buffer[_left._block][_left._pos].~T();
        ++_left;
    }


    template<bool is_const>
    struct base_iterator {

        operator base_iterator<true>() const {
            return base_iterator<true>(_block, _pos);
        }

        using difference_type = std::ptrdiff_t;
        using value_type = typename std::conditional<is_const, const T, T>::type;
        using pointer = typename std::conditional<is_const, const T*, T*>::type;
        using reference = typename std::conditional<is_const, const T&, T&>::type;
        using iterator_category = std::bidirectional_iterator_tag;

        base_iterator(size_t block, size_t pos) : _block(block), _pos(pos) {}
        base_iterator(size_t block, size_t pos, Deque<T>* container) : _block(block), _pos(pos),
                                                                       _container(container) {}
        size_t _block;
        size_t _pos;
        Deque<T>* _container;

        base_iterator& operator++() {
            if (_pos == _block_size - 1) {
                ++_block;
                _pos = 0;
                return *this;
            }
            ++_pos;
            return *this;
        }

        base_iterator& operator--() {
            if (_pos == 0) {
                --_block;
                _pos = _block_size - 1;
                return *this;
            }
            --_pos;
            return *this;
        }

        base_iterator& operator+=(const size_t delta) {
            _block = (_block * _block_size + _pos + delta) / _block_size;
            _pos = (_pos + delta) % _block_size;
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
            if (_block < other._block || (_block == other._block && _pos < other._pos)) {
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
            return *this <= other && *this >= other && _container == other._container;
        }
        bool operator!=(const base_iterator& other) const {
            return !(*this == other);
        }

        difference_type operator-(const base_iterator& other) const {
            if (_block == other._block) {
                return _pos - other._pos;
            }
            return (_block - other._block - 1) * _block_size + _block_size - other._pos + _pos;
        }

        reference operator*() {
            return *((_container->_buffer)[_block] + _pos);
        }
        pointer operator->() {
            return ((_container->_buffer)[_block] + _pos);
        }

    };

    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() {
        return _left;
    }

    const_iterator begin() const {
        return _left;
    }

    iterator end() {
        return _right;
    }

    const_iterator end() const {
        return _right;
    }

    const_iterator cbegin() const {
        return _left;
    }

    const_iterator cend() const {
        return _right;
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
    size_t _block_number = 10;
    static const size_t _block_size = 32;
    base_iterator<false> _left;
    base_iterator<false> _right;
    std::vector<T*> _buffer{_block_number};
};

