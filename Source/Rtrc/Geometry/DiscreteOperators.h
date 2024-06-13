#pragma once

#include <Eigen/Eigen>

#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Geometry/FlatHalfedgeMesh.h>

RTRC_BEGIN

// Build a |V|x|V| matrix encoding the cotan operator
template<typename Scalar>
Eigen::SparseMatrix<Scalar> BuildCotanLaplacianMatrix(const FlatHalfedgeMesh &mesh, Span<Vector3<Scalar>> positions);

RTRC_END
