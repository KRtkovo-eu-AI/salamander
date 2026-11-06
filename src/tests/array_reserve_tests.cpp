// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the shipped template; only realloc is replaced to inject one bounded failure.
#include <windows.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

static int ReallocCalls = 0;
static bool FailNextRealloc = false;
static void* TestRealloc(void* data, size_t bytes)
{
    ++ReallocCalls;
    if (FailNextRealloc)
    {
        FailNextRealloc = false;
        return NULL;
    }
    return std::realloc(data, bytes);
}
#define realloc TestRealloc
#include "../common/array.h"
#undef realloc

template<class T> class InspectArray : public TDirectArray<T>
{
public:
    InspectArray(int base, int delta) : TDirectArray<T>(base, delta) {}
    int Capacity() const { return this->Available; }
    int Minimum() const { return this->Base; }
    int Growth() const { return this->Delta; }
};
static int Failures = 0;
static void Check(bool condition, const char* text)
{
    printf("%s %s\n", condition ? "PASS" : "FAIL", text);
    if (!condition) ++Failures;
}
struct Tracked
{
    static int Copies;
    static int Destructions;
    int Value;
    explicit Tracked(int value) : Value(value) {}
    Tracked(const Tracked& other) : Value(other.Value) { ++Copies; }
    ~Tracked() { ++Destructions; }
};
int Tracked::Copies = 0;
int Tracked::Destructions = 0;

int main()
{
    InspectArray<int> array(4, 3);
    array.Add(10); array.Add(20); array.Add(30);
    Check(array.IsGood() && array.Count == 3 && array.Capacity() == 4,
          "ordinary constructor and insertion establish real template state");
    Check(array.Reserve(101) && array.Capacity() >= 101 && array.Count == 3 &&
          array.At(0) == 10 && array.At(1) == 20 && array.At(2) == 30,
          "reserving a populated array preserves every item and Count");
    Check(array.Minimum() == 4 && array.Growth() == 3,
          "reserve preserves Base and Delta");
    int* reserved = array.GetData();
    const int capacity = array.Capacity();
    int calls = ReallocCalls;
    Check(array.Reserve(capacity) && array.Reserve(2) && array.Reserve(0) &&
          array.GetData() == reserved && array.Capacity() == capacity && ReallocCalls == calls,
          "equal smaller and zero reservations are allocation-free no-ops");
    array.Insert(1, 15);
    const int extra[] = {40, 50};
    array.Add(extra, 2);
    Check(array.Count == 6 && array.At(0) == 10 && array.At(1) == 15 &&
          array.At(2) == 20 && array.At(3) == 30 && array.At(4) == 40 && array.At(5) == 50 &&
          array.GetData() == reserved && ReallocCalls == calls,
          "insertion and bulk append use reserved storage without reallocating");
    array.DestroyMembers();
    Check(array.Count == 0 && array.Capacity() == 4 && array.Minimum() == 4 && array.Growth() == 3,
          "DestroyMembers returns populated reserved storage to Base");
    for (int i = 0; i < 5; ++i) array.Add(i);
    Check(array.Count == 5 && array.Capacity() == 7 && array.At(4) == 4,
          "ordinary growth still uses the original Delta after cleanup");

    InspectArray<int> empty(4, 3);
    Check(empty.Reserve(25) && empty.Count == 0, "empty array can reserve without constructing items");
    empty.DestroyMembers();
    Check(empty.IsGood() && empty.Count == 0 && empty.Capacity() == 4,
          "DestroyMembers releases an empty reservation back to Base");
    empty.Reserve(25);
    empty.DetachMembers();
    Check(empty.IsGood() && empty.Count == 0 && empty.Capacity() == 4,
          "DetachMembers releases an empty reservation back to Base");

    InspectArray<int> failure(4, 3);
    failure.Add(71); failure.Add(72);
    int* oldData = failure.GetData();
    const int oldCapacity = failure.Capacity();
    calls = ReallocCalls;
    FailNextRealloc = true;
    Check(!failure.Reserve(64) && !FailNextRealloc && ReallocCalls == calls + 1 &&
          failure.State == etNone && failure.Count == 2 && failure.GetData() == oldData &&
          failure.Capacity() == oldCapacity && failure.Minimum() == 4 && failure.Growth() == 3 &&
          failure.At(0) == 71 && failure.At(1) == 72,
          "injected realloc failure preserves pointer data count state and growth policy");
    failure.Insert(1, 99);
    Check(failure.IsGood() && failure.Count == 3 && failure.At(1) == 99 && failure.Reserve(64),
          "failed reserve does not poison subsequent insert or reserve");
    calls = ReallocCalls;
    oldData = failure.GetData();
    Check(!failure.Reserve(-1) && failure.State == etNone && failure.GetData() == oldData &&
          failure.Count == 3 && ReallocCalls == calls,
          "negative capacity fails without mutation or allocation");
    failure.State = etBadInsert;
    Check(!failure.Reserve(128) && failure.State == etBadInsert && failure.GetData() == oldData &&
          failure.Count == 3 && ReallocCalls == calls,
          "reserve neither clears an existing failure nor mutates failed storage");
    failure.ResetState();

    InspectArray<int> overflow(4, 4);
    calls = ReallocCalls;
    Check(!overflow.Reserve(INT_MAX) && overflow.State == etNone && overflow.Count == 0 &&
          overflow.Capacity() == 4 && ReallocCalls == calls,
          "rounding overflow fails before attempting any gigantic allocation");

    InspectArray<int> shrinking(4, 3);
    shrinking.Reserve(11); // deliberately not Base + n*Delta
    for (int i = 0; i < 11; ++i) shrinking.Add(i);
    bool shrinkOk = true;
    while (shrinking.Count != 0)
    {
        shrinking.Delete(shrinking.Count - 1);
        shrinkOk = shrinkOk && shrinking.IsGood() && shrinking.Capacity() >= shrinking.Minimum();
        for (int i = 0; i < shrinking.Count; ++i) shrinkOk = shrinkOk && shrinking.At(i) == i;
    }
    Check(shrinkOk && shrinking.Capacity() == shrinking.Minimum() && shrinking.Growth() == 3,
          "single-item deletion after nonaligned reserve never shrinks below Base");

    InspectArray<int> destroyed(4, 3);
    destroyed.Reserve(64);
    destroyed.Destroy();
    calls = ReallocCalls;
    Check(!destroyed.Reserve(64) && destroyed.State == etDestructed && destroyed.GetData() == NULL &&
          ReallocCalls == calls, "reserve cannot revive a destroyed array");
    destroyed.DestroyMembers();
    destroyed.DetachMembers();
    Check(destroyed.State == etDestructed && destroyed.GetData() == NULL && ReallocCalls == calls,
          "empty cleanup cannot revive a destroyed reserved array");

    {
        InspectArray<Tracked> objects(2, 3);
        Tracked one(5);
        objects.Add(one);
        const int copies = Tracked::Copies;
        const int destructions = Tracked::Destructions;
        Check(objects.Reserve(40) && objects.Count == 1 && objects.At(0).Value == 5 &&
              Tracked::Copies == copies && Tracked::Destructions == destructions,
              "reserve preserves actual objects without invoking constructors or destructors");
        objects.DestroyMembers();
        Check(objects.Count == 0 && objects.Capacity() == 2 && Tracked::Destructions == destructions + 1,
              "cleanup destroys each reserved live object exactly once");
    }
    return Failures == 0 ? 0 : 1;
}
