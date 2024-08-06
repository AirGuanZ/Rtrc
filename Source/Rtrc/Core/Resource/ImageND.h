#pragma once

#include <array>
#include <cassert>
#include <vector>

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector2.h>
#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/Math/Vector4.h>

RTRC_BEGIN

namespace ImageNDDetail
{

    template<size_t D>
    struct Dim2SizeVec
    {
        struct Type { private: Type() { } };
        struct TypeS { private: TypeS() { } };
    };
    template<> struct Dim2SizeVec<2> { using Type = Vector2u; using TypeS = Vector3i; };
    template<> struct Dim2SizeVec<3> { using Type = Vector3u; using TypeS = Vector3i; };
    template<> struct Dim2SizeVec<4> { using Type = Vector4u; using TypeS = Vector3i; };

} // namespace ImageNDDetail

template<typename T, size_t D>
class ImageND
{
public:

    static_assert(D >= 2);

    using Texel = T;
    using TexelRef = std::conditional_t<
        std::is_same_v<T, bool>, std::vector<bool>::reference, Texel &>;
    using TexelConstRef = std::conditional_t<
        std::is_same_v<T, bool>, std::vector<bool>::const_reference, const Texel &>;

    using Dimensions = typename ImageNDDetail::Dim2SizeVec<D>::Type;
    using DimensionsS = typename ImageNDDetail::Dim2SizeVec<D>::TypeS;

    ImageND();
    explicit ImageND(const Dimensions &dims);
    explicit ImageND(const DimensionsS &dims);
    explicit ImageND(const std::array<uint32_t, D> &dims);
    template<typename...TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
    explicit ImageND(TIs...tis);

    ImageND(const ImageND &) = default;
    ImageND &operator=(const ImageND &) = default;

    ImageND(ImageND &&other) noexcept;
    ImageND &operator=(ImageND &&other) noexcept;

    void Swap(ImageND &other) noexcept;

    operator bool() const;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetDepth() const;

    uint32_t GetSize(size_t dim) const;
    const std::array<uint32_t, D> &GetSizes() const;

    TexelRef      operator()(const Dimensions &indices);
    TexelConstRef operator()(const Dimensions &indices) const;

    TexelRef      operator()(const DimensionsS &indices);
    TexelConstRef operator()(const DimensionsS &indices) const;
    
    TexelRef      operator()(const Span<uint32_t> &indices);
    TexelConstRef operator()(const Span<uint32_t> &indices) const;

    template<typename...TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
    TexelRef operator()(TIs...indices);
    template<typename...TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
    TexelConstRef operator()(TIs...indices) const;

    template<typename U>
    ImageND<U, D> To() const;
    template<typename F>
    auto Map(const F &func) const;

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }

    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    auto GetData()            { return data_.data(); }
    auto GetData() const { return data_.data(); }
    
    auto &GetStorage()       { return data_; }
    auto &GetStorage() const { return data_; }

private:

    size_t ComputeLinearIndex(const uint32_t *indices) const;

    std::array<uint32_t, D> dims_;
    std::vector<T> data_;
};

template<typename T>
using Image3D = ImageND<T, 3>;
template<typename T>
using Image4D = ImageND<T, 4>;

template <typename T, size_t D>
ImageND<T, D>::ImageND()
    : dims_{}
{
    
}

template <typename T, size_t D>
ImageND<T, D>::ImageND(const Dimensions& dims)
{
    size_t elemCount = 1;
    for(size_t i = 0; i < D; ++i)
    {
        elemCount *= dims[i];
        dims_[i] = dims[i];
    }
    data_.resize(elemCount);
}

template <typename T, size_t D>
ImageND<T, D>::ImageND(const DimensionsS& dims)
    : ImageND(dims.template To<uint32_t>())
{
    
}

template <typename T, size_t D>
ImageND<T, D>::ImageND(const std::array<uint32_t, D>& dims)
{
    size_t elemCount = 1;
    for(size_t i = 0; i < D; ++i)
    {
        elemCount *= dims[i];
        dims_[i] = dims[i];
    }
    data_.resize(elemCount);
}

template <typename T, size_t D>
template <typename... TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
ImageND<T, D>::ImageND(TIs... tis)
    : ImageND(std::array<uint32_t, D>{ static_cast<uint32_t>(tis)... })
{
    
}

