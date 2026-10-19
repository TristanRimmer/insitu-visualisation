#ifndef FIRST_CUBOID_TEST_HH
#define FIRST_CUBOID_TEST_HH

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <vector>

/**
 * TODO:
 * In the work flow, the user passes an X, Y, Z target axis.
 *
 * Lets call these:
 *  X_c and X_m (x chosen and X max)
 *  Y_c and Y_m (y chosen and y max)
 *  Z_c and Z_m (z chosen and z max)
 *
 *  Currently, no matter the chosen *_c, the visualisation will show the full
 * planes as *_m
 *
 * This creates unrealistic outputs:
 *
 * EG: The chosen X slice was half way through, the chosen Y slice was half
 * way, and the chosen z slice was the furthest.
 *
 * the output image would not have the edges line up correctly
 *
 * SOLUTION:
 *
 * The barycentric coordinates have a range (0-1) which is then altered by
 * whichever axis it depends on
 *
 * the X slice (X axis is fixed, therefore its the ZY plane) would need:
 * - X bary (z axis)'s lower range to be affected by the XY plane -> Z_c
 * - Y bary (y axis)'s upper range to be affected by the XZ plane -> Y_m - Y_c
 *
 * the Y slice (y axis is fixed, XZ plane) depends on:
 * - X bary (X axis)'s upper range to be affected by the ZY plane -> X_m - X_c
 * - Y bary (Z axis)'s lower range to be affected by the XY plane -> just the
 *   number
 *
 * The Z slice (z is fixed, XY plane) depends on:
 * - X bary (X axis)'s upper range to be affected by the ZY plane -> just the
 *   number
 * - Y bary (Y axis)'s upper range to be affected by the XZ plane -> Y_m - Y_c
 *
 *
 * It is important that this doesnt affect the range before barycentrics, and
 * instead after.
 *
 * Eg/ one who's lower portion has been decreased by 35% should have a range
 * of 0.35 to 1 Or one who's upper portion hasbeen decreased by 25% should
 * have a range of 0 to 0.75
 */

constexpr auto correct_barycentric_scale(
    int face_index, double &face_a_lower, double &face_a_mod,
    double &face_b_lower, double &face_b_mod,
    std::pair<std::array<std::size_t, 3>, std::array<std::size_t, 3>>
        chosen_and_full_dims) -> void {
  double original_face_a = face_a_mod;
  double original_face_b = face_b_mod;

  auto chosens = chosen_and_full_dims.first;
  auto fulls = chosen_and_full_dims.second;

  auto scale_x = chosens[0] / fulls[0];
  auto scale_y = chosens[1] / fulls[1];
  auto scale_z = chosens[2] / fulls[2];

  if (face_index == 0) {
    // XY plane
    face_a_lower = (original_face_a * scale_z);
    face_a_mod = face_a_mod;
    face_b_lower = 0;
    face_b_mod = original_face_b - (original_face_b * (scale_y));
  } else if (face_index == 1) {
    // ZY plane
    face_a_lower = original_face_a * scale_z;
    face_a_mod = original_face_a - (original_face_a * (scale_x));
    face_b_lower = original_face_b * scale_y;
    face_b_mod = face_b_mod;
  } else {
    // XZ plane
    face_a_lower = 0;
    face_a_mod = original_face_a - (original_face_a * scale_x);
    face_b_lower = 0;
    face_b_mod = original_face_b - (original_face_b * scale_z);
  }
}

