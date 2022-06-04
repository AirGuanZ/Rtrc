#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <set>
#include <tuple>

#include <Rtrc/Utils/Uncopyable.h>

RTRC_BEGIN

template<typename Event>
class ReceiverSet;

template<typename Event>
class Receiver;

template<typename Event>
struct ReceiverLess
{
    using is_transparent = int;

    bool operator()(const std::shared_ptr<Receiver<Event>> &l, const std::shared_ptr<Receiver<Event>> &r) const;

    bool operator()(const std::shared_ptr<Receiver<Event>> &l, const Receiver<Event> *r) const;

    bool operator()(const Receiver<Event> *l, const std::shared_ptr<Receiver<Event>> &r) const;

    bool operator()(const Receiver<Event> *l, const Receiver<Event> *r) const;
};

template<typename Event>
class Receiver : public std::enable_shared_from_this<Receiver<Event>>, public Uncopyable
{
public:

    virtual ~Receiver();

    virtual void Handle(const Event &e) = 0;

private:

    friend class ReceiverSet<Event>;

    mutable std::set<ReceiverSet<Event> *> containedSets_;
    mutable std::set<ReceiverSet<Event> *> containedSetsWithOwnership_;
};

template<typename Event>
class ReceiverSet : public Uncopyable
{
public:

    ~ReceiverSet();

    void Send(const Event &e) const;

    void Attach(Receiver<Event> *handler);

    void Attach(std::shared_ptr<Receiver<Event>> handlerWithOwnership);

    void Detach(const Receiver<Event> *handler);

    void Detach(const std::shared_ptr<Receiver<Event>> &handler);

    void Detach(const std::shared_ptr<const Receiver<Event>> &handler);

    void DetachAll();

private:

    std::set<Receiver<Event> *, ReceiverLess<Event>> handlers_;
    std::set<std::shared_ptr<Receiver<Event>>, ReceiverLess<Event>> handlersWithOwnership_;
};

template<typename...Events>
class Sender : public Uncopyable
{
public:

    template<typename Event>
    void Send(const Event &e) const;

    template<typename Event>
    void Attach(Receiver<Event> *handler);

    template<typename Event>
    void Attach(std::shared_ptr<Receiver<Event>> handlerWithOwnership);

    template<typename Event>
    void Detach(const Receiver<Event> *handler);

    template<typename Event>
    void Detach(const std::shared_ptr<Receiver<Event>> &handler);

    template<typename Event>
    void Detach(const std::shared_ptr<const Receiver<Event>> &handler);

    template<typename Event>
    void DetachAll();

    void DetachAllTypes();

private:

    std::tuple<ReceiverSet<Events>...> receiverSets_;
};

template<typename Event>
class FunctionalReceiver : public Receiver<Event>
{
public:

    using Function = std::function<void(const Event &)>;

    explicit FunctionalReceiver(std::function<void()> f);

    explicit FunctionalReceiver(Function f = {});

    void SetFunction(std::function<void()> f);

    void SetFunction(Function f);

    void Handle(const Event &e) override;

private:

    Function f_;
};

template<typename Event, typename Class>
class ClassReceiver : public Receiver<Event>
{
public:

    using MemberFunctionPointer = void(Class::*)(const Event &);

    ClassReceiver();

    ClassReceiver(Class *c, MemberFunctionPointer f);

    void SetClassInstance(Class *c);

    void SetMemberFunction(MemberFunctionPointer f);

    void Handle(const Event &e) override;

private:

    Class c_;
    MemberFunctionPointer f_;
};

#define RTRC_DECLARE_EVENT_SENDER(Event)                                \
    void Attach(Rtrc::Receiver<Event> *handler);                        \
    void Attach(std::shared_ptr<Rtrc::Receiver<Event>> handler);        \
    void Attach(std::function<void(const Event &)> f);                  \
    void Detach(Rtrc::Receiver<Event> *handler);                        \
    void Detach(const std::shared_ptr<Rtrc::Receiver<Event>> &handler); \
    void Detach(const std::shared_ptr<const Rtrc::Receiver<Event>> &handler);

#define RTRC_DEFINE_EVENT_SENDER(Class, Sender, Event)                                             \
    void Class::Attach(Rtrc::Receiver<Event> *handler)                                             \
        { Sender.Attach<Event>(handler); }                                                         \
    void Class::Attach(std::shared_ptr<Rtrc::Receiver<Event>> handler)                             \
        { Sender.Attach<Event>(std::move(handler)); }                                              \
    void Class::Attach(std::function<void(const Event &)> f)                                       \
        { Sender.Attach<Event>(std::make_shared<Rtrc::FunctionalReceiver<Event>>(std::move(f))); } \
    void Class::Detach(Rtrc::Receiver<Event> *handler)                                             \
        { Sender.Detach<Event>(handler); }                                                         \
    void Class::Detach(const std::shared_ptr<Rtrc::Receiver<Event>> &handler)                      \
        { Sender.Detach<Event>(handler); }                                                         \
    void Class::Detach(const std::shared_ptr<const Rtrc::Receiver<Event>> &handler)                \
        { Sender.Detach<Event>(handler); }

template<typename Event>
bool ReceiverLess<Event>::operator()(const Receiver<Event> *l, const Receiver<Event> *r) const
{
    return l < r;
}

template<typename Event>
bool ReceiverLess<Event>::operator()(const Receiver<Event> *l, const std::shared_ptr<Receiver<Event>> &r) const
{
    return l < r.get();
}

template<typename Event>
bool ReceiverLess<Event>::operator()(const std::shared_ptr<Receiver<Event>> &l, const Receiver<Event> *r) const
{
    return l.get() < r;
}

