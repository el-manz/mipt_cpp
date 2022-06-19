#include <iostream>
#include <vector>
#include <memory>
#include <list>


template<size_t N>
class StackStorage {
public:
    uint8_t data[N];
    uint8_t *last = data;

    StackStorage() = default;

    StackStorage(const StackStorage<N> &other) = delete;

    StackStorage &operator=(const StackStorage &other) = delete;

    StackStorage(StackStorage &&other) : data(std::move(other.data)), last(std::move(other.last)) {}

    StackStorage &operator=(StackStorage &&other) {
        data = std::move(other.data);
        last = std::move(other.last);
        return *this;
    }

};

template<typename T, size_t N>
class StackAllocator {
public:

    StackAllocator() : storage(nullptr) {}

    StackAllocator(StackStorage<N> &_storage) {
        storage = &(_storage);
    }

    StackAllocator(const StackAllocator<T, N> &other) {
        storage = other.storage;
    }

    template<typename U>
    StackAllocator(const StackAllocator<U, N> &other) {
        storage = other.storage;
    }

    StackAllocator(StackAllocator<T, N> &&other) {
        storage = std::move(other.storage);
    }

    template<typename U>
    StackAllocator(StackAllocator<U, N> &&other) {
        storage = std::move(other.storage);
    }

    StackAllocator &operator=(const StackAllocator<T, N> &other) {
        storage = other.storage;
        return *this;
    }

    template<typename U>
    StackAllocator &operator=(const StackAllocator<U, N> &other) {
        storage = other.storage;
        return *this;
    }

    template<typename U>
    StackAllocator &operator=(StackAllocator<U, N> &&other) {
        storage = std::move(other.storage);
        return *this;
    }

    ~StackAllocator() = default;

    T *allocate(size_t n) const {

        auto begin_ptr = storage->data;
        auto last_ptr = storage->last;
        int remainder = (last_ptr - begin_ptr) % sizeof(T);
        if (remainder) {
            storage->last += (sizeof(T) - remainder);
        }

        void *ptr = storage->last;
        storage->last += n * sizeof(T);
        return static_cast<T *>(ptr);
    }

    void deallocate(T *, size_t) {
    }

    template<typename... Args>
    void construct(T *ptr, const Args &... args) {
        new(ptr) T(args...);
    }

    void destroy(T *ptr) {
        ptr->~T();
    }

    template<class U>
    struct rebind {
        typedef StackAllocator<U, N> other;
    };

    bool operator==(const StackAllocator &) const {
        return true;
    }

    bool operator!=(const StackAllocator &) const {
        return false;
    }

    using value_type = T;
    using pointer = T *;

public:

    StackStorage<N> *storage = nullptr;

};

template<typename T, typename Alloc = std::allocator<T>>
class List {

public:
    struct BaseNode;
    struct Node;

private:

    BaseNode *fake_node;

    size_t sz;
    using AllocTraits = std::allocator_traits<Alloc>;
    typename AllocTraits::template rebind_alloc<Node> node_alloc = Alloc();
    typename AllocTraits::template rebind_alloc<BaseNode> basenode_alloc = Alloc();
    using node_alloc_type = typename AllocTraits::template rebind_alloc<Node>;
    using basenode_alloc_type = typename AllocTraits::template rebind_alloc<BaseNode>;

public:

    List() : sz(0) {
        fake_node = basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(basenode_alloc, fake_node, fake_node, fake_node);
    }

