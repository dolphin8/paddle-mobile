/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once
#define IMAGE_ALIGNMENT 16  // Aligned to 16
namespace paddle_mobile {
namespace fpga {
namespace image {

void convert_to_hwc(float** data_in, int channel, int height, int width);
void align_element_conv(float** data_in, int height, int cw);
void format_image(float** data_in, int channel, int height, int width);
}  // namespace image
}  // namespace fpga
}  // namespace paddle_mobile
