#pragma once

#include <Eigen/Eigen>

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

RTRC_END
