#pragma once

#include <ipc/utils/eigen_ext.hpp>

#include <array>
#include <limits>

namespace ipc {

/// @brief A stencil representing a collision between at most four vertices.
template <int max_vert = 4> class CollisionStencil {
public:
    constexpr static int element_size = max_vert;
    CollisionStencil() = default;
    virtual ~CollisionStencil() = default;

    /// @brief Get the number of vertices in the collision stencil.
    virtual int num_vertices() const = 0;

    /// @brief Get the dimension of the collision stencil.
    /// @param ndof Number of degrees of freedom in the stencil.
    /// @return The dimension of the collision stencil.
    int dim(const int ndof) const
    {
        assert(ndof % num_vertices() == 0);
        return ndof / num_vertices();
    }

    /// @brief Get the vertex IDs of the collision stencil.
    /// @param edges Collision mesh edges
    /// @param faces Collision mesh faces
    /// @return The vertex IDs of the collision stencil. Size is always max_vert, but elements i > num_vertices() are -1.
    virtual std::array<long, max_vert> vertex_ids(
        const Eigen::MatrixXi& edges, const Eigen::MatrixXi& faces) const = 0;

    /// @brief Get the vertex attributes of the collision stencil.
    /// @tparam T Type of the attributes
    /// @param vertices Vertex attributes
    /// @param edges Collision mesh edges
    /// @param faces Collision mesh faces
    /// @return The vertex positions of the collision stencil. Size is always max_vert, but elements i > num_vertices() are NaN.
    template <typename T>
    std::array<VectorMax3<T>, max_vert> vertices(
        const MatrixX<T>& vertices,
        const Eigen::MatrixXi& edges,
        const Eigen::MatrixXi& faces) const
    {
        constexpr double NaN = std::numeric_limits<double>::signaling_NaN();

        const std::array<long, max_vert> vertex_ids =
            this->vertex_ids(edges, faces);

        std::array<VectorMax3<T>, max_vert> stencil_vertices;
        for (int i = 0; i < max_vert; i++) {
            if (vertex_ids[i] >= 0) {
                stencil_vertices[i] = vertices.row(vertex_ids[i]);
            } else {
                stencil_vertices[i].setConstant(vertices.cols(), T(NaN));
            }
        }

        return stencil_vertices;
    }

    /// @brief Select this stencil's DOF from the full matrix of DOF.
    /// @tparam T Type of the DOF
    /// @param X Full matrix of DOF (rowwise).
    /// @param edges Collision mesh edges
    /// @param faces Collision mesh faces
    /// @return This stencil's DOF.
    template <typename T>
    Vector<T, -1, 3 * max_vert>
    dof(const MatrixX<T>& X,
        const Eigen::MatrixXi& edges,
        const Eigen::MatrixXi& faces) const
    {
        const int dim = X.cols();
        Vector<T, -1, 3 * max_vert> x(num_vertices() * dim);
        const std::array<long, max_vert> idx = vertex_ids(edges, faces);
        for (int i = 0; i < num_vertices(); i++) {
            x.segment(i * dim, dim) = X.row(idx[i]);
        }
        return x;
    }

    // ----------------------------------------------------------------------
    // NOTE: The following functions take stencil vertices as output by dof()
    // ----------------------------------------------------------------------

    /// @brief Compute the distance of the stencil.
    /// @param positions Stencil's vertex positions.
    /// @note positions can be computed as stencil.dof(vertices, edges, faces)
    /// @return Distance of the stencil.
    virtual double compute_distance(
        const Vector<double, -1, 3 * max_vert>& positions) const = 0;

    /// @brief Compute the distance gradient of the stencil w.r.t. the stencil's vertex positions.
    /// @param positions Stencil's vertex positions.
    /// @note positions can be computed as stencil.dof(vertices, edges, faces)
    /// @return Distance gradient of the stencil w.r.t. the stencil's vertex positions.
    virtual Vector<double, -1, 3 * max_vert> compute_distance_gradient(
        const Vector<double, -1, 3 * max_vert>& positions) const = 0;

    /// @brief Compute the distance Hessian of the stencil w.r.t. the stencil's vertex positions.
    /// @param positions Stencil's vertex positions.
    /// @note positions can be computed as stencil.dof(vertices, edges, faces)
    /// @return Distance Hessian of the stencil w.r.t. the stencil's vertex positions.
    virtual MatrixMax<double, 3 * max_vert, 3 * max_vert>
    compute_distance_hessian(
        const Vector<double, -1, 3 * max_vert>& positions) const = 0;
};

} // namespace ipc