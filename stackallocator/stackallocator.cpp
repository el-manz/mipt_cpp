#include <iostream>
#include <vector>
#include <memory>
#include <list>


template<size_t N>
class StackStorage {
public:
    uint8_t data[N];
    uint8_t* last = data;

    StackStorage() = default;

    StackStorage(const StackStorage<N>& other) = delete;

    StackStorage& operator=(const StackStorage& other) = delete;

    StackStorage(StackStorage&& other) : data(std::move(other.data)), last(std::move(other.last)) {}

    StackStorage& operator=(StackStorage &&other) {
        data = std::move(other.data);
        last = std::move(other.last);
        return *this;
    }

};

template<typename T, size_t N>
class StackAllocator {
public:

    StackAllocator() : _storage(nullptr) {}

    StackAllocator(StackStorage<N>& _storage) {
        _storage = &(_storage);
    }

    StackAllocator(const StackAllocator<T, N>& other) {
        _storage = other._storage;
    }

    template<typename U>
    StackAllocator(const StackAllocator<U, N>& other) {
        _storage = other._storage;
    }

    StackAllocator(StackAllocator<T, N>&& other) {
        _storage = std::move(other._storage);
    }

    template<typename U>
    StackAllocator(StackAllocator<U, N>&& other) {
        _storage = std::move(other._storage);
    }

    StackAllocator& operator=(const StackAllocator<T, N>& other) {
        _storage = other._storage;
        return *this;
    }

    template<typename U>
    StackAllocator& operator=(const StackAllocator<U, N>& other) {
        _storage = other._storage;
        return *this;
    }

    template<typename U>
    StackAllocator& operator=(StackAllocator<U, N> &&other) {
        _storage = std::move(other._storage);
        return *this;
    }

    ~StackAllocator() = default;

    T* allocate(size_t n) const {

        auto begin_ptr = _storage->data;
        auto last_ptr = _storage->last;
        int remainder = (last_ptr - begin_ptr) % sizeof(T);
        if (remainder) {
            _storage->last += (sizeof(T) - remainder);
        }

        void *ptr = _storage->last;
        _storage->last += n * sizeof(T);
        return static_cast<T *>(ptr);
    }

    void deallocate(T*, size_t) {
    }

    template<typename... Args>
    void construct(T* ptr, const Args&... args) {
        new(ptr) T(args...);
    }

    void destroy(T* ptr) {
        ptr->~T();
    }

    template<class U>
    struct rebind {
        typedef StackAllocator<U, N> other;
    };

    bool operator==(const StackAllocator&) const {
        return true;
    }

    bool operator!=(const StackAllocator&) const {
        return false;
    }

    using value_type = T;
    using pointer = T*;

private:

    StackStorage<N>* _storage = nullptr;

};

template<typename T, typename Alloc = std::allocator<T>>
class List {

private:

    struct BaseNode;
    struct Node;
    BaseNode* _fake_node;

    size_t _sz;
    using AllocTraits = std::allocator_traits<Alloc>;
    typename AllocTraits::template rebind_alloc<Node> _node_alloc = Alloc();
    typename AllocTraits::template rebind_alloc<BaseNode> _basenode_alloc = Alloc();
    using node_alloc_type = typename AllocTraits::template rebind_alloc<Node>;
    using basenode_alloc_type = typename AllocTraits::template rebind_alloc<BaseNode>;

public:

    List() : _sz(0) {
        _fake_node = _basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(_basenode_alloc, _fake_node, _fake_node, _fake_node);
    }