template<typename Event>
bool ReceiverLess<Event>::operator()(
    const std::shared_ptr<Receiver<Event>> &l, const std::shared_ptr<Receiver<Event>> &r) const
{
    return l < r;
}

template<typename Event>
Receiver<Event>::~Receiver()
{
    while(!containedSets_.empty())
    {
        (*containedSets_.begin())->Detach(this);
    }
}

template<typename Event>
ReceiverSet<Event>::~ReceiverSet()
{
    DetachAll();
}

template<typename Event>
void ReceiverSet<Event>::Send(const Event &e) const
{
    for(auto h : handlers_)
    {
        h->Handle(e);
    }
    for(auto &h : handlersWithOwnership_)
    {
        h->Handle(e);
    }
}

template<typename Event>
void ReceiverSet<Event>::Attach(Receiver<Event> *handler)
{
    assert(handler);
    handler->containedSets_.insert(this);
    handlers_.insert(handler);
}

template<typename Event>
void ReceiverSet<Event>::Attach(std::shared_ptr<Receiver<Event>> handlerWithOwnership)
{
    assert(handlerWithOwnership);
    handlerWithOwnership->containedSetsWithOwnership_.insert(this);
    handlersWithOwnership_.insert(std::move(handlerWithOwnership));
}

template<typename Event>
void ReceiverSet<Event>::Detach(const Receiver<Event> *handler)
{
    assert(handler);
    assert(handler->containedSets_.contains(this) || handler->containedSetsWithOwnership_.contains(this));
    assert(handlers_.contains(handler) || handlersWithOwnership_.contains(handler));
    handler->containedSets_.erase(this);
    handler->containedSetsWithOwnership_.erase(this);
    handlers_.erase(handler);
    handlersWithOwnership_.erase(handlersWithOwnership_.find(handler));
}

template<typename Event>
void ReceiverSet<Event>::Detach(const std::shared_ptr<Receiver<Event>> &handler)
{
    this->Detach(handler.get());
}

template<typename Event>
void ReceiverSet<Event>::Detach(const std::shared_ptr<const Receiver<Event>> &handler)
{
    this->Detach(handler.get());
}

template<typename Event>
void ReceiverSet<Event>::DetachAll()
{
    while(!handlers_.empty())
    {
        this->Detach(*handlers_.begin());
    }
    while(!handlersWithOwnership_.empty())
    {
        this->Detach(*handlersWithOwnership_.begin());
    }
}

template<typename...Events>
template<typename Event>
void Sender<Events...>::Send(const Event &e) const
{
    std::get<ReceiverSet<Event>>(receiverSets_).Send(e);
}

template<typename...Events>
template<typename Event>
void Sender<Events...>::Attach(Receiver<Event> *handler)
{
    std::get<ReceiverSet<Event>>(receiverSets_).Attach(handler);
}

template<typename...Events>
template<typename Event>
void Sender<Events...>::Attach(std::shared_ptr<Receiver<Event>> handlerWithOwnership)
{
    std::get<ReceiverSet<Event>>(receiverSets_).Attach(std::move(handlerWithOwnership));
}

template<typename...Events>
template<typename Event>
void Sender<Events...>::Detach(const Receiver<Event> *handler)
{
    std::get<ReceiverSet<Event>>(receiverSets_).Detach(handler);
}

template<typename...Events>
template<typename Event>
void Sender<Events...>::Detach(const std::shared_ptr<Receiver<Event>> &handler)
{
    std::get<ReceiverSet<Event>>(receiverSets_).Detach(handler);
}

template<typename...Events>
template<typename Event>
void Sender<Events...>::Detach(const std::shared_ptr<const Receiver<Event>> &handler)
{
    std::get<ReceiverSet<Event>>(receiverSets_).Detach(handler);
}

template<typename...Events>
template<typename Event>
void Sender<Events...>::DetachAll()
{
    std::get<ReceiverSet<Event>>(receiverSets_).DetachAll();
}

template<typename...Events>
void Sender<Events...>::DetachAllTypes()
{
    ((std::get<ReceiverSet<Events>>(receiverSets_).DetachAll()), ...);
}

template<typename Event>
FunctionalReceiver<Event>::FunctionalReceiver(std::function<void()> f)
{
    SetFunction(std::move(f));
}

template<typename Event>
FunctionalReceiver<Event>::FunctionalReceiver(Function f)
    : f_(std::move(f))
{
    
}

template<typename Event>
void FunctionalReceiver<Event>::SetFunction(std::function<void()> f)
{
    if(f)
    {
        f_ = [nf = std::move(f)](const Event &){ nf(); };
    }
    else
    {
        f_ = {};
    }
}

template<typename Event>
void FunctionalReceiver<Event>::SetFunction(Function f)
{
    f_ = std::move(f);
}

template<typename Event>
void FunctionalReceiver<Event>::Handle(const Event &e)
{
    if(f_)
    {
        f_(e);
    }
}

template<typename Event, typename Class>
ClassReceiver<Event, Class>::ClassReceiver()
    : ClassReceiver({}, {})
{
    
}

template<typename Event, typename Class>
ClassReceiver<Event, Class>::ClassReceiver(Class *c, MemberFunctionPointer f)
    : c_(c), f_(f)
{
    
}

template<typename Event, typename Class>
void ClassReceiver<Event, Class>::SetClassInstance(Class *c)
{
    c_ = c;
}

template<typename Event, typename Class>
void ClassReceiver<Event, Class>::SetMemberFunction(MemberFunctionPointer f)
{
    f_ = f;
}

template<typename Event, typename Class>
void ClassReceiver<Event, Class>::Handle(const Event &e)
{
    if(c_ && f_)
    {
        (c_->*f_)(e);
    }
}

RTRC_END
