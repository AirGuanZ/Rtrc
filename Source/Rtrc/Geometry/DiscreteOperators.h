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

// Build a |F|x|V| matrix M, encoding the gradient operator.
// M * u gives the x component of face gradient.
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildFaceGradientMatrix_X(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildFaceGradientMatrix_Y(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildFaceGradientMatrix_Z(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

// Build a |V| x 3|F| matrix M, encoding the divergence operator.
// M * u gives the divergences of faces.
// `u` should be like:
//    [x0, y0, z0, x1, y1, z1, ...]^T
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildVertexDivergenceMatrix(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

enum class CotanLaplacianBoundaryType
{
    Neumann,
    Dirichlet
};

// Build a |V|x|V| matrix encoding the cotan operator.
// Note that `boundaryConditions` can only affect the boundary type. The boundary values are always 0.
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildCotanLaplacianMatrix(
    const FlatHalfedgeMesh    &mesh,
    Span<Vector3<Scalar>>      positions,
    CotanLaplacianBoundaryType boundaryConditions);

RTRC_END
