#include <Rtrc/Core/PrivateBackdoor.h>
#include <Rtrc/Core/Serialization/SerializeEigen.h>

RTRC_BEGIN

namespace SerializeEigenDetail
{

    void WriteBytes(std::vector<unsigned char> &stream, const void *data, size_t bytes)
    {
        const size_t offset = stream.size();
        stream.resize(offset + bytes);
        std::memcpy(stream.data() + offset, data, bytes);
    }

    void ReadBytes(Span<unsigned char> &stream, void *data, size_t bytes)
    {
        assert(stream.size() >= bytes);
        std::memcpy(data, stream.GetData(), bytes);
        stream = stream.GetSubSpan(bytes);
    }

    template<typename T>
    void Write(std::vector<unsigned char> &stream, const T &value)
    {
        static_assert(std::is_trivially_copy_constructible_v<T>);
        WriteBytes(stream, &value, sizeof(value));
    }

    template<typename T>
    T Read(Span<unsigned char> &stream)
    {
        static_assert(std::is_trivially_copy_constructible_v<T>);
        T ret;
        ReadBytes(stream, &ret, sizeof(ret));
        return ret;
    }

    template<typename T>
    struct SparseMatrixElement
    {
        uint32_t row;
        uint32_t col;
        T val;
    };

    using SimplicialLDLT           = Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>>;
    using SimplicialLDLTBase       = Eigen::SimplicialCholeskyBase<SimplicialLDLT>;
    using SimplicialLDLTSolverBase = Eigen::SparseSolverBase<SimplicialLDLT>;

    using PermutationMatrix = Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic, SimplicialLDLT::StorageIndex>;

#define RTRC_DEFINE_SIMPLICIAL_LDLT_BASE_BACKDOOR(TYPE, NAME) \
    RTRC_DEFINE_PRIVATE_BACKDOOR(                             \
        Access_##NAME,                                        \
        SimplicialLDLTSolverBase,                             \
        TYPE,                                                 \
        &SimplicialLDLTSolverBase::NAME)

#define RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(TYPE, NAME) \
    RTRC_DEFINE_PRIVATE_BACKDOOR(                        \
        Access_##NAME,                                   \
        SimplicialLDLTBase,                              \
        TYPE,                                            \
        &SimplicialLDLTBase::NAME)

    RTRC_DEFINE_SIMPLICIAL_LDLT_BASE_BACKDOOR(bool, m_isInitialized);

    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(Eigen::ComputationInfo,         m_info);
    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(bool,                           m_factorizationIsOk);
    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(bool,                           m_analysisIsOk);
    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(SimplicialLDLT::CholMatrixType, m_matrix);
    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(SimplicialLDLT::VectorType,     m_diag);
    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(SimplicialLDLT::VectorI,        m_parent);
    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(SimplicialLDLT::VectorI,        m_nonZerosPerCol);
    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(PermutationMatrix,              m_P);
    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(PermutationMatrix,              m_Pinv);
    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(SimplicialLDLT::RealScalar,     m_shiftOffset);
    RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR(SimplicialLDLT::RealScalar,     m_shiftScale);

#undef RTRC_DEFINE_SIMPLICIAL_LDLT_BASE_BACKDOOR
#undef RTRC_DEFINE_SIMPLICIAL_LDLT_BACKDOOR

    template<typename T>
    void WriteSparseMatrixImpl(const T &m, std::vector<unsigned char> &stream)
    {
        using Scalar = typename T::Scalar;

        std::vector<SparseMatrixElement<Scalar>> records;

        for(int k = 0; k < m.outerSize(); ++k)
        {
            for(typename T::InnerIterator it(m, k); it; ++it)
            {
                static_assert(std::is_same_v<std::remove_cvref_t<decltype(it.value())>, Scalar>);

                const uint32_t row = it.row();
                const uint32_t col = it.col();
                const Scalar value = it.value();
                if(value != Scalar(0))
                {
                    records.push_back({ row, col, value });
                }
            }
        }

        const uint32_t rows = m.rows();
        const uint32_t cols = m.cols();

        Write(stream, rows);
        Write(stream, cols);
        Write(stream, static_cast<uint64_t>(records.size()));
        WriteBytes(stream, records.data(), sizeof(SparseMatrixElement<Scalar>) * records.size());
    }

    template<typename T>
    T ReadSparseMatrixImpl(Span<unsigned char> &stream)
    {
        using Scalar = typename T::Scalar;

        const uint32_t rows = Read<uint32_t>(stream);
        const uint32_t cols = Read<uint32_t>(stream);
        const uint64_t elems = Read<uint64_t>(stream);

        std::vector<SparseMatrixElement<Scalar>> records(elems);
        ReadBytes(stream, records.data(), elems * sizeof(SparseMatrixElement<Scalar>));

        std::vector<Eigen::Triplet<Scalar>> triplets;
        triplets.reserve(elems);
        for(auto &record : records)
        {
            triplets.emplace_back(record.row, record.col, record.val);
        }

        Eigen::SparseMatrix<Scalar> ret(rows, cols);
        ret.setFromTriplets(triplets.begin(), triplets.end());
        return ret;
    }

