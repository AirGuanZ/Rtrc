#pragma once

#include <Rtrc/Core/Math/Vector.h>

RTRC_BEGIN

template<typename T>
class Rect
{
public:

    Vector2<T> lower = Vector2<T>((std::numeric_limits<T>::max)());
    Vector2<T> upper = Vector2<T>(std::numeric_limits<T>::lowest());

    bool IsEmpty() const;
    Vector2<T> GetCenter() const;
    Vector2<T> GetExtent() const;
    T GetWidthOverHeight() const;
    T GetHeightOverWidth() const;

    Rect &operator|=(const Vector2<T> &p);
    Rect &operator|=(const Rect &r);
};

using Rectf = Rect<float>;
using Rectd = Rect<double>;
using Recti = Rect<int32_t>;
using Rectu = Rect<uint32_t>;

template<typename T>
Rect<T> operator|(const Rect<T> &rect, const Vector2<T> &p);
template<typename T>
Rect<T> operator|(const Vector2<T> &p, const Rect<T> &rect);
template<typename T>
Rect<T> operator|(const Rect<T> &rect1, const Rect<T> &rect2);

template <typename T>
bool Rect<T>::IsEmpty() const
{
    return lower.x >= upper.x || lower.y >= upper.y;
}

template <typename T>
Vector2<T> Rect<T>::GetCenter() const
{
    return T(0.5) * (lower + upper);
}

template <typename T>
Vector2<T> Rect<T>::GetExtent() const
{
    return upper - lower;
}

template <typename T>
T Rect<T>::GetWidthOverHeight() const
{
    const Vector2<T> size = GetExtent();
    return size.x / size.y;
}

template <typename T>
T Rect<T>::GetHeightOverWidth() const
{
    const Vector2<T> size = GetExtent();
    return size.y / size.x;
}

template <typename T>
Rect<T> &Rect<T>::operator|=(const Vector2<T> &p)
{
    *this = *this | p;
    return *this;
}

template <typename T>
Rect<T> &Rect<T>::operator|=(const Rect &r)
{
    *this = *this | r;
    return *this;
}

template<typename T>
Rect<T> operator|(const Rect<T> &rect, const Vector2<T> &p)
{
    return { Min(rect.lower, p), Max(rect.upper, p) };
}

template<typename T>
Rect<T> operator|(const Vector2<T> &p, const Rect<T> &rect)
{
    return rect | p;
}

template<typename T>
Rect<T> operator|(const Rect<T> &rect1, const Rect<T> &rect2)
{
    if(rect1.IsEmpty())
    {
        return rect2;
    }
    if(rect2.IsEmpty())
    {
        return rect1;
    }
    return { Min(rect1.lower, rect2.lower), Max(rect1.upper, rect2.upper) };
}

RTRC_END