namespace Vector3D {
//==// Vec3 Utility
template <typename T> using Vec3 = std::array<T, 3>;

template <typename T>
constexpr auto operator+(const Vec3<T> &a, const Vec3<T> &b) -> Vec3<T> {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}
template <typename T>
constexpr auto operator-(const Vec3<T> &a, const Vec3<T> &b) -> Vec3<T> {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}
template <typename T>
constexpr auto operator*(const Vec3<T> &a, const Vec3<T> &b) -> Vec3<T> {
  return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
}
// NOTE: this is a vec * scalar overload
template <typename T>
constexpr auto operator*(T b, const Vec3<T> &a) -> Vec3<T> {
  return {a[0] * b, a[1] * b, a[2] * b};
}
template <typename T>
constexpr auto operator/(const Vec3<T> &a, const Vec3<T> &b) -> Vec3<T> {
  return {a[0] / b[0], a[1] / b[1], a[2] / b[2]};
}
// NOTE: this is Vec / scalar
template <typename T>
constexpr auto operator/(const Vec3<T> &a, T b) -> Vec3<T> {
  return {a[0] / b, a[1] / b, a[2] / b};
}
template <typename T> auto stringVec(const Vec3<T> &v) -> std::string {
  return "[" + std::to_string(v[0]) + ", " + std::to_string(v[1]) + ", " +
         std::to_string(v[2]) + "]";
}

template <typename T> auto dot(const Vec3<T> &a, const Vec3<T> &b) -> T {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
template <typename T> auto mod(const Vec3<T> &v) -> T {
  return std::sqrt(dot(v, v));
}

} // namespace Vector3D

namespace RasteriserComponents {
using namespace Vector3D;
template <typename T> struct Face {
  Vec3<T> origin;
  Vec3<T> a;
  Vec3<T> b;
};

template <typename T> auto printFace(const Face<T> &face) -> void {
  std::cout << "Face: " << std::endl;
  std::cout << "- Origin: " << stringVec(face.origin) << std::endl;
  std::cout << "- Dir Vec A: " << stringVec(face.a) << std::endl;
  std::cout << "- Dir Vec B: " << stringVec(face.b) << std::endl;
}

template <typename T> struct Camera {
  Vec3<T> origin;
  Vec3<T> direction;
};

enum HitSurface {
  // Not enum class as this lets us use implicit conversion
  NONE = -1,
  FIRST = 0,
  SECOND = 1,
  THIRD = 2,
};

enum CameraPerspectiveX { LEFT = -1, RIGHT = 1 };
enum CameraPerspectiveY { DOWN = -1, UP = 1 };
enum CameraPerspectiveZ { BACK = -1, FRONT = 1 };

struct CameraPerspective {
  CameraPerspectiveX x;
  CameraPerspectiveY y;
  CameraPerspectiveZ z;
};

} // namespace RasteriserComponents
namespace Renderer {
using namespace Vector3D;
using namespace RasteriserComponents;

template <typename T>
auto get_full_dims_as_faces(const std::array<std::size_t, 3> &dims,
                            const CameraPerspective &perspective)
    -> std::array<Face<T>, 3> {
  auto dims_T =
      std::array<T, 3>{static_cast<T>(dims[0]), static_cast<T>(dims[1]),
                       static_cast<T>(dims[2])};

  auto xy_face = Face<T>{};
  xy_face.a = {dims_T[0], T{0}, T{0}};
  xy_face.b = {T{0}, dims_T[1], T{0}};

  auto zy_face = Face<T>{};
  zy_face.a = {T{0}, T{0}, dims_T[2]};
  zy_face.b = {T{0}, dims_T[1], T{0}};

  auto xz_face = Face<T>{};
  xz_face.a = {dims_T[0], T{0}, T{0}};
  xz_face.b = {T{0}, T{0}, dims_T[2]};

  auto xy_origin = Vec3<T>{T{0}, T{0}, T{0}};
  auto zy_origin = Vec3<T>{T{0}, T{0}, T{0}};
  auto xz_origin = Vec3<T>{T{0}, T{0}, T{0}};

  if (perspective.x == CameraPerspectiveX::RIGHT) {
    zy_origin[0] = dims_T[0];
  }

  if (perspective.y == CameraPerspectiveY::UP) {
    xz_origin[1] = dims_T[1];
  }

  if (perspective.z == CameraPerspectiveZ::FRONT) {
    xy_origin[2] = dims_T[2];
  }

  xy_face.origin = xy_origin;
  zy_face.origin = zy_origin;
  xz_face.origin = xz_origin;

  return {xy_face, zy_face, xz_face};
}

constexpr auto correct_lambda_and_mu(std::size_t &lambda, std::size_t &mu,
                                     std::array<std::size_t, 3> chosen_dims,
                                     std::size_t face_index,
                                     CameraPerspective perspective) {
  if (face_index == 0) {
    // XY
    if (perspective.x == LEFT)
      lambda = lambda + chosen_dims[0];
    if (perspective.y == DOWN)
      mu = mu + chosen_dims[1];
  } else if (face_index == 1) {
    // ZY
    if (perspective.z == BACK)
      lambda = lambda + chosen_dims[2];
    if (perspective.y == DOWN)
      mu = mu + chosen_dims[1];
  } else /* if (face_index == 2) */ {
    // XZ
    if (perspective.x == LEFT)
      lambda = lambda + chosen_dims[0];
    if (perspective.z == BACK)
      mu = mu + chosen_dims[2];
  }
}

/*
 * Given the dimensions, this will return an array containing the faces:
 * {XY Face, ZY Face, XZ Face};
 */
template <typename T>
auto get_faces_of_cuboid(const std::array<std::size_t, 3> &dims,
                         const std::array<std::size_t, 3> &chosen_slices,
                         const CameraPerspective &perspective)
    -> std::array<Face<T>, 3> {
  auto dims_T =
      std::array<T, 3>{static_cast<T>(dims[0]), static_cast<T>(dims[1]),
                       static_cast<T>(dims[2])};
  auto chosens_T = std::array<T, 3>{static_cast<T>(chosen_slices[0]),
                                    static_cast<T>(chosen_slices[1]),
                                    static_cast<T>(chosen_slices[2])};
  // The widths take the opposite calculation to the origins
  auto x_axis_width = dims_T[0];
  auto y_axis_width = dims_T[1];
  auto z_axis_width = dims_T[2];

  auto xy_axis_origin = 0;
  auto xz_axis_origin = 0;
  auto zy_axis_origin = 0;

  if (perspective.x == CameraPerspectiveX::RIGHT) {
    // If its from the right, the x-axis width should be just the chosen slice
    x_axis_width = chosens_T[0];
    zy_axis_origin = chosens_T[0];
  } else {
    // If its from the left, you want the x-axis width to be everything AFTER
    // the slice
    x_axis_width = dims_T[0] - chosens_T[0];
  }
  if (perspective.y == CameraPerspectiveY::UP) {
    // If its from above, the y-axis width should just be the the chosen slice
    y_axis_width = chosens_T[1];
    xz_axis_origin = chosens_T[1];
  } else {
    y_axis_width = dims_T[1] - chosens_T[1];
  }
  if (perspective.z == CameraPerspectiveZ::FRONT) {
    // If its from the front, it should be the chosen slice
    z_axis_width = chosens_T[2];
    xy_axis_origin = chosens_T[2];
  } else {
    z_axis_width = dims_T[2] - chosens_T[2];
  }
  x_axis_width = x_axis_width <= 0 ? 1 : x_axis_width;
  y_axis_width = y_axis_width <= 0 ? 1 : y_axis_width;
  z_axis_width = z_axis_width <= 0 ? 1 : z_axis_width;

  // XY viewing plane is affected by the Front/Back Camera
  auto xy_face = Face<T>{};
  xy_face.a = {x_axis_width, T{0}, T{0}};
  xy_face.b = {T{0}, y_axis_width, T{0}};

  // XY viewing plane is affected by the Left/Right Camera
  auto zy_face = Face<T>{};
  zy_face.a = {T{0}, T{0}, z_axis_width};
  zy_face.b = {T{0}, y_axis_width, T{0}};

  // XY viewing plane is affected by the Up/Down Camera
  auto xz_face = Face<T>{};
  xz_face.a = {x_axis_width, T{0}, T{0}};
  xz_face.b = {T{0}, T{0}, z_axis_width};

  auto xy_origin = Vec3<T>{T{0}, T{0}, T{0}};
  auto zy_origin = Vec3<T>{T{0}, T{0}, T{0}};
  auto xz_origin = Vec3<T>{T{0}, T{0}, T{0}};

  zy_origin[0] = zy_axis_origin;
  xz_origin[1] = xz_axis_origin;
  xy_origin[2] = xy_axis_origin;

  xy_face.origin = xy_origin;
  zy_face.origin = zy_origin;
  xz_face.origin = xz_origin;

  return {xy_face, zy_face, xz_face};
}

/*
 * Given the dimensions of the cuboid, this will return a camera object, pre
 * position relative to the dimensions of the cuboid, with its angle aiming
 * towards the center of the cuboid
 */
template <typename T>
auto get_position_camera(const std::array<std::size_t, 3> &dims,
                         const CameraPerspective &perspective) -> Camera<T> {

  auto dims_T =
      std::array<T, 3>{static_cast<T>(dims[0]), static_cast<T>(dims[1]),
                       static_cast<T>(dims[2])};
  auto distance_multipler = 0.8;

  auto camera_pos =
      Vec3<T>{2 * dims_T[0] * perspective.x, 1.75 * dims_T[1] * perspective.y,
              2.2 * dims_T[2] * perspective.z};

  camera_pos = distance_multipler * camera_pos;
  auto center_of_cuboid = Vec3<T>{
      dims[0] / T{2},
      dims[1] / T{2},
      dims[2] / T{2},
  };

  auto camera_angle = center_of_cuboid - camera_pos;

  auto mod = Vector3D::mod(camera_angle);

  camera_angle = {camera_angle[0] / mod, -1 * camera_angle[1] / mod,
                  camera_angle[2] / mod};

  return Camera<T>{camera_pos, camera_angle};
}

/*
 * WARN: SRP Violations
 * TODO: Amend SRP Violations
 *
 * Given the Camera, Faces and image metadata, this function will - for each
 * pixel ->
 * - Form a 3D line and shoot a ray
 * - Calculate whether the line intersects with any of the surfaces
 * - If it does, it will calculate where the point on the face translates to
 * the colour on the mesh
 * - Apply the calculated colour to that pixel in the new image
 * Before returning the pixel buffer of the new image
 */
template <typename T>
auto get_pixel_buffer(
    std::pair<std::size_t, std::size_t> image_dimensions, Camera<T> camera,
    std::array<Face<T>, 3> faces, // TODO: swap for 3 individual faces with
                                  // names and the array is built in function
    std::array<Face<T>, 3> full_faces,
    const std::array<std::vector<std::uint8_t>, 3> &image_buffers,
    const std::array<std::pair<std::size_t, std::size_t>, 3> &image_buffer_dims,
    std::pair<std::array<std::size_t, 3>, std::array<std::size_t, 3>>
        chosen_and_full_dims,
    CameraPerspective perspective) -> std::vector<std::uint8_t> {

  auto pixel_buffer = std::vector<std::uint8_t>{};

  /*
   * Constants that will be used per-pixel to calculate the correclty rotated
   * direction vector
   */
  // INFO: Perhaps make the field of view an argument
  constexpr auto field_of_view = 60 * 3.1415926535 / 180;
  const auto aspect_ratio = image_dimensions.first / image_dimensions.second;
  // The Projection plane can be though of as the pixel buffer, but existing
  // in 3D space
  const auto projection_plane_w = tan(field_of_view / 2) * 2;
  const auto projection_plane_h = projection_plane_w / aspect_ratio;
  // Just saying that the camera should be position so that the center ray
  // goes through the center of the projection plane
  const auto proj_plane_camera_offset_x = projection_plane_w / 2;
  const auto proj_plane_camera_offset_y = projection_plane_h / 2;
  // How much of the projection plane's colour will be determined per pixel
  const auto pixel_increment_x =
      projection_plane_w / static_cast<double>(image_dimensions.first);
  const auto pixel_increment_y =
      projection_plane_h / static_cast<double>(image_dimensions.second);
  // Camera Pitch and Yaw, assuming the forward vector is (0,0,1)
  const auto camera_yaw = std::atan2(camera.direction[0], camera.direction[2]);
  const auto camera_pitch =
      std::atan2(camera.direction[1],
                 std::sqrt(camera.direction[0] * camera.direction[0] +
                           camera.direction[2] * camera.direction[2]));

  /*
   * Helper method which allows the primary computation to use both the
   *  -> r̲·n̂ = d
   * form and
   *  -> o̲ + λa̲ + μb̲
   * form of a plane
   */
  const auto calculate_normal_and_d_of_face =
      [](const Face<T> &face) -> std::pair<Vec3<T>, T> {
    auto surface_normal =
        Vec3<T>{face.a[1] * face.b[2] - face.b[1] * face.a[2],
                -1 * face.a[0] * face.b[2] + face.b[0] * face.a[2],
                face.a[0] * face.b[1] - face.b[0] * face.a[1]};

    auto normal_mod = Vector3D::mod(surface_normal);

    auto n_hat =
        Vec3<T>{surface_normal[0] / normal_mod, surface_normal[1] / normal_mod,
                surface_normal[2] / normal_mod};

    return {n_hat, Vector3D::dot(face.origin, n_hat)};
  };

  auto pre_calculated_face_normals_and_ds =
      std::array<std::pair<Vec3<T>, T>, 3>{
          {calculate_normal_and_d_of_face(faces[0]),
           calculate_normal_and_d_of_face(faces[1]),
           calculate_normal_and_d_of_face(faces[2])}};

  // INFO: Main Computation
  for (int pixel_y = 0; pixel_y < image_dimensions.second; pixel_y++) {
    for (int pixel_x = 0; pixel_x < image_dimensions.second; pixel_x++) {
      // Pint on projection plane
      auto proj_plane_point = Vec3<double>{
          (pixel_increment_x * pixel_x) - proj_plane_camera_offset_x,
          (pixel_increment_y * pixel_y) - proj_plane_camera_offset_y, 0.0};

      auto dir_vector = proj_plane_point - Vec3<T>{T{0}, T{0}, T{-1}};

      /* If the camera were to have a 'roll' rotation, it would be here */

      auto pitched_vec =
          Vec3<double>{dir_vector[0],
                       dir_vector[1] * std::cos(camera_pitch) -
                           dir_vector[2] * std::sin(camera_pitch),
                       dir_vector[1] * std::sin(camera_pitch) +
                           dir_vector[2] * std::cos(camera_pitch)};

      auto rotated_dir_vec = Vec3<double>{
          pitched_vec[0] * std::cos(camera_yaw) +
              pitched_vec[2] * std::sin(camera_yaw),
          pitched_vec[1],
          pitched_vec[2] * std::cos(camera_yaw) -
              pitched_vec[0] * std::sin(camera_yaw),
      };

      // Normalised
      rotated_dir_vec = rotated_dir_vec / Vector3D::mod(rotated_dir_vec);

      auto hit_surface_status = HitSurface::NONE;
      auto lambda_mu = std::pair<std::size_t, std::size_t>{0, 0};

      auto face_intersected = Face<T>{};
      auto full_face_intersected = Face<T>{};

      for (auto face_i = 0; face_i < faces.size(); face_i++) {
        auto face = Face<T>{faces[face_i]};
        auto full_face = Face<T>{full_faces[face_i]};
        auto face_n_and_d = pre_calculated_face_normals_and_ds[face_i];

        //=// Get the point of intersection //=//

        // Abbreviations to make equation shorter:
        // (a + lambda b) dot n = d
        // solving for lambda
        auto &a = camera.origin;
        auto &b = rotated_dir_vec;
        auto &n = face_n_and_d.first;
        auto &d = face_n_and_d.second;

        // Direction vector must not be parallel with surface normal
        if (std::abs(Vector3D::dot(n, b)) < 1e-09)
          continue;

        auto lambda = (d - (Vector3D::dot(a, n))) / Vector3D::dot(b, n);

        if (lambda < 0)
          continue;

        auto point_of_intersection = a + (lambda * b);

        auto local_point_on_surface = point_of_intersection - face.origin;

        auto face_a_lower_bound = 0.0;
        auto face_b_lower_bound = 0.0;
        auto face_a_mod = Vector3D::mod(face.a);
        auto face_b_mod = Vector3D::mod(face.b);

        // // Updates the above as refs
        // correct_barycentric_scale(face_i, face_a_lower_bound, face_a_mod,
        //                           face_b_lower_bound, face_b_mod,
        //                           chosen_and_full_dims);

        // How much of the local position vector is made up by face.a and
        // face.b
        auto lambda_dot =
            Vector3D::dot(local_point_on_surface, face.a / face_a_mod);
        auto mu_dot =
            Vector3D::dot(local_point_on_surface, face.b / face_b_mod);

        if (!(face_a_lower_bound < lambda_dot && lambda_dot <= face_a_mod))
          continue;

        if (!(face_b_lower_bound < mu_dot && mu_dot <= face_b_mod))
          continue;

        hit_surface_status = static_cast<HitSurface>(face_i);
        lambda_mu = {lambda_dot, mu_dot};
        face_intersected = face;
        full_face_intersected = full_face;
        break;
      }

      if (hit_surface_status != HitSurface::NONE) {
        Renderer::correct_lambda_and_mu(
            lambda_mu.first, lambda_mu.second, chosen_and_full_dims.first,
            static_cast<std::size_t>(hit_surface_status), perspective);

        pixel_buffer.push_back(
            image_buffers[hit_surface_status]
                         [lambda_mu.second *
                              (image_buffer_dims[hit_surface_status].first *
                               3) +
                          (3 * lambda_mu.first) + 0]);
        pixel_buffer.push_back(
            image_buffers[hit_surface_status]
                         [lambda_mu.second *
                              (image_buffer_dims[hit_surface_status].first *
                               3) +
                          (3 * lambda_mu.first) + 1]);
        pixel_buffer.push_back(
            image_buffers[hit_surface_status]
                         [lambda_mu.second *
                              (image_buffer_dims[hit_surface_status].first *
                               3) +
                          (3 * lambda_mu.first) + 2]);
      } else {
        pixel_buffer.emplace_back(120);
        pixel_buffer.emplace_back(120);
        pixel_buffer.emplace_back(120);
      }
    }
  }

  return pixel_buffer;
}

} // namespace Renderer

#endif
