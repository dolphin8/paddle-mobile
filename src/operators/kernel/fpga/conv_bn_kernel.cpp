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

#ifdef FUSION_CONVBN_OP

#include "operators/kernel/conv_bn_kernel.h"
#include "fpga/api.h"

namespace paddle_mobile {
namespace operators {

template <>
bool ConvBNKernel<FPGA, float>::Init(FusionConvBNParam<FPGA> *param) {
  bool relu_enabled = false;
  Tensor *input = const_cast<Tensor *>(param->Input());
  auto input_ptr = input->data<float>();
  Tensor *filter = param->Filter();

  Tensor *out = param->Output();
  auto bn_mean_ptr = param->InputMean()->data<float>();
  auto bn_var_ptr = param->InputVariance()->data<float>();
  auto bn_scale_ptr = param->InputScale()->data<float>();
  auto bn_bias_ptr = param->InputBias()->data<float>();
  const float epsilon = param->Epsilon();
  PADDLE_MOBILE_ENFORCE(out->dims()[1] == param->InputBias()->dims()[0],
                        "Output channel should be equal to bias number");

  const int channel = out->dims()[1];
  float *bs_ptr =
      reinterpret_cast<float *>(fpga::fpga_malloc(2 * channel * sizeof(float)));
  Tensor *new_scale = new Tensor();
  Tensor *new_bias = new Tensor();
  auto new_scale_ptr = new_scale->mutable_data<float>({channel});
  auto new_bias_ptr = new_bias->mutable_data<float>({channel});

  for (int i = 0; i < channel; i++) {
    new_scale_ptr[i] = bn_scale_ptr[i] /
                       static_cast<float>(pow((bn_var_ptr[i] + epsilon), 0.5));
    new_bias_ptr[i] = bn_bias_ptr[i] + (0 - bn_mean_ptr[i]) * new_scale_ptr[i];
    bs_ptr[i + channel] = new_scale_ptr[i];
    bs_ptr[i] = new_bias_ptr[i];
  }
  param->SetNewScale(new_scale);
  param->SetNewBias(new_bias);

  float max_value = fpga::filter_find_max(filter);
  fpga::format_filter(filter, max_value, param->Groups());
  auto filter_ptr = filter->data<float>();

  int element_num_per_div =
      fpga::get_element_num_per_div(filter, param->Groups());
  fpga::format_bias_scale_array(&bs_ptr, element_num_per_div, channel);

  fpga::format_ofm(out);
  auto out_ptr = out->mutable_data<float>();

  fpga::ConvArgs convArgs;
  convArgs.relu_enabled = relu_enabled;
  convArgs.filter_address = (void *)filter_ptr;
  convArgs.filter_num = filter->dims()[0];
  convArgs.group_num = param->Groups();
  convArgs.sb_address = (void *)bs_ptr;
  convArgs.kernel.stride_h = param->Strides()[0];
  convArgs.kernel.stride_w = param->Strides()[1];
  convArgs.kernel.height = filter->dims()[2];
  convArgs.kernel.width = filter->dims()[3];
  convArgs.image.address = (void *)input_ptr;
  convArgs.image.channels = input->dims()[1];
  convArgs.image.height = input->dims()[2];
  convArgs.image.width = input->dims()[3];
  convArgs.image.pad_height = param->Paddings()[0];
  convArgs.image.pad_width = param->Paddings()[1];
  convArgs.image.scale_address = input->scale;
  convArgs.output.address = (void *)out_ptr;
  convArgs.output.scale_address = out->scale;
  param->SetFpgaArgs(convArgs);

  return true;
}

template <>
void ConvBNKernel<FPGA, float>::Compute(
    const FusionConvBNParam<FPGA> &param) const {
  fpga::ComputeFpgaConv(param.FpgaArgs());
}
template class ConvBNKernel<FPGA, float>;

}  // namespace operators
}  // namespace paddle_mobile

#endif