template <typename T, size_t D>
ImageND<T, D>::ImageND(ImageND&& other) noexcept
    : ImageND()
{
    this->Swap(other);
}

template <typename T, size_t D>
ImageND<T, D>& ImageND<T, D>::operator=(ImageND&& other) noexcept
{
    this->Swap(other);
    return *this;
}

template <typename T, size_t D>
void ImageND<T, D>::Swap(ImageND& other) noexcept
{
    std::swap(dims_, other.dims_);
    data_.swap(other.data_);
}

template <typename T, size_t D>
ImageND<T, D>::operator bool() const
{
    return !data_.empty();
}

template <typename T, size_t D>
uint32_t ImageND<T, D>::GetWidth() const
{
    return dims_[0];
}

template <typename T, size_t D>
uint32_t ImageND<T, D>::GetHeight() const
{
    return dims_[1];
}

template <typename T, size_t D>
uint32_t ImageND<T, D>::GetDepth() const
{
    static_assert(D >= 3 || AlwaysFalse<T>);
    return dims_[2];
}

template <typename T, size_t D>
uint32_t ImageND<T, D>::GetSize(size_t dim) const
{
    return dims_[dim];
}

template <typename T, size_t D>
const std::array<uint32_t, D>& ImageND<T, D>::GetSizes() const
{
    return dims_;
}

template <typename T, size_t D>
typename ImageND<T, D>::TexelRef ImageND<T, D>::operator()(const Dimensions& indices)
{
    return data_[this->ComputeLinearIndex(&indices[0])];
}

template <typename T, size_t D>
typename ImageND<T, D>::TexelConstRef ImageND<T, D>::operator()(const Dimensions& indices) const
{
    return data_[this->ComputeLinearIndex(&indices[0])];
}

template <typename T, size_t D>
typename ImageND<T, D>::TexelRef ImageND<T, D>::operator()(const DimensionsS& indices)
{
    return this->operator()(indices.template To<uint32_t>());
}

template <typename T, size_t D>
typename ImageND<T, D>::TexelConstRef ImageND<T, D>::operator()(const DimensionsS& indices) const
{
    return this->operator()(indices.template To<uint32_t>());
}

template <typename T, size_t D>
typename ImageND<T, D>::TexelRef ImageND<T, D>::operator()(const Span<uint32_t>& indices)
{
    return data_[this->ComputeLinearIndex(&indices[0])];
}

template <typename T, size_t D>
typename ImageND<T, D>::TexelConstRef ImageND<T, D>::operator()(const Span<uint32_t>& indices) const
{
    return data_[this->ComputeLinearIndex(&indices[0])];
}

template <typename T, size_t D>
template <typename... TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
typename ImageND<T, D>::TexelRef ImageND<T, D>::operator()(TIs... indices)
{
    return this->operator()(Span<uint32_t>(std::array<uint32_t, D>{ static_cast<uint32_t>(indices)... }));
}

template <typename T, size_t D>
template <typename... TIs> requires (sizeof...(TIs) == D) && std::conjunction_v<std::is_integral<TIs>...>
typename ImageND<T, D>::TexelConstRef ImageND<T, D>::operator()(TIs... indices) const
{
    return this->operator()(Span<uint32_t>(std::array<uint32_t, D>{ static_cast<uint32_t>(indices)... }));
}

template <typename T, size_t D>
template <typename U>
ImageND<U, D> ImageND<T, D>::To() const
{
    return this->Map([](const T &v) { return static_cast<U>(v); });
}

template <typename T, size_t D>
template <typename F>
auto ImageND<T, D>::Map(const F& func) const
{
    using DstTexel = decltype(func(std::declval<Texel>()));
    if(!*this)
    {
        return ImageND<DstTexel, D>();
    }
    const size_t elemCount = 1;
    for(size_t i = 0; i < D; ++i)
    {
        elemCount *= dims_[i];
    }
    ImageND<DstTexel, D> ret(dims_);
    for(size_t i = 0; i < elemCount; ++i)
    {
        ret.GetStorage()[i] = func(this->GetStorage()[i]);
    }
    return ret;
}

template <typename T, size_t D>
size_t ImageND<T, D>::ComputeLinearIndex(const uint32_t* indices) const
{
    size_t ret = 0;
    for(int i = D - 1; i >= 0; --i)
    {
        assert(indices[i] < dims_[i]);
        ret = dims_[i] * ret + indices[i];
    }
    return ret;
}

RTRC_END
