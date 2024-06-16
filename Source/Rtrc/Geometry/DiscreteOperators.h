#pragma once

#include <Eigen/Eigen>

#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Geometry/FlatHalfedgeMesh.h>

RTRC_BEGIN

// Build a |V|-dimension vector containing the area of faces incident to each vertex
template<typename Scalar>
Eigen::VectorX<Scalar> BuildVertexAreaVector(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildVertexAreaDiagonalMatrix(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

enum class LaplacianBoundaryConditions
{
    zeroNeumann,
    zeroDirichlet
};

// Build a |V|x|V| matrix encoding the cotan operator.
// In default this gives the zero-Neumann boundary conditions.
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildCotanLaplacianMatrix(
    const FlatHalfedgeMesh     &mesh,
    Span<Vector3<Scalar>>       positions,
    LaplacianBoundaryConditions boundaryConditions = LaplacianBoundaryConditions::zeroNeumann);

RTRC_END
