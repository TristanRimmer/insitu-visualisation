#ifndef SLICE_VISUALISER_3D_INSITU_HH
#define SLICE_VISUALISER_3D_INSITU_HH

#include "3d_cuboid_renderer.hh"
#include <SDL_keycode.h>
#include <functional>
#include <unordered_map>
namespace SliceVisualiser {

using namespace RasteriserComponents;

struct RunTimeVariantData {
  RasteriserComponents::CameraPerspectiveX x =
      RasteriserComponents::CameraPerspectiveX::RIGHT;
  RasteriserComponents::CameraPerspectiveY y =
      RasteriserComponents::CameraPerspectiveY::UP;
  RasteriserComponents::CameraPerspectiveZ z =
      RasteriserComponents::CameraPerspectiveZ::FRONT;

  std::array<std::size_t, 3> dimensions = {0, 0, 0};
  std::array<std::size_t, 3> slice = {0, 0, 0};
  bool state_changed = true;

  void update_slice(std::size_t index, bool add) {
    int incr = add ? 1 : -1;

    this->slice[index] += incr;

    if (this->slice[index] < 0)
      this->slice[index] = 0;

    if (this->slice[index] >= this->dimensions[index])
      this->slice[index] = this->dimensions[index] - 1;
  }
};

const static auto InputToBehaviourMapper =
    std::unordered_map<SDL_Keycode, std::function<void(RunTimeVariantData &)>>{
        {SDLK_x,
         [](RunTimeVariantData &data) {
           data.x = (data.x == CameraPerspectiveX::LEFT)
                        ? CameraPerspectiveX::RIGHT
                        : CameraPerspectiveX::LEFT;
           data.state_changed = true;
         }},
        {SDLK_y,
         [](RunTimeVariantData &data) {
           data.y = (data.y == CameraPerspectiveY::DOWN)
                        ? CameraPerspectiveY::UP
                        : CameraPerspectiveY::DOWN;
           data.state_changed = true;
         }},
        {SDLK_z,
         [](RunTimeVariantData &data) {
           data.z = (data.z == CameraPerspectiveZ::BACK)
                        ? CameraPerspectiveZ::FRONT
                        : CameraPerspectiveZ::BACK;
           data.state_changed = true;
         }},
        {SDLK_UP,
         [](RunTimeVariantData &data) {
           data.update_slice(1, true);
           data.state_changed = true;
         }},
        {SDLK_DOWN,
         [](RunTimeVariantData &data) {
           data.update_slice(1, false);
           data.state_changed = true;
         }},
        {SDLK_LEFT,
         [](RunTimeVariantData &data) {
           data.update_slice(0, false);
           data.state_changed = true;
         }},
        {SDLK_RIGHT,
         [](RunTimeVariantData &data) {
           data.update_slice(0, true);
           data.state_changed = true;
         }},
        {SDLK_RIGHTBRACKET,
         [](RunTimeVariantData &data) {
           data.update_slice(2, true);
           data.state_changed = true;
         }},
        {SDLK_LEFTBRACKET,
         [](RunTimeVariantData &data) {
           data.update_slice(2, false);
           data.state_changed = true;
         }},
};
} // namespace SliceVisualiser

#endif
