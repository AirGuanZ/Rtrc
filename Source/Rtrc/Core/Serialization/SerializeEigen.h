#pragma once

#include <Eigen/Eigen>

#include <Rtrc/Core/Archive/Archive.h>
#include <Rtrc/Core/Base64.h>
#include <Rtrc/Core/Container/Span.h>

RTRC_BEGIN

void WriteSparseMatrixToByteStream(const Eigen::SparseMatrix<double> &m, std::vector<unsigned char> &stream);
Eigen::SparseMatrix<double> ReadSparseMatrixFromByteStream(Span<unsigned char> &stream);

void WriteSimplicialLDLTToByteStream(
    const Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> &solver,
    std::vector<unsigned char>                               &stream);
void ReadSimplicialLDLTFromByteStream(
    Span<unsigned char>                                &stream,
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> &solver);

template<>
struct ArchiveTransferTrait<Eigen::SparseMatrix<double>>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, Eigen::SparseMatrix<double> &matrix)
    {
        if(ar.IsWriting())
        {
            std::vector<unsigned char> data;
            WriteSparseMatrixToByteStream(matrix, data);
            std::string base64Str = ToBase64(data);
            ar.Transfer(name, base64Str);
        }
        else
        {
            assert(ar.IsReading());
            std::string base64Str;
            ar.Transfer(name, base64Str);
            if(ar.DidReadLastProperty())
            {
                const std::string data = FromBase64(base64Str);
                auto stream = Span(reinterpret_cast<const unsigned char *>(data.data()), data.size());
                matrix = ReadSparseMatrixFromByteStream(stream);
            }
        }
    }

    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const Eigen::SparseMatrix<double> &matrix)
    {
        static_assert(Archive::StaticIsWriting());
        std::vector<unsigned char> data;
        WriteSparseMatrixToByteStream(matrix, data);
        std::string base64Str = ToBase64(data);
        ar.Transfer(name, base64Str);
    }
};

template<>
struct ArchiveTransferTrait<Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>>>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> &solver)
    {
        if(ar.IsWriting())
        {
            std::vector<unsigned char> data;
            WriteSimplicialLDLTToByteStream(solver, data);
            std::string base64Str = ToBase64(data);
            ar.Transfer(name, base64Str);
        }
        else
        {
            assert(ar.IsReading());
            std::string base64Str;
            ar.Transfer(name, base64Str);
            if(ar.DidReadLastProperty())
            {
                const std::string data = FromBase64(base64Str);
                auto stream = Span(reinterpret_cast<const unsigned char *>(data.data()), data.size());
                ReadSimplicialLDLTFromByteStream(stream, solver);
            }
        }
    }

    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> &solver)
    {
        static_assert(Archive::StaticIsWriting());
        std::vector<unsigned char> data;
        WriteSimplicialLDLTToByteStream(solver, data);
        std::string base64Str = ToBase64(data);
        ar.Transfer(name, base64Str);
    }
};

RTRC_END