    List(const List& other) : _sz(0),
                              _node_alloc(std::allocator_traits<Alloc>::select_on_container_copy_construction(other._node_alloc)),
                              _basenode_alloc(
                                      std::allocator_traits<Alloc>::select_on_container_copy_construction(other._basenode_alloc)) {

        _fake_node = _basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(_basenode_alloc, _fake_node, _fake_node, _fake_node);
        auto to_copy = other.begin();
        try {
            for (size_t i = 0; i < other._sz; ++i) {
                emplace(end(), *to_copy);
                to_copy++;
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    void swap(List& other) {
        std::swap(_fake_node, other._fake_node);
        std::swap(_node_alloc, other._node_alloc);
        std::swap(_basenode_alloc, other._basenode_alloc);
        std::swap(_sz, other._sz);
    }

    List& operator=(const List& other) {
        List temp(std::allocator_traits<Alloc>::propagate_on_container_copy_assignment::value ? other._node_alloc : _node_alloc);
        auto to_copy = other.begin();
        for (size_t i = 0; i < other._sz; ++i) {
            temp.emplace(temp.end(), *to_copy);
            to_copy++;
        }
        swap(temp);
        return *this;
    }

    explicit List(const Alloc& _alloc) : _sz(0), _node_alloc(_alloc), _basenode_alloc(_alloc) {
        _fake_node = _basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(_basenode_alloc, _fake_node, _fake_node, _fake_node);
    }

    List(size_t _sz, const Alloc& _alloc) : _sz(0), _node_alloc(_alloc), _basenode_alloc(_alloc) {
        _fake_node = _basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(_basenode_alloc, _fake_node, _fake_node, _fake_node);
        try {
            for (size_t i = 0; i < _sz; ++i) {
                emplace(end());
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    explicit List(size_t _sz) : _sz(0) {
        _fake_node = _basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(_basenode_alloc, _fake_node, _fake_node, _fake_node);
        try {
            for (size_t i = 0; i < _sz; ++i) {
                emplace(end());
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    List(size_t _sz, const T& value, const Alloc& _alloc) : _sz(0), _node_alloc(_alloc), _basenode_alloc(_alloc) {
        _fake_node = _basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(_basenode_alloc, _fake_node, _fake_node, _fake_node);
        try {
            for (size_t i = 0; i < _sz; ++i) {
                emplace(end(), value);
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    ~List() {
        size_t save_sz = _sz;
        for (size_t i = 0; i < save_sz; ++i) {
            pop_back();
        }
        std::allocator_traits<basenode_alloc_type>::destroy(_basenode_alloc, _fake_node);
        _basenode_alloc.deallocate(_fake_node, 1);
    }

    void clear() {
        size_t save_sz = _sz;
        for (size_t i = 0; i < save_sz; ++i) {
            pop_back();
        }
        std::allocator_traits<basenode_alloc_type>::destroy(_basenode_alloc, _fake_node);
        _basenode_alloc.deallocate(_fake_node, 1);
    }

    size_t size() const {
        return _sz;
    }

    node_alloc_type get_allocator() const {
        return _node_alloc;
    }

    basenode_alloc_type get_base_allocator() const {
        return _basenode_alloc;
    }

    void push_back(const T& value) {
        Node *add = _node_alloc.allocate(1);
        try {
            std::allocator_traits<node_alloc_type>::construct(_node_alloc, add, value);
        } catch (...) {
            _node_alloc.deallocate(add, 1);
            throw;
        }
        BaseNode *last_node = _fake_node->_previous; // последняя значащая node
        // обновляем для add
        add->_previous = last_node;
        add->_next = _fake_node;
        // обновляем для last_node и _fake_node
        last_node->_next = add;
        _fake_node->_previous = add;
        ++_sz;
    }

    void push_front(const T& value) {
        Node *add = _node_alloc.allocate(1);
        try {
            std::allocator_traits<node_alloc_type>::construct(_node_alloc, add, value);
        } catch (...) {
            _node_alloc.deallocate(add, 1);
            throw;
        }
        BaseNode *first_node = _fake_node->_next; // первая значащая node
        // обновляем для add
        add->_previous = _fake_node;
        add->_next = first_node;
        // обновляем для first_node и _fake_node
        first_node->_previous = add;
        _fake_node->_next = add;
        ++_sz;
    }

    void pop_back() {
        Node *last_node = static_cast<Node *>(_fake_node->_previous);
        Node *penult_node = static_cast<Node *>(last_node->_previous);
        std::allocator_traits<node_alloc_type>::destroy(_node_alloc, last_node);
        _node_alloc.deallocate(last_node, 1);
        // обновляем для penult_node
        penult_node->_next = _fake_node;
        // обновляем для _fake_node
        _fake_node->_previous = penult_node;
        --_sz;
    }

    void pop_front() {
        Node *first_node = static_cast<Node *>(_fake_node->_next);
        Node *second_node = static_cast<Node *>(first_node->_next);
        std::allocator_traits<node_alloc_type>::destroy(_node_alloc, first_node);
        _node_alloc.deallocate(first_node, 1);
        // обновляем для second_node
        second_node->_previous = _fake_node;
        // обновляем для _fake_node
        _fake_node->_next = second_node;
        --_sz;
    }

private:

    struct BaseNode {

        BaseNode() = default;

        BaseNode(BaseNode *next, BaseNode *previous) : _next(next), _previous(previous) {}

        ~BaseNode() = default;

        BaseNode *_next = nullptr;
        BaseNode *_previous = nullptr;
    };

    struct Node : BaseNode {

        T _value;

        T& get_value() {
            return _value;
        }

        Node() = default;

        explicit Node(const T& _value) : _value(_value) {}

        explicit Node(T&& _value) : _value(_value) {}

    };

    template<bool is_const>
    struct base_iterator {

        operator base_iterator<true>() const {
            return base_iterator<true>(_position);
        }

        using difference_type = std::ptrdiff_t;
        using value_type = typename std::conditional<is_const, const T, T>::type;
        using pointer = typename std::conditional<is_const, const T *, T *>::type;
        using reference = typename std::conditional<is_const, const T &, T &>::type;
        using iterator_category = std::bidirectional_iterator_tag;

        base_iterator(BaseNode* pos) : _position(pos) {}

        base_iterator(BaseNode* pos, List<T>* container) : _position(pos), _container(container) {}

        BaseNode* _position;
        List<T>* _container;

        base_iterator& operator++() {
            _position = _position->_next;
            return *this;
        }

        base_iterator operator++(int) {
            base_iterator update = *this;
            ++(*this);
            return update;
        }

        base_iterator& operator--() {
            _position = _position->_previous;
            return *this;
        }

        base_iterator operator--(int) {
            base_iterator update = *this;
            --(*this);
            return update;
        }

        base_iterator& operator+=(const size_t delta) {
            for (size_t i = 0; i < delta; ++i) {
                _position = _position->_next;
            }
            return *this;
        }

        base_iterator& operator-=(const size_t delta) {
            for (size_t i = 0; i < delta; ++i) {
                _position = _position->_previous;
            }
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

        bool operator==(const base_iterator& other) const {
            return _position == other._position && _container == other._container;
        }

        bool operator!=(const base_iterator& other) const {
            return !(*this == other);
        }

        reference operator*() {
            return static_cast<Node*>(_position)->get_value();
        }

        pointer operator->() {
            return &(static_cast<Node*>(_position)->get_value());
        }

    };

    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() {
        return (reinterpret_cast<Node*>(_fake_node))->_next;
    }

    const_iterator begin() const {
        return (reinterpret_cast<Node*>(_fake_node))->_next;
    }

    iterator end() {
        return _fake_node;
    }

    const_iterator end() const {
        return _fake_node;
    }

    const_iterator cbegin() const {
        return (reinterpret_cast<Node*>(_fake_node))->_next;
    }

    const_iterator cend() const {
        return _fake_node;
    }

    reverse_iterator rbegin() {
        return std::make_reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const {
        return std::make_reverse_iterator(end());
    }

    reverse_iterator rend() {
        return std::make_reverse_iterator(begin());
    }

    const_reverse_iterator rend() const {
        return std::make_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const {
        return std::make_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const {
        return std::make_reverse_iterator(cbegin());
    }

    template<bool is_const>
    base_iterator<is_const> insert_node(base_iterator<is_const> pos, Node* add) {
        BaseNode* before_node = pos._position->_previous; // первая значащая node
        // обновляем для add
        add->_previous = before_node;
        add->_next = pos._position;
        // обновляем для before_node и pos._position
        pos._position->_previous = add;
        before_node->_next = add;
        ++_sz;
        return (pos - 1);
    }

    template<bool is_const>
    base_iterator<is_const> insert(base_iterator<is_const> pos, const T& value) {
        Node* add = _node_alloc.allocate(1);
        try {
            std::allocator_traits<node_alloc_type>::construct(_node_alloc, add, value);
        } catch (...) {
            _node_alloc.deallocate(add, 1);
            throw;
        }
        BaseNode* before_node = pos._position->_previous; // первая значащая node
        // обновляем для add
        add->_previous = before_node;
        add->_next = pos._position;
        // обновляем для before_node и pos._position
        pos._position->_previous = add;
        before_node->_next = add;
        ++_sz;
        return (pos - 1);
    }

    template<bool is_const>
    base_iterator<is_const> erase(base_iterator<is_const> pos) {
        BaseNode* before_node = pos._position->_previous;
        BaseNode* after_node = pos._position->_next;
        // обновляем для before_node
        before_node->_next = after_node;
        // обновляем для after_node
        after_node->_previous = before_node;
        --_sz;
        return after_node;
    }

    template<typename... Args>
    void emplace(const_iterator it, Args&&... args) {
        Node* add = _node_alloc.allocate(1);
        try {
            std::allocator_traits<node_alloc_type>::construct(_node_alloc, add, std::forward<Args>(args)...);
        } catch (...) {
            _node_alloc.deallocate(add, 1);
            throw;
        }
        insert_node(it, add);
    }

};