    List(const List &other) : sz(0),
                              node_alloc(std::allocator_traits<Alloc>::select_on_container_copy_construction(other.node_alloc)),
                              basenode_alloc(
                                      std::allocator_traits<Alloc>::select_on_container_copy_construction(other.basenode_alloc)) {

        fake_node = basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(basenode_alloc, fake_node, fake_node, fake_node);
        auto to_copy = other.begin();
        try {
            for (size_t i = 0; i < other.sz; ++i) {
                emplace(end(), *to_copy);
                to_copy++;
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    void swap(List &other) {
        std::swap(fake_node, other.fake_node);
        std::swap(node_alloc, other.node_alloc);
        std::swap(basenode_alloc, other.basenode_alloc);
        std::swap(sz, other.sz);
    }

    List& operator=(const List& other) {
        List temp(std::allocator_traits<Alloc>::propagate_on_container_copy_assignment::value ? other.node_alloc : node_alloc);
        auto to_copy = other.begin();
        for (size_t i = 0; i < other.sz; ++i) {
            temp.emplace(temp.end(), *to_copy);
            to_copy++;
        }
        swap(temp);
        return *this;
    }

    explicit List(const Alloc &_alloc) : sz(0), node_alloc(_alloc), basenode_alloc(_alloc) {
        fake_node = basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(basenode_alloc, fake_node, fake_node, fake_node);
    }

    List(size_t _sz, const Alloc &_alloc) : sz(0), node_alloc(_alloc), basenode_alloc(_alloc) {
        fake_node = basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(basenode_alloc, fake_node, fake_node, fake_node);
        try {
            for (size_t i = 0; i < _sz; ++i) {
                emplace(end());
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    explicit List(size_t _sz) : sz(0) {
        fake_node = basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(basenode_alloc, fake_node, fake_node, fake_node);
        try {
            for (size_t i = 0; i < _sz; ++i) {
                emplace(end());
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    List(size_t _sz, const T &value, const Alloc &_alloc) : sz(0), node_alloc(_alloc), basenode_alloc(_alloc) {
        fake_node = basenode_alloc.allocate(1);
        std::allocator_traits<basenode_alloc_type>::construct(basenode_alloc, fake_node, fake_node, fake_node);
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
        size_t save_sz = sz;
        for (size_t i = 0; i < save_sz; ++i) {
            pop_back();
        }
        std::allocator_traits<basenode_alloc_type>::destroy(basenode_alloc, fake_node);
        basenode_alloc.deallocate(fake_node, 1);
    }

    void clear() {
        size_t save_sz = sz;
        for (size_t i = 0; i < save_sz; ++i) {
            pop_back();
        }
        std::allocator_traits<basenode_alloc_type>::destroy(basenode_alloc, fake_node);
        basenode_alloc.deallocate(fake_node, 1);
    }

    size_t size() const {
        return sz;
    }

    node_alloc_type get_allocator() const {
        return node_alloc;
    }

    void push_back(const T &value) {
        Node *add = node_alloc.allocate(1);
        try {
            std::allocator_traits<node_alloc_type>::construct(node_alloc, add, value);
        } catch (...) {
            node_alloc.deallocate(add, 1);
            throw;
        }
        BaseNode *last_node = fake_node->previous; // последняя значащая node
        // обновляем для add
        add->previous = last_node;
        add->next = fake_node;
        // обновляем для last_node и fake_node
        last_node->next = add;
        fake_node->previous = add;
        ++sz;
    }

    void push_front(const T &value) {
        Node *add = node_alloc.allocate(1);
        try {
            std::allocator_traits<node_alloc_type>::construct(node_alloc, add, value);
        } catch (...) {
            node_alloc.deallocate(add, 1);
            throw;
        }
        BaseNode *first_node = fake_node->next; // первая значащая node
        // обновляем для add
        add->previous = fake_node;
        add->next = first_node;
        // обновляем для first_node и fake_node
        first_node->previous = add;
        fake_node->next = add;
        ++sz;
    }

    void pop_back() {
        Node *last_node = static_cast<Node *>(fake_node->previous);
        Node *penult_node = static_cast<Node *>(last_node->previous);
        std::allocator_traits<node_alloc_type>::destroy(node_alloc, last_node);
        node_alloc.deallocate(last_node, 1);
        // обновляем для penult_node
        penult_node->next = fake_node;
        // обновляем для fake_node
        fake_node->previous = penult_node;
        --sz;
    }

    void pop_front() {
        Node *first_node = static_cast<Node *>(fake_node->next);
        Node *second_node = static_cast<Node *>(first_node->next);
        std::allocator_traits<node_alloc_type>::destroy(node_alloc, first_node);
        node_alloc.deallocate(first_node, 1);
        // обновляем для second_node
        second_node->previous = fake_node;
        // обновляем для fake_node
        fake_node->next = second_node;
        --sz;
    }

    struct BaseNode {

        BaseNode() = default;

        BaseNode(BaseNode *next, BaseNode *previous) : next(next), previous(previous) {}

        ~BaseNode() = default;

        BaseNode *next = nullptr;
        BaseNode *previous = nullptr;
    };

    struct Node : BaseNode {

        T value;

        T &get_value() {
            return value;
        }

        Node() = default;

        explicit Node(const T &_value) : value(_value) {}

        explicit Node(T &&_value) : value(_value) {}

    };

    template<bool is_const>
    struct base_iterator {

        operator base_iterator<true>() const {
            return base_iterator<true>(position);
        }

        using difference_type = std::ptrdiff_t;
        using value_type = typename std::conditional<is_const, const T, T>::type;
        using pointer = typename std::conditional<is_const, const T *, T *>::type;
        using reference = typename std::conditional<is_const, const T &, T &>::type;
        using iterator_category = std::bidirectional_iterator_tag;

        base_iterator(BaseNode *pos) : position(pos) {}

        base_iterator(BaseNode *pos, List<T> *container) : position(pos), container(container) {}

        BaseNode *position;
        List<T> *container;

        base_iterator &operator++() {
            position = position->next;
            return *this;
        }

        base_iterator operator++(int) {
            base_iterator update = *this;
            ++(*this);
            return update;
        }

        base_iterator &operator--() {
            position = position->previous;
            return *this;
        }

        base_iterator operator--(int) {
            base_iterator update = *this;
            --(*this);
            return update;
        }

        base_iterator &operator+=(const size_t delta) {
            for (size_t i = 0; i < delta; ++i) {
                position = position->next;
            }
            return *this;
        }

        base_iterator &operator-=(const size_t delta) {
            for (size_t i = 0; i < delta; ++i) {
                position = position->previous;
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

        bool operator==(const base_iterator &other) const {
            return position == other.position && container == other.container;
        }

        bool operator!=(const base_iterator &other) const {
            return !(*this == other);
        }

        reference operator*() {
            return static_cast<Node *>(position)->get_value();
        }

        pointer operator->() {
            return &(static_cast<Node *>(position)->get_value());
        }

    };

    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() {
        return (reinterpret_cast<Node *>(fake_node))->next;
    }

    const_iterator begin() const {
        return (reinterpret_cast<Node *>(fake_node))->next;
    }

    iterator end() {
        return fake_node;
    }

    const_iterator end() const {
        return fake_node;
    }

    const_iterator cbegin() const {
        return (reinterpret_cast<Node *>(fake_node))->next;
    }

    const_iterator cend() const {
        return fake_node;
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
    base_iterator<is_const> insert_node(base_iterator<is_const> pos, Node *add) {
        BaseNode *before_node = pos.position->previous; // первая значащая node
        // обновляем для add
        add->previous = before_node;
        add->next = pos.position;
        // обновляем для before_node и pos.position
        pos.position->previous = add;
        before_node->next = add;
        ++sz;
        return (pos - 1);
    }

    template<bool is_const>
    base_iterator<is_const> insert(base_iterator<is_const> pos, const T &value) {
        Node *add = node_alloc.allocate(1);
        try {
            std::allocator_traits<node_alloc_type>::construct(node_alloc, add, value);
        } catch (...) {
            node_alloc.deallocate(add, 1);
            throw;
        }
        BaseNode *before_node = pos.position->previous; // первая значащая node
        // обновляем для add
        add->previous = before_node;
        add->next = pos.position;
        // обновляем для before_node и pos.position
        pos.position->previous = add;
        before_node->next = add;
        ++sz;
        return (pos - 1);
    }

    template<bool is_const>
    base_iterator<is_const> erase(base_iterator<is_const> pos) {
        BaseNode *before_node = pos.position->previous;
        BaseNode *after_node = pos.position->next;
        // обновляем для before_node
        before_node->next = after_node;
        // обновляем для after_node
        after_node->previous = before_node;
        --sz;
        return after_node;
    }

    template<typename... Args>
    void emplace(const_iterator it, Args &&... args) {
        Node *add = node_alloc.allocate(1);
        try {
            std::allocator_traits<node_alloc_type>::construct(node_alloc, add, std::forward<Args>(args)...);
        } catch (...) {
            node_alloc.deallocate(add, 1);
            throw;
        }
        insert_node(it, add);
    }


};

