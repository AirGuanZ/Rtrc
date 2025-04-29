#pragma once

#include <Eigen/Eigen>

#include <Rtrc/Core/Math/Vector.h>
#include <Rtrc/Geometry/HalfedgeMesh.h>

RTRC_GEO_BEGIN

// Build a |V|-dimension vector containing the area of faces incident to each vertex
template<typename Scalar>
Eigen::VectorX<Scalar> BuildVertexAreaVector(const HalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildVertexAreaDiagonalMatrix(const HalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

// Build a |F|x|V| matrix M, encoding the gradient operator.
// M * u gives the x component of face gradient.
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildFaceGradientMatrix_X(const HalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildFaceGradientMatrix_Y(const HalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildFaceGradientMatrix_Z(const HalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

// Build a |V| x 3|F| matrix M, encoding the divergence operator.
// M * u gives the divergences of faces.
// `u` should be like:
//    [x0, y0, z0, x1, y1, z1, ...]^T
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildVertexDivergenceMatrix(const HalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

// Similar to BuildVertexDivergenceMatrix, except the computed divergence is scaled by 1/boundary_length of each vertex.
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildVertexDivergenceMatrix_NormalizedByBoundaryLength(const HalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

enum class CotanLaplacianBoundaryType
{
    Neumann,
    Dirichlet
};

// Build a |V|x|V| matrix encoding the cotan operator.
// Note that `boundaryConditions` can only affect the boundary type. The boundary values are always 0.
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildCotanLaplacianMatrix(
    const HalfedgeMesh        &mesh,
    Span<Vector3<Scalar>>      positions,
    CotanLaplacianBoundaryType boundaryConditions);

// Build a |V|x|F| matrix M.
// Given a per-face value vector u, M * u transforms u into a per-vertex value vector using inner-angle based weights.
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildFaceToVertexMatrix(const HalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

RTRC_GEO_END