    template<typename T>
    void WriteDenseMatrixImpl(const T &m, std::vector<unsigned char> &stream)
    {
        Write(stream, static_cast<uint32_t>(m.rows()));
        Write(stream, static_cast<uint32_t>(m.cols()));
        if(m.size())
        {
            WriteBytes(stream, m.data(), m.size() * sizeof(typename T::Scalar));
        }
    }

    template<typename T>
    T ReadDenseMatrixImpl(Span<unsigned char> &stream)
    {
        const uint32_t rows = Read<uint32_t>(stream);
        const uint32_t cols = Read<uint32_t>(stream);
        T ret;
        ret.resize(rows, cols);
        if(ret.size())
        {
            ReadBytes(stream, ret.data(), sizeof(typename T::Scalar) * ret.size());
        }
        return ret;
    }

} // namespace SerializeEigenDetail

void WriteSparseMatrixToByteStream(const Eigen::SparseMatrix<double> &m, std::vector<unsigned char> &stream)
{
    SerializeEigenDetail::WriteSparseMatrixImpl(m, stream);
}

Eigen::SparseMatrix<double> ReadSparseMatrixFromByteStream(Span<unsigned char> &stream)
{
    return SerializeEigenDetail::ReadSparseMatrixImpl<Eigen::SparseMatrix<double>>(stream);
}

void WriteSimplicialLDLTToByteStream(
    const Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> &solver, std::vector<unsigned char> &stream)
{
    using namespace SerializeEigenDetail;

    auto &m_isInitialized     = Access_m_isInitialized    (solver);
    auto &m_info              = Access_m_info             (solver);
    auto &m_factorizationIsOk = Access_m_factorizationIsOk(solver);
    auto &m_analysisIsOk      = Access_m_analysisIsOk     (solver);
    auto &m_matrix            = Access_m_matrix           (solver);
    auto &m_diag              = Access_m_diag             (solver);
    auto &m_parent            = Access_m_parent           (solver);
    auto &m_nonZerosPerCol    = Access_m_nonZerosPerCol   (solver);
    auto &m_P                 = Access_m_P                (solver);
    auto &m_Pinv              = Access_m_Pinv             (solver);
    auto &m_shiftOffset       = Access_m_shiftOffset      (solver);
    auto &m_shiftScale        = Access_m_shiftScale       (solver);

    Write(stream, m_isInitialized);
    Write(stream, m_info);
    Write(stream, m_factorizationIsOk);
    Write(stream, m_analysisIsOk);

    WriteSparseMatrixImpl(m_matrix, stream);

    WriteDenseMatrixImpl(m_diag, stream);
    WriteDenseMatrixImpl(m_parent, stream);
    WriteDenseMatrixImpl(m_nonZerosPerCol, stream);

    WriteDenseMatrixImpl(m_P.indices(), stream);
    WriteDenseMatrixImpl(m_Pinv.indices(), stream);

    Write(stream, m_shiftOffset);
    Write(stream, m_shiftScale);
}

void ReadSimplicialLDLTFromByteStream(
    Span<unsigned char> &stream, Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> &solver)
{
    using namespace SerializeEigenDetail;

    auto &m_isInitialized     = Access_m_isInitialized    (solver);
    auto &m_info              = Access_m_info             (solver);
    auto &m_factorizationIsOk = Access_m_factorizationIsOk(solver);
    auto &m_analysisIsOk      = Access_m_analysisIsOk     (solver);
    auto &m_matrix            = Access_m_matrix           (solver);
    auto &m_diag              = Access_m_diag             (solver);
    auto &m_parent            = Access_m_parent           (solver);
    auto &m_nonZerosPerCol    = Access_m_nonZerosPerCol   (solver);
    auto &m_P                 = Access_m_P                (solver);
    auto &m_Pinv              = Access_m_Pinv             (solver);
    auto &m_shiftOffset       = Access_m_shiftOffset      (solver);
    auto &m_shiftScale        = Access_m_shiftScale       (solver);

#define RTRC_TYPE(V) std::remove_cvref_t<decltype(V)>

    m_isInitialized     = Read<RTRC_TYPE(m_isInitialized)>(stream);
    m_info              = Read<RTRC_TYPE(m_info)>(stream);
    m_factorizationIsOk = Read<RTRC_TYPE(m_factorizationIsOk)>(stream);
    m_analysisIsOk      = Read<RTRC_TYPE(m_analysisIsOk)>(stream);

    m_matrix = ReadSparseMatrixImpl<RTRC_TYPE(m_matrix)>(stream);

    m_diag           = ReadDenseMatrixImpl<RTRC_TYPE(m_diag)>(stream);
    m_parent         = ReadDenseMatrixImpl<RTRC_TYPE(m_parent)>(stream);
    m_nonZerosPerCol = ReadDenseMatrixImpl<RTRC_TYPE(m_nonZerosPerCol)>(stream);

    m_P.indices()    = ReadDenseMatrixImpl<RTRC_TYPE(m_P.indices())>(stream);
    m_Pinv.indices() = ReadDenseMatrixImpl<RTRC_TYPE(m_Pinv.indices())>(stream);

    m_shiftOffset = Read<RTRC_TYPE(m_shiftOffset)>(stream);
    m_shiftScale  = Read<RTRC_TYPE(m_shiftScale)>(stream);

#undef RTRC_TYPE
}

RTRC_END
