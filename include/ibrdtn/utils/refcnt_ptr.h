#ifndef refcnt_ptr_h
#define refcnt_ptr_h 1

template <class T> class refcnt_ptr {
protected:
    // a helper class that holds the pointer to the managed object
    // and its reference count
    class Holder {
    public:
        Holder( T* ptr) : ptr_(ptr), count_(1) {};
        ~Holder() { delete ptr_;};

        T* ptr_;
        unsigned count_;
    };

    Holder* h_;

public:
    // ctor of refcnt_ptr (p must not be NULL)
    explicit refcnt_ptr( T* p) : h_(new Holder(p)) {}
    // dtor of refcnt_ptr
    ~refcnt_ptr() { if (--h_->count_ == 0) delete h_; }
    // copy and assignment of refcnt_ptr
    refcnt_ptr (const refcnt_ptr<T>& right) : h_(right.h_) {
        ++h_->count_;
    }
    refcnt_ptr<T>& operator= (const refcnt_ptr<T>& right) {
        ++right.h_->count_;
        if (--h_->count_ == 0) delete h_;
        h_ = right.h_;
        return *this;
    }
    refcnt_ptr<T>& operator= (refcnt_ptr<T>& right) {
        ++right.h_->count_;
        if (--h_->count_ == 0) delete h_;
        h_ = right.h_;
        return *this;
    }
    // access to the managed object
    T* operator-> () { return h_->ptr_; }
    T& operator* ()  { return *h_->ptr_; }
    T* getPointer() { return h_->ptr_; }
};
#endif
